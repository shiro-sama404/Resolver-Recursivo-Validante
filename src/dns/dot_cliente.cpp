#include "dot_cliente.hpp"

using namespace std;

DOTCliente::DOTCliente(const string& servidor, uint16_t porta) : servidor(servidor), porta(porta) {

    mbedtls_net_init(&network_socket);
    mbedtls_ssl_init(&tls_session);
    mbedtls_ssl_config_init(&tls_config);
    mbedtls_entropy_init(&entropy_source);
    mbedtls_ctr_drbg_init(&random_generator);
    mbedtls_x509_crt_init(&trusted_cert);

}

/*
bool DOTCliente::handshakeTLS() {

    bool DOTCliente::handshakeTLS() {
    int ret;

    // Inicializa contexto SSL
    mbedtls_ssl_init(&ssl_context);
    mbedtls_ssl_config_init(&ssl_config);

    // Configura SSL padrão para cliente
    ret = mbedtls_ssl_config_defaults(
        &ssl_config,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    if (ret != 0) {
        cerr << "Erro ao configurar SSL: " << ret << endl;
        return false;
    }

    // Usa o RNG inicializado
    mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &random_generator);

    // Configura os certificados confiáveis
    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED); // obriga verificação
    mbedtls_ssl_conf_ca_chain(&ssl_config, &trusted_cert, nullptr);

    // Associa configuração ao contexto
    ret = mbedtls_ssl_setup(&ssl_context, &ssl_config);
    if (ret != 0) {
        cerr << "Erro ao associar configuração SSL: " << ret << endl;
        return false;
    }

    // Associa socket TCP ao contexto SSL
    mbedtls_ssl_set_bio(&ssl_context, &network_socket, mbedtls_net_send, mbedtls_net_recv, nullptr);

    // Realiza handshake
    while ((ret = mbedtls_ssl_handshake(&ssl_context)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            cerr << "Handshake TLS falhou: " << ret << endl;
            return false;
        }
    }

    // Verifica certificado do servidor
    uint32_t flags = mbedtls_ssl_get_verify_result(&ssl_context);
    if (flags != 0) {
        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", flags);
        cerr << "Erro de verificação do certificado: " << vrfy_buf << endl;
        return false;
    }

    return true;
}
*/

/*
    // inicia a conexão com tls lado cliente
    if (mbedtls_ssl_config_defaults(&tls_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        throw runtime_error("Erro ao configurar SSL");
    }



    // manda fazer verificações de segurança
    mbedtls_ssl_conf_authmode(&tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&tls_config, &trusted_cert, nullptr);
    mbedtls_ssl_conf_rng(&tls_config, mbedtls_ctr_drbg_random, &random_generator);



    // estabelece conexão segura
    if (mbedtls_ssl_setup(&tls_session, &tls_config) != 0) {
        throw runtime_error("Erro ao inicializar contexto SSL");
    }




    // define sni - qual servidor quero acessar
    if (mbedtls_ssl_set_hostname(&tls_session, servidor.c_str()) != 0) {
        throw runtime_error("Erro ao configurar SNI");
    }



    mbedtls_ssl_set_bio(&tls_session, &network_socket, mbedtls_net_send, mbedtls_net_recv, nullptr);
    int status;



    // handshake
    while ((status = mbedtls_ssl_handshake(&tls_session)) != 0) {

        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            char buf[100];
            mbedtls_strerror(status, buf, sizeof(buf));
            throw runtime_error(string("Erro no handshake TLS: ") + buf);
        }
    }



    // valida certificado
    uint32_t flags = mbedtls_ssl_get_verify_result(&tls_session);



    if (flags != 0) {

        char msg_erro[512];
        mbedtls_x509_crt_verify_info(msg_erro, sizeof(msg_erro), "", flags);
        throw runtime_error(string("Falha na validação do certificado: ") + msg_erro);

    }
    return true;
}





bool DOTCliente::conectar() {
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt_init(&cacert);

    // carregar os certificados do arquivo local
    int ret = mbedtls_x509_crt_parse_file(&cacert, "cacert.pem");
    if (ret != 0) {
        cerr << "Erro ao carregar certificados CA: " << ret << endl;
        return false;
    }

    bool DOTCliente::conectar() {
    // Inicializa CA root
    mbedtls_x509_crt_init(&trusted_cert);
    int ret = mbedtls_x509_crt_parse_file(&trusted_cert, "cacert.pem"); // usa arquivo do mesmo diretório
    if (ret != 0) {
        cerr << "Erro ao carregar certificados CA: " << ret << endl;
        return false;
    }

    const char* personalization = "dot_client";

    // Inicializa RNG
    ret = mbedtls_ctr_drbg_seed(&random_generator, mbedtls_entropy_func, &entropy_source,
                                reinterpret_cast<const unsigned char*>(personalization),
                                strlen(personalization));
    if (ret != 0) {
        throw runtime_error("Erro ao inicializar RNG");
    }

    // Conecta TCP ao servidor DoT
    ret = mbedtls_net_connect(&network_socket, servidor.c_str(), to_string(porta).c_str(), MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        throw runtime_error("Erro ao conectar ao servidor " + servidor);
    }

    // Realiza handshake TLS
    if (!handshakeTLS()) {
        fecharConexao();
        throw runtime_error("Falha no handshake TLS com " + servidor);
    }

    return true;
}
*/

