set(rjcpt_core_SRCS
   source/CharacterParser.cpp
   source/Grammar.cpp
   source/GrammarCompiler.cpp
   source/IniReader.cpp
   source/Parser.cpp
   source/ProtoLexer.cpp
   )

set(rjcpt_core_HDRS
   source/CharacterParser.hpp
   source/FlatMap.hpp
   source/Grammar.hpp
   source/GrammarCompiler.hpp
   source/GrammarUtils.hpp
   source/IniReader.hpp
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
