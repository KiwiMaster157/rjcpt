#pragma once

#include "FlatMap.hpp"
#include "Lexicon.hpp"
#include "ProtoLexer.hpp"
#include "SmallString.hpp"

#include <cassert>
#include <span>

namespace rjcpt::lex
{
   using lex::name_t;
   
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
      Type          mType  : 2 { Type::Invalid };
      std::uint32_t mValue : 30 { 0 };
   };

   class GrammarCompiler;

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
      std::uint32_t mEnd   = 0;
   };

   //! An opaque enum type that Parsers and Grammars use to refer to the input token type.
   //! The special value "Epsilon" refers to both the special "epsilon" and "end-of-input" types.
   enum class InType : std::uint32_t
   {
      Epsilon = 0
   };

   //! Grammar takes a textual grammar defintion and turns it into a lookup table that can be used by a Parser.
   //! Note: Parsers contain a pointer to the Grammar they use. This allows the Grammar to be a flywheel object.
   class RJCPT_CORE_EXPORT Grammar final
   {
   public:
      static constexpr std::uint32_t npos = 0xffffffff;

      struct CompileOptions
      {
         std::optional<unsigned int> maxInput;
      };

      //! Compiles a grammar definition string.
      //! Returns any errors that occurred in the process.
      std::vector<std::string> Compile(std::string_view aDefinition, CompileOptions aOptions = {});
      
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
      friend class GrammarCompiler;

      //! Returns the index (of mRules) that the parser should jump to for the given top rule type and current input type.
      //! Returns npos on failure.
      std::uint32_t GetJumpIndex(RuleRef aRuleType, InType aInputType) const;
      std::uint32_t GetJumpIndexP(RuleRef aRuleType, std::uint32_t aInputIndex) const;

      std::vector<GrammarRule> mRules;
      std::vector<GrammarNode> mNodes;
      std::vector<InType>      mInputTypes;

      // The jump table.
      // Each row corresponds to a differently-named rule.
      //    Multiple rules can share a name. All rules with the same name appear on the same row.
      // Each column corresponds to a different input type.
      //    The column of an input type is the same as that type's index in mInputTypes.
      // Column 0 is reserved for the "epsilon" value, which has special meaning.
      // The stored values are indices into the mRules vector, or npos to mean "no value".
      std::vector<std::uint32_t> mJumpIndices;
      std::uint32_t mJumpTableRows = 0;
      std::uint32_t mJumpTableColumns = 0;
   };
}
