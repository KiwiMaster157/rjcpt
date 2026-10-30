#pragma once

#include "FlatMap.hpp"
#include "Lexicon.hpp"
#include "ProtoLexer.hpp"
#include "GrammarCompiler.hpp"
#include "GrammarUtils.hpp"

#include <cassert>
#include <span>

namespace rjcpt::lex
{
   //! Grammar takes a textual grammar defintion and turns it into a lookup table that can be used by a Parser.
   //! Note: Parsers contain a pointer to the Grammar they use. This allows the Grammar to be a flywheel object.
   class RJCPT_CORE_EXPORT Grammar final
   {
   public:
      //! Compiles a grammar definition string.
      //! Returns any errors that occurred in the process.
      std::vector<std::string> Compile(GrammarInput&& aInput);

      //! @brief Returns the GrammarRule that the parser should jump to for the given top rule type and input type.
      //! @param aRuleType The rule on top of the parse stack.
      //! @param aInputType The type of the next input token.
      std::optional<GrammarRule> GetJump(RuleRef aRuleType, InType aInputType) const;

      //! @brief Returns the GrammarRule that the parser should jump to for the given top rule and input type index.
      //! @param aRuleType The rule on top of the parse stack.
      //! @param aInputIndex The index of the next input token type.
      //! @note  Using GetJump is preferred, but this function can be used to bypass input type lookup.
      std::optional<GrammarRule> GetQuickJump(RuleRef aRuleType, std::uint32_t aInputIndex) const;

      //! Converts aType to the index used in the jump table.
      //! Returns npos on failure.
      std::uint32_t GetInputTypeIndex(InType aInputType) const;

      //! Returns the node at the given index.
      //! Throws on failure.
      GrammarNode GetNode(std::uint32_t aNode) const;

      //! Finds and returns the RuleRef for the given name.
      std::optional<RuleRef> FindRule(name_t aName) const;

      //! Finds and returns the name of the given rule ref.
      //! Returns nullopt if it does not exist.
      std::optional<name_t> FindRuleName(RuleRef aRuleType) const;

      //! Returns the Grammar to its initial state.
      void Clear();

   private:
      using IndexRange = std::pair<std::uint32_t, std::uint32_t>;

      enum class RuleStatus : char
      {
         NotStarted, InProgress, Complete
      };

      // TODO: Move to CPP?
      struct CompileState
      {
         std::string        mDefinition;
         std::vector<Token> mRuleTokens;
         std::vector<Token> mNodeTokens;

         std::vector<RuleRef>       mInProgressStack;
         std::vector<RuleStatus>    mStatusTable;
         std::vector<std::uint32_t> mRuleRanges;

         std::vector<std::string> mErrors;
      };

      void SortRules(CompileState& aState);
      void LinkRules(CompileState& aState);
      void BuildJumpTable(CompileState& aState);

      void EnsureJumpTableRowDefined(CompileState& aState, RuleRef aRule);
      void BuildRule(CompileState& aState, std::uint32_t aRuleIndex, RuleRef aRule);
      IndexRange& GetRuleRange(CompileState& aState, RuleRef aRule) const;

      std::vector<GrammarRule> mRules;
      std::vector<GrammarNode> mNodes;

      grammar_compiler::JumpTable mJumpTable;
   };
}
