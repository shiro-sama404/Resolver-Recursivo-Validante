# Resolver DNS Recursivo Validante em C++

Este projeto é uma implementação de um resolvedor DNS recursivo, validante, com suporte a cache e DNSSEC, desenvolvido para a disciplina de Redes de Computadores (2025/2).

O resolvedor foi construído em C++20, utilizando CMake como sistema de build e apenas chamadas de socket de baixo nível para comunicação de rede, conforme os requisitos deste trabalho.

## Funcionalidades
* **Resolução Recursiva Completa**: Capaz de resolver nomes de domínio a partir dos servidores raiz, seguindo delegações (NS), tratando CNAMEs e respostas negativas (NXDOMAIN, NODATA).
* **Fallback para TCP**: Realiza consultas via TCP automaticamente quando uma resposta UDP é recebida truncada (`TC=1`).
* **DNS over TLS (DoT)**: Suporte a consultas seguras na porta 853, com validação de certificado e SNI.
* **Validação DNSSEC (Em emplementação)**: Valida a cadeia de confiança (DS → DNSKEY → RRSIG) e define a flag `AD=1` em respostas autenticadas.
* **Cache em Memória**: Utiliza um daemon de cache separado para armazenar respostas positivas (respeitando o TTL) e negativas, otimizando o tempo de resposta.
* **Multithreading (Parcialmente implementado na cache)**: Realiza consultas a múltiplos servidores de nomes em paralelo (fan-out) e gerencia requisições concorrentes com um pool de threads.

## Pré-requisitos
Para compilar e executar este projeto, você precisará de um ambiente POSIX (Linux, macOS, WSL) com as seguintes ferramentas instaladas:

* Compilador C++20: g++ (versão 10+) ou Clang (versão 12+).
* CMake: Versão 3.16 ou superior.
* Git: Necessário para clonar o repositório e seus submódulos.
* Biblioteca mbedtls: Necessária para a funcionalidade de DNS over TLS.
* Em sistemas baseados em Debian/Ubuntu, pode ser instalada com:
    - ```sudo apt-get update && sudo apt-get install libmbedtls-dev```
* O projeto utiliza o Google Test como um submódulo do Git, que é baixado automaticamente durante o clone.
  -```git clone --recurse-submodules <URL_DO_SEU_REPOSITORIO>```

## Compilação
O projeto utiliza um sistema de build out-of-source com CMake. Para compilar, siga os passos abaixo a partir da raiz do projeto:

#### 1. Criar um diretório de build
```
mkdir build
```
#### 2. Entrar no diretório e executar o CMake
```
cd build
cmake ..
```
#### 3. Compilar o projeto com o make
```
make
```

Após a compilação, os executáveis principais (resolver e run_tests) estarão localizados no diretório build/bin/ e build/tests/, respectivamente.

## Utilização do `resolver`
O executável `resolver` possui dois modos de operação principais: realizar consultas DNS e gerenciar o daemon de cache.

#### Realizando Consultas DNS

A sintaxe básica para uma consulta é: ```./build/bin/resolver --name <domínio> --qtype <tipo>```

Principais Argumentos de Consulta:
Argumento	Descrição	Exemplo de Uso
--name	(Obrigatório) O nome de domínio a ser consultado.	--name google.com
--qtype	O tipo de registro a ser solicitado (A, AAAA, MX, etc.).	--qtype AAAA
--ns	O endereço IP do servidor de nomes a ser consultado. Padrão: 8.8.8.8.	--ns 1.1.1.1
--mode	O modo de operação do resolvedor. Padrão: recursive.	--mode iterative
--trace	Ativa o rastreamento detalhado da resolução, exibindo cada passo.	--trace
--sni	Define o Server Name Indication para conexões DoT.	--sni cloudflare-dns.com

Exemplos de Comandos:
```
# Consulta A recursiva para example.com usando o servidor padrão
./build/bin/resolver --name example.com --qtype A

# Consulta MX para ufg.br usando o servidor da Cloudflare
./build/bin/resolver --name ufg.br --qtype MX --ns 1.1.1.1

# Consulta iterativa, partindo de um servidor raiz
./build/bin/resolver --name github.com --qtype A --mode iterative --ns 198.41.0.4

# Consulta via DNS over TLS (DoT)
./build/bin/resolver --name cloudflare.com --mode dot --ns 1.1.1.1 --sni cloudflare-dns.com
```

#### Utilização do Servidor de Cache
O resolver também funciona como uma ferramenta de linha de comando para gerenciar o daemon de cache.

Para ativar o daemon (que rodará em segundo plano):

```./build/bin/resolver --activate```

Para desativar o daemon:

```./build/bin/resolver --deactivate```

Outros Comandos de Gerenciamento

| Comando           | Descrição                                                           |
| :---------------- | -----                                                               |
| --status          | Exibe o número de entradas atuais e a capacidade máxima das caches. |
| -list-positive    | Lista todas as entradas na cache positiva.                          |
|--list-negative    |	Lista todas as entradas na cache negativa.                          |
|--list-all	        | Lista o conteúdo de ambas as caches.                                |
|--purge-positive   |	Limpa (expurga) todas as entradas da cache positiva.                |
|--purge-negative   |	Limpa (expurga) todas as entradas da cache negativa.                |
|--purge-all        |	Limpa ambas as caches.                                              |
|--set-positive <n> |	Define um novo tamanho máximo n para a cache positiva.              |
|--set-negative <n> |	Define um novo tamanho máximo n para a cache negativa.              |

Exemplo de Fluxo de Cache:

1. Ativa o daemon
   
```./build/bin/resolver --activate```

2. Faz uma consulta (será um 'cache miss')

```./resolver --ns 8.8.8.8 --name nomeinexistentealeatorioqualquer.com --qtype MX --mode validating --sni dns.google --trust-anchor ./root.keys --fanout 5 --workers 10 --timeout 3000 --trace```

4. Faz a mesma consulta novamente (agora será um 'cache hit')

```./resolver --ns 8.8.8.8 --name nomeinexistentealeatorioqualquer.com --qtype MX --mode validating --sni dns.google --trust-anchor ./root.keys --fanout 5 --workers 10 --timeout 3000 --trace```

4. Verifica o status da cache

```./build/bin/resolver --status```

5. Desativa o daemon

```./build/bin/resolver --deactivate```

## :busts_in_silhouette: Autores
| [<img loading="lazy" src="https://avatars.githubusercontent.com/u/68046889?v=4" width=115><br><sub>Arthur de Andrade</sub>](https://github.com/shiro-sama404) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/91064992?v=4" width=115><br><sub>Fernanda Neves</sub>](https://github.com/Fernanda-Neves410) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/144397400?v=4" width=115><br><sub>Jenniffer Checchia</sub>](https://github.com/Jenn-Checchia) |
| :---: | :---: | :---: |
