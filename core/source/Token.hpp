#pragma once

#include <cstdint>

namespace rjcpt
{
   enum class TokenType
   {
      // Special tokens
      EndOfData,
      Error,
      Number,
      Identifier,
      
      // Normal operators
      Plus,
      Hyphen,
      Asterisk,
      Slash,
      Comma,
      DollarSign,
      
      // Grouping tokens
      LeftParenthesis,
      RightParenthesis,

      // Function call
      LeftBracket,
      RightBracket,
      
      // Comparison tokens
      Equals,
      NotEquals,
      LessThan,
      LessOrEqual,
      GreaterThan,
      GreaterOrEqual,

      // Keyword tokens
      KeywordAnd,
      KeywordOr,
      KeywordNot,
      KeywordTrue,
      KeywordFalse,

      // Number of token types.
      // Useful for sizing static arrays.
      cMAX_TOKEN
   };

   //! A Token is a sequence of characters that represent an indivisible logical unit in an expression.
   //! Rather than storing the characters themselves, the Token type stores the start index and length of the substring.
   struct Token
   {
      TokenType     mType       = TokenType::Error;
      std::uint32_t mStartIndex = 0;
      std::uint32_t mLength     = 0;
   };
}
