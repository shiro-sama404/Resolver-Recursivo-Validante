#include "dot_cliente.hpp"

using namespace std;

DOTCliente::DOTCliente(const string& servidor, int porta) : servidor(servidor), porta(porta), socket_fd(-1) {

    mbedtls_ssl_init(&tls_session);
    mbedtls_ssl_config_init(&tls_config);
    mbedtls_ctr_drbg_init(&random_generator);
    mbedtls_entropy_init(&entropy_source);
    mbedtls_x509_crt_init(&trusted_cert);
}

static int enviar_dados(void* ctx_socket, const unsigned char* dados_enviar, size_t tam_dados) {
    int descriptor_socket = *static_cast<int*>(ctx_socket);
    int bytes_send = send(descriptor_socket, dados_enviar, tam_dados, 0);
    return bytes_send;
}

static int receber_dados(void* ctx_socket, unsigned char* dados_enviar, size_t tam_dados) {
    int descriptor_socket = *static_cast<int*>(ctx_socket);
    int bytes_recv = recv(descriptor_socket, dados_enviar, tam_dados, 0);
    return bytes_recv;
}

void DOTCliente::fecharConexao() {
    if (socket_fd != -1) {
        close(socket_fd); 
        socket_fd = -1;    
    }
}


bool DOTCliente::conectar(int timeout_seg) {
    
    int result = mbedtls_x509_crt_parse_file(&trusted_cert, "cacert.pem");
    
    if (result != 0)
        throw runtime_error("Erro ao carregar cacert.pem: " + to_string(result));


    const char* personalisar = "dot_client";

    int result = mbedtls_ctr_drbg_seed(&random_generator, mbedtls_entropy_func, &entropy_source, 
                        reinterpret_cast<const unsigned char*>(personalisar), strlen(personalisar));

    if (result != 0)
        throw runtime_error("Erro ao inicializar RNG: " + to_string(result));


    struct addrinfo hints{};
    struct addrinfo* result_rede;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    if (getaddrinfo(servidor.c_str(), to_string(porta).c_str(), &hints, &result_rede) != 0)
        throw runtime_error("Falha ao resolver endereço do servidor " + servidor + " na porta " + to_string(porta));

    
        socket_fd = socket(result_rede->ai_family, result_rede->ai_socktype, result_rede->ai_protocol);
    
    
    if (socket_fd < 0)
        throw runtime_error("Falha ao criar socket");

    int flags = fcntl(socket_fd, F_GETFL, 0);
   
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    result = connect(socket_fd, result_rede->ai_addr, result_rede->ai_addrlen);
    
    if (result < 0 && errno != EINPROGRESS) {
        freeaddrinfo(result_rede);
        throw runtime_error("Erro ao conectar socket");
    }

    if (result < 0) {
        fd_set sockets_escrita;
        FD_ZERO(&sockets_escrita);
        FD_SET(socket_fd, &sockets_escrita);
       
        struct timeval tempo_limite{timeout_seg, 0};
        
        result = select(socket_fd + 1, nullptr, &sockets_escrita, nullptr, &tempo_limite);
        
        if (result <= 0) {
            freeaddrinfo(result_rede);
            fecharConexao();
            throw runtime_error("Timeout ao conectar ao servidor " + servidor);
        }

        int cod_erro = 0;
        socklen_t tam_erro = sizeof(cod_erro);
       
        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &cod_erro, &tam_erro) < 0 || cod_erro != 0) {
            
            freeaddrinfo(result_rede);
            fecharConexao();
            
            throw runtime_error("Erro na conexão TCP: " + to_string(cod_erro));
        }
    }

    freeaddrinfo(result_rede);
    fcntl(socket_fd, F_SETFL, flags); // volta para bloqueante


    mbedtls_ssl_config_defaults(&tls_config,MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_authmode(&tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&tls_config, mbedtls_ctr_drbg_random, &random_generator);

    result = mbedtls_ssl_setup(&tls_session, &tls_config);


    if (result != 0)
        throw runtime_error("Falha ao configurar SSL: " + to_string(result));


    result = mbedtls_ssl_set_hostname(&tls_session, servidor.c_str());


    if (result != 0)
        throw runtime_error("Falha ao configurar SNI: " + to_string(result));


    mbedtls_ssl_set_bio(&tls_session, &socket_fd, enviar_dados, receber_dados, nullptr);

    auto start = chrono::steady_clock::now();

    while ((result = mbedtls_ssl_handshake(&tls_session)) != 0) {
      
        if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE)
            throw runtime_error("Falha handshake TLS: " + to_string(result));

        auto tempo = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
       
        if (tempo > timeout_seg) {
            fecharConexao();
            throw runtime_error("Timeout no handshake TLS");
        }
        
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    uint32_t result_validacao = mbedtls_ssl_get_verify_result(&tls_session);

    if (result_validacao != 0) {

        char msg_erro[512];
        mbedtls_x509_crt_verify_info(msg_erro, sizeof(msg_erro), "", result_validacao);
        
        throw runtime_error(string("Certificado inválido: ") + msg_erro);
    }

    return true;
}


