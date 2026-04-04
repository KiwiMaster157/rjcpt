#pragma once

#include "SmallString.hpp"

#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "rjcpt_core_export.h"

namespace rjcpt::lex
{
   inline constexpr std::size_t cMAX_NAME = 16;
   using name_t = SmallString<cMAX_NAME>;

   struct Token final
   {
      std::string_view Get(std::string_view aData) const
      {
         return aData.substr(mPosition, mSize);
      }

      friend bool operator==(const Token&, const Token&) noexcept = default;

      name_t        mTypeName;
      std::uint32_t mTypeIndex = 0;

      std::int32_t  mRow = 1;
      std::int32_t  mColumn = 1;
      std::uint32_t mPosition = 0;
      std::uint32_t mSize = 0;
   };

   //! An optimized std::set<char> or std::set<unsigned char>.
   using char_set_t = std::bitset<256>;
   //! Wrapper for a char_set_t with a name.
   //! While the underlying char_set_t is accessible,
   //! it is less error prone to use the provided API.
   struct RJCPT_CORE_EXPORT CharacterClass final
   {
      //! Adds aCharacter to the set.
      void Add(char aCharacter);
      //! Removes aCharacter from the set.
      void Remove(char aCharacter);
      //! Checks whether aCharacter is in the set.
      bool Test(char aCharacter) const;

      friend bool operator==(const CharacterClass&,
                             const CharacterClass&) noexcept = default;

      name_t     mName;
      char_set_t mData;
   };

   struct Sequence final
   {
      friend bool operator==(const Sequence&,
                             const Sequence&) noexcept = default;

      name_t        mName;
      std::uint32_t mBegin = 0;
      std::uint32_t mSize = 0;
   };

   struct SequenceRule final
   {
      enum class Type : std::uint8_t
      {
         EndOfSequence,
         RawCharacters,
         CharacterClass,
         Sequence,
      };
      enum class Quantity : std::uint8_t
      {
         Once,
         ZeroPlus,
         OnePlus,
         Optional,
         Never
      };
      using enum Type;

      friend bool operator==(const SequenceRule&,
                             const SequenceRule&) noexcept = default;

      Type          mType = Type::EndOfSequence;
      Quantity      mQuantity = Quantity::Once;
      std::uint32_t mData = 0;
      std::uint32_t mSize = 0;
   };

   struct SequenceRuleData final
   {
      SequenceRule::Quantity mQuantity;
      std::variant<std::monostate, std::string_view, char_set_t, Sequence>
         mAction;
   };

   //! Returns the first line from aInput, and removes the characters from the front of aInput.
   //! If aInput is empty, returns an empty optional.
   RJCPT_CORE_EXPORT std::optional<std::string_view> ExtractLine(std::string_view& aInput);
   //! Returns the first word from aInput, and removes the characters from the front of aInput.
   //! Skips over any leading whitespace.
   //! If there are no more words in aInput, returns an empty optional.
   RJCPT_CORE_EXPORT std::optional<std::string_view> ExtractWord(std::string_view& aInput);
   //! If aHex is a string beginning with the '%' character,
   //! treats the string as a single hexadecimal ASCII character.
   //! If the string cannot be interpreted as one, returns an empty optional.
   RJCPT_CORE_EXPORT std::optional<char> ReadHexCharacter(std::string_view aHex);

   //! Lexicon is a runtime-reprogrammable lexer for a grammar.
   //! To reprogram it, call Lexicon::Compile.
   //! The passed string_view will be parsed in the following way:
   //! After removing comments ('#' to end of line), each non-empty line defines one item.
   //! The first word in the line names the item.
   //! Names beginning with '$' are character classes.
   //! Names beginning with '.' are character sequences.
   //! Names beginning with '!' are ignored token types.
   //! Names beginning with any other character are token types.
   struct RJCPT_CORE_EXPORT Lexicon final
   {
      //! Resets the Lexicon.
      void Reset();

      SequenceRuleData GetRule(std::size_t aIndex) const;

      std::vector<CharacterClass> mCharClasses;
      std::vector<Sequence>       mSequences;
      std::vector<Sequence>       mTokenTypes;
      std::vector<SequenceRule>   mRules;
      std::string                 mCharData;
   };
}
