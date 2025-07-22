file(GLOB rjcpt_core_SRCS "source/*.cpp")
file(GLOB rjcpt_core_HDRS "source/*.hpp")

add_library(rjcpt_core SHARED
   ${rjcpt_core_SRCS}
   ${rjcpt_core_HDRS})

generate_export_header(rjcpt_core)
target_include_directories(rjcpt_core PUBLIC source ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(test)
