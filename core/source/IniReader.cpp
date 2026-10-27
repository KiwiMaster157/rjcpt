#include "IniReader.hpp"

#include <algorithm>
#include <istream>

namespace
{
   void StripWhitespace(std::string& aStr)
   {
      const std::size_t first = aStr.find_first_not_of(" \t\r\n");
      const std::size_t last = aStr.find_last_not_of(" \t\r\n");

      if (last == std::string::npos)
      {
         aStr.clear();
      }
      else
      {
         aStr = aStr.substr(first, last - first);
      }

      if (aStr.starts_with('"') && aStr.ends_with('"') && aStr.size() > 1)
      {
         aStr = aStr.substr(1, aStr.size() - 2);
      }
   }
}

rjcpt::core::IniReader::IniReader(std::istream& aInput)
   : mInput(aInput)
{}

auto rjcpt::core::IniReader::Read(std::istream& aInput) -> std::vector<value_type>
{
   IniReader reader(aInput);

   value_type value;

   std::vector<value_type> retval;
   while (reader.ReadValue(value))
   {
      retval.emplace_back(value);
   }
   return retval;
}

bool rjcpt::core::IniReader::ReadValue(value_type& aOut)
{
   std::string line;
   while (std::getline(mInput, line))
   {
      ++mLine;
      StripWhitespace(line);
      if (line.empty())
      {
         continue;
      }
      else if (line.starts_with('[') && line.ends_with(']'))
      {
         mPrefix = line.substr(1, line.size() - 2);
         if (!mPrefix.empty())
         {
            mPrefix += "/";
         }
         continue;
      }
      const std::size_t eq = line.find('=');
      if (eq != std::string::npos)
      {
         aOut.key   = mPrefix + line.substr(0, eq);
         aOut.value = line.substr(eq + 1);
         aOut.line = mLine;
         StripWhitespace(aOut.value);
         return true;
      }
      mInput.setstate(std::ios::badbit, true);
   }
   return false;
}

auto rjcpt::core::GetSubrange(std::span<const IniReader::value_type> aInput, const std::string& aRange) -> std::span<const IniReader::value_type>
{
   auto proj = [](const IniReader::value_type& x) -> decltype(auto) { return x.key; };
   auto begin = std::ranges::lower_bound(aInput, aRange, {}, proj);
   std::string endKey = aRange;
   if (!endKey.empty())
   {
      ++endKey.back();
   }
   auto end = std::ranges::lower_bound(begin, aInput.end(), endKey, {}, proj);
   return { begin, end };
}