bool enviarQuery(DNSMensagem& msg, int timeout_ms) {
    std::vector<uint8_t> pacote = msg.montarQuery();
    uint16_t tam = pacote.size();
    std::vector<uint8_t> buffer;
    buffer.push_back(tam >> 8);
    buffer.push_back(tam & 0xFF);
    buffer.insert(buffer.end(), pacote.begin(), pacote.end());

    size_t total = 0;
    auto start = std::chrono::steady_clock::now();
    
    // Configura o timeout para a operação de escrita
    fd_set wfds;
    struct timeval tv;

    while (total < buffer.size()) {
        FD_ZERO(&wfds);
        FD_SET(socket_fd, &wfds);

        long long remaining_ms = timeout_ms - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (remaining_ms <= 0) {
            return false; // Timeout
        }
        tv.tv_sec = remaining_ms / 1000;
        tv.tv_usec = (remaining_ms % 1000) * 1000;

        int sel = select(socket_fd + 1, nullptr, &wfds, nullptr, &tv);

        if (sel < 0) {
            return false; // Erro em select
        }
        if (sel == 0) {
            return false; // Timeout
        }
        if (!FD_ISSET(socket_fd, &wfds)) {
            continue; // Socket não está pronto para escrita, continue
        }

        int ret = mbedtls_ssl_write(&tls_session, buffer.data() + total, buffer.size() - total);
        if (ret > 0) {
            total += ret;
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false;
        }
    }
    return true;
}

bool receberResposta(DNSMensagem& msg, int timeout_ms) {
        uint8_t prefixo[2];
        size_t lido = 0;
        auto start = std::chrono::steady_clock::now();

        // lê tamanho
        while (lido < 2) {
            int ret = mbedtls_ssl_read(&ssl, prefixo + lido, 2 - lido);
            if (ret > 0) lido += ret;
            else if (ret == 0) throw std::runtime_error("Conexão TLS fechada");
            else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
                throw std::runtime_error("Erro TLS ao ler tamanho");

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            if (elapsed > timeout_ms) throw std::runtime_error("Timeout na leitura do tamanho");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        uint16_t tam = (prefixo[0] << 8) | prefixo[1];
        if (tam < 12 || tam > 4096) throw std::runtime_error("Tamanho DNS inválido");

        std::vector<uint8_t> buffer(tam);
        size_t total_lido = 0;

        // lê payload
        while (total_lido < tam) {
            int ret = mbedtls_ssl_read(&ssl, buffer.data() + total_lido, tam - total_lido);
            if (ret > 0) total_lido += ret;
            else if (ret == 0) throw std::runtime_error("Conexão TLS fechada");
            else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
                throw std::runtime_error("Erro TLS ao ler payload");

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            if (elapsed > timeout_ms) throw std::runtime_error("Timeout na leitura do payload");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        msg.parseResposta(buffer);
        return true;
}

bool DOTCliente::receberResposta(DNSMensagem& mensagem, int timeout_ms) {
    std::vector<uint8_t> buffer_recebido;
    uint16_t tamanho_pacote = 0;
    bool tamanho_lido = false;

    size_t bytes_totais = 0;
    auto inicio = std::chrono::steady_clock::now();

    fd_set conjunto_leitura;
    struct timeval tempo_limite;

    while (true) {
        FD_ZERO(&conjunto_leitura);
        FD_SET(socket_fd, &conjunto_leitura);

        // Calcula tempo restante para timeout
        long long tempo_passado_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - inicio)
                                         .count();
        long long tempo_restante_ms = timeout_ms - tempo_passado_ms;
        if (tempo_restante_ms <= 0) return false; // Timeout

        tempo_limite.tv_sec = tempo_restante_ms / 1000;
        tempo_limite.tv_usec = (tempo_restante_ms % 1000) * 1000;

        int select_resultado = select(socket_fd + 1, &conjunto_leitura, nullptr, nullptr, &tempo_limite);
        if (select_resultado < 0) return false; // Erro no select
        if (select_resultado == 0) return false; // Timeout
        if (!FD_ISSET(socket_fd, &conjunto_leitura)) continue; // Socket ainda não pronto

        // Recebe dados do socket TLS
        uint8_t temp_buffer[512];
        int bytes_recebidos = mbedtls_ssl_read(&tls_session, temp_buffer, sizeof(temp_buffer));

        if (bytes_recebidos > 0) {
            // Insere dados recebidos no buffer
            buffer_recebido.insert(buffer_recebido.end(), temp_buffer, temp_buffer + bytes_recebidos);

            // Lê tamanho do pacote se ainda não leu
            if (!tamanho_lido && buffer_recebido.size() >= 2) {
                tamanho_pacote = (buffer_recebido[0] << 8) | buffer_recebido[1];
                tamanho_lido = true;
            }

            // Checa se todo o pacote foi recebido
            if (tamanho_lido && buffer_recebido.size() >= tamanho_pacote + 2) {
                std::vector<uint8_t> pacote_final(buffer_recebido.begin() + 2, buffer_recebido.begin() + 2 + tamanho_pacote);
                mensagem.parseResposta(pacote_final);
                return true;
            }
        } else if (bytes_recebidos == 0) {
            return false; // Conexão fechada
        } else if (bytes_recebidos != MBEDTLS_ERR_SSL_WANT_READ &&
                   bytes_recebidos != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false; // Erro de leitura
        }
    }
}




DOTCliente::~DOTCliente() {
    fecharConexao();

    mbedtls_ssl_free(&tls_session);
    mbedtls_ssl_config_free(&tls_config);
    mbedtls_ctr_drbg_free(&random_generator);
    mbedtls_entropy_free(&entropy_source);
    mbedtls_x509_crt_free(&trusted_cert);
}