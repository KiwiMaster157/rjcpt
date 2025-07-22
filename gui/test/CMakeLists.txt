file(GLOB rjcpt_gui_test_SRCS "*.cpp")

add_executable(rjcpt_gui_test ${rjcpt_gui_test_SRCS})

find_package(GTest REQUIRED)

target_link_libraries(rjcpt_gui_test PRIVATE rjcpt_core GTest::GTest)

add_test(NAME rjcpt_gui_test
   COMMAND rjcpt_gui_test
   WORKING_DIRECTORY .)
