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
   OutType ToOutType(RuleRef aRule)
   {
      return OutType(std::uint32_t(aRule) & 0x3fff'ffff);
   }

   struct Out
   {
      std::uint32_t mIndex = 0;
      OutType       mType = OutType();
   };

   class RJCPT_CORE_EXPORT Parser
   {
   public:
      enum class Result : char
      {
         Okay, Error, InternalError
      };

      explicit Parser(const Grammar* aGrammar)
         : mGrammar(aGrammar)
      {}
      virtual ~Parser() = default;

      Result Parse(std::span<const InType> aIn, name_t aStartup)
      {
         auto root = mGrammar->FindRule(aStartup);
         if (!root)
         {
            Error(0, "Internal error. Invalid startup rule: " + aStartup.str());
            return Result::Error;
         }

         mStack = { RuleRef::Epsilon, *root };
         mIndex = 0;

         while (mIndex <= aIn.size())
         {
            InType it = (mIndex < aIn.size()) ? aIn[mIndex] : InType::Epsilon;
            if (ParseP(it) == Result::Error)
            {
               return Result::Error;
            }
         }

         

         return Result::Okay;
      }

   protected:
      Result Init(const Grammar* aGrammar, name_t aStartup)
      {
         mGrammar = aGrammar;
         auto root = mGrammar->FindRule(aStartup);
         if (!root)
         {
            Error(0, "Internal error. Invalid startup rule: " + aStartup.str());
            return Result::InternalError;
         }
         mStack = { RuleRef::MakeInputTerminal(0), *root };
         return Result::Okay;
      }

      Result PushInput(std::uint32_t aIndex, InType aValue)
      {
         if (mStack.empty())
         {
            Error(aIndex, aValue, "Cannot process any more input.");
            return Result::InternalError;
         }

         // TODO: This does not handle the case where multiple tops 
         const RuleRef top = mStack.back();
         if (top.IsInputTerminal())
         {

         }
         else if (top.IsOutputTerminal())
         {

         }
         else if (top.IsNonTerminal())
         {

         }
         else
         {
            Error(aIndex, aValue, "Unknown error.");
            return Result::InternalError;
         }
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
      Result ParseP(std::uint32_t aIndex, InType aType)
      {
         if (mStack.empty())
         {
            // TODO: Add indices and values.
            Error(mIndex, "Cannot process any more input.");
            return Result::Error;
         }
         const RuleRef top = mStack.back();
         if (IsInputTerminal(top))
         {
            if (std::to_underlying(aType) == std::to_underlying(top))
            {
               mStack.pop_back();
               ++mIndex;
               return Result::Okay;
            }
            else
            {
               // TODO: Add indices and values.
               Error(mIndex, "Token does not match expected. TODO: Add details.");
               return Result::Error;
            }
         }
         else if (IsOutputTerminal(top))
         {
            mStack.pop_back();
            Output(mIndex, ToOutType(top));
         }
         else // is rule
         {
            const auto rule = GetJump(aType);
            if (rule == std::nullopt)
            {
               // TODO: Add indices and values.
               Error(mIndex, "Token does not fit grammar. TODO: Use Epsilon (with error checking).");
               return Result::Error;
            }

            mStack.pop_back();
            std::uint32_t i = rule->mEnd;
            while (i > rule->mBegin)
            {
               --i;
               mStack.push_back(mGrammar->GetNode(i).mRule);
            }
            return Result::Okay;
         }
      }

      std::optional<GrammarRule> GetJump(InType aNext)
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
                     Error(mIndex, aNext, err);
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
