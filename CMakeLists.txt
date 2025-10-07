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
)

# -------------------------------------------------------------------
# LINKA DEPENDÊNCIAS OPCIONAIS (OpenSSL, Threads, etc.)
# -------------------------------------------------------------------

# Threads (para std::thread, mutex, etc.)
find_package(Threads REQUIRED)
target_link_libraries(resolver PRIVATE Threads::Threads)

# OpenSSL (para DNSSEC e TLS)
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    target_include_directories(resolver PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(resolver PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# -------------------------------------------------------------------
# MENSAGENS DE CONFIGURAÇÃO
# -------------------------------------------------------------------
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")