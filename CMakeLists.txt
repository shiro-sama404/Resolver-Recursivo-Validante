# CMakeLists.txt (na raiz do projeto)

cmake_minimum_required(VERSION 3.16)
project(DNSResolver LANGUAGES CXX)

# --- Configurações Gerais ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# --- Fontes da Biblioteca ---
file(GLOB CLI_SOURCES    CONFIGURE_DEPENDS "src/cli/*.cpp")
file(GLOB DNS_SOURCES    CONFIGURE_DEPENDS "src/dns/*.cpp")
file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS "src/server/*.cpp")
file(GLOB UTILS_SOURCES  CONFIGURE_DEPENDS "src/utils/*.cpp")

set(RESOLVER_LIB_SOURCES
    ${CLI_SOURCES}
    ${DNS_SOURCES}
    ${SERVER_SOURCES}
    ${UTILS_SOURCES}
)

# --- Criação da Biblioteca ---
add_library(resolver_lib ${RESOLVER_LIB_SOURCES})

target_include_directories(resolver_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# --- Dependências da Biblioteca ---
find_package(Threads REQUIRED)
find_library(MBEDTLS_LIB mbedtls REQUIRED)
find_library(MBEDX509_LIB mbedx509 REQUIRED)
find_library(MBEDCRYPTO_LIB mbedcrypto REQUIRED)

target_link_libraries(resolver_lib PRIVATE
    Threads::Threads
    ${MBEDTLS_LIB}
    ${MBEDX509_LIB}
    ${MBEDCRYPTO_LIB}
)

# --- Executável Principal ---
add_executable(resolver src/main.cpp)
target_link_libraries(resolver PRIVATE resolver_lib)
# Coloca o executável na pasta bin/ para facilitar a execução dos testes
set_target_properties(resolver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# --- Integração com os Testes ---
enable_testing()
add_subdirectory(tests)

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")