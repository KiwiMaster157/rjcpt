#pragma once

#include "rjcpt_core_export.h"

#include "Grammar.hpp"

#include <expected>
#include <ranges>
#include <vector>
#include <span>

namespace rjcpt::lex
{
   class Grammar;
   
   enum class OutType : std::uint32_t;

   class RJCPT_CORE_EXPORT Parser
   {
   public:
      enum class Result : char
      {
         Okay, Error, InternalError
      };

      virtual ~Parser() = default;

      const Grammar* GetGrammar() const { return mGrammar; }
      bool           IsParsing() const { return !mStack.empty(); }

   protected:
      Result Init(const Grammar* aGrammar, name_t aStartup)
      {
         mGrammar = aGrammar;
         mStack.clear();
         auto root = mGrammar->FindRule(aStartup);
         if (!root)
         {
            Error(0, InType(), "Internal error. Invalid startup rule: " + aStartup.str());
            return Result::InternalError;
         }
         mStack = { RuleRef::MakeInputTerminal(0), *root };
         return Result::Okay;
      }

      Result PushInput(std::uint32_t aIndex, InType aValue)
      {
         std::optional<Result> r;
         while (!r)
         {
            r = ParseP(aIndex, aValue);
         }
         return *r;
      }

      Result EndOfInput(std::uint32_t aIndex)
      {
         const Result r = PushInput(aIndex, InType::Epsilon);
         if (r == Result::Okay && !mStack.empty())
         {
            Error(aIndex, InType::Epsilon, "Unexpected end of input.");
            return Result::Error;
         }
         return r;
      }

      virtual void Output(std::uint32_t aIndex, InType aValue, OutType aType) = 0;
      virtual void Error(std::uint32_t aIndex, InType aValue, const std::string& aError) = 0;

   private:
      //! Returns true to move on to the next input element.
      std::optional<Result> ParseP(std::uint32_t aIndex, InType aValue)
      {
         if (mStack.empty())
         {
            // TODO: Add indices and values.
            Error(aIndex, aValue, "Cannot process any more input.");
            return Result::Error;
         }
         const RuleRef top = mStack.back();
         if (top.IsInputTerminal())
         {
            if (std::to_underlying(aValue) == top.GetValue())
            {
               mStack.pop_back();
               return Result::Okay;
            }
            else
            {
               // TODO: Add indices and values.
               Error(aIndex, aValue, "Token does not match expected. TODO: Add details.");
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
               // TODO: Add indices and values.
               Error(aIndex, aValue, "Token does not fit grammar. TODO: Use Epsilon (with error checking).");
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

      std::optional<GrammarRule> GetJump(std::uint32_t aIndex, InType aNext)
      {
         assert(!mStack.empty());
         std::size_t index = mStack.size() - 1;
         auto next = mGrammar->GetJump(mStack[index], aNext);
         if (!next)
         {
            next = mGrammar->GetJump(mStack[index], InType::Epsilon);
            if (next)
            {
               while (index > 0)
               {
                  --index;
                  auto ref = mGrammar->GetJump(mStack[index], aNext);
                  if (ref)
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
         return next;
      }

      std::vector<RuleRef> mStack;
      const Grammar* mGrammar = nullptr;
   };
}