/*
    const char* personalisar = "dot_client";

    // inicializa random number generator
    if (mbedtls_ctr_drbg_seed(&random_generator, mbedtls_entropy_func, &entropy_source, reinterpret_cast<const unsigned char*>(personalisar),
                              strlen(personalisar)) != 0) {

        throw runtime_error("Erro ao inicializar RNG"); // random number generator
    }



    // carrega certificados confiáveis
    if (mbedtls_x509_crt_parse_path(&trusted_cert, "/etc/ssl/certs") != 0) {
        throw runtime_error("Erro ao carregar certificados CA");
    }



    // conecta tcp ao dot
    if (mbedtls_net_connect(&network_socket, servidor.c_str(), to_string(porta).c_str(), MBEDTLS_NET_PROTO_TCP) != 0) {
        throw runtime_error("Erro ao conectar ao servidor " + servidor);
    }



    if (!handshakeTLS()) { 
        fecharConexao();
        throw runtime_error("Falha no handshake TLS com " + servidor);
    }

    return true;
}
*/

// conectar() — carrega cacert.pem em trusted_cert, inicializa RNG, conecta e faz handshake
bool DOTCliente::conectar() {
    // Carrega o CA bundle (arquivo colocado no diretório src/dns)
    int ret = mbedtls_x509_crt_parse_file(&trusted_cert, "cacert.pem");
    if (ret != 0) {
        throw std::runtime_error(std::string("Erro ao carregar cacert.pem: ") + std::to_string(ret));
    }

    // Inicializa o RNG (importantíssimo antes do handshake/config)
    const char* personalization = "dot_client";
    ret = mbedtls_ctr_drbg_seed(&random_generator, mbedtls_entropy_func, &entropy_source,
                                reinterpret_cast<const unsigned char*>(personalization),
                                std::strlen(personalization));
    if (ret != 0) {
        throw std::runtime_error(std::string("Erro ao inicializar RNG: ") + std::to_string(ret));
    }

    // Conecta TCP ao servidor DoT (porta 853 por padrão)
    ret = mbedtls_net_connect(&network_socket, servidor.c_str(), std::to_string(porta).c_str(), MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        throw std::runtime_error(std::string("Erro ao conectar ao servidor ") + servidor + ": " + std::to_string(ret));
    }

    // Realiza handshake (essa função assume trusted_cert e random_generator já inicializados)
    if (!handshakeTLS()) {
        fecharConexao();
        throw std::runtime_error(std::string("Falha no handshake TLS com ") + servidor);
    }

    return true;
}


