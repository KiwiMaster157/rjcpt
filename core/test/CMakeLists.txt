file(GLOB core_test_SRCS "*.cpp")

add_executable(core_test ${core_test_SRCS})

target_link_libraries(core_test PRIVATE core GTest::GTest)

add_test(NAME core_test
   COMMAND core_test
   WORKING_DIRECTORY .)
