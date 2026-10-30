#include "IniReader.hpp"

#include <algorithm>
#include <istream>

rjcpt::core::IniReader::IniReader(std::string aInput)
   : mInput(std::move(aInput))
{}

auto rjcpt::core::IniReader::Read(std::string aInput) -> std::vector<value_type>
{
   IniReader reader(std::move(aInput));

   std::vector<value_type> retval;

   value_type value;
   while (reader.ReadValue(value))
   {
      retval.emplace_back(value);
   }
   return retval;
}

bool rjcpt::core::IniReader::ReadValue(value_type& aOut)
{
   rjcpt::lex::Token line;
   while (GetLine(line))
   {
      StripWhitespace(line);
      if (line.mSize == 0)
      {
         continue;
      }

      const auto sv = line.Get(mInput);
      if (sv.starts_with('[') && sv.ends_with(']'))
      {
         mSection = sv.substr(1, sv.size() - 2);
         continue;
      }
      const std::size_t eq = sv.find('=');
      if (eq != std::string::npos)
      {
         aOut.section = mSection;

         aOut.key       = line;
         aOut.key.mSize = eq;

         aOut.value = line;
         aOut.value.mPosition += eq + 1;
         aOut.value.mColumn += eq + 1;
         aOut.value.mSize -= eq + 1;

         Normalize(aOut.key);
         Normalize(aOut.value);
         return true;
      }
      mPosition = std::string::npos;
   }
   return false;
}

bool rjcpt::core::IniReader::GetLine(rjcpt::lex::Token& aLine)
{
   aLine = rjcpt::lex::Token();
   aLine.mPosition = mPosition;
   if (mPosition < mInput.size())
   {
      const std::size_t pos = mInput.find('\n', mPosition);
      ++mLine;
      aLine.mRow = mLine;
      if (pos == std::string::npos)
      {
         aLine.mSize = mInput.size() - mPosition;
         aLine.mRow = mLine;
         mPosition = mInput.size();
      }
      else
      {
         aLine.mSize = pos - mPosition;
         mPosition = pos + 1;
      }
      return true;
   }
   aLine.mRow = mLine;
   return false;
}

void rjcpt::core::IniReader::Normalize(rjcpt::lex::Token& aToken) const
{
   const std::size_t first = mInput.find_first_not_of(" \t\r\n", aToken.mPosition);
   const std::size_t last = mInput.find_last_not_of(" \t\r\n", aToken.mPosition + aToken.mSize);

   if (last == std::string::npos)
   {
      aToken.mPosition += aToken.mSize;
      aToken.mColumn += aToken.mSize;
      aToken.mSize = 0;
   }
   else
   {
      aToken.mColumn += first - aToken.mPosition;
      aToken.mPosition = first;
      aToken.mSize = last - first;
   }
}

void rjcpt::core::IniReader::Normalize(rjcpt::lex::Token& aToken) const
{
   StripWhitespace(aToken);
   if (mInput[aToken.mPosition] == '"' && mInput[aToken.mPosition + aToken.mSize - 1] == '"' && aToken.mSize > 1)
   {
      aToken.mColumn++;
      aToken.mPosition++;
      aToken.mSize -= 2;
   }
}

auto rjcpt::core::GetSubrange(std::span<const IniReader::value_type> aInput, const std::string& aRange) -> std::span<const IniReader::value_type>
{
   auto proj = [](const IniReader::value_type& x) -> decltype(auto) { return x.section; };
   auto begin = std::ranges::lower_bound(aInput, aRange, {}, proj);
   std::string endKey = aRange;
   if (!endKey.empty())
   {
      ++endKey.back();
   }
   auto end = std::ranges::lower_bound(begin, aInput.end(), endKey, {}, proj);
   return { begin, end };
}
