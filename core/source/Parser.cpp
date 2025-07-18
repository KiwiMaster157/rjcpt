#include "Parser.hpp"

rjcpt::ParseResult rjcpt::ParseTokens(std::string_view aExpression, const std::vector<Token>& aTokens)
{
   if (aTokens.empty())
   {
      return std::unexpected("Cannot parse empty tokens list.");
   }
   else if (aTokens.back().mType != TokenType::EndOfData)
   {
      return std::unexpected("Cannot parse tokens without end-of-data terminator.");
   }

   ParseResult retval;
   std::vector<ParseNode> operatorStack;

   for (std::size_t i = 0; i < aTokens.size(); i++)
   {
      const Token& t = aTokens[i];
      const ParseNodeType topType = operatorStack.empty() ? ParseNodeType::cMAX_PARSE_NODE : operatorStack.back().mType;
      ParseNode node = detail::MakeParseNode(t, topType);
      switch (node.mType)
      {
      case ParseNodeType::Finished:
         if (i + 1 == aTokens.size())
         {
            return retval;
         }
         else
         {
            return std::unexpected("Unexpected end of input.");
         }
         break;
      case ParseNodeType::Number:
      case ParseNodeType::Identifier:
         // TODO...
         break;
      case ParseNodeType::cMAX_PARSE_NODE:
      case detail::cLEFT_PAREN:
      case detail::cLEFT_BRACKET:
         // TODO...
         break;
      default:
         
      }
   }

   return retval;
}
