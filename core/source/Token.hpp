#pragma once

#include <string_view>

namespace rjcpt::lex
{
   //! Represents a substring 
   struct Token
   {
      std::string_view Get(std::string_view aText) const
      {
         return aText.substr(mPosition, mSize);
      }
      void Get(const char*) const = delete;

      friend bool operator==(const Token&, const Token&) noexcept = default;

      std::uint32_t mRow      = 1;
      std::uint32_t mColumn   = 1;
      std::uint32_t mPosition = 0;
      std::uint32_t mSize     = 0;
   };
}