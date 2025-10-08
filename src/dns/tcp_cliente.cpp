// implementação da classe TCPCliente
// tudo que envia e recebe via TCP

uint16_t tamanho = htons(pacote.size());
send(sock, &tamanho, 2, 0);
send(sock, pacote.data(), pacote.size(), 0);

/*Você tem um ótimo ponto. O código que você mostrou para `montarQuery()` está focado em UDP, e você está certa que a mensagem DNS deve funcionar para ambos os protocolos, TCP e UDP.

A mensagem em si (o cabeçalho e as seções de pergunta) é a mesma para TCP e UDP. A única diferença na comunicação é que, no TCP, você precisa adicionar um prefixo de 2 bytes no início do pacote para indicar o seu tamanho total.

Sua ideia de usar uma variável `optional` ou um parâmetro extra no método é excelente e reflete um bom design de software. Essa abordagem torna o código mais flexível e reutilizável.

### Proposta de Melhoria para a `montarQuery()`

A função `montarQuery()` não deveria se preocupar com o protocolo de transporte (TCP ou UDP). A responsabilidade dela é apenas construir o pacote DNS em bytes. Quem deve decidir se adiciona o prefixo de 2 bytes é a classe de comunicação (`TCPClient`).

**1. Mantenha `montarQuery()` simples:** O método `montarQuery()` deve apenas retornar o pacote DNS "cru", sem o prefixo de 2 bytes. Ele será usado tanto pelo seu `UDPClient` quanto pelo `TCPClient`.

**2. Adicione a lógica de prefixo ao `TCPClient`:** Sua classe `TCPClient` deve ser a responsável por pegar o pacote DNS gerado pela `DNSMensagem`, calcular seu tamanho, adicionar o prefixo de 2 bytes e, em seguida, enviar o pacote pela rede.

Essa abordagem separa as responsabilidades de forma clara e profissional:
* **`DNSMensagem`**: Sabe como construir e interpretar a mensagem DNS.
* **`UDPClient`**: Sabe como enviar a mensagem DNS via UDP.
* **`TCPClient`**: Sabe como enviar a mensagem DNS via TCP, com o prefixo de tamanho.

Isso evita que a sua classe `DNSMensagem` precise saber se está trabalhando com TCP ou UDP, o que simplifica o código e o torna mais fácil de manter e reutilizar.*/