#include "ProtoLexer.hpp"

#include <cassert>

// static
std::vector<rjcpt::lex::Token> rjcpt::lex::ProtoLexer::LexInput(std::string_view aText)
{
   std::vector<rjcpt::lex::Token> retval;
   ProtoLexer lexer(aText);
   Token token;
   while (lexer.ReadToken(token))
   {
      retval.push_back(token);
   }
   return retval;
}

rjcpt::lex::ProtoLexer::ProtoLexer(std::string_view aText)
   : mText(aText)
{}

bool rjcpt::lex::ProtoLexer::ReadToken(Token& aToken)
{
   std::size_t pos = mPosition;

found_comment:
   if (pos < mText.size())
   {
      pos = mText.find_first_not_of(cWHITESPACE, pos);
      if (pos >= mText.size())
      {
         Seek(mText.size());
         return false;
      }
      else if (mText[pos] == cCOMMENT_BEGIN)
      {
         pos = mText.find(cCOMMENT_END, pos);
         goto found_comment;
      }

      std::size_t fin = mText.find_first_of(cWORD_TERMINATOR, pos);
      fin = std::min(fin, mText.size());

      Seek(pos);

      aToken.mRow = mRow;
      aToken.mColumn = mColumn;
      aToken.mPosition = pos;
      aToken.mSize = fin - pos;
      //aToken.mType     = 0;
      //aToken.mSubtype  = 0;

      Seek(fin);

      return true;
   }
   aToken.mRow = mRow;
   aToken.mColumn = mColumn;
   aToken.mPosition = mPosition;
   aToken.mSize = 0;
   return false;
}

bool rjcpt::lex::ProtoLexer::PeekToken(Token& aToken) const
{
   ProtoLexer temp = *this;
   return temp.ReadToken(aToken);
}

void rjcpt::lex::ProtoLexer::Seek(std::size_t aPosition)
{
   assert(aPosition <= mText.size());
   while (mPosition < aPosition)
   {
      if (mText[mPosition] == '\n')
      {
         mRow++;
         mColumn = 1;
      }
      else
      {
         mColumn++;
      }
      mPosition++;
   }
}
