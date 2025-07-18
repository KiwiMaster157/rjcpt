#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "ParseNode.hpp"
#include "Token.hpp"

namespace rjcpt
{
   // Functions the detail namespace are deemed complicated enough to warrant unit testing.
   // Otherwise, they could be fully defined in the cpp file.
   namespace detail
   {
      // These two pseudo-ParseNodeTypes are used for handling '(' and '[' tokens in the operator stack.
      inline constexpr rjcpt::ParseNodeType cLEFT_PAREN{static_cast<unsigned>(rjcpt::ParseNodeType::cMAX_PARSE_NODE) + 1};
      inline constexpr rjcpt::ParseNodeType cRIGHT_PAREN{static_cast<unsigned>(rjcpt::ParseNodeType::cMAX_PARSE_NODE) + 2};
      inline constexpr rjcpt::ParseNodeType cLEFT_BRACKET{static_cast<unsigned>(rjcpt::ParseNodeType::cMAX_PARSE_NODE) + 3};
      inline constexpr rjcpt::ParseNodeType cRIGHT_BRACKET{static_cast<unsigned>(rjcpt::ParseNodeType::cMAX_PARSE_NODE) + 4};

      // Creates a ParseNode from aToken and the top of the operator stack.
      // May return a node with a proper ParseNodeType, or one of the pseudo-types above.
      ParseNode MakeParseNode(const Token& aToken, ParseNodeType aTopOperator);

      enum class PrecedesResult
      {
         LeftPrecedes,
         RightPrecedes,
         NonAssociative,
         InvalidInput
      };

      // Returns the precedence for operators aLeft and aRight.
      // For example, multiplication precedes addition because it comes first in order-of-operations.
      // If two operations are tied, returns true if the operation is left-associative.
      PrecedesResult Precedes(ParseNodeType aLeft, ParseNodeType aRight);
   }

   using ParseResult = std::expected<std::vector<ParseNode>, std::string>;
   ParseResult ParseTokens(std::string_view aExpression, const std::vector<Token>& aTokens);
}
