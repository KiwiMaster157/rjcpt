#include "Grammar.hpp"

#include <charconv>
#include <format>
#include <ranges>

class rjcpt::lex::GrammarCompiler
{
public:
   using IndexRange = std::pair<std::uint32_t, std::uint32_t>;

   enum class RuleStatus : char
   {
      NotStarted, InProgress, Complete
   };

   GrammarCompiler(Grammar& aGrammar);

   std::vector<std::string> Compile(std::string_view aDefinition, Grammar::CompileOptions aOptions);

private:
   // These are the major steps, in order, for compiling the grammar.
   void ReadDefinition(std::string_view aDefinition);
   void SortRules();
   void LinkRules();
   void SetInputTypes(unsigned int aMaxInput);
   void FindInputTypes();
   void BuildJumpTable();

   // Helper functions for ReadDefinition().
   bool ReadDeclaration(Token aToken);
   RuleRef ReadTerminal(Token aToken);
   void PushNode(Token aToken, name_t aName, RuleRef aRule);

   // Helper functions for BuildJumpTable().
   void EnsureJumpTableRowDefined(RuleRef aRuleType);
   void BuildRule(std::uint32_t aRuleIndex, RuleRef aRef);
   void SetJump(RuleRef aRuleType, InType aTokenType, std::uint32_t aJump);
   void SetJumpP(RuleRef aRuleType, std::uint32_t aTokenIndex, std::uint32_t aJump);
   RuleStatus& GetStatus(RuleRef aRuleType);

   IndexRange GetRuleRange(RuleRef aRuleType) const;

   void PushReadError(std::string_view aErrorMessage, Token aToken);

   Grammar&           mGrammar;
   std::string_view   mDefinition;
   std::vector<Token> mGrammarRuleTokens;
   std::vector<Token> mGrammarNodeTokens;

   std::vector<RuleRef>       mInProgressStack;
   std::vector<RuleStatus>    mStatusTable;
   std::vector<std::uint32_t> mRuleRanges;

   std::vector<std::string> mErrors;
};

rjcpt::lex::GrammarCompiler::GrammarCompiler(Grammar& aGrammar)
   : mGrammar(aGrammar)
{}

std::vector<std::string> rjcpt::lex::GrammarCompiler::Compile(std::string_view aDefinition, Grammar::CompileOptions aOptions)
{
   mGrammar.Clear();
   ReadDefinition(aDefinition);
   SortRules();
   LinkRules();
   if (aOptions.maxInput)
   {
      SetInputTypes(*aOptions.maxInput);
   }
   else
   {
      FindInputTypes();
   }
   BuildJumpTable();
   return std::move(mErrors);
}

void rjcpt::lex::GrammarCompiler::ReadDefinition(std::string_view aDefinition)
{
   auto lexer = lex::ProtoLexer(aDefinition);
   mDefinition = aDefinition;

   bool expectDeclaration = true;
   lex::Token tok;
   while (lexer.ReadToken(tok))
   {
      std::string_view text = tok.Get(aDefinition);
      if (expectDeclaration)
      {
         expectDeclaration = !ReadDeclaration(tok);
      }
      else if (text.contains('='))
      {
         PushReadError("Unexpected declaration.", tok);
      }
      else if (text == ";")
      {
         expectDeclaration = true;
      }
      else if (text == "|")
      {
         auto back = mGrammar.mRules.back();
         GrammarRule next;
         next.mName = back.mName;
         next.mBegin = back.mEnd;
         next.mEnd = back.mEnd;
         mGrammar.mRules.push_back(next);
         mGrammarRuleTokens.push_back(tok);
      }
      else if (text.starts_with('$') || text.starts_with('&'))
      {
         PushNode(tok, text, ReadTerminal(tok));
      }
      else if (text.size() >= lex::cMAX_NAME)
      {
         PushReadError("Name too long.", tok);
      }
      else
      {
         PushNode(tok, text, RuleRef());
      }
   }
   if (!expectDeclaration)
   {
      // May have to handle this one differently.
      PushReadError("Incomplete rule definition.", Token());
   }
}

