#pragma once

#include "rjcpt_core_export.h"

#include "Grammar.hpp"

#include <expected>
#include <ranges>
#include <vector>
#include <span>

namespace rjcpt::lex
{
   class Grammar;
   
   enum class OutType : std::uint32_t;

   //! Abstract base class for LL(1) parsers.
   //! Derived classes should override Output() and Error().
   class RJCPT_CORE_EXPORT Parser
   {
   public:
      enum class Result : char
      {
         Okay, Error, InternalError
      };

      explicit Parser(const Grammar* aGrammar);
      virtual ~Parser() = default;

      const Grammar* GetGrammar() const { return mGrammar; }
      bool           IsParsing() const { return !mStack.empty(); }

   protected:
      //! @brief Prepares the parser to parse a new input.
      //! @param aGrammar The grammar to use for parsing. aGrammar must be properly compiled.
      //! @param aStartup The entry point rule.
      //! @return Whether the parser was able to initialize successfully.
      Result Init(RuleRef aStartup);

      //! @brief Precesses a single input token.
      //! @param aIndex The input index forwarded to Output and Error.
      //! @param aValue The type of input token.
      //! @return Whether an error occurred.
      Result PushInput(std::uint32_t aIndex, InType aValue);

      //! @brief Called to indicate that the input is finished.
      //! @param aIndex The input index forwarded to Output and Error.
      //! @return Whether an error occurred.
      Result EndOfInput(std::uint32_t aIndex);

      //! @brief Called by PushInput or EndOfInput when an output callback is encountered in the grammar.
      //! @param aIndex The index passed to PushInput or EndOfInput.
      //! @param aValue The valued passed to PushInput, or InType::Epislon (0) if EndOfInput is called.
      //! @param aType The output type as defined in the grammar.
      virtual void Output(std::uint32_t aIndex, InType aValue, OutType aType) = 0;

      //! @brief Called by PushInput or EndOfInput when an error occurs while processing an input token.
      //! @param aIndex The index passed to PushInput or EndOfInput.
      //! @param aValue The value passed to PushInput, or InType::Epsilon (0) if EndOfInput is called.
      //! @param aError An error string.
      virtual void Error(std::uint32_t aIndex, InType aValue, const std::string& aError) = 0;

   private:
      //! @brief Applies aValue to the top of the parser stack.
      //! @param aIndex The index to be forwarded to Output/Error.
      //! @param aValue The value being input.
      //! @return The parse result if complete, or nullopt if the function needs to be called again.
      std::optional<Result> ParseP(std::uint32_t aIndex, InType aValue);

      //! @brief Returns the next GrammarRule to jump to. Automatically applies fallthrough.
      //! @param aIndex The index to be forwarded to Error if necessary.
      //! @param aNext The value being input.
      //! @return The GrammarRule on success, or nullopt on failure.
      std::optional<GrammarRule> GetJump(std::uint32_t aIndex, InType aNext);

      std::vector<RuleRef> mStack;
      const Grammar* mGrammar = nullptr;
   };
}
