#include "Lexer.hpp"

#include <cctype>

namespace
{
   bool IsSpace(char aCharacter)
   {
      return std::isspace(static_cast<unsigned char>(aCharacter));
   }

   bool IsDigit(char aCharacter)
   {
      return std::isdigit(static_cast<unsigned char>(aCharacter));
   }

   bool IsNumberBegin(char aCharacter)
   {
      return IsDigit(aCharacter) || aCharacter == '.';
   }

   bool IsIdentifierAny(char aCharacter)
   {
      return std::isalpha(static_cast<unsigned char>(aCharacter)) || aCharacter == '_' || aCharacter == '%';
   }

   bool IsIdentifierBegin(char aCharacter)
   {
      return IsIdentifierAny(aCharacter) || aCharacter == '%';
   }

   bool IsIdentifierContinue(char aCharacter)
   {
      return IsIdentifierAny(aCharacter) || IsNumberBegin(aCharacter);
   }

   char NextChar(std::string_view aExpression, std::uint32_t aIndex)
   {
      return (aIndex + 1 >= aExpression.size()) ? '\0' : aExpression[aIndex + 1];
   }

   rjcpt::TokenType GetWordType(std::string_view aWord)
   {
      using TT = rjcpt::TokenType;
      if (aWord == "and")
      {
         return TT::KeywordAnd;
      }
      else if (aWord == "or")
      {
         return TT::KeywordOr;
      }
      else if (aWord == "not")
      {
         return TT::KeywordNot;
      }
      else if (aWord == "true")
      {
         return TT::KeywordTrue;
      }
      else if (aWord == "false")
      {
         return TT::KeywordFalse;
      }
      return TT::Identifier;
   }

   template<bool IsWordChar(char)>
   std::uint32_t FindWordEnd(std::string_view aExpression, std::uint32_t aStart)
   {
      for (std::uint32_t i = aStart; i < aExpression.size(); i++)
      {
         if (!IsWordChar(aExpression[i]))
         {
            return i;
         }
      }
      return static_cast<std::uint32_t>(aExpression.size());
   }
}

std::pair<std::uint32_t, bool> rjcpt::detail::FindExponentEnd(std::string_view aExpression, std::uint32_t aStart)
{
   if (aStart >= aExpression.size())
   {
      return {aExpression.size(), false};
   }
   const char first = aExpression[aStart];
   if (first == '+' || first == '-')
   {
      ++aStart;
   }
   const std::uint32_t endPos = FindWordEnd<::IsDigit>(aExpression, aStart);
   return {endPos, endPos > aStart};
}

std::pair<std::uint32_t, bool> rjcpt::detail::FindNumberEnd(std::string_view aExpression, std::uint32_t aStart)
{
   bool hasDigits  = false;
   bool hasDecimal = false;

   for (std::uint32_t i = aStart; i < aExpression.size(); i++)
   {
      const char c = aExpression[i];
      if (std::isdigit(static_cast<unsigned char>(c)))
      {
         hasDigits = true;
      }
      else if (c == '.')
      {
         if (hasDecimal)
         {
            return {FindWordEnd<::IsIdentifierAny>(aExpression, i), false};
         }
         hasDecimal = true;
      }
      else if (c == 'e' || c == 'E')
      {
         return FindExponentEnd(aExpression, i + 1);
      }
      else if (std::isalpha(static_cast<unsigned char>(c)))
      {
         return {FindWordEnd<::IsIdentifierAny>(aExpression, i), false};
      }
      else
      {
         return {i, hasDigits};
      }
   }
   return {aExpression.size(), hasDigits};
}

rjcpt::Token rjcpt::detail::FindNextToken(std::string_view aExpression, std::uint32_t aIndex)
{
   const std::uint32_t wordStart = ::FindWordEnd<::IsSpace>(aExpression, aIndex);

   if (wordStart >= aExpression.size())
   {
      return Token{TokenType::EndOfData, wordStart, 0};
   }
   const char first = aExpression[wordStart];
   switch (first)
   {
   case '+':
      return Token{TokenType::Plus, wordStart, 1};
   case '-':
      return Token{TokenType::Hyphen, wordStart, 1};
   case '*':
      return Token{TokenType::Asterisk, wordStart, 1};
   case '/':
      return Token{TokenType::Slash, wordStart, 1};
   case ',':
      return Token{TokenType::Comma, wordStart, 1};
   case '$':
      return Token{TokenType::DollarSign, wordStart, 1};
   case '(':
      return Token{TokenType::LeftParenthesis, wordStart, 1};
   case ')':
      return Token{TokenType::RightParenthesis, wordStart, 1};
   case '[':
      return Token{TokenType::LeftBracket, wordStart, 1};
   case ']':
      return Token{TokenType::RightBracket, wordStart, 1};
   case '=':
      return Token{TokenType::Equals, wordStart, 1};
   case '<':
      if (const char c = ::NextChar(aExpression, wordStart); c == '=')
      {
         return Token{TokenType::LessOrEqual, wordStart, 2};
      }
      else if (c == '>')
      {
         return Token{TokenType::NotEquals, wordStart, 2};
      }
      return Token{TokenType::LessThan, wordStart, 1};
   case '>':
      if (::NextChar(aExpression, wordStart) == '=')
      {
         return Token{TokenType::GreaterOrEqual, wordStart, 2};
      }
      return Token{TokenType::GreaterThan, wordStart, 1};
   default:
      if (::IsNumberBegin(first))
      {
         const auto endPos = detail::FindNumberEnd(aExpression, wordStart);
         const auto type   = endPos.second ? TokenType::Number : TokenType::Error;
         return Token{type, wordStart, endPos.first - wordStart};
      }
      else if (::IsIdentifierBegin(first))
      {
         const std::uint32_t endPos = ::FindWordEnd<::IsIdentifierContinue>(aExpression, wordStart);
         const std::string_view word = aExpression.substr(wordStart, endPos - wordStart);
         const TokenType     type   = ::GetWordType(word);
         return Token{type, wordStart, endPos - wordStart};
      }
      return Token{TokenType::Error, wordStart, 1};
   }
}

std::vector<rjcpt::Token> rjcpt::TokenizeExpression(std::string_view aExpression)
{
   std::vector<Token> retval;
   std::uint32_t      index = 0;
   while (true)
   {
      const Token t = detail::FindNextToken(aExpression, index);
      index = t.mStartIndex + t.mLength;
      retval.push_back(t);
      if (t.mType == TokenType::EndOfData || t.mType == TokenType::Error)
      {
         break;
      }
   }
   return retval;
}
