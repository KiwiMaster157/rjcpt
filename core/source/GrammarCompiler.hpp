#pragma once

#include "GrammarUtils.hpp"
#include "Token.hpp"

#include <expected>
#include <vector>

#include "rjcpt_core_export.h"

namespace rjcpt::lex
{
   //! The input to a grammar is first read into a GrammarInput object.
   //! Then this object is passed to the Grammar::Compile function to generate the jump table.
   class RJCPT_CORE_EXPORT GrammarInput final
   {
   public:
      explicit GrammarInput(std::string aRawInput);

      void PushRule(Token aToken, name_t aName);
      void PushRuleAlternate(Token aToken);
      void PushNode(Token aToken, name_t aName, RuleRef aRule);

      void PushReadError(std::string_view aErrorMessage, Token aToken);

      struct RawData
      {
         std::string mInput;

         std::vector<GrammarRule> mRules;
         std::vector<GrammarNode> mNodes;

         std::vector<Token> mRuleTokens;
         std::vector<Token> mNodeTokens;
      };
      RawData mRawData;

      std::vector<std::string> mErrors;
   };

   namespace grammar_compiler
   {
      RJCPT_CORE_EXPORT GrammarInput ReadStandaloneInput(std::string aGrammar);

      RJCPT_CORE_EXPORT std::pair<GrammarInput, GrammarInput> ReadTwoPhaseInput(std::string aGrammar);

      // Each row corresponds to a differently-named rule.
      //    Multiple rules can share a name. All rules with the same name appear on the same row.
      // Each column corresponds to a different input type.
      //    The column of an input type is the same as that type's index in mInputTypes.
      // Column 0 is reserved for the "epsilon" value, which has special meaning.
      // The stored values are indices into the mRules vector, or npos to mean "no value".
      class RJCPT_CORE_EXPORT JumpTable
      {
      public:
         JumpTable(std::uint32_t aNumRules, std::vector<InType> aInputs);

         std::uint32_t GetNumRules() const;
         std::uint32_t GetNumInputs() const;

         //! Returns the column index associated with the given input type.
         std::uint32_t GetInputTypeIndex(InType aType) const;

         //! Returns the rule index that the parser should jump to for the given top rule type and current input type.
         //! Returns npos on failure.
         std::uint32_t GetJumpIndex(RuleRef aRuleType, InType aInputType) const;
         //! Returns the rule index that the parser should jump to for the given top rule type and current input type's index.
         //! Returns npos on failure.
         std::uint32_t GetJumpIndexP(RuleRef aRuleType, std::uint32_t aInputIndex) const;

         [[nodiscard("SetJump returns false on a FIRST/FIRST ambiguity.")]]
         bool SetJump(RuleRef aRuleType, InType aInputType, std::uint32_t aJump);

         [[nodiscard("SetJumpAt returns false on a FIRST/FIRST ambiguity.")]]
         bool SetJumpAt(RuleRef aRuleType, std::uint32_t aInputIndex, std::uint32_t aJump);

      private:
         std::vector<std::uint32_t> mJumpIndices;
         std::vector<InType>        mInputTypes;
         std::uint32_t              mJumpTableRows = 0;
      };
   }
}

