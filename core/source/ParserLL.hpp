#pragma once

#include "GrammarLL.hpp"
#include "Token.hpp"

#include <expected>
#include <span>

#include "rjcpt_core_export.h"

namespace rjcpt
{
   class ParseContext
   {
   public:
      virtual ~ParseContext() = default;

      virtual const CompiledGrammar& GetGrammar() const = 0;

      virtual GrammarNode GetEOF_Node() const = 0;
      virtual GrammarNode GetStartRule() const = 0;

      virtual void BeginParsing() {}
      virtual bool CheckValidator(const Token& aToken, std::uint16_t aValidator) const = 0;
      virtual bool RunActor(const Token& aToken, std::uint16_t aActor) = 0;
   };

   //! Parses the list of tokens using aContext.
   RJCPT_CORE_EXPORT std::string Parse(ParseContext& aContext, std::span<const Token> aTokens);

   namespace parser_util
   {
      RJCPT_CORE_EXPORT bool TryPeek(const ParseContext& aContext, const Token& aToken, std::uint16_t aRuleIndex);
      RJCPT_CORE_EXPORT std::expected<std::uint16_t, std::string> FindRule(const ParseContext& aContext, const Token& aToken, const GrammarNode& aNode);
   }
}
