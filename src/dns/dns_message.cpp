#include "dns_message.hpp"

#include <bitset>
#include <iomanip>
#include <map>
#include <sstream>

using namespace std;

DNSMessage::DNSMessage()
{
    header.id = 0;
    header.flags = 0x0100;
    header.query_domain_count = 1;
    header.answer_count = 0;
    header.name_server_count = 0;
    header.additional_count = 0;

    question.name = "";
    question.type = 1; // Default A record
    question.class_code = 1; // IN
}

void DNSMessage::configureQuery(const string& name, uint16_t type)
{
    question.name = name;
    question.type = type;
    question.class_code = 1;
    header.id = rand() % 65536;
}

vector<uint8_t> DNSMessage::buildQuery()
{
    vector<uint8_t> packet;
    addUint16(packet, header.id);
    addUint16(packet, header.flags);
    addUint16(packet, header.query_domain_count);
    addUint16(packet, header.answer_count);
    addUint16(packet, header.name_server_count);
    addUint16(packet, header.additional_count);
    addQuestion(packet);
    return packet;
}

void DNSMessage::parseResponse(const vector<uint8_t>& data)
{
    if (data.size() < 12)
        throw runtime_error("Pacote DNS inválido (muito pequeno)");

    size_t pos = 0;
    readHeader(data, pos);
    
    // Zera os vetores para permitir que sejam reutilizados
    answers.clear();
    authorities.clear();
    additionals.clear();

    if (header.query_domain_count > 0)
        readQuestion(data, pos);
    if (header.answer_count > 0)
        readAnswers(data, pos);
    if (header.name_server_count > 0)
        readAuthorities(data, pos);
    if (header.additional_count > 0)
        readAdditionals(data, pos);
}

uint8_t DNSMessage::getRcode() const
{
    return static_cast<uint8_t>(header.flags & 0x000F);
}

void DNSMessage::addUint16(vector<uint8_t>& packet, uint16_t value)
{
    packet.push_back(value >> 8);
    packet.push_back(value & 0xFF);
}

void DNSMessage::addQuestion(vector<uint8_t>& packet)
{
    size_t start = 0;
    size_t dot_pos;
    string current_name = question.name;

    while ((dot_pos = current_name.find('.', start)) != string::npos)
    {
        uint8_t length = dot_pos - start;
        packet.push_back(length);
        for (size_t i = start; i < dot_pos; ++i)
            packet.push_back(current_name[i]);
        start = dot_pos + 1;
    }

    uint8_t length = current_name.length() - start;
    if (length > 0)
    {
        packet.push_back(length);
        for (size_t i = start; i < current_name.length(); ++i)
            packet.push_back(current_name[i]);
    }

    packet.push_back(0); // Null terminator for the name
    addUint16(packet, question.type);
    addUint16(packet, question.class_code);
}

ResourceRecord DNSMessage::readRecord(const vector<uint8_t>& data, size_t& pos)
{
    ResourceRecord rr;
    rr.name = readName(data, pos);
    rr.type = readUint16(data, pos);
    rr.class_code = readUint16(data, pos);
    rr.ttl = readUint32(data, pos);
    rr.data_length = readUint16(data, pos);

    if (pos + rr.data_length > data.size())
        throw runtime_error("data_lengthgth do registro excede o tamanho do pacote.");

    rr.raw_data.assign(data.begin() + pos, data.begin() + pos + rr.data_length);
    pos += rr.data_length;

    // Decodifica raw_data com base no tipo do registro
    switch (static_cast<DNSRecordType>(rr.type))
    {
        case DNSRecordType::A:      decodeA(rr);      break;
        case DNSRecordType::NS:     decodeNS(rr);     break;
        case DNSRecordType::CNAME:  decodeCNAME(rr);  break;
        case DNSRecordType::SOA:    decodeSOA(rr);    break;
        case DNSRecordType::MX:     decodeMX(rr);     break;
        case DNSRecordType::TXT:    decodeTXT(rr);    break;
        case DNSRecordType::AAAA:   decodeAAAA(rr);   break;
        case DNSRecordType::OPT:    decodeOPT(rr);    break;
        case DNSRecordType::DS:     decodeDS(rr);     break;
        case DNSRecordType::RRSIG:  decodeRRSIG(rr);  break;
        case DNSRecordType::DNSKEY: decodeDNSKEY(rr); break;
        default:
            rr.parsed_data = "Tipo de Registro " + to_string(rr.type) + " não suportado para decodificação.";
            break;
    }
    return rr;
}

uint16_t DNSMessage::readUint16(const vector<uint8_t>& data, size_t& pos) {
    if (pos + 2 > data.size()) throw runtime_error("Erro ao ler uint16");
    uint16_t value = 0;
    for (int i = 0; i < 2; ++i) {
        //  | -> operador bit a bit, usado para combinar bytes em um único número.
        value = (value << 8) | data[pos + i];
    }
    pos += 2;
    return value;
}

