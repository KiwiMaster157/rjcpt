#include "Grammar.hpp"

#include <charconv>
#include <format>
#include <ranges>

class rjcpt::lex::GrammarCompiler
{
public:
   struct Symbol
   {
      RuleRef mRule;
      Token   mToken;
   };

   using SymbolTable = rjcpt::FlatMap<lex::name_t, Symbol>;
   using IndexRange = std::pair<std::uint32_t, std::uint32_t>;

   enum class RuleStatus : char
   {
      NotStarted, InProgress, Complete
   };

   GrammarCompiler(Grammar& aGrammar);

   std::vector<std::string> Compile(std::string_view aDefinition);

private:
   void ReadDefinition(std::string_view aDefinition);
   void SortRules();
   void BuildJumpTable();

   void ReadDeclaration(Token aToken);
   std::string_view ParseDeclaredName(Token aToken, RuleRef& aOutType);
   //! Parses aStr as a number.
   //! Returns npos on failure.
   std::uint32_t ParseValue(std::string_view aStr);

   void EnsureJumpTableRowDefined(RuleRef aRuleType);
   void BuildRule(std::uint32_t aRuleIndex, RuleRef aRef);
   void SetJump(RuleRef aRuleType, InType aTokenType, std::uint32_t aJump);
   void SetJumpP(RuleRef aRuleType, std::uint32_t aTokenIndex, std::uint32_t aJump);
   RuleStatus& GetStatus(RuleRef aRuleType);

   IndexRange GetRuleRange(RuleRef aRuleType) const;

   void PushReadError(std::string_view aErrorMessage, Token aToken);

   Grammar& mGrammar;
   std::string_view mDefinition;
   SymbolTable mSymbolTable;
   std::vector<Token> mGrammarRuleTokens;

   std::vector<RuleRef>    mInProgressStack;
   std::vector<RuleStatus> mStatusTable;
   std::vector<std::uint32_t> mRuleRanges;

   std::vector<std::string> mErrors;
};

rjcpt::lex::GrammarCompiler::GrammarCompiler(Grammar& aGrammar)
   : mGrammar(aGrammar)
{}

std::vector<std::string> rjcpt::lex::GrammarCompiler::Compile(std::string_view aDefinition)
{
   mGrammar.Clear();
   mGrammar.mInputTypes.push_back(InType::Epsilon);
   ReadDefinition(aDefinition);
   SortRules();
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
         ReadDeclaration(tok);
         expectDeclaration = false;
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
         next.mEnd = next.mBegin;
         mGrammar.mRules.push_back(next);
         mGrammarRuleTokens.push_back(tok);
      }
      else if (text.size() >= lex::cMAX_NAME)
      {
         PushReadError("Name too long.", tok);
      }
      else
      {
         mGrammar.mNodes.emplace_back(text);
         ++mGrammar.mRules.back().mEnd;
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
            mSymbolTable.emplace(rule.mName, Symbol{ RuleRef::MakeNonTerminal(nextRule), mGrammarRuleTokens[i]});
            mRuleRanges.push_back(i);
            ++nextRule;
         }
         rule.mRef = RuleRef::MakeNonTerminal(nextRule);
      }
   }
   mRuleRanges.push_back(mGrammar.mRules.size());
   mSymbolTable.sort();

   {
      auto it = std::ranges::adjacent_find(mSymbolTable, {}, SymbolTable::projection());
      while (it != mSymbolTable.end())
      {
         PushReadError("Duplicate input/output rule definition.", (it + 1)->second.mToken);
         PushReadError("    See previous declaration here:", it->second.mToken);
         it = std::ranges::adjacent_find(it + 1, mSymbolTable.end(), {}, SymbolTable::projection());
      }
   }

   {
      std::ranges::sort(mGrammar.mInputTypes);
      auto it = std::ranges::adjacent_find(mGrammar.mInputTypes);
      while (it != mGrammar.mInputTypes.end())
      {
         const auto rule = RuleRef::MakeInputTerminal(std::to_underlying(*it));
         std::string err = "Duplicate input type values.";
         for (const auto& sym : mSymbolTable)
         {
            if (sym.second.mRule == rule)
            {
               err += std::format("\n   See declaration of {0}. Row {1}, column {2}.", sym.first.view(), sym.second.mToken.mRow, sym.second.mToken.mColumn);
            }
         }
         mErrors.push_back(std::move(err));
         it = std::ranges::adjacent_find(it + 1, mGrammar.mInputTypes.end());
      }
   }
}

void rjcpt::lex::GrammarCompiler::BuildJumpTable()
{
   // Check mNodes for unknown types.
   for (auto& node : mGrammar.mNodes)
   {
      auto it = mSymbolTable.find(node.mName);
      if (it == mSymbolTable.end())
      {
         mErrors.push_back("Unable to find " + node.mName.str() + ". TODO: Add row/column info by storing a Token in mSymbolTable.");
      }
      else
      {
         node.mRule = it->second.mRule;
      }
   }

   // Initialize data structures.
   mStatusTable.resize(mGrammar.mRules.size(), RuleStatus::NotStarted);
   mGrammar.mJumpTableRows = mRuleRanges.size() - 1;
   mGrammar.mJumpTableColumns = mGrammar.mInputTypes.size();
   mGrammar.mJumpIndices.resize(mGrammar.mJumpTableRows * mGrammar.mJumpTableColumns, Grammar::npos);

   // Ensure jump table fully filled in.
   for (std::uint32_t i = 0; i < mGrammar.mRules.size(); i++)
   {
      EnsureJumpTableRowDefined(RuleRef::MakeNonTerminal(i));
      assert(mInProgressStack.empty());
   }
}

void rjcpt::lex::GrammarCompiler::ReadDeclaration(Token aToken)
{
   RuleRef          ref = RuleRef();
   std::string_view name = ParseDeclaredName(aToken, ref);

   if (name.empty())
   {
      PushReadError("Expected declaration.", aToken);
   }
   else if (name.size() >= lex::cMAX_NAME)
   {
      PushReadError("Declared name too long.", aToken);
   }
   else if (ref.IsNonTerminal())
   {
      auto& rule = mGrammar.mRules.emplace_back();
      rule.mName = name;
      rule.mBegin = static_cast<std::uint32_t>(mGrammar.mNodes.size());
      rule.mEnd = rule.mBegin;
      mGrammarRuleTokens.push_back(aToken);

      // Non-terminals are added to the symbol table later.
   }
   else
   {
      if (ref.IsInputTerminal())
      {
         mGrammar.mInputTypes.push_back(InType(ref.GetValue()));
      }
      mSymbolTable.emplace(name, Symbol{ ref, aToken });
   }
}

std::string_view rjcpt::lex::GrammarCompiler::ParseDeclaredName(Token aToken, RuleRef& aOutType)
{
   std::string_view text = aToken.Get(mDefinition);
   auto eq = text.find('=');
   if (eq != std::string_view::npos)
   {
      const std::string_view name = text.substr(0, eq);
      const std::uint32_t value = ParseValue(text.substr(eq + 1));
      if (name.starts_with('$'))
      {
         if (value >= 0x4000'0000)
         {
            PushReadError("Value too large.", aToken);
         }
         aOutType = RuleRef::MakeInputTerminal(value);
      }
      else if (name.starts_with('&'))
      {
         if (value >= 0x4000'0000)
         {
            PushReadError("Value too large.", aToken);
         }
         aOutType = RuleRef::MakeOutputTerminal(value);
      }
      else
      {
         // Just to indicate that this is a non-terminator rule.
         aOutType = RuleRef::MakeNonTerminal(0);
      }
      return name;
   }
   return std::string_view();
}

std::uint32_t rjcpt::lex::GrammarCompiler::ParseValue(std::string_view aStr)
{
   auto readBase = [](std::string_view sv, int b)
      {
         std::uint32_t x;
         auto result = std::from_chars(sv.data(), sv.data() + sv.size(), x);
         if (result.ec == std::errc() && result.ptr == sv.data() + sv.size())
         {
            return x;
         }
         return Grammar::npos;
      };

   if (aStr.empty())
   {
      return Grammar::npos;
   }
   else if (aStr.starts_with("0x"))
   {
      return readBase(aStr.substr(2), 16);
   }
   else if (aStr.starts_with("0b"))
   {
      return readBase(aStr.substr(2), 2);
   }
   else if (aStr.starts_with('0'))
   {
      return readBase(aStr, 8);
   }
   else
   {
      return readBase(aStr, 10);
   }
}

void rjcpt::lex::GrammarCompiler::EnsureJumpTableRowDefined(RuleRef aRuleType)
{
   RuleStatus& status = GetStatus(aRuleType);
   if (status == RuleStatus::InProgress)
   {
      // TODO: error message should indicate what the circular definition is.
      mErrors.push_back("Circular rule definition. TODO: more details in error message.");
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
         SetJump(aRef, static_cast<InType>(node.mRule.GetValue()), i);
      }
      else if (node.mRule.IsNonTerminal())
      {
         EnsureJumpTableRowDefined(node.mRule);

         for (std::uint32_t t = 0; t < mGrammar.mInputTypes.size(); t++)
         {
            SetJumpP(aRef, t, mGrammar.GetJumpIndexP(node.mRule, t));
         }
         if (mGrammar.GetJumpIndex(node.mRule, InType::Epsilon) != Grammar::npos)
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

void rjcpt::lex::GrammarCompiler::SetJumpP(RuleRef aRuleType, std::uint32_t aTokenIndex, std::uint32_t aJump)
{
   assert(aTokenIndex < mGrammar.mJumpIndices.size());
   if (mGrammar.mJumpIndices[aTokenIndex] != Grammar::npos)
   {
      // TODO: error message should indicate which rule and which token caused the error.
      mErrors.push_back("Cannot set jump index. TODO: indicate rule and token type.");
   }
   else
   {
      mGrammar.mJumpIndices[aTokenIndex] = aJump;
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

std::vector<std::string> rjcpt::lex::Grammar::Compile(std::string_view aDefinition)
{
   GrammarCompiler c(*this);
   return c.Compile(aDefinition);
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

void rjcpt::lex::Grammar::Clear()
{
   *this = Grammar();
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
