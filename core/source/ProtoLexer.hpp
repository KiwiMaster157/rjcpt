#pragma once

#include "SmallString.hpp"
#include "Token.hpp"

#include <vector>

#include "rjcpt_core_export.h"

namespace rjcpt::lex
{
   //! A simple lexer that divides on whitespace and supports ignoring comments.
   class RJCPT_CORE_EXPORT ProtoLexer
   {
   public:
      inline static constexpr char cWHITESPACE[]      = " \n\r\t";
      inline static constexpr char cWORD_TERMINATOR[] = " \n\r\t#";
      inline static constexpr char cCOMMENT_BEGIN     = '#';
      inline static constexpr char cCOMMENT_END       = '\n';

      //! @brief Lexes a string into tokens.
      //! @param aText The text to be lexed.
      //! @return The list of tokens.
      static std::vector<Token> LexInput(std::string_view aText);

      //! @brief Initializes a lexer to stream tokens from aText with the given settings.
      //! @param aText The text to parse. It is the user's responsibility to ensure this remains valid for the lifetime of the lexer.
      //! @param aRow The index of the starting line. Generally one, but may be something else when lexing a subset of a larger string.
      explicit ProtoLexer(std::string_view aText, std::uint32_t aRow = 1);

      //! @brief Reads into aToken.
      //!        Stops reading once the first of the separator characters is read.
      //!        Intended to be used as `while (lexer.ReadToken(out)) { ... }`
      //! @param aToken The returned token.
      //! @return True on success, or false on failure.
      bool ReadToken(Token& aToken);

      //! @brief Peeks at the next token, but doesn't read it from the input.
      //! @param aToken 
      //! @param aOption 
      //! @return True if a token was read, or false otherwise.
      bool PeekToken(Token& aToken) const;

      std::string_view GetText() const { return mText; }
      std::uint32_t    GetRow() const { return mRow; }
      std::uint32_t    GetColumn() const { return mColumn; }
      std::uint32_t    GetPosition() const { return mPosition; }

   private:
      //! @brief Advances mPosition to aPosition, updating mRow and mColumn as necessary.
      //! @param aPosition The new position.
      void Seek(std::size_t aPosition);

      std::string_view mText;
      
      std::uint32_t mRow = 1;
      std::uint32_t mColumn = 1;
      std::uint32_t mPosition = 0;
   };
}
