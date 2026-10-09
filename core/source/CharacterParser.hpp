#pragma once

#include "rjcpt_core_export.h"

#include "Parser.hpp"

namespace rjcpt::lex
{
   class RJCPT_CORE_EXPORT CharacterParser : public Parser
   {
   public:
      static constexpr OutType cBEGIN_TOKEN{ 1000000 };

      explicit CharacterParser(const Grammar* aGrammar);

      std::string ParseText(std::string_view aText, RuleRef aStart);

   protected:
      virtual void EmitToken(Token aToken, OutType aType) = 0;

      void Output(std::uint32_t aIndex, InType aInput, OutType aOut) final;
      void Error(std::uint32_t aIndex, InType aInput, const std::string& aError) override;

      std::string_view mText;

      bool  mHasCurrentToken = false;
      Token mCurrentToken;

      std::string mError;

      std::uint32_t mRow      = 1;
      std::uint32_t mColumn   = 1;
   };
}