void rjcpt::lex::GrammarCompiler::SortRules()
{
   auto cmp = [](const auto& aRule) { return std::get<0>(aRule).mName; };
   std::ranges::stable_sort(std::views::zip(mGrammar.mRules, mGrammarRuleTokens), {}, cmp);

   if (!mGrammar.mRules.empty())
   {
      assert(mGrammar.mRules.size() == mGrammarRuleTokens.size());

      std::uint32_t nextRule = 0;
      lex::name_t   lastName = mGrammar.mRules.front().mName;
      mRuleRanges.push_back(0);
      for (std::uint32_t i = 0; i < mGrammar.mRules.size(); i++)
      {
         auto& rule = mGrammar.mRules[i];
         if (rule.mName != lastName)
         {
            lastName = rule.mName;
            mRuleRanges.push_back(i);
            ++nextRule;
         }
         rule.mRef = RuleRef::MakeNonTerminal(nextRule);
      }
   }
   mRuleRanges.push_back(mGrammar.mRules.size());
   std::ranges::sort(mGrammar.mInputTypes);
}

void rjcpt::lex::GrammarCompiler::LinkRules()
{
   assert(mGrammar.mNodes.size() == mGrammarNodeTokens.size());
   for (std::size_t i = 0; i < mGrammar.mNodes.size(); i++)
   {
      GrammarNode& node = mGrammar.mNodes[i];
      if (node.mRule == RuleRef())
      {
         auto rule = mGrammar.FindRule(node.mName);
         if (rule)
         {
            node.mRule = *rule;
         }
         else
         {
            PushReadError("Grammar rule does not exist.", mGrammarNodeTokens[i]);
         }
      }
   }
}

void rjcpt::lex::GrammarCompiler::SetInputTypes(unsigned int aMaxInput)
{
   assert(mGrammar.mInputTypes.empty());
   assert(mGrammar.mNodes.size() == mGrammarNodeTokens.size());
   for (std::size_t i = 0; i < mGrammar.mNodes.size(); i++)
   {
      const GrammarNode& node = mGrammar.mNodes[i];
      const auto         value = node.mRule.GetValue();
      if (node.mRule.IsInputTerminal() && value > aMaxInput)
      {
         std::string err = std::format("Input terminal value ({}) is greater than maximum ({}).", value, aMaxInput);
         PushReadError(err, mGrammarNodeTokens[i]);
      }
   }

   const std::uint32_t max = aMaxInput;
   mGrammar.mInputTypes.resize(max + 1);
   for (std::uint32_t i = 0; i <= max; i++)
   {
      mGrammar.mInputTypes[i] = InType(i);
   }
}

void rjcpt::lex::GrammarCompiler::FindInputTypes()
{
   assert(mGrammar.mInputTypes.empty());
   mGrammar.mInputTypes.push_back(InType::Epsilon);
   for (const auto& node : mGrammar.mNodes)
   {
      if (node.mRule.IsInputTerminal())
      {
         mGrammar.mInputTypes.push_back(InType(node.mRule.GetValue()));
      }
   }

   std::ranges::sort(mGrammar.mInputTypes);
   auto it = std::ranges::unique(mGrammar.mInputTypes);
   mGrammar.mInputTypes.erase(it.begin(), it.end());
}

void rjcpt::lex::GrammarCompiler::BuildJumpTable()
{
   assert(mGrammar.mJumpIndices.empty());

   // Initialize data structures.
   mStatusTable.resize(mGrammar.mRules.size(), RuleStatus::NotStarted);
   mGrammar.mJumpTableRows = mRuleRanges.size() - 1;
   mGrammar.mJumpTableColumns = mGrammar.mInputTypes.size();
   mGrammar.mJumpIndices.resize(mGrammar.mJumpTableRows * mGrammar.mJumpTableColumns, Grammar::npos);

   // Ensure jump table fully filled in.
   for (std::uint32_t i = 0; i < mGrammar.mJumpTableRows; i++)
   {
      EnsureJumpTableRowDefined(RuleRef::MakeNonTerminal(i));
      assert(mInProgressStack.empty());
   }
}

bool rjcpt::lex::GrammarCompiler::ReadDeclaration(Token aToken)
{
   std::string_view text = aToken.Get(mDefinition);
   if (!text.ends_with('=') || text == "=")
   {
      PushReadError("Expected declaration.", aToken);
   }
   else if (text.size() >= lex::cMAX_NAME)
   {
      PushReadError("Declared name too long.", aToken);
   }
   else if (text.starts_with('$') || text.starts_with('&'))
   {
      PushReadError("Rule cannot start with '$' or '&'.", aToken);
   }
   else
   {
      auto& rule = mGrammar.mRules.emplace_back();
      rule.mName = text.substr(0, text.size() - 1);
      rule.mBegin = static_cast<std::uint32_t>(mGrammar.mNodes.size());
      rule.mEnd = rule.mBegin;
      mGrammarRuleTokens.push_back(aToken);
      return true;
   }
   return false;
}

