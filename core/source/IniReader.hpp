#pragma once

#include <rjcpt_core_export.h>

#include "Token.hpp"

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
         std::string       section;
         rjcpt::lex::Token key;
         rjcpt::lex::Token value;
      };

      explicit IniReader(std::string aInput);

      static std::vector<value_type> Read(std::string aInput);

      bool ReadValue(value_type& aOut);

      std::string_view GetInput() const { return mInput; }
      const std::string& GetInputRef() const { return mInput; }

   private:
      bool GetLine(rjcpt::lex::Token& aLine);
      void StripWhitespace(rjcpt::lex::Token& aToken) const;
      void Normalize(rjcpt::lex::Token& aToken) const;

      std::string   mInput;
      std::string   mSection;
      std::uint32_t mPosition = 0;
      std::uint32_t mLine = 0;
   };

   RJCPT_CORE_EXPORT
   std::span<const IniReader::value_type> GetSubrange(std::span<const IniReader::value_type> aInput, const std::string& aRange);
}
