file(GLOB gui_SRCS "source/*.cpp")
file(GLOB gui_HDRS "source/*.hpp")

add_library(gui SHARED
   ${gui_SRCS}
   ${gui_HDRS})

target_include_directories(gui PUBLIC source)
target_link_libraries(gui PUBLIC core Qt::Qt6)

add_subdirectory(test)
