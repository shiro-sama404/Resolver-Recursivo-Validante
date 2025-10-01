# Resolver DNS Recursivo Validante em C++

Este projeto é uma implementação de um resolvedor DNS recursivo, validante, com suporte a cache e DNSSEC, desenvolvido para a disciplina de Redes de Computadores (2025/2).

O resolvedor foi construído em C++, utilizando apenas chamadas de socket de baixo nível para comunicação de rede, conforme os requisitos do trabalho.

## Funcionalidades

* **Resolução Recursiva Completa**: Capaz de resolver nomes de domínio a partir dos servidores raiz, seguindo delegações (NS), tratando CNAMEs e respostas negativas (NXDOMAIN, NODATA).
* **Fallback para TCP**: Realiza consultas via TCP automaticamente quando uma resposta UDP é recebida truncada (`TC=1`).
* **DNS over TLS (DoT)**: Suporte a consultas seguras na porta 853, com validação de certificado e SNI.
* **Validação DNSSEC (Bônus)**: Valida a cadeia de confiança (DS → DNSKEY → RRSIG) e define a flag `AD=1` em respostas autenticadas.
* **Cache em Memória**: Utiliza um daemon de cache separado para armazenar respostas positivas (respeitando o TTL) e negativas, otimizando o tempo de resposta.
* **Multithreading (Bônus)**: Realiza consultas a múltiplos servidores de nomes em paralelo (fan-out) e gerencia requisições concorrentes com um pool de threads.

## Pré-requisitos
A adicionar

## Compilação
A adicionar

## Utilização do `resolver`
A adicionar

## Utilização do Servidor de Cache

## :busts_in_silhouette: Autores
| [<img loading="lazy" src="https://avatars.githubusercontent.com/u/68046889?v=4" width=115><br><sub>Arthur de Andrade</sub>](https://github.com/shiro-sama404) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/91064992?v=4" width=115><br><sub>Fernanda Neves</sub>](https://github.com/Fernanda-Neves410) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/144397400?v=4" width=115><br><sub>Jenniffer Checchia</sub>](https://github.com/Jenn-Checchia) |
| :---: | :---: | :---: |