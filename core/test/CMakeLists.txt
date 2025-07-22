file(GLOB rjcpt_core_test_SRCS "*.cpp")

add_executable(rjcpt_core_test ${rjcpt_core_test_SRCS})

find_package(GTest REQUIRED)

target_link_libraries(rjcpt_core_test PRIVATE rjcpt_core GTest::GTest)

add_test(NAME rjcpt_core_test
   COMMAND rjcpt_core_test
   WORKING_DIRECTORY .)
