set(rjcpt_core_test_SRCS
   test_grammar.cpp
   test_lexer.cpp
   test_lexicon_compiler.cpp
   )

add_executable(rjcpt_core_test
   main.cpp
   ${rjcpt_core_test_SRCS})

find_package(GTest REQUIRED)

target_link_libraries(rjcpt_core_test PRIVATE rjcpt_core GTest::GTest)

add_test(NAME rjcpt_core_test
   COMMAND rjcpt_core_test
   WORKING_DIRECTORY .)