uint32_t DNSMessage::readUint32(const vector<uint8_t>& data, size_t& pos) {
    if (pos + 4 > data.size())
        throw runtime_error("Erro ao ler uint32");
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
        value = (value << 8) | data[pos + i];
    pos += 4;
    return value;
}

string DNSMessage::readName(const vector<uint8_t>& data, size_t& pos) {
    string name;
    size_t original_pos = pos;
    bool jumped = false;
    int jump_count = 0;  // limite de segurança

    while (pos < data.size()) {
        if (++jump_count > 20)
            throw runtime_error("Loop de compressão de nomes DNS detectado");

        uint8_t len = data[pos];
        if (len == 0)
        {
            pos++;
            break;
        }

        // Caso seja ponteiro (compressão)
        if ((len & 0xC0) == 0xC0)
        {
            if (pos + 1 >= data.size()) throw runtime_error("Ponteiro de nome DNS inválido");
            uint16_t offset = ((len & 0x3F) << 8) | data[pos + 1];
            if (!jumped)
            {
                original_pos = pos + 2;
                jumped = true;
            }
            pos = offset;
            continue;
        }

        pos++;
        if (!name.empty())
            name += '.';
        if (pos + len > data.size())
            throw runtime_error("Pacote DNS truncado");

        name.append(reinterpret_cast<const char*>(&data[pos]), len);
        pos += len;
    }

    if (jumped)
        pos = original_pos;

    return name;
}

void DNSMessage::readHeader(const vector<uint8_t>& data, size_t& pos) {
    header.id = readUint16(data, pos);
    header.flags = readUint16(data, pos);
    header.query_domain_count = readUint16(data, pos);
    header.answer_count = readUint16(data, pos);
    header.name_server_count = readUint16(data, pos);
    header.additional_count = readUint16(data, pos);
}

void DNSMessage::readQuestion(const vector<uint8_t>& data, size_t& pos) {
    question.name = readName(data, pos);
    question.type = readUint16(data, pos);
    question.class_code = readUint16(data, pos);
}

// funçoes de decodificaçao
void DNSMessage::decodeA(ResourceRecord& rr) {
    if (rr.data_length == 4)
        rr.parsed_data = to_string(rr.raw_data[0]) + "." + to_string(rr.raw_data[1]) + "." + to_string(rr.raw_data[2]) + "." + to_string(rr.raw_data[3]);
    else
        rr.parsed_data = "ERRO: Registro A inválido. Esperado 4 bytes, recebeu " + to_string(rr.data_length);
}

void DNSMessage::decodeAAAA(ResourceRecord& rr) {
    if (rr.data_length == 16) {
        char buf[40];
        sprintf(buf, "%x:%x:%x:%x:%x:%x:%x:%x",
                (rr.raw_data[0] << 8) | rr.raw_data[1], (rr.raw_data[2] << 8) | rr.raw_data[3],
                (rr.raw_data[4] << 8) | rr.raw_data[5], (rr.raw_data[6] << 8) | rr.raw_data[7],
                (rr.raw_data[8] << 8) | rr.raw_data[9], (rr.raw_data[10] << 8) | rr.raw_data[11],
                (rr.raw_data[12] << 8) | rr.raw_data[13], (rr.raw_data[14] << 8) | rr.raw_data[15]);
        rr.parsed_data = string(buf);
    } else
        rr.parsed_data = "ERRO: Registro AAAA inválido. Esperado 16 bytes, recebeu " + to_string(rr.data_length);
}

void DNSMessage::decodeCNAME(ResourceRecord& rr) {
    try {
        size_t local_pos = 0;
        rr.parsed_data = readName(rr.raw_data, local_pos);
    } catch (const exception& error) {
        rr.parsed_data = "ERRO ao decodificar CNAME: " + string(error.what());
    }
}

void DNSMessage::decodeNS(ResourceRecord& rr) {
    try {
        size_t local_pos = 0;
        rr.parsed_data = readName(rr.raw_data, local_pos);
    } catch (const exception& error) {
        rr.parsed_data = "ERRO ao decodificar NS: " + string(error.what());
    }
}

