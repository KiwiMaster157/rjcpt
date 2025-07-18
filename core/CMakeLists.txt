file(GLOB core_SRCS "source/*.cpp")
file(GLOB core_HDRS "source/*.hpp")

add_library(core SHARED
   ${core_SRCS}
   ${core_HDRS})

target_include_directories(core PUBLIC source)

add_subdirectory(test)
