#!/bin/bash

# Este script testa os diferentes modos de operação do resolvedor,
# fazendo consultas reais à internet.

# --- Configuração ---
: ${EXECUTABLE_PATH:="../bin/resolver"} # Garante que o executável seja encontrado

VALID_DOMAIN="google.com"
INVALID_DOMAIN="dominio-quase-certo-que-nao-existe-12345.com"
DNS_SERVER="8.8.8.8"
ROOT_SERVER="198.41.0.4"
DOT_SERVER="1.1.1.1"
DOT_SNI="cloudflare-dns.com"

# --- Função de Ajuda ---
# Parâmetros: 1: Nome do Teste, 2: Comando a ser executado, 3: Padrão regex esperado na saída
assert_output_contains() {
    local test_name="$1"
    local command="$2"
    local expected_pattern="$3"

    echo "  -> Testando: $test_name..."
    
    local output
    output=$(eval "$command")
    local exit_code=$?

    if [ $exit_code -eq 0 ] && echo "$output" | grep -qE "$expected_pattern"; then
        echo "     [PASS]"
    else
        echo "     [FAIL] Padrão esperado não encontrado: '$expected_pattern'"
        echo "     Saída recebida:"
        echo "$output"
        exit 1
    fi
}

# --- Início dos Testes ---

# Teste do modo RECURSIVE (Padrão)
assert_output_contains \
    "Modo Recursive (Domínio Válido)" \
    "$EXECUTABLE_PATH --name $VALID_DOMAIN --qtype A --ns $DNS_SERVER --mode recursive" \
    "Respostas:.*[1-9]"

assert_output_contains \
    "Modo Recursive (Domínio Inválido)" \
    "$EXECUTABLE_PATH --name $INVALID_DOMAIN --qtype A --ns $DNS_SERVER --mode recursive" \
    "RCODE:.*3"

# Teste do modo ITERATIVE
assert_output_contains \
    "Modo Iterative (Delegação)" \
    "$EXECUTABLE_PATH --name $VALID_DOMAIN --qtype A --ns $ROOT_SERVER --mode iterative" \
    "Respostas:.*0"

# Teste do modo DoT (DNS over TLS)
assert_output_contains \
    "Modo DoT (DNS over TLS)" \
    "$EXECUTABLE_PATH --name $DOT_SNI --qtype A --ns $DOT_SERVER --mode dot --sni $DOT_SNI" \
    "1\.1\.1\.1"

echo "  [PASS] Todos os testes de modo de operação passaram."
exit 0