void DNSMessage::decodeSOA(ResourceRecord& rr) {
    try {
        size_t local_pos = 0;
        string mname = readName(rr.raw_data, local_pos);
        string rname = readName(rr.raw_data, local_pos);
        if (local_pos + 20 <= rr.raw_data.size()) {
            uint32_t serial = (rr.raw_data[local_pos] << 24) | (rr.raw_data[local_pos+1] << 16) | (rr.raw_data[local_pos+2] << 8) | rr.raw_data[local_pos+3];
            uint32_t refresh = (rr.raw_data[local_pos+4] << 24) | (rr.raw_data[local_pos+5] << 16) | (rr.raw_data[local_pos+6] << 8) | rr.raw_data[local_pos+7];
            uint32_t retry = (rr.raw_data[local_pos+8] << 24) | (rr.raw_data[local_pos+9] << 16) | (rr.raw_data[local_pos+10] << 8) | rr.raw_data[local_pos+11];
            uint32_t expire = (rr.raw_data[local_pos+12] << 24) | (rr.raw_data[local_pos+13] << 16) | (rr.raw_data[local_pos+14] << 8) | rr.raw_data[local_pos+15];
            uint32_t minimum = (rr.raw_data[local_pos+16] << 24) | (rr.raw_data[local_pos+17] << 16) | (rr.raw_data[local_pos+18] << 8) | rr.raw_data[local_pos+19];
            rr.parsed_data = "MNAME: " + mname + ", RNAME: " + rname + ", SERIAL: " + to_string(serial) + ", REFRESH: " + to_string(refresh) + ", RETRY: " + to_string(retry) + ", EXPIRE: " + to_string(expire) + ", MINIMUM: " + to_string(minimum);
        } else
            rr.parsed_data = "ERRO: SOA inválido. Dados insuficientes";
    } catch (const exception& error) {
        rr.parsed_data = "ERRO ao decodificar SOA: " + string(error.what());
    }
}

void DNSMessage::decodeMX(ResourceRecord& rr) {
    if (rr.data_length < 3) {
        rr.parsed_data = "ERRO: MX inválido. data_lengthgth insuficiente: " + to_string(rr.data_length);
        return;
    }
    uint16_t preference = (rr.raw_data[0] << 8) | rr.raw_data[1]; // quanto menor o numero maior a prioridade
    size_t local_pos = 2;
    try {
        string exchange = readName(rr.raw_data, local_pos);
        rr.parsed_data = "Preference: " + to_string(preference) + ", Exchange: " + exchange;
    } catch (const exception& error) {
        rr.parsed_data = "ERRO ao decodificar MX: " + string(error.what());
    }
}

void DNSMessage::decodeTXT(ResourceRecord& rr) {
    try {
        string txt_data;
        size_t local_pos = 0;
        while (local_pos < rr.raw_data.size()) {
            uint8_t len = rr.raw_data[local_pos++];
            if (local_pos + len > rr.raw_data.size()) break;
            if (!txt_data.empty()) txt_data += " ";
            txt_data += string(rr.raw_data.begin() + local_pos, rr.raw_data.begin() + local_pos + len);
            local_pos += len;
        }
        rr.parsed_data = txt_data;
    } catch (const exception& error) {
        rr.parsed_data = "ERRO ao decodificar TXT: " + string(error.what());
    }
}

void DNSMessage::decodeOPT(ResourceRecord& rr) {
    if (rr.data_length == 0) {
        rr.parsed_data = "OPT RR vazio";
        return;
    }

    _edns_udp_size = rr.class_code;
    uint32_t ttl = rr.ttl;
    uint8_t ext_rcode = (ttl >> 24) & 0xFF;
    uint8_t version = (ttl >> 16) & 0xFF;
    uint16_t z = ttl & 0xFFFF;

    _edns_version = version;
    _edns_z_field = z;

    bool dnssec_ok = (z & 0x8000) != 0;

    ostringstream oss;
    oss << "EDNS UDP size=" << _edns_udp_size
        << ", version=" << (int)version
        << ", DO=" << (dnssec_ok ? "1" : "0");

    if (ext_rcode != 0)
        oss << ", extended RCODE=" << (int)ext_rcode;

    // Opções EDNS (se existirem)
    size_t pos = 0;
    edns_options.clear();
    while (pos + 4 <= rr.raw_data.size()) {
        EDNSOption opt;
        opt.code = (rr.raw_data[pos] << 8) | rr.raw_data[pos+1];
        uint16_t len = (rr.raw_data[pos+2] << 8) | rr.raw_data[pos+3];
        pos += 4;

        if (pos + len > rr.raw_data.size())
            break;

        opt.data.assign(rr.raw_data.begin() + pos, rr.raw_data.begin() + pos + len);
        pos += len;
        edns_options.push_back(opt);
    }

    if (!edns_options.empty())
        oss << ", opções=" << edns_options.size();

    rr.parsed_data = oss.str();
}

void DNSMessage::decodeDS(ResourceRecord& rr) {
    rr.parsed_data = "Registro DS (Delegation Signer), dados brutos: " + to_string(rr.data_length) + " bytes";
}

void DNSMessage::decodeRRSIG(ResourceRecord& rr) {
    rr.parsed_data = "RRSIG RR (assinatura DNSSEC), dados brutos: " + to_string(rr.data_length) + " bytes";
}

