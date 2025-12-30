#pragma once

#include "SmallString.hpp"

#include <bitset>
#include <cassert>
#include <functional>
#include <string_view>
#include <vector>
#include <span>

#include "rjcpt_core_export.h"

namespace rjcpt
{
   //! A GrammarNode is a terminal or non-terminal item in a compiled grammar.
   //! Each GrammarRule refers to a sequence of GrammarNodes that are expected to be matched in order.
   //! A terminal GrammarNode holds the indices of an optional "validator" and an optional "actor".
   //!   * The validator checks whether the current token matches the expected input.
   //!   * The actor is a function that acts on the information held in the token.
   //!   * An index of zero indicates the validator or actor is not present.
   //! A non-terminal GrammarNode references a list of GrammarRules it could refer to.
   class GrammarNode
   {
   public:
      static constexpr std::uint16_t cTERMINAL_MASK = 0x8000U;
      using string_type = SmallString<28>;

      GrammarNode() = default;

      void SetTerminal(std::uint16_t aValidator, std::uint16_t aActor)
      {
         assert(!(aValidator & cTERMINAL_MASK));
         mData1 = aValidator | cTERMINAL_MASK;
         mData2 = aActor;
      }
      void SetNonTerminal(std::uint16_t aBegin, std::uint16_t aEnd)
      {
         assert(!(aBegin & cTERMINAL_MASK));
         mData1 = aBegin & ~cTERMINAL_MASK;
         mData2 = aEnd;
      }
      
      bool          IsTerminal() const { return mData1 & cTERMINAL_MASK; }
      std::uint16_t ValidatorIndex() const { assert(IsTerminal()); return mData1 & ~cTERMINAL_MASK; }
      std::uint16_t ActorIndex() const { assert(IsTerminal()); return mData2; }
      std::uint16_t RulesBegin() const { assert(!IsTerminal()); return mData1 & ~cTERMINAL_MASK; }
      std::uint16_t RulesEnd() const { assert(!IsTerminal()); return mData2; }
      
      void               SetText(const string_type& aText) { mRawText = aText; }
      const string_type& RawText() const { return mRawText; }

   private:
      // Debug info
      string_type mRawText;

      std::uint16_t mData1 = 0;
      std::uint16_t mData2 = 0;
   };
   struct GrammarRule
   {
      SmallString<16> mName;
      std::uint32_t   mBegin = 0;
      std::uint32_t   mEnd   = 0;
   };

   class GrammarLocator
   {
   public:
      // Virtual destructor shouldn't be necessary, but it silences compiler warnings.
      virtual ~GrammarLocator() = default;
      //! Returns the numeric index/id of the validator with the given name.
      //! Returns 0 on failure.
      virtual std::uint16_t FindValidator(std::string_view aName) const = 0;
      //! Returns the numeric index/id of the actor with the given name.
      //! Returns 0 on failure.
      virtual std::uint16_t FindActor(std::string_view aName) const = 0;
   };

   struct CompiledGrammar
   {
      std::vector<GrammarRule> mRules;
      std::vector<GrammarNode> mNodes;
   };

   CompiledGrammar CompileGrammar(const GrammarLocator& aLocators, std::string_view aGrammarText);

   namespace grammar_util
   {
      //! Returns true if aChar is a whitespace character.
      constexpr bool IsSpace(char aChar)
      {
         return aChar == ' ' || aChar == '\t' || aChar == '\r' || aChar == '\n';
      }

      //! Returns a pointer to the start of the next word after aStart.
      //! If '#' is the first character in the next word, treats the rest of the line as a comment.
      constexpr const char* SkipSpace(const char* aStart)
      {
         while (true)
         {
            while (IsSpace(*aStart))
            {
               ++aStart;
            }
            if (*aStart != '#')
            {
               return aStart;
            }
            while (*aStart && *aStart != '\n')
            {
               ++aStart;
            }
         }
      }

      //! Extracts a word from aInput and sets it in aOutput.
      //! aInput is modified to the next character after the read word.
      //! If there is no more data left to extract, does nothing and returns false.
      //! Automatically skips whitespace and comments ('#' through end-of-line).
      RJCPT_CORE_EXPORT bool ExtractWord(const char*& aInput, std::string_view& aOutput);

      struct GrammarSize
      {
         std::size_t mNumRules = 0;
         std::size_t mNumNodes = 0;
      };
      //! Returns the number of rules and nodes in a grammar.
      RJCPT_CORE_EXPORT GrammarSize GetGrammarSize(std::string_view aGrammarText);

      //! Creates a GrammarNode that corresponds to the given grammar string.
      //! If the string is a terminal, the node is fully initialized.
      //! If the string is a non-terminal, the node has indices (0, 0).
      RJCPT_CORE_EXPORT GrammarNode MakeGrammarNode(const GrammarLocator& aLocator, std::string_view aGrammarString);

      //! Performs the second step of grammar compilation: setting the being/end indices in non-terminal nodes.
      //! The order of the rules may be modified.
      RJCPT_CORE_EXPORT void SetNonTerminalIndices(CompiledGrammar& aGrammar);
   }
}

