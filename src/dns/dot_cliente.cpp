#include "dot_cliente.hpp"

using namespace std;

DOTCliente::DOTCliente(const string& servidor, int porta) : servidor(servidor), porta(porta), socket_fd(-1) {

    mbedtls_ssl_init(&tls_session);
    mbedtls_ssl_config_init(&tls_config);
    mbedtls_ctr_drbg_init(&random_generator);
    mbedtls_entropy_init(&entropy_source);
    mbedtls_x509_crt_init(&trusted_cert);
}

int DOTCliente::enviar_dados(void* ctx_socket, const unsigned char* dados_enviar, size_t tam_dados) {
    int descriptor_socket = *static_cast<int*>(ctx_socket);
    int bytes_send = send(descriptor_socket, dados_enviar, tam_dados, 0);
    return bytes_send;
}

int DOTCliente::receber_dados(void* ctx_socket, unsigned char* dados_enviar, size_t tam_dados) {
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

DOTCliente::~DOTCliente() {
    fecharConexao();

    mbedtls_ssl_free(&tls_session);
    mbedtls_ssl_config_free(&tls_config);
    mbedtls_ctr_drbg_free(&random_generator);
    mbedtls_entropy_free(&entropy_source);
    mbedtls_x509_crt_free(&trusted_cert);
}

bool DOTCliente::conectar(int timeout_seg) {
    
    int certificados = mbedtls_x509_crt_parse_file(&trusted_cert, "dns/cacert.pem");
    
    if (certificados != 0)
        throw runtime_error("Erro ao carregar cacert.pem: " + to_string(certificados));


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


bool DOTCliente::enviarQuery(DNSMensagem& msg, int timeout_ms) {
    vector<uint8_t> pacote = msg.montarQuery();
    uint16_t tam_pct = pacote.size();
    vector<uint8_t> buffer_envio;
   
    buffer_envio.push_back(tam_pct >> 8);
    buffer_envio.push_back(tam_pct & 0xFF);
    buffer_envio.insert(buffer_envio.end(), pacote.begin(), pacote.end());

    size_t total_bytes = 0;
    auto start = chrono::steady_clock::now();
    
    fd_set escritores;
    struct timeval tempo_limite;

    while (total_bytes < buffer_envio.size()) {
        FD_ZERO(&escritores);
        FD_SET(socket_fd, &escritores);


        long long temp_decorrido_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
        long long tempo_restante_ms = timeout_ms - temp_decorrido_ms;


        if (tempo_restante_ms <= 0) return false; 

        tempo_limite.tv_sec = tempo_restante_ms / 1000;
        tempo_limite.tv_usec = (tempo_restante_ms % 1000) * 1000;


        int socket_pronto = select(socket_fd + 1, nullptr, &escritores, nullptr, &tempo_limite);

        if (socket_pronto < 0) 
            return false; // Erro
        
        if (socket_pronto == 0) 
            return false; // Timeout

        if (!FD_ISSET(socket_fd, &escritores)) 
            continue; // socket nao esta pronto

        int bytes_enviados = mbedtls_ssl_write(&tls_session, buffer_envio.data() + total_bytes, buffer_envio.size() - total_bytes);


        if (bytes_enviados > 0) {
            total_bytes += bytes_enviados;

        } 
        
        else if (bytes_enviados != MBEDTLS_ERR_SSL_WANT_READ && bytes_enviados != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false; // Erro de escrita
        }
    }

    return true;
}



bool DOTCliente::receberResposta(DNSMensagem& mensagem, int timeout_ms) {
    vector<uint8_t> pct_recebido;
    uint16_t tam_pct = 0;
    bool vrfy_pct = false;

    //size_t bytes_totais = 0;
    auto start = chrono::steady_clock::now();

    fd_set leitores;
    struct timeval tempo_limite;

    while (true) {
        FD_ZERO(&leitores);
        FD_SET(socket_fd, &leitores);

        
        long long tempo_decorrido_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
        long long tempo_restante_ms = timeout_ms - tempo_decorrido_ms;
        
        
        if (tempo_restante_ms <= 0) 
            return false; // Timeout

        tempo_limite.tv_sec = tempo_restante_ms / 1000;
        tempo_limite.tv_usec = (tempo_restante_ms % 1000) * 1000;

        int socket_pronto = select(socket_fd + 1, &leitores, nullptr, nullptr, &tempo_limite);
        
        if (socket_pronto < 0) 
            return false; 

        if (socket_pronto == 0) 
            return false; 

        if (!FD_ISSET(socket_fd, &leitores)) 
            continue; 


        uint8_t temp_buffer[512];
        int bytes_recebidos = mbedtls_ssl_read(&tls_session, temp_buffer, sizeof(temp_buffer));

        if (bytes_recebidos > 0) {
            pct_recebido.insert(pct_recebido.end(), temp_buffer, temp_buffer + bytes_recebidos);


            if (!vrfy_pct && pct_recebido.size() >= 2) {
                tam_pct = (pct_recebido[0] << 8) | pct_recebido[1];
                vrfy_pct = true;
            }

            if (vrfy_pct && pct_recebido.size() >= tam_pct + 2) {
                vector<uint8_t> pacote_final(pct_recebido.begin() + 2, pct_recebido.begin() + 2 + tam_pct);
                mensagem.parseResposta(pacote_final);
                return true;
            }

        } 
        else if (bytes_recebidos == 0) {
            return false;

        } 
        else if (bytes_recebidos != MBEDTLS_ERR_SSL_WANT_READ && bytes_recebidos != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false;
        }
    }
}