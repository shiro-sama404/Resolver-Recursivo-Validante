cmake_minimum_required(VERSION 3.15)
project(DNSResolver LANGUAGES CXX)

# -------------------------------------------------------------------
# CONFIGURAÇÕES GERAIS
# -------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # usa gnu++17 (permite getopt, etc.)

# Habilita warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Ativa otimização em builds Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -------------------------------------------------------------------
# DETECÇÃO AUTOMATICA DE FONTES
# -------------------------------------------------------------------
# Busca todos os .cpp dentro de src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     src/*.cpp)

# -------------------------------------------------------------------
# ADICIONA EXECUTÁVEL PRINCIPAL
# -------------------------------------------------------------------
add_executable(resolver ${SRC_FILES})

# Inclui diretórios de cabeçalhos (para que #include "cli/arguments.h" funcione)
target_include_directories(resolver PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/dns
    ${CMAKE_SOURCE_DIR}/src/utils
    /usr/include              # Diretório geral para mbedTLS
)

# -------------------------------------------------------------------
# LINKA DEPENDÊNCIAS OPCIONAIS (OpenSSL, Threads, etc.)
# -------------------------------------------------------------------

# Threads (para std::thread, mutex, etc.)
find_package(Threads REQUIRED)
target_link_libraries(resolver PRIVATE Threads::Threads)

# mbedTLS
find_library(MBEDTLS_LIB mbedtls)
find_library(MBEDX509_LIB mbedx509)
find_library(MBEDCRYPTO_LIB mbedcrypto)

if(MBEDTLS_LIB AND MBEDX509_LIB AND MBEDCRYPTO_LIB)
    target_link_libraries(resolver PRIVATE ${MBEDTLS_LIB} ${MBEDX509_LIB} ${MBEDCRYPTO_LIB})
    message(STATUS "Linking with mbedTLS")
else()
    message(FATAL_ERROR "mbedTLS not found. Por favor instale-o.")
endif()
# -------------------------------------------------------------------
# MENSAGENS DE CONFIGURAÇÃO
# -------------------------------------------------------------------
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
