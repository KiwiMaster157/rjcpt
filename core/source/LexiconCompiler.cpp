#include "LexiconCompiler.hpp"

#include <algorithm>
#include <cassert>

rjcpt::lex::CompileLexiconResult rjcpt::lex::CompileLexicon(std::string_view aInput)
{
   detail::LexiconCompiler compiler;
   compiler.Compile(aInput);
   return compiler.mResult;
}

void rjcpt::lex::detail::LexiconCompiler::Compile(std::string_view aInput)
{
   mResult.emplace();
   mResult->Reset();
   mRow = 0;

   auto line = ExtractLine(aInput);
   for (; line && mResult.has_value(); line = ExtractLine(aInput))
   {
      const std::size_t beginComment = line->find('#');
      std::string_view  data = line->substr(0, beginComment);

      ++mRow;
      const auto name = ExtractWord(data);
      if (!name)
      {
         continue;
      }
      else if (!name_t::CanConvert(*name))
      {
         SetError({ "Name too long: ", *name });
      }
      else if (name->starts_with('$'))
      {
         AppendCharacterClass(*name, data);
      }
      else if (name->starts_with('.'))
      {
         AppendSequence(*name, data);
      }
      else
      {
         AppendTokenType(*name, data);
      }
   }
}

bool rjcpt::lex::detail::LexiconCompiler::AppendCharacterClass(name_t aName, std::string_view aDefinition)
{
   auto compareName = [](const CharacterClass& cc) { return cc.mName; };
   const bool exists = std::ranges::contains(mResult->mCharClasses, aName, compareName);
   if (exists)
   {
      SetError({ "A character class with this name already exists: ", aName.view() });
      return false;
   }
   if (aName == "$")
   {
      SetError({ "Character class must be given a name." });
      return false;
   }

   CharacterClass cc;
   cc.mName = aName;

   while (auto word = ExtractWord(aDefinition))
   {
      if (!ReadCharacterClassElement(cc, *word))
      {
         return false;
      }
   }
   mResult->mCharClasses.push_back(cc);
   return true;
}

bool rjcpt::lex::detail::LexiconCompiler::AppendSequence(name_t aName, std::string_view aDefinition)
{
   auto compareName = [](const Sequence& cc) { return cc.mName; };
   const bool exists = std::ranges::contains(mResult->mSequences, aName, compareName);
   if (exists)
   {
      SetError({ "A sequence with this name already exists: ", aName.view() });
      return false;
   }
   if (aName == ".")
   {
      SetError({ "Sequence must be given a name." });
      return false;
   }

   if (auto seq = ReadFullSequence(aName, aDefinition))
   {
      mResult->mSequences.push_back(*seq);
      return true;
   }
   return false;
}

bool rjcpt::lex::detail::LexiconCompiler::AppendTokenType(name_t aName, std::string_view aDefinition)
{
   if (auto index = ReadHexCharacter(aName.view()))
   {
      const std::size_t ttCount = mResult->mTokenTypes.size();
      if (*index != ttCount)
      {
         SetError({ "Explicit index (", aName.view(), ") does not match expected (", std::to_string(ttCount), ")." });
         return false;
      }
      auto name2 = ExtractWord(aDefinition);
      if (!name2)
      {
         SetError({ "Cannot determine token type name." });
         return false;
      }
      else if (!name_t::CanConvert(*name2))
      {
         SetError({ "Name too long: ", *name2 });
      }
      aName = *name2;
   }

   auto compareName = [](const Sequence& cc) { return cc.mName; };
   const bool exists = std::ranges::contains(mResult->mTokenTypes, aName, compareName);
   if (exists)
   {
      SetError({ "A token type with this name already exists: ", aName.view() });
      return false;
   }

   if (auto seq = ReadFullSequence(aName, aDefinition))
   {
      mResult->mTokenTypes.push_back(*seq);
      return true;
   }
   return false;
}

rjcpt::lex::SequenceRule::Quantity rjcpt::lex::detail::LexiconCompiler::ExtractQuantity(std::string_view& aWord) const
{
   assert(!aWord.empty());
   switch (aWord.back())
   {
   case '*':
      aWord.remove_suffix(1);
      return SequenceRule::Quantity::ZeroPlus;
   case '+':
      aWord.remove_suffix(1);
      return SequenceRule::Quantity::OnePlus;
   case '?':
      aWord.remove_suffix(1);
      return SequenceRule::Quantity::Optional;
   case '~':
      aWord.remove_suffix(1);
      return SequenceRule::Quantity::Never;
   default:
      return SequenceRule::Quantity::Once;
   }
}

std::optional<rjcpt::lex::Sequence> rjcpt::lex::detail::LexiconCompiler::ReadFullSequence(name_t aName, std::string_view aDefinition)
{
   Sequence seq;
   seq.mName = aName;
   seq.mBegin = mResult->mRules.size();
   seq.mSize = 0;

   while (auto word = ExtractWord(aDefinition))
   {
      auto rule = ReadSequenceRule(*word);
      if (!rule)
      {
         return std::nullopt;
      }
      mResult->mRules.push_back(*rule);
   }

   seq.mSize = mResult->mRules.size() - seq.mBegin;
   mResult->mRules.emplace_back(SequenceRule::Type::EndOfSequence);
   return seq;
}

std::optional<rjcpt::lex::SequenceRule> rjcpt::lex::detail::LexiconCompiler::ReadSequenceRule(std::string_view aWord)
{
   assert(!aWord.empty());
   auto compareName = [](const auto& x) { return x.mName; };
   std::string_view temp = aWord;
   const auto q = ExtractQuantity(aWord);
   if (aWord.empty())
   {
      SetError({ "Quantity modifier does not have an object to modify: ", temp });
      return std::nullopt;
   }

   switch (aWord.front())
   {
   case '$':
   {
      auto& ccs = mResult->mCharClasses;
      auto it = std::ranges::find(ccs, aWord, compareName);
      if (it == ccs.end())
      {
         SetError({ "Unable to find character class: ", aWord });
         return std::nullopt;
      }
      return SequenceRule(SequenceRule::CharacterClass, q, it - ccs.begin());
   }
   case '%':
      if (auto c = ReadHexCharacter(aWord))
      {
         auto& cd = mResult->mCharData;
         const std::size_t pos = cd.size();
         cd += *c;
         return SequenceRule(SequenceRule::RawCharacters, q, pos, 1);
      }
      return std::nullopt;
   case '.':
   {
      auto& seqs = mResult->mSequences;
      auto it = std::ranges::find(seqs, aWord, compareName);
      if (it == seqs.end())
      {
         SetError({ "Unable to find sequence: ", aWord });
         return std::nullopt;
      }
      return SequenceRule(SequenceRule::Sequence, q, it - seqs.begin());
   }
   default:
   {
      auto& cd = mResult->mCharData;
      const std::size_t pos = cd.size();
      cd += aWord;
      return SequenceRule(SequenceRule::RawCharacters, q, pos, aWord.size());
   }
   }
}

bool rjcpt::lex::detail::LexiconCompiler::ReadCharacterClassElement(CharacterClass& aOut, std::string_view aWord)
{
   assert(!aWord.empty());
   switch (aWord.front())
   {
   case '$':
   {
      auto compareName = [](const CharacterClass& cc) { return cc.mName; };
      auto it = std::ranges::find(mResult->mCharClasses, aWord, compareName);
      if (it == mResult->mCharClasses.end())
      {
         SetError({ "Unable to find character class: ", aWord });
         return false;
      }
      aOut.mData |= it->mData;
      return true;
   }
   case '%':
      if (auto c = ReadHexCharacter(aWord))
      {
         aOut.Add(*c);
         return true;
      }
      SetError({ "Invalid hex character: ", aWord });
      return false;
   case '-':
      if (aWord.size() <= 1)
      {
         SetError({ "Nothing to negate." });
      }
      else
      {
         CharacterClass temp;
         if (ReadCharacterClassElement(temp, aWord.substr(1)))
         {
            aOut.mData &= ~temp.mData;
            return true;
         }
      }
      return false;
   default:
      for (char c : aWord)
      {
         aOut.Add(c);
      }
      return true;
   }
}

void rjcpt::lex::detail::LexiconCompiler::SetError(std::initializer_list<std::string_view> aData)
{
   std::string error = "Line ";
   error += std::to_string(mRow);
   error += ": ";
   for (auto s : aData)
   {
      error.append(s);
   }
   mResult = std::unexpected(std::move(error));
}
