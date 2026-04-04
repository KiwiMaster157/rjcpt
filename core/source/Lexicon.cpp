#include "Lexicon.hpp"

#include <algorithm>
#include <charconv>

void rjcpt::lex::CharacterClass::Add(char aCharacter)
{
   mData.set(static_cast<std::uint8_t>(aCharacter));
}

void rjcpt::lex::CharacterClass::Remove(char aCharacter)
{
   mData.reset(static_cast<std::uint8_t>(aCharacter));
}

bool rjcpt::lex::CharacterClass::Test(char aCharacter) const
{
   return mData.test(static_cast<std::uint8_t>(aCharacter));
}

std::optional<std::string_view>
rjcpt::lex::ExtractLine(std::string_view& aInput)
{
   if (!aInput.empty())
   {
      const std::size_t eol = aInput.find('\n') + 1;
      const std::size_t lineSize = eol ? eol : aInput.size();
      const std::string_view line = aInput.substr(0, lineSize);
      aInput.remove_prefix(lineSize);
      return line;
   }
   return std::nullopt;
}

std::optional<std::string_view>
rjcpt::lex::ExtractWord(std::string_view& aInput)
{
   const std::size_t beginData = aInput.find_first_not_of(" \r\n");
   if (beginData != std::string_view::npos)
   {
      std::size_t endData = aInput.find_first_of(" \r\n", beginData);
      endData = std::min(endData, aInput.size());
      const std::string_view word = aInput.substr(beginData, endData - beginData);
      aInput.remove_prefix(endData);
      return word;
   }
   return std::nullopt;
}

std::optional<char> rjcpt::lex::ReadHexCharacter(std::string_view aHex)
{
   if (aHex.starts_with('%') && aHex.size() == 3)
   {
      const char* begin = aHex.data() + 1;
      const char* end = aHex.data() + 3;
      unsigned value = 0;
      auto result = std::from_chars(begin, end, value, 16);
      if (result.ec == std::errc() && result.ptr == end)
      {
         return static_cast<char>(value);
      }
   }
   return std::nullopt;
}

void rjcpt::lex::Lexicon::Reset()
{
   mCharClasses.clear();
   mSequences.clear();
   mTokenTypes.clear();
   mRules.clear();
   mCharData.clear();

   auto& all = mCharClasses.emplace_back("$$");
   all.mData.set();
}

rjcpt::lex::SequenceRuleData
rjcpt::lex::Lexicon::GetRule(std::size_t aIndex) const
{
   if (aIndex >= mRules.size())
   {
      throw std::out_of_range("Lexicon rule index out of range.");
   }
   const auto& rule = mRules[aIndex];
   switch (rule.mType)
   {
   case SequenceRule::CharacterClass:
      if (rule.mData < mCharClasses.size())
      {
         return { rule.mQuantity, mCharClasses[rule.mData].mData };
      }
      throw std::out_of_range("Lexicon character class out of range.");
   case SequenceRule::RawCharacters:
     // All three checks are necessary.
      if (rule.mData < mCharData.size() && rule.mSize < mCharData.size() &&
          rule.mData + rule.mSize < mCharData.size())
      {
         return { rule.mQuantity,
                 std::string_view(mCharData).substr(rule.mData, rule.mSize) };
      }
      throw std::out_of_range("Lexicon raw characters out of range.");
   case SequenceRule::Sequence:
      if (rule.mData < mSequences.size())
      {
         return { rule.mQuantity, mSequences[rule.mData] };
      }
      throw std::out_of_range("Lexicon sequence out of range.");
   case SequenceRule::EndOfSequence:
      return { rule.mQuantity, std::monostate() };
   default:
      throw std::runtime_error("Lexicon rule invalid type.");
   }
}
