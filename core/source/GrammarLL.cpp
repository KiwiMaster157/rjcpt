#include "GrammarLL.hpp"

#include <algorithm>
#include <stdexcept>

rjcpt::CompiledGrammar rjcpt::CompileGrammar(const GrammarLocator& aLocator, std::string_view aGrammarText)
{
   rjcpt::CompiledGrammar retval;

   const auto size = grammar_util::GetGrammarSize(aGrammarText);
   retval.mRules.reserve(size.mNumRules);
   retval.mNodes.reserve(size.mNumNodes);

   const char* iter = aGrammarText.data();
   std::string_view word;
   while (grammar_util::ExtractWord(iter, word))
   {
      if (word.ends_with('='))
      {
         // Begin new rule.
         auto& rule  = retval.mRules.emplace_back();
         rule.mName  = word.substr(0, word.size() - 1);
         rule.mBegin = static_cast<std::uint32_t>(retval.mNodes.size());
         rule.mEnd   = rule.mBegin;
      }
      else if (retval.mRules.empty())
      {
         throw std::logic_error("Grammar must begin with a named rule.");
      }
      else if (word == "|")
      {
         auto  alternateOf = retval.mRules.back();
         auto& rule        = retval.mRules.emplace_back();
         rule.mName        = alternateOf.mName;
         rule.mBegin       = alternateOf.mEnd;
         rule.mEnd         = rule.mBegin;
      }
      else
      {
         retval.mNodes.push_back(grammar_util::MakeGrammarNode(aLocator, word));
         ++retval.mRules.back().mEnd;
      }
   }

   grammar_util::SetNonTerminalIndices(retval);

   return retval;
}

bool rjcpt::grammar_util::ExtractWord(const char*& aInput, std::string_view& aOutput)
{
   const char* beginWord = SkipSpace(aInput);
   aInput = beginWord;
   if (*beginWord)
   {
      while (*aInput && !IsSpace(*aInput))
      {
         ++aInput;
      }
      aOutput = std::string_view(beginWord, aInput);
      return true;
   }
   return false;
}

rjcpt::grammar_util::GrammarSize rjcpt::grammar_util::GetGrammarSize(std::string_view aGrammarText)
{
   GrammarSize retval;

   const char* iter = aGrammarText.data();
   std::string_view word;
   while (ExtractWord(iter, word))
   {
      if (word.ends_with('='))
      {
         ++retval.mNumRules;
      }
      else if (word == "|")
      {
         ++retval.mNumRules;
      }
      else
      {
         ++retval.mNumNodes;
      }
   }
   return retval;
}

rjcpt::GrammarNode rjcpt::grammar_util::MakeGrammarNode(const GrammarLocator& aLocator, std::string_view aGrammarString)
{
   GrammarNode retval;
   retval.SetText(aGrammarString);
   if (aGrammarString.contains('>'))
   {
      // Make terminal. Set validator and actor indices.
      std::size_t index = aGrammarString.find('>');
      std::string_view validatorName = aGrammarString.substr(0, index);
      std::string_view actorName = aGrammarString.substr(index + 1);

      std::uint16_t validatorIndex = 0;
      std::uint16_t actorIndex = 0;

      if (!validatorName.empty())
      {
         validatorIndex = aLocator.FindValidator(validatorName);
         if (validatorIndex == 0)
         {
            throw std::runtime_error("Unknown validator: " + std::string(validatorName));
         }
      }
      if (!actorName.empty())
      {
         actorIndex = aLocator.FindActor(actorName);
         if (actorIndex == 0)
         {
            throw std::runtime_error("Unknown actor: " + std::string(actorName));
         }
      }

      retval.SetTerminal(validatorIndex, actorIndex);
   }
   else
   {
      // Add non-terminal. Set dummy indices to be overwritten later.
      retval.SetNonTerminal(0, 0);
   }
   return retval;
}

void rjcpt::grammar_util::SetNonTerminalIndices(CompiledGrammar& aGrammar)
{
   using cmpLess = std::less<void>;
   auto ruleName = [](const GrammarRule& aRule) -> decltype(auto) { return aRule.mName; };
   std::ranges::stable_sort(aGrammar.mRules, cmpLess(), ruleName);
   const auto begin = aGrammar.mRules.begin();
   for (GrammarNode& node : aGrammar.mNodes)
   {
      if (!node.IsTerminal())
      {
         const auto rules = std::ranges::equal_range(aGrammar.mRules, node.RawText(), cmpLess(), ruleName);
         if (rules.empty())
         {
            throw std::runtime_error("Cannot find rule: " + node.RawText().str());
         }
         
         node.SetNonTerminal(static_cast<std::uint16_t>(rules.begin() - begin),
                             static_cast<std::uint16_t>(rules.end() - begin));
      }
   }
}
