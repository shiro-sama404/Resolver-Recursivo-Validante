#!/bin/bash

# Este script executa todos os testes .sh

# --- Configuração ---
EXECUTABLE_PATH="./build/bin/resolver"
TEST_DIR=$(dirname "$0")

# --- Cores para a Saída ---
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[1;33m'
COLOR_RESET='\033[0m'

# --- Contadores ---
tests_passed=0
tests_failed=0

# --- Função de Ajuda para Rodar Testes ---
run_test() {
    local test_script="$1"
    local test_name=$(basename "$test_script" .sh)
    
    echo -e "${COLOR_YELLOW}-----------------------------------------------------${COLOR_RESET}"
    echo -e "${COLOR_YELLOW}▶ EXECUTANDO: $test_name${COLOR_RESET}"
    
    export EXECUTABLE_PATH
    
    bash "$test_script"
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${COLOR_GREEN}✔ SUCESSO: $test_name${COLOR_RESET}"
        tests_passed=$((tests_passed + 1))
    else
        echo -e "${COLOR_RED}✖ FALHA: $test_name (Código de saída: $exit_code)${COLOR_RESET}"
        tests_failed=$((tests_failed + 1))
    fi
    echo -e "${COLOR_YELLOW}-----------------------------------------------------\n${COLOR_RESET}"
}

# --- Execução Principal ---

# Verifica se o executável existe no caminho esperado
if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo -e "${COLOR_RED}ERRO: Executável não encontrado em '$EXECUTABLE_PATH'. Você compilou o projeto com 'make'?${COLOR_RESET}"
    exit 1
fi

# Garante que o daemon de cache não esteja rodando antes de começar
echo "Limpando ambiente antes dos testes..."
$EXECUTABLE_PATH --deactivate > /dev/null 2>&1
sleep 1

# Encontra e executa todos os scripts de teste no diretório
for test_file in "$TEST_DIR"/test_*.sh; do
    run_test "$test_file"
done

# Garante que o daemon de cache seja desativado ao final de tudo
echo "Limpando ambiente após os testes..."
$EXECUTABLE_PATH --deactivate > /dev/null 2>&1

# --- Impressão dos resultados ---
echo "================== RESUMO DOS TESTES =================="
echo -e "${COLOR_GREEN}Testes Aprovados: $tests_passed${COLOR_RESET}"
if [ $tests_failed -gt 0 ]; then
    echo -e "${COLOR_RED}Testes Reprovados: $tests_failed${COLOR_RESET}"
    exit 1
else
    echo "Todos os testes passaram com sucesso!"
    exit 0
fi