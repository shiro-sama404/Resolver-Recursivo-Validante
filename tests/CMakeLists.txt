# tests/CMakeLists.txt

# Adiciona o diretório do Google Test ao build
add_subdirectory(googletest)

add_executable(
    run_tests
    main.cpp
    test_dns_message.cpp
    test_command_parser.cpp
)

# Linka os testes com a biblioteca do Google Test e a biblioteca da nossa aplicação
target_link_libraries(
    run_tests
    PRIVATE
    GTest::gtest_main
    resolver_lib 
)

# Integra com o CTest
include(GoogleTest)
gtest_discover_tests(run_tests)