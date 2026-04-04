#pragma once

#include "Lexicon.hpp"

#include <expected>

#include "rjcpt_core_export.h"

namespace rjcpt::lex
{
   using CompileLexiconResult = std::expected<Lexicon, std::string>;
   RJCPT_CORE_EXPORT CompileLexiconResult CompileLexicon(std::string_view aInput);

   namespace detail
   {
      struct LexiconCompiler
      {
         void Compile(std::string_view aInput);

         bool AppendCharacterClass(name_t aName, std::string_view aDefinition);
         bool AppendSequence(name_t aName, std::string_view aDefinition);
         bool AppendTokenType(name_t aName, std::string_view aDefinition);

         SequenceRule::Quantity ExtractQuantity(std::string_view& aWord) const;
         std::optional<Sequence> ReadFullSequence(name_t aOut,
                                                  std::string_view aDefinition);
         std::optional<SequenceRule> ReadSequenceRule(std::string_view aWord);

         bool ReadCharacterClassElement(CharacterClass& aOut, std::string_view aWord);

         void SetError(std::initializer_list<std::string_view> aData);

         CompileLexiconResult mResult;
         std::size_t mRow = 0;
      };
   } // namespace detail
} // namespace rjcpt::lex
