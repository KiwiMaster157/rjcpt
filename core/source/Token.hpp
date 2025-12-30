#pragma once

#include <cstdint>

namespace rjcpt
{
   enum class TokenType
   {
      // Special tokens
      EndOfData = 0,
      Error,
      Number,
      Identifier,
      
      // Normal operators
      Plus,
      Hyphen,
      Asterisk,
      Slash,
      Caret,
      Comma,
      Colon,
      DollarSign,
      
      // Grouping tokens
      LeftParenthesis,
      RightParenthesis,
      LeftBracket,
      RightBracket,
      LeftBrace,
      RightBrace,
      
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
   inline constexpr int cNUM_TOKEN_TYPES = static_cast<int>(TokenType::cMAX_TOKEN);

   //! A Token is a sequence of characters that represent an indivisible logical unit in an expression.
   //! Rather than storing the characters themselves, the Token type stores the start index and length of the substring.
   struct Token
   {
      TokenType     mType       = TokenType::Error;
      std::uint32_t mStartIndex = 0;
      std::uint32_t mLength     = 0;
   };
}
