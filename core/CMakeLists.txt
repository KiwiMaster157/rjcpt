set(rjcpt_core_SRCS
   source/Lexer.cpp
   source/Lexicon.cpp
   source/LexiconCompiler.cpp
   )

set(rjcpt_core_HDRS
   source/FlatMap.hpp
   source/Lexer.hpp
   source/Lexicon.hpp
   source/LexiconCompiler.hpp
   source/SmallString.hpp
   source/Stack.hpp
   )

add_library(rjcpt_core STATIC
   ${rjcpt_core_SRCS}
   ${rjcpt_core_HDRS})

generate_export_header(rjcpt_core)
target_include_directories(rjcpt_core PUBLIC source ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(test)
