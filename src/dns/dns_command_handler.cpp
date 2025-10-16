#include "dns_command_handler.hpp"

#include <iostream>
#include <sstream>

using namespace std;

int DNSCommandHandler::execute()
{
    if (!validateArgs())
        return EXIT_FAILURE;
    _args.printSummary();

    CacheKey cache_key = {
        _args.getName(),
        qtypeToUint16(_args.getQtype()),
        1 // qclass IN
    };

    if (CacheClient::isCacheActive())
    {
        stringstream get_cmd;
        get_cmd << "GET " << cache_key.qname << " " << cache_key.qtype << " " << cache_key.qclass;
        string cached_response = CacheClient::sendCommand(get_cmd.str());

        if (cached_response.find("[CACHE HIT]") == string::npos)
        {
            stringstream get_negative_cmd;
            get_negative_cmd << "GET NEGATIVE" << cache_key.qname << " " << cache_key.qtype << " " << cache_key.qclass;
            cached_response = CacheClient::sendCommand(get_negative_cmd.str());
        }
        
        if (cached_response.find("[CACHE HIT]") != string::npos)
        {
            cout << "\n[CACHE] Resposta encontrada na cache:" << endl;
            cout << cached_response << endl;
            return EXIT_SUCCESS;
        }
        cout << "\n[DNS] Cache miss. Realizando resolução de rede..." << endl;
    }

    try{
        configureClient(_args.getMode());
    }
    catch(const exception& e){
        cerr << "Erro: " << e.what() << endl;
        return EXIT_FAILURE; 
    }
    vector<uint8_t> response_bytes = _client.resolve(cache_key.qname, cache_key.qtype);

    if (CacheClient::isCacheActive() && !response_bytes.empty())
    {
        DNSMessage response_message;
        response_message.parseResponse(response_bytes);
        uint8_t rcode = response_message.getRcode();
        
        stringstream put_cmd;
        if (rcode == 0 && !response_message.answers.empty()) 
        {
            // Resposta Positiva
            const auto& first_answer = response_message.answers[0];
            put_cmd << "PUT " << cache_key.qname << " " << cache_key.qtype << " " << cache_key.qclass
                    << " \"" << first_answer.parsed_data << "\" " 
                    << first_answer.ttl << " " << 0; 
        }
        else 
        {
            // Resposta Negativa
            string reason = (rcode == 3) ? "NXDOMAIN" : "NODATA";
            put_cmd << "PUT_NEGATIVE " << cache_key.qname << " " << cache_key.qtype << " " << cache_key.qclass
                    << " " << reason << " " << 300;
        }
        cout << endl << "[CACHE] Cache atualizada com a nova resposta:" << endl;
        cout << CacheClient::sendCommand(put_cmd.str());
    }

    processResponse(response_bytes);

    return EXIT_SUCCESS;
}

bool DNSCommandHandler::validateArgs()
{
    if (_args.getName().empty())
    {
        cerr << "Nome de domínio (--name) não informado." << endl;
        return false;
    }

    if (_args.getQtype().empty())
    {
        cerr << "Tipo de consulta (--qtype) não informado." << endl;
        return false;
    }

    if (_args.getNs().empty() && _args.getMode() == Mode::Forwarder)
    {
        cerr << "Servidor NS (--ns) obrigatório no modo forwarder." << endl;
        return false;
    }    
    return true;
}

void DNSCommandHandler::configureClient(Mode mode)
{
    switch (mode) {
        case Mode::Recursive:
            _client.setRecursion(true);
            _client.setUseDot(false);
            _client.setValidateDnssec(_args.getTrustAnchor().has_value());
            break;

        case Mode::Forwarder:
            _client.setRecursion(true);
            _client.setForwarder(_args.getNs());
            break;

        case Mode::Iterative:
            _client.setRecursion(false);
            break;

        case Mode::Validating:
            _client.setRecursion(true);
            _client.setValidateDnssec(true);
            break;

        case Mode::Insecure:
            _client.setRecursion(true);
            _client.setValidateDnssec(false);
            break;

        case Mode::StrictDNSSEC:
            _client.setRecursion(true);
            _client.setStrictDnssec(true);
            break;

        case Mode::Dot:
            _client.setRecursion(true);
            _client.setUseDot(true);
            if (_args.getSni().has_value())
                _client.setSni(_args.getSni().value());
            break;

        default:
            throw invalid_argument("Modo (--mode) inválido ou não especificado.");
    }
}

void DNSCommandHandler::processResponse(const vector<uint8_t>& response_bytes)
{
    if (response_bytes.empty()) 
    {
        cerr << "\n\nNao encontrou resposta para " << _args.getName() << endl;
        return;
    }

    DNSMessage response;
    response.parseResponse(response_bytes);

    cout << "\n\n================== RESPOSTA DO RESOLVEDOR ==================" << endl;
    cout << "==============--------------------------------==============\n" << endl;
    response.printResponse();

    uint8_t rcode = response.getRcode();
    if (rcode != 0) {
        cerr << "Aviso: resposta DNS contém erro (RCODE=" << static_cast<int>(rcode) << ").\n";
    }
}