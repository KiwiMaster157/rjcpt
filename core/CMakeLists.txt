set(rjcpt_core_SRCS
   source/Grammar.cpp
   source/Lexer.cpp
   source/Lexicon.cpp
   source/LexiconCompiler.cpp
   source/Parser.cpp
   source/ProtoLexer.cpp
   )

set(rjcpt_core_HDRS
   source/FlatMap.hpp
   source/Grammar.hpp
   source/Lexer.hpp
   source/Lexicon.hpp
   source/LexiconCompiler.hpp
   source/Parser.hpp
   source/ProtoLexer.hpp
   source/SmallString.hpp
   source/Stack.hpp
   source/Token.hpp
   )

add_library(rjcpt_core STATIC
   ${rjcpt_core_SRCS}
   ${rjcpt_core_HDRS}
   rjcpt_core.natvis)

generate_export_header(rjcpt_core)
target_include_directories(rjcpt_core PUBLIC source ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(test)
add_subdirectory(grammar_explorer)
