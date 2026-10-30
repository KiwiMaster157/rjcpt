#include "Grammar.hpp"

#include <charconv>
#include <format>
#include <ranges>

std::vector<std::string> rjcpt::lex::Grammar::Compile(GrammarInput&& aInput)
{
   if (!aInput.mErrors.empty())
   {
      return std::move(aInput.mErrors);
   }

   CompileState state;
   state.mDefinition.swap(aInput.mRawData.mInput);
   state.mNodeTokens.swap(aInput.mRawData.mNodeTokens);
   state.mRuleTokens.swap(aInput.mRawData.mRuleTokens);

   mRules.swap(aInput.mRawData.mRules);
   mNodes.swap(aInput.mRawData.mNodes);

   SortRules(state);
   LinkRules(state);
   BuildJumpTable(state);

   return state.mErrors;
}

std::optional<rjcpt::lex::GrammarRule> rjcpt::lex::Grammar::GetJump(RuleRef aRuleType, InType aInputType) const
{
   const std::uint32_t jumpIndex = mJumpTable.GetJumpIndex(aRuleType, aInputType);
   if (jumpIndex < mRules.size())
   {
      return mRules[jumpIndex];
   }
   return std::nullopt;
}

rjcpt::lex::GrammarNode rjcpt::lex::Grammar::GetNode(std::uint32_t aNode) const
{
   return mNodes.at(aNode);
}

std::optional<rjcpt::lex::RuleRef> rjcpt::lex::Grammar::FindRule(name_t aName) const
{
   auto cmp = [](const GrammarRule& aRule) { return aRule.mName; };
   auto it = std::ranges::lower_bound(mRules, aName, {}, cmp);
   if (it != mRules.end() && it->mName == aName)
   {
      return it->mRef;
   }
   return std::nullopt;
}

std::optional<rjcpt::lex::name_t> rjcpt::lex::Grammar::FindRuleName(RuleRef aRuleType) const
{
   assert(aRuleType.IsNonTerminal());
   auto cmp = [](const GrammarRule& aRule)
      {
         assert(aRule.mRef.IsNonTerminal());
         return aRule.mRef.GetValue();
      };
   auto it = std::ranges::lower_bound(mRules, aRuleType.GetValue(), {}, cmp);
   if (it != mRules.end() && it->mRef == aRuleType)
   {
      return it->mName;
   }
   return std::nullopt;
}

void rjcpt::lex::Grammar::Clear()
{
   *this = Grammar();
}

void rjcpt::lex::Grammar::SortRules(CompileState& aState)
{
   auto cmp = [](const auto& aRule) { return std::get<0>(aRule).mName; };
   std::ranges::stable_sort(std::views::zip(mRules, aState.mRuleTokens), {}, cmp);

   if (!mRules.empty())
   {
      assert(mRules.size() == aState.mRuleTokens.size());

      std::uint32_t nextRule = 0;
      name_t        lastName = mRules.front().mName;
      aState.mRuleRanges.push_back(0);
      for (std::uint32_t i = 0; i < mRules.size(); i++)
      {
         auto& rule = mRules[i];
         if (rule.mName != lastName)
         {
            lastName = rule.mName;
            aState.mRuleRanges.push_back(i);
            ++nextRule;
         }
         rule.mRef = RuleRef::MakeNonTerminal(nextRule);
      }
   }
   aState.mRuleRanges.push_back(mRules.size());
}

void rjcpt::lex::Grammar::LinkRules(CompileState& aState)
{
   assert(mNodes.size() == aState.mNodeTokens.size());
   for (std::size_t i = 0; i < mNodes.size(); i++)
   {
      GrammarNode& node = mNodes[i];
      if (node.mRule == RuleRef())
      {
         auto rule = FindRule(node.mName);
         if (rule)
         {
            node.mRule = *rule;
         }
         else
         {
            PushReadError("Grammar rule does not exist.", aState.mNodeTokens[i]);
         }
      }
   }
}

