#pragma once

#include "SmallString.hpp"

#include <cassert>
#include <utility>

namespace rjcpt::lex
{
   inline constexpr std::size_t cMAX_NAME = 16;
   using name_t = SmallString<cMAX_NAME>;

   inline constexpr std::uint32_t npos = 0xffff'ffffu;

   //! @brief References a terminal or non-terminal in a grammar.
   class RuleRef final
   {
   public:
      enum class Type : std::uint32_t
      {
         Invalid, NonTerminal, InputTerminal, OutputTerminal
      };

      RuleRef() = default;
      RuleRef(std::in_place_t, Type aType, std::uint32_t aValue)
         : mType(aType)
         , mValue(aValue)
      {
         assert(mType == aType); // Handles bit truncation.
         assert(mValue == aValue); // Handles bit truncation.
      }

      inline static RuleRef MakeInputTerminal(std::uint32_t aValue) { return RuleRef(std::in_place, Type::InputTerminal, aValue); }
      inline static RuleRef MakeOutputTerminal(std::uint32_t aValue) { return RuleRef(std::in_place, Type::OutputTerminal, aValue); }
      inline static RuleRef MakeNonTerminal(std::uint32_t aValue) { return RuleRef(std::in_place, Type::NonTerminal, aValue); }

      inline bool IsValid() const noexcept { return mType != Type::Invalid; }
      inline bool IsNonTerminal() const noexcept { return mType == Type::NonTerminal; }
      inline bool IsInputTerminal() const noexcept { return mType == Type::InputTerminal; }
      inline bool IsOutputTerminal() const noexcept { return mType == Type::OutputTerminal; }

      inline Type          GetType() const noexcept { return mType; }
      inline std::uint32_t GetValue() const noexcept { return mValue; }

      friend bool operator==(RuleRef aLeft, RuleRef aRight) = default;

   private:
      Type          mType : 2 { Type::Invalid };
      std::uint32_t mValue : 30 { 0 };
   };

   //! A GrammarNode is a token, rule reference, or output result.
   //! The first character of mName indicates the type.
   //! If it is a token, mIndex stores the mTypeIndex.
   //! If it is a rule reference, mIndex stores the row index in the jump table.
   //! If it is an output result, mIndex is ignored.
   struct GrammarNode final
   {
      name_t  mName;
      RuleRef mRule = RuleRef();
   };

   //! A GrammarRule is a named sequences of GrammarNodes.
   //! There may be multiple GrammarRules with the same name.
   //! When this is the case, they will be consecutive.
   struct GrammarRule final
   {
      name_t  mName;
      RuleRef mRef = RuleRef();

      //! mBegin and mEnd define the range in Grammar::mNodes that this rule has.
      std::uint32_t mBegin = 0;
      std::uint32_t mEnd = 0;
   };

   //! An opaque enum type that Parsers and Grammars use to refer to the input token type.
   //! The special value "Epsilon" refers to both the special "epsilon" and "end-of-input" types.
   enum class InType : std::uint32_t
   {
      Epsilon = 0
   };
}
