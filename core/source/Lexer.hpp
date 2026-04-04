#pragma once

#include "Lexicon.hpp"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rjcpt_core_export.h"

namespace rjcpt::lex
{
   enum class Option { DropIgnored, KeepIgnored };
   //! Error type returned from Lexer
   struct LexError final
   {
      std::int32_t mRow = 1;
      std::int32_t mColumn = 1;
      std::uint32_t mPosition = 0;
   };

   struct LexResult final
   {
      bool IsValid() const { return !mError; }
      operator bool() const { return IsValid(); }

      std::vector<Token> mTokens;
      std::optional<LexError> mError;
   };

   //! Lexer is a short-lived object for reading a single input string.
   //! It provides 2 public APIs:
   //! (1) LexInput: single function call lexes the entire input into a vector of Tokens.
   //! (2) ReadToken: reads Tokens from the input one at a time.
   class RJCPT_CORE_EXPORT Lexer final
   {
   public:
      using enum Option;

      Lexer(const Lexicon* aLexicon, std::string_view aInput);

      //! Lexes aString using aLexicon.
      //! This is the primary public API of the Lexer.
      static LexResult LexInput(const Lexicon& aLexicon, std::string_view aString,
                                Option aOption = Option::DropIgnored);

      //! Returns the next token from mInput; skips ignored tokens depending on
      //! aOption. If there are no more tokens left to read, returns false.
      bool ReadToken(Token& aOut, Option aOption = Option::DropIgnored);

   private:
     //! Returns the next token from mInput, without special handling for
     //! ignored tokens. If there are no more tokens left to read, returns
     //! false.
      bool ReadTokenP(Token& aOut);

      //! Evaluates whether aRule appears at the start of aInput.
      //! If it does, returns the number of characters the matching substring
      //! contains. Otherwise, returns an empty optional.
      std::optional<std::uint32_t> EvaluateRule(const SequenceRule& aRule,
                                                std::string_view aInput) const;
      //! Evaluates whether aSequence appears at the start of aInput.
      //! If it does, returns the number of characers the matching substring
      //! contains. Otherwise, returns an empty optional.
      std::optional<std::uint32_t> EvaluateSequence(const Sequence& aSequence,
                                                    std::string_view aInput) const;

      //! Helper function for EvaluateRule.
      //! EvaluateRuleP handles the different rule types, while EvaluateRule
      //! handles quantity information.
      std::optional<std::uint32_t> EvaluateRuleP(const SequenceRule& aRule,
                                                 std::string_view aInput) const;

      //! Populates aOut with the given kind and size.
      //! Updates mRow, mColumn, and mPosition.
      bool ExtractToken(Token& aOut, std::uint32_t aKind, std::uint32_t aSize);

      const Lexicon& mLexicon;
      std::string_view mInput;
      std::int32_t mRow = 1;
      std::int32_t mColumn = 1;
      std::uint32_t mPosition = 0;
   };

} // namespace rjcpt::lex
