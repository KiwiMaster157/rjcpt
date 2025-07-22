file(GLOB rjcpt_gui_SRCS "source/*.cpp")
file(GLOB rjcpt_gui_HDRS "source/*.hpp")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_library(rjcpt_gui SHARED
   ${rjcpt_gui_SRCS}
   ${rjcpt_gui_HDRS})

target_include_directories(rjcpt_gui PUBLIC source)
target_link_libraries(rjcpt_gui PUBLIC rjcpt_core Qt6::Widgets)

add_subdirectory(test)