rjcpt::lex::RuleRef rjcpt::lex::GrammarCompiler::ReadTerminal(Token aToken)
{
   std::string_view text = aToken.Get(mDefinition);

   assert(text.starts_with('$') || text.starts_with('&'));
   std::uint32_t x;
   auto result = std::from_chars(text.data() + 1, text.data() + text.size(), x);
   if (result.ec != std::errc() || result.ptr != text.data() + text.size())
   {
      PushReadError("Cannot read value.", aToken);
   }
   else if (x == 0)
   {
      PushReadError("Cannot have a terminal of value 0.", aToken);
   }
   else if (x > 0x4000'0000)
   {
      PushReadError("Value too large.", aToken);
   }
   else if (text.starts_with('$'))
   {
      return RuleRef::MakeInputTerminal(x);
   }
   else if (text.starts_with('&'))
   {
      return RuleRef::MakeOutputTerminal(x);
   }
   return RuleRef();
}

void rjcpt::lex::GrammarCompiler::PushNode(Token aToken, name_t aName, RuleRef aRule)
{
   assert(mGrammar.mNodes.size() == mGrammarNodeTokens.size());
   mGrammar.mNodes.push_back(GrammarNode{ .mName = aName, .mRule = aRule });
   ++mGrammar.mRules.back().mEnd;
   mGrammarNodeTokens.push_back(aToken);
}

void rjcpt::lex::GrammarCompiler::EnsureJumpTableRowDefined(RuleRef aRuleType)
{
   RuleStatus& status = GetStatus(aRuleType);
   if (status == RuleStatus::InProgress)
   {
      Token t = mGrammarRuleTokens.at(aRuleType.GetValue());
      std::string err = std::format("Circular rule definition for {} defined at row {}, column {}.",
                                    t.Get(mDefinition), t.mRow, t.mColumn);
      for (RuleRef rule : std::views::reverse(mInProgressStack))
      {
         t = mGrammarRuleTokens.at(rule.GetValue());
         err += std::format(" Required from...\n   {} defined at row {}, column {}.", t.Get(mDefinition), t.mRow, t.mColumn);
         if (rule == aRuleType)
         {
            break;
         }
      }
      mErrors.push_back(err);
   }
   else if (status == RuleStatus::NotStarted)
   {
      mInProgressStack.push_back(aRuleType);
      status = RuleStatus::InProgress;

      const auto r = GetRuleRange(aRuleType);
      for (auto i = r.first; i != r.second; i++)
      {
         BuildRule(i, aRuleType);
      }

      assert(!mInProgressStack.empty() && mInProgressStack.back() == aRuleType);
      assert(status == RuleStatus::InProgress);
      mInProgressStack.pop_back();
      status = RuleStatus::Complete;
   }
}

void rjcpt::lex::GrammarCompiler::BuildRule(std::uint32_t aRuleIndex, RuleRef aRef)
{
   assert(aRuleIndex < mGrammar.mRules.size());
   GrammarRule rule = mGrammar.mRules[aRuleIndex];
   assert(rule.mRef == aRef);
   for (std::uint32_t i = rule.mBegin; i < rule.mEnd; i++)
   {
      const auto& node = mGrammar.mNodes[i];
      if (node.mRule.IsInputTerminal())
      {
         SetJump(aRef, static_cast<InType>(node.mRule.GetValue()), aRuleIndex);
         return;
      }
      else if (node.mRule.IsNonTerminal())
      {
         EnsureJumpTableRowDefined(node.mRule);

         // Skip over t=0, aka the epsilon rule.
         for (std::uint32_t t = 1; t < mGrammar.mInputTypes.size(); t++)
         {
            std::uint32_t jumpIndex = mGrammar.GetJumpIndexP(node.mRule, t);
            if (jumpIndex != Grammar::npos)
            {
               SetJumpP(aRef, t, aRuleIndex);
            }
         }
         if (mGrammar.GetJumpIndex(node.mRule, InType::Epsilon) == Grammar::npos)
         {
            return;
         }
      }
   }
   SetJump(aRef, InType::Epsilon, aRuleIndex);
}

void rjcpt::lex::GrammarCompiler::SetJump(RuleRef aRuleType, InType aTokenType, std::uint32_t aJump)
{
   const std::uint32_t index = mGrammar.GetInputTypeIndex(aTokenType);
   SetJumpP(aRuleType, index, aJump);
}

void rjcpt::lex::GrammarCompiler::SetJumpP(RuleRef aRuleType, std::uint32_t aInputIndex, std::uint32_t aJump)
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();

   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex < mGrammar.mJumpTableRows);
   assert(aInputIndex < mGrammar.mJumpTableColumns);
   const std::uint32_t tableIndex = ruleIndex * mGrammar.mJumpTableColumns + aInputIndex;

   if (mGrammar.mJumpIndices[tableIndex] != Grammar::npos)
   {
      name_t        name = mGrammar.FindRuleName(aRuleType).value_or("<error>");
      std::uint32_t input = static_cast<std::uint32_t>(mGrammar.mInputTypes[aInputIndex]);
      std::string   err = std::format("FIRST/FIRST ambiguity for rule '{}', input {}.", name.view(), input);
      mErrors.push_back(err);
   }
   else
   {
      mGrammar.mJumpIndices[tableIndex] = aJump;
   }
}

