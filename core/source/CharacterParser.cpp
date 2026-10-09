#include "CharacterParser.hpp"

rjcpt::lex::CharacterParser::CharacterParser(const Grammar* aGrammar)
   : Parser(aGrammar)
{}

std::string rjcpt::lex::CharacterParser::ParseText(std::string_view aText, RuleRef aStart)
{
   mText = aText;
   mError.clear();
   Init(aStart, ParseOptions{ .checkFirstFollow = false });
   for (std::uint32_t i = 0; i < aText.size(); i++)
   {
      const auto e = PushInput(i, InType(aText[i]));

      if (aText[i] == '\n')
      {
         ++mRow;
         mColumn = 1;
      }
      else
      {
         ++mColumn;
      }
      
      if (e != Result::Okay)
      {
         return mError;
      }
   }
   if (EndOfInput(aText.size()) != Result::Okay)
   {
      return mError;
   }
   return std::string();
}

void rjcpt::lex::CharacterParser::Output(std::uint32_t aIndex, InType aInput, OutType aOut)
{
   if (mHasCurrentToken)
   {
      if (aOut == cBEGIN_TOKEN)
      {
         Error(aIndex, aInput, "Cannot begin a new token.");
      }
      else
      {
         mHasCurrentToken = false;
         mCurrentToken.mSize = aIndex - mCurrentToken.mPosition;
         EmitToken(mCurrentToken, aOut);
      }
   }
   else
   {
      if (aOut == cBEGIN_TOKEN)
      {
         mHasCurrentToken = true;
         mCurrentToken.mRow = mRow;
         mCurrentToken.mColumn = mColumn;
         mCurrentToken.mPosition = aIndex;
         mCurrentToken.mSize = 0;
      }
      else
      {
         Error(aIndex, aInput, "No token in progress.");
      }
   }
}

void rjcpt::lex::CharacterParser::Error(std::uint32_t aIndex, InType aInput, const std::string& aError)
{
   mError = aError;
   if (mHasCurrentToken)
   {
      Token t = mCurrentToken;
      t.mSize = aIndex - t.mPosition;
   }

   mError += " Near row " + std::to_string(mRow) + ", column " + std::to_string(mColumn);
}