void DNSMessage::decodeDNSKEY(ResourceRecord& rr) {
    rr.parsed_data = "DNSKEY RR (chave pública), dados brutos: " + to_string(rr.data_length) + " bytes";
}

void DNSMessage::readAnswers(const vector<uint8_t>& data, size_t& pos)
{
    for (uint16_t i = 0; i < header.answer_count; ++i)
        answers.push_back(readRecord(data, pos));
}

void DNSMessage::readAuthorities(const vector<uint8_t>& data, size_t& pos)
{
    for (uint16_t i = 0; i < header.name_server_count; ++i)
        authorities.push_back(readRecord(data, pos));
}

void DNSMessage::readAdditionals(const vector<uint8_t>& data, size_t& pos)
{
    for (uint16_t i = 0; i < header.additional_count; ++i)
        additionals.push_back(readRecord(data, pos));
}

// Classes u
string qtypeToString(uint16_t qtype) {
    const static map<uint16_t, string> type_map = {
        {1, "A"}, {2, "NS"}, {5, "CNAME"}, {6, "SOA"}, {15, "MX"},
        {16, "TXT"}, {28, "AAAA"}, {41, "OPT"}, {43, "DS"},
        {46, "RRSIG"}, {48, "DNSKEY"}
    };
    auto it = type_map.find(qtype);
    if (it != type_map.end()) {
        return it->second;
    }
    return to_string(qtype);
}

string qclassToString(uint16_t qclass) {
    return (qclass == 1) ? "IN" : to_string(qclass);
}

void DNSMessage::printResponse() const
{
    // Códigos de cores ANSI
    const std::string CLR_HEADER = "\033[1;34m"; // Azul Negrito
    const std::string CLR_LABEL  = "\033[1;37m"; // Branco Negrito
    const std::string CLR_VALUE  = "\033[0;33m"; // Amarelo
    const std::string CLR_RESET  = "\033[0m";
    const std::string CLR_GRAY   = "\033[90m";

    // --- CABEÇALHO ---
    std::cout << CLR_HEADER << "\n┌──────────────────  CABEÇALHO DNS  ──────────────────┐" << CLR_RESET << std::endl;
    std::cout << CLR_LABEL << "  ID: " << CLR_VALUE << header.id << CLR_RESET;
    std::cout << CLR_LABEL << "                Flags: " << CLR_VALUE << "0x" 
              << std::hex << std::setw(4) << std::setfill('0') << header.flags << std::dec
              << std::setfill(' ') 
              << CLR_RESET << std::endl;
    
    std::bitset<16> flags(header.flags);
    std::cout << CLR_GRAY << "     └─ "
              << "QR:" << flags[15] << " AA:" << flags[10] << " TC:" << flags[9] 
              << " RD:" << flags[8] << " RA:" << flags[7] << " RCODE:" << (header.flags & 0xF)
              << CLR_RESET << std::endl;

    std::cout << CLR_LABEL << "  Perguntas: " << CLR_VALUE << header.query_domain_count << CLR_RESET;
    std::cout << CLR_LABEL << "     Respostas: " << CLR_VALUE << header.answer_count << CLR_RESET;
    std::cout << CLR_LABEL << "     Autoridades: " << CLR_VALUE << header.name_server_count << CLR_RESET;
    std::cout << CLR_LABEL << "     Adicionais: " << CLR_VALUE << header.additional_count << CLR_RESET << std::endl;
    std::cout << CLR_HEADER << "└─────────────────────────────────────────────────────┘" << CLR_RESET << std::endl;

    // --- SEÇÕES DE REGISTROS ---
    const auto print_section = [&](const std::string& title, const std::vector<ResourceRecord>& records) {
        std::cout << CLR_HEADER << "\n;; " << title << " SECTION:" << CLR_RESET << std::endl;
        if (records.empty()) {
            std::cout << CLR_GRAY << "; (Seção vazia)" << CLR_RESET << std::endl;
            return;
        }
        for (const auto& rr : records) {
            std::cout << std::left << std::setw(30) << rr.name
                      << std::right << std::setw(8) << rr.ttl << " "
                      << std::left << std::setw(8) << qclassToString(rr.class_code)
                      << std::setw(8) << qtypeToString(rr.type)
                      << CLR_VALUE << rr.parsed_data << CLR_RESET << std::endl;
        }
    };
    
    std::cout << CLR_HEADER << "\n;; QUESTION SECTION:" << CLR_RESET << std::endl;
    std::cout << std::left << std::setw(30) << question.name
              << std::right << std::setw(8) << "" << " "
              << std::left << std::setw(8) << qclassToString(question.class_code)
              << std::setw(8) << qtypeToString(question.type)
              << CLR_VALUE << "" << CLR_RESET << std::endl;

    print_section("ANSWER", answers);
    print_section("AUTHORITY", authorities);
    print_section("ADDITIONAL", additionals);
    std::cout << std::endl;
}