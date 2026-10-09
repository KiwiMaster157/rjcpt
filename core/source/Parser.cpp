#include "Parser.hpp"

#include <format>

rjcpt::lex::Parser::Parser(const Grammar* aGrammar)
   : mGrammar(aGrammar)
{}

rjcpt::lex::Parser::Result rjcpt::lex::Parser::Init(RuleRef aStartup, ParseOptions aOptions)
{
   mStack.clear();
   mStack = { RuleRef::MakeInputTerminal(0), aStartup };
   mOptions = aOptions;
   if (mOptions.useQuickJump)
   {
      // TODO: Check mGrammar for if quick jump is valid.
   }
   return Result::Okay;
}

rjcpt::lex::Parser::Result rjcpt::lex::Parser::PushInput(std::uint32_t aIndex, InType aValue)
{
   assert(IsParsing());
   std::optional<Result> r;
   while (!r)
   {
      r = ParseP(aIndex, aValue);
   }
   return *r;
}

rjcpt::lex::Parser::Result rjcpt::lex::Parser::EndOfInput(std::uint32_t aIndex)
{
   assert(IsParsing());
   const Result r = PushInput(aIndex, InType::Epsilon);
   if (r == Result::Okay && !mStack.empty())
   {
      Error(aIndex, InType::Epsilon, "Unexpected end of input.");
      return Result::Error;
   }
   mStack.clear();
   return r;
}

std::optional<rjcpt::lex::Parser::Result> rjcpt::lex::Parser::ParseP(std::uint32_t aIndex, InType aValue)
{
   if (mStack.empty())
   {
      Error(aIndex, aValue, "Cannot process any more input.");
      return Result::Error;
   }
   const RuleRef top = mStack.back();
   if (top.IsInputTerminal())
   {
      const auto expected = top.GetValue();
      if (std::to_underlying(aValue) == expected)
      {
         mStack.pop_back();
         return Result::Okay;
      }
      else
      {
         auto err = std::format("Token does not match expected ({}). Stack, top first:", expected);
         for (auto ref : std::views::reverse(mStack))
         {
            static const char* cTYPES[] = {"Invalid", "NonTerminal", "Input", "Output"};
            const auto name = ref.IsNonTerminal() ? mGrammar->FindRuleName(ref).value_or("<invalid>") : name_t("<none>");
            err += std::format("\n   {}: {} ({}).", cTYPES[(int) ref.GetType()], ref.GetValue(), name.view());
         }
         Error(aIndex, aValue, err);
         return Result::Error;
      }
   }
   else if (top.IsOutputTerminal())
   {
      mStack.pop_back();
      Output(aIndex, aValue, OutType(top.GetValue()));
      return std::nullopt;
   }
   else // is non-terminal.
   {
      const auto rule = GetJump(aIndex, aValue);
      if (rule == std::nullopt)
      {
         Error(aIndex, aValue, "Cannot determine jump rule for token. TODO: Add stack info.");
         return Result::Error;
      }

      mStack.pop_back();
      std::uint32_t i = rule->mEnd;
      while (i > rule->mBegin)
      {
         --i;
         mStack.push_back(mGrammar->GetNode(i).mRule);
      }
      return std::nullopt;
   }
}

std::optional<rjcpt::lex::GrammarRule> rjcpt::lex::Parser::GetJump(std::uint32_t aIndex, InType aNext)
{
   assert(!mStack.empty());
   std::size_t index = mStack.size() - 1;
   // TODO: Use GetQuickJump if applicable.
   auto next = mGrammar->GetJump(mStack[index], aNext);
   auto ifEps = mGrammar->GetJump(mStack[index], InType::Epsilon);
   if (mOptions.checkFirstFollow && next && ifEps)
   {
      while (index > 0)
      {
         --index;
         const auto ref = mStack[index];
         if (ref.IsInputTerminal())
         {
            if (ref.GetValue() == std::to_underlying(aNext))
            {
               std::string err = "Warning: Grammar is ambiguous, using " + next->mName.str() + ". TODO: Add details.";
               Error(aIndex, aNext, err);
               break;
            }
            break;
         }
         else if (ref.IsNonTerminal())
         {
            const auto jump = mGrammar->GetJump(mStack[index], aNext);
            if (jump)
            {
               std::string err = "Warning: Grammar is ambiguous, using " + next->mName.str() + ". TODO: Add details.";
               Error(aIndex, aNext, err);
               break;
            }
            else if (!mGrammar->GetJump(mStack[index], InType::Epsilon))
            {
               break;
            }
         }
      }
   }
   return next ? next : ifEps;
}
