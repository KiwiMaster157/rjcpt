#pragma once

#include <rjcpt_core_export.h>

#include <iosfwd>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace rjcpt::core
{
   class RJCPT_CORE_EXPORT IniReader final
   {
   public:
      struct value_type
      {
         std::string   key;
         std::string   value;
         std::uint32_t line = 0;
      };
      
      IniReader(std::istream& aInput);

      static std::vector<value_type> Read(std::istream& aInput);

      bool ReadValue(value_type& aOut);

   private:
      std::istream& mInput;
      std::string   mPrefix;
      std::uint32_t mLine = 0;
   };

   RJCPT_CORE_EXPORT
   std::span<const IniReader::value_type> GetSubrange(std::span<const IniReader::value_type> aInput, const std::string& aRange);
}