rjcpt::lex::GrammarCompiler::RuleStatus& rjcpt::lex::GrammarCompiler::GetStatus(RuleRef aRuleType)
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();
   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex < mStatusTable.size());
   return mStatusTable[ruleIndex];
}

rjcpt::lex::GrammarCompiler::IndexRange rjcpt::lex::GrammarCompiler::GetRuleRange(RuleRef aRuleType) const
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();
   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex + 1 < mRuleRanges.size());
   return { mRuleRanges[ruleIndex], mRuleRanges[ruleIndex + 1] };
}

void rjcpt::lex::GrammarCompiler::PushReadError(std::string_view aErrorMessage, Token aToken)
{
   std::string err = std::format("{0}\n   Near \"{1}\", row {2}, column {3}.", aErrorMessage, aToken.Get(mDefinition), aToken.mRow, aToken.mColumn);
   mErrors.push_back(std::move(err));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> rjcpt::lex::Grammar::Compile(std::string_view aDefinition, CompileOptions aOptions)
{
   GrammarCompiler c(*this);
   return c.Compile(aDefinition, aOptions);
}

std::optional<rjcpt::lex::GrammarRule> rjcpt::lex::Grammar::GetJump(RuleRef aRuleType, InType aInputType) const
{
   const std::uint32_t jumpIndex = GetJumpIndex(aRuleType, aInputType);
   if (jumpIndex < mRules.size())
   {
      return mRules[jumpIndex];
   }
   return std::nullopt;
}

std::optional<rjcpt::lex::GrammarRule> rjcpt::lex::Grammar::GetQuickJump(RuleRef aRuleType, std::uint32_t aInputIndex) const
{
   if (aInputIndex < mJumpTableColumns)
   {
      const std::uint32_t jumpIndex = GetJumpIndexP(aRuleType, aInputIndex);
      if (jumpIndex < mRules.size())
      {
         return mRules[jumpIndex];
      }
   }
   return std::nullopt;
}

std::uint32_t rjcpt::lex::Grammar::GetInputTypeIndex(InType aType) const
{
   auto it = std::ranges::lower_bound(mInputTypes, aType);
   if (it != mInputTypes.end() && *it == aType)
   {
      return static_cast<std::uint32_t>(it - mInputTypes.begin());
   }
   return npos;
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

std::uint32_t rjcpt::lex::Grammar::GetJumpIndex(RuleRef aRuleType, InType aInputType) const
{
   const std::uint32_t inputIndex = GetInputTypeIndex(aInputType);
   return GetJumpIndexP(aRuleType, inputIndex);
}

std::uint32_t rjcpt::lex::Grammar::GetJumpIndexP(RuleRef aRuleType, std::uint32_t aInputIndex) const
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();

   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex < mJumpTableRows);
   assert(aInputIndex < mJumpTableColumns);
   const std::uint32_t tableIndex = ruleIndex * mJumpTableColumns + aInputIndex;
   assert(tableIndex < mJumpIndices.size());
   return mJumpIndices[tableIndex];
}