// handshakeTLS() — configura SSL/TLS, SNI e valida o certificado
bool DOTCliente::handshakeTLS() {
    int ret;

    // Configura defaults do SSL (cliente)
    ret = mbedtls_ssl_config_defaults(&tls_config,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        char errbuf[200];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(std::string("Erro ao configurar SSL: ") + errbuf);
    }

    // Exigir verificação de certificado e informar a cadeia CA carregada
    mbedtls_ssl_conf_authmode(&tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&tls_config, &trusted_cert, nullptr);

    // Configura RNG (usa o random_generator que já foi seedado em conectar())
    mbedtls_ssl_conf_rng(&tls_config, mbedtls_ctr_drbg_random, &random_generator);

    // Associa config ao contexto TLS/SSL
    ret = mbedtls_ssl_setup(&tls_session, &tls_config);
    if (ret != 0) {
        char errbuf[200];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(std::string("Erro ao inicializar contexto SSL: ") + errbuf);
    }

    // Configura SNI (hostname) para validação correta do certificado
    ret = mbedtls_ssl_set_hostname(&tls_session, servidor.c_str());
    if (ret != 0) {
        char errbuf[200];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(std::string("Erro ao configurar SNI: ") + errbuf);
    }

    // Liga I/O (socket) ao contexto TLS
    mbedtls_ssl_set_bio(&tls_session, &network_socket, mbedtls_net_send, mbedtls_net_recv, nullptr);

    // Handshake: aceita WANT_READ/WANT_WRITE e tenta até completar ou erro fatal
    while ((ret = mbedtls_ssl_handshake(&tls_session)) != 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue; // tente novamente
        } else {
            char errbuf[200];
            mbedtls_strerror(ret, errbuf, sizeof(errbuf));
            throw std::runtime_error(std::string("Erro no handshake TLS: ") + errbuf);
        }
    }

    // Validação do certificado peer
    uint32_t flags = mbedtls_ssl_get_verify_result(&tls_session);
    if (flags != 0) {
        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", flags);
        throw std::runtime_error(std::string("Falha na validação do certificado: ") + vrfy_buf);
    }

    return true;
}


bool DOTCliente::enviarQuery(DNSMensagem& msg) {

    vector<uint8_t> pacote = msg.montarQuery();
    uint16_t tamanho_pct = pacote.size();
    vector<uint8_t> buffer_tls;

    buffer_tls.push_back(tamanho_pct >> 8);
    buffer_tls.push_back(tamanho_pct & 0xFF);
    buffer_tls.insert(buffer_tls.end(), pacote.begin(), pacote.end());

    int bytes_enviados = mbedtls_ssl_write(&tls_session, buffer_tls.data(), buffer_tls.size());
    bool result = (bytes_enviados > 0);
    
    return result;

}



bool DOTCliente::receberResposta(DNSMensagem& msg) {


    uint8_t prefixo[2];      
    int prefixo_lido = 0;


    // lê o tamanho da msg
    while (prefixo_lido < 2) {
        int status = mbedtls_ssl_read(&tls_session, prefixo + prefixo_lido, 2 - prefixo_lido);

        if (status > 0) {  
            prefixo_lido += status;
            continue;
        }


        if (status == 0)
            throw runtime_error("Conexao TLS fechada pelo servidor ao ler tamanho");


        // não foi possivel completar sua chamada, tente novamente
        if (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue; 


        char msg_erro[100];
        mbedtls_strerror(status, msg_erro, sizeof(msg_erro));
        throw runtime_error(string("Erro ao ler tamanho da resposta TLS: ") + msg_erro);
    }


    uint16_t tam_resposta = (static_cast<uint16_t>(prefixo[0]) << 8) | prefixo[1];
    const uint16_t MAX_DNS_PACKET = 4096;
    
    
    if (tam_resposta < 12 || tam_resposta > MAX_DNS_PACKET)
        throw runtime_error("Tamanho da resposta DNS invalido: " + to_string(tam_resposta));


    vector<uint8_t> buffer_resposta(tam_resposta);
    size_t total_lido = 0;


    // lê a msg
    while (total_lido < tam_resposta) {

        int status = mbedtls_ssl_read(&tls_session, buffer_resposta.data() + total_lido, tam_resposta - total_lido);


        if (status > 0) {
            total_lido += status;
            continue;
        }


        if (status == 0)
            throw runtime_error("Conexao TLS fechada durante leitura do payload");


        if (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;


        char msg_erro[100];
        mbedtls_strerror(status, msg_erro, sizeof(msg_erro));
        throw runtime_error(string("Erro ao ler resposta TLS: ") + msg_erro);
    }


    msg.parseResposta(buffer_resposta);
    return true;
}


void DOTCliente::fecharConexao() {
    mbedtls_ssl_close_notify(&tls_session);  
    mbedtls_net_free(&network_socket);      
}

DOTCliente::~DOTCliente() {

    fecharConexao();
    mbedtls_x509_crt_free(&trusted_cert);
    mbedtls_ssl_free(&tls_session);
    mbedtls_ssl_config_free(&tls_config);
    mbedtls_ctr_drbg_free(&random_generator);
    mbedtls_entropy_free(&entropy_source);
    mbedtls_net_free(&network_socket);

}