void rjcpt::lex::Grammar::BuildJumpTable(CompileState& aState)
{
   std::vector<InType> inputTypes{ InType::Epsilon };
   for (const auto& node : mNodes)
   {
      if (node.mRule.IsInputTerminal())
      {
         inputTypes.push_back(InType(node.mRule.GetValue()));
      }
   }

   std::ranges::sort(inputTypes);
   auto it = std::ranges::unique(inputTypes);
   inputTypes.erase(it.begin(), it.end());

   const std::uint32_t ruleCount = static_cast<std::uint32_t>(aState.mRuleRanges.size() - 1);
   mJumpTable = grammar_compiler::JumpTable(ruleCount, std::move(inputTypes));

   // Ensure jump table fully filled in.
   for (std::uint32_t i = 0; i < mJumpTable.GetNumRules(); i++)
   {
      EnsureJumpTableRowDefined(aState, RuleRef::MakeNonTerminal(i));
      assert(aState.mInProgressStack.empty());
   }
}

void rjcpt::lex::Grammar::EnsureJumpTableRowDefined(CompileState& aState, RuleRef aRule)
{
   assert(aRule.IsNonTerminal());
   RuleStatus& status = aState.mStatusTable[aRule.GetValue()];
   if (status == RuleStatus::InProgress)
   {
      Token t = aState.mRuleTokens.at(aRule.GetValue());
      std::string err = std::format("Circular rule definition for {} defined at row {}, column {}.",
                                    t.Get(aState.mDefinition), t.mRow, t.mColumn);
      for (RuleRef rule : std::views::reverse(aState.mInProgressStack))
      {
         t = aState.mRuleTokens.at(rule.GetValue());
         err += std::format(" Required from...\n   {} defined at row {}, column {}.", t.Get(aState.mDefinition), t.mRow, t.mColumn);
         if (rule == aRule)
         {
            break;
         }
      }
      aState.mErrors.push_back(err);
   }
   else if (status == RuleStatus::NotStarted)
   {
      aState.mInProgressStack.push_back(aRule);
      status = RuleStatus::InProgress;

      const auto r = GetRuleRange(aState, aRule);
      for (auto i = r.first; i != r.second; i++)
      {
         BuildRule(aState, i, aRule);
      }

      assert(!aState.mInProgressStack.empty() && aState.mInProgressStack.back() == aRule);
      assert(status == RuleStatus::InProgress);
      aState.mInProgressStack.pop_back();
      status = RuleStatus::Complete;
   }
}

void rjcpt::lex::Grammar::BuildRule(CompileState& aState, std::uint32_t aRuleIndex, RuleRef aRef)
{
   assert(aRuleIndex < mRules.size());
   GrammarRule rule = mRules[aRuleIndex];
   assert(rule.mRef == aRef);
   for (std::uint32_t i = rule.mBegin; i < rule.mEnd; i++)
   {
      const auto node = mNodes[i];
      if (node.mRule.IsInputTerminal())
      {
         mJumpTable.SetJump(aRef, static_cast<InType>(node.mRule.GetValue()), aRuleIndex);
         return;
      }
      else if (node.mRule.IsNonTerminal())
      {
         EnsureJumpTableRowDefined(aState, node.mRule);

         // Skip over t=0, aka the epsilon rule.
         for (std::uint32_t t = 1; t < mJumpTable.GetNumInputs(); t++)
         {
            std::uint32_t jumpIndex = mJumpTable.GetJumpIndexP(node.mRule, t);
            if (jumpIndex != rjcpt::lex::npos)
            {
               mJumpTable.SetJumpAt(aRef, t, aRuleIndex);
            }
         }
         if (mJumpTable.GetJumpIndex(node.mRule, InType::Epsilon) == rjcpt::lex::npos)
         {
            return;
         }
      }
   }
   mJumpTable.SetJump(aRef, InType::Epsilon, aRuleIndex);
}
