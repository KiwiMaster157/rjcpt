#pragma once

#include <cstdint>

namespace rjcpt
{
   // Once parse nodes are ordered in postfix notation, it is possible to "execute" them on a stack machine by iterating in order.
   // Many of the descriptions for the ParseNodeTypes below include instructions for how to evaluate them in this manner.
   // In practice, ParseNodeTypes will be passed to a compiler to perform identifier lookup, semantic analysis, and optimization
   // to produce a list of EvaluatorInstructions.
   enum class ParseNodeType : unsigned
   {
      // The parse tree is finished.
      // There should be one value left on the stack: the final result.
      Finished,
      // Pushes a number onto the evaluation stack.
      Number,
      // Pushes an identifier onto the evaluation stack.
      Identifier,
      // Pushes a boolean onto the evaluation stack.
      LogicalTrue,
      LogicalFalse,

      // Unary prefix operators.
      // Replace the top value on the stack.
      UnaryPlus,  // '+'
      UnaryMinus, // '-'
      RowLookup,  // '$'
      LogicalNot, // 'not'

      // Binary infix operators.
      // Pop the top two values from the stack and push a result.
      Addition,       // '+'
      Subtraction,    // '-'
      Multiplication, // '*'
      Division,       // '/'

      // Concatenation is a form of multiplication which occurs when there is
      // no operator separating two value-yielding subexpressions.
      // It is a form of multiplication.
      Concatenation,

      // Logical infix operators.
      // When fully compiled, short-circuiting logic may be applied to these.
      // Pop the top two values from the stack and push a result.
      LogicalAnd,
      LogicalOr,

      // Comparison operators.
      // Similar comparison operators can chain.
      // E.g. "a < b < c" is equivalent to "(a < b) and (b < c)", except that b is only evaluated once.
      // Pop the top two values from the stack, push the second arg.
      // The result is combined with the current value in a separate 'comparison results' stack.
      CompareEqual,
      CompareNotEqual,
      CompareLessThan,
      CompareLessOrEqual,
      CompareGreateThan,
      CompareGreaterOrEqual,
      // Begins a chain of comparisons, just before the first comparison.
      // Push 'true' to the 'comparison results' stack.
      CompareBegin,
      // Ends a chain of comparisons.
      // Pop the top value from the stack.
      // Then, push the top value of the 'comparison results' stack onto the evaluation stack.
      // Then, pop the top value of the 'comparison results' stack.
      CompareEnd,

      // The value stored in the node's mAuxData member is the number of arguments.
      // Pop the invocable from the top of the execution stack.
      // Then, pop [mAuxData] arguments from the execution stack.
      // Then, push the result onto the stack.
      Invoke,

      // Number of parse node types.
      // Useful for sizing static arrays.
      cMAX_PARSE_NODE,

      // Any ParseNodeType numbered cBEGIN_PARSE_NODE or above is considered an error code.
      cBEGIN_PARSE_ERRORS = 1000,
      cERROR_INVALID_TOKEN,
      cERROR_NON_ASSOCIATIVE_OPERATOR,
      cERROR_UNEXPECTED_PAREN,
      cERROR_UNEXPECTED_BRACKET,
      cERROR_UNEXPECTED_SYMBOL,
      cERROR_MISSING_RIGHT_PAREN,
      cERROR_MISSING_RIGHT_BRACKET
   };

   //! Returns true if aType represents an error code.
   inline constexpr bool IsErrorType(ParseNodeType aType)
   {
      return static_cast<unsigned>(aType) >= static_cast<unsigned>(ParseNodeType::cBEGIN_PARSE_ERRORS);
   }
   
   //! ParseNodes are nodes in a parse tree ordered in postfix notation.
   //! This parse tree is based entirely on the input tokens;
   //! it does not handle symbol lookup to short-circuiting logic.
   //! Similar to Tokens, ParseNodes store the start index and length
   //! of the substring they're generated from instead of raw character data.
   struct ParseNode
   {
      ParseNodeType mType       = ParseNodeType::cMAX_PARSE_NODE;
      std::uint32_t mStartIndex = 0;
      std::uint32_t mLength     = 0;
      std::uint32_t mAuxData    = 0;
   };
}
