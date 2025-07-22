#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include "Token.hpp"

#include "rjcpt_core_export.h"

namespace rjcpt
{
   // Functions the detail namespace are deemed complicated enough to warrant unit testing.
   // Otherwise, they could be fully defined in the cpp file.
   namespace detail
   {
      //! Returns the index of the first character after the end of a number.
      //! Requires that aStart refers to the first character of a number.
      //! The boolean returns 'true' for success, or 'false' for failure.
      RJCPT_CORE_EXPORT std::pair<std::uint32_t, bool> FindExponentEnd(std::string_view aExpression, std::uint32_t aStart);
      RJCPT_CORE_EXPORT std::pair<std::uint32_t, bool> FindNumberEnd(std::string_view aExpression, std::uint32_t aStart);
      
      //! Given an expression and a start index, returns the next token from the string.
      RJCPT_CORE_EXPORT Token FindNextToken(std::string_view aExpression, std::uint32_t aIndex);
   }

   //! Takes an expression and tokenizes it as if by calling FindNextToken repeatedly.
   RJCPT_CORE_EXPORT std::vector<Token> TokenizeExpression(std::string_view aExpression);
}
