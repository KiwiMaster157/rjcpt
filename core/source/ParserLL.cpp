#include "ParserLL.hpp"

#include "Stack.hpp"

std::string rjcpt::Parse(ParseContext& aContext, std::span<const Token> aTokens)
{
   Stack<GrammarNode, 256> stack;
   stack.Push(aContext.GetEOF_Node());
   stack.Push(aContext.GetStartRule());

   auto iter = aTokens.begin();
   aContext.BeginParsing();
   while (stack.Size() > 0)
   {
      const Token       tok = *iter;
      const GrammarNode next = stack.Pop();
      if (next.IsTerminal())
      {
         if (next.ValidatorIndex())
         {
            if (!aContext.CheckValidator(tok, next.ValidatorIndex()))
            {
               return "Failed validator...";
            }
            ++iter;
         }
         if (next.ActorIndex() && !aContext.RunActor(tok, next.ActorIndex()))
         {
            return "Action failed...";
         }
      }
      else
      {
         auto nextRule = parser_util::FindRule(aContext, tok, next);
         if (!nextRule)
         {
            return nextRule.error();
         }
         const auto& grammar = aContext.GetGrammar();
         const GrammarRule rule = grammar.mRules[*nextRule];
         for (std::uint32_t i = rule.mEnd; i > rule.mBegin; i--)
         {
            stack.Push(grammar.mNodes[i - 1]);
         }
      }
   }

   // Parsing successful.
   return std::string();
}

bool rjcpt::parser_util::TryPeek(const ParseContext& aContext, const Token& aToken, std::uint16_t aRuleIndex)
{
   struct PeekContext
   {
      std::uint32_t mRuleIndex = 0;
      std::uint32_t mNodeIndex = 0;
   };
   Stack<PeekContext, 64> stack;
   const auto& grammar = aContext.GetGrammar();
   stack.Emplace(aRuleIndex, grammar.mRules[aRuleIndex].mBegin);
   while (stack.Size() > 0)
   {
      const auto next = stack.Pop();
      const auto nextNode = grammar.mNodes[next.mNodeIndex];
      if (next.mNodeIndex + 1 < grammar.mRules[next.mRuleIndex].mEnd)
      {
         // Push the next item in the rule for future iterations.
         stack.Emplace(next.mRuleIndex, next.mNodeIndex + 1);
      }
      if (nextNode.IsTerminal())
      {
         if (nextNode.ValidatorIndex())
         {
            return aContext.CheckValidator(aToken, nextNode.ValidatorIndex());
         }
      }
      else
      {
         const auto& rule = grammar.mRules[nextNode.RulesBegin()];
         stack.Emplace(nextNode.RulesBegin(), rule.mBegin);
      }
   }
   // If we get all the way through the rule without failure, assume success.
   
   // KNOWN ISSUE: We cannot always assume that this is a valid look-ahead.
   //              Need to take into account current parse context.
   return true;
}

std::expected<std::uint16_t, std::string> rjcpt::parser_util::FindRule(const ParseContext& aContext, const Token& aToken, const GrammarNode& aNode)
{
   assert(!aNode.IsTerminal());
   assert(aNode.RulesEnd() > aNode.RulesBegin());
   if (aNode.RulesEnd() - aNode.RulesBegin() == 1)
   {
      return static_cast<int>(aNode.RulesBegin());
   }
   int retval = -1;
   for (std::uint16_t i = aNode.RulesBegin(); i < aNode.RulesEnd(); i++)
   {
      if (TryPeek(aContext, aToken, i))
      {
         if (retval >= 0)
         {
            return std::unexpected("Ambiguous rule...");
         }
         retval = i;
      }
   }
   if (retval < 0)
   {
      return std::unexpected("No matching rule...");
   }
   return static_cast<std::uint16_t>(retval);
}