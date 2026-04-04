#include "Lexer.hpp"

rjcpt::lex::Lexer::Lexer(const Lexicon* aLexicon, std::string_view aInput)
   : mLexicon(*aLexicon), mInput(aInput)
{}

rjcpt::lex::LexResult rjcpt::lex::Lexer::LexInput(const Lexicon& aLexicon,
                                                  std::string_view aString,
                                                  Option aOption)
{
   LexResult retval;
   Lexer lexer{ &aLexicon, aString };
   Token t;
   while (lexer.ReadToken(t, aOption))
   {
      retval.mTokens.push_back(t);
   }
   if (lexer.mPosition < aString.size())
   {
      auto& err = retval.mError.emplace();
      err.mPosition = lexer.mPosition;
      err.mRow = lexer.mRow;
      err.mColumn = lexer.mColumn;
   }
   return retval;
}

bool rjcpt::lex::Lexer::ReadToken(Token& aOut, Option aOption)
{
start:
   if (ReadTokenP(aOut))
   {
      if (aOption == Option::DropIgnored &&
          aOut.mTypeName.view().starts_with('!'))
      {
         goto start;
      }
      return true;
   }
   return false;
}

bool rjcpt::lex::Lexer::ReadTokenP(Token& aOut)
{
   std::string_view text = mInput.substr(mPosition);
   std::uint32_t maxLength = 0;
   std::optional<std::uint32_t> maxIndex;
   for (std::uint32_t i = 0; i < mLexicon.mTokenTypes.size(); i++)
   {
      auto count = EvaluateSequence(mLexicon.mTokenTypes[i], text);
      if (count.has_value() && (*count > maxLength))
      {
         maxLength = *count;
         maxIndex = i;
      }
   }
   if (maxIndex.has_value())
   {
      return ExtractToken(aOut, *maxIndex, maxLength);
   }
   return false;
}

std::optional<std::uint32_t>
rjcpt::lex::Lexer::EvaluateRule(const SequenceRule& aRule,
                                std::string_view aInput) const
{
   switch (aRule.mQuantity)
   {
   case SequenceRule::Quantity::Once:
      return EvaluateRuleP(aRule, aInput);
   case SequenceRule::Quantity::ZeroPlus:
   {
      std::uint32_t i = 0;
      auto temp = EvaluateRuleP(aRule, aInput.substr(i));
      while (temp)
      {
         i += *temp;
         temp = EvaluateRuleP(aRule, aInput.substr(i));
      }
      return i;
   }
   case SequenceRule::Quantity::OnePlus:
   {
      auto temp = EvaluateRuleP(aRule, aInput);
      if (temp)
      {
         std::uint32_t i = *temp;
         while (temp)
         {
            i += *temp;
            temp = EvaluateRuleP(aRule, aInput.substr(i));
         }
         return i;
      }
      return std::nullopt;
   }
   case SequenceRule::Quantity::Optional:
      return EvaluateRuleP(aRule, aInput).value_or(0);
   case SequenceRule::Quantity::Never:
      return EvaluateRuleP(aRule, aInput) ? std::nullopt : std::make_optional(0U);
   }
   assert(false);
   return std::nullopt;
}

std::optional<std::uint32_t>
rjcpt::lex::Lexer::EvaluateSequence(const Sequence& aSequence,
                                    std::string_view aInput) const
{
   std::uint32_t charsRead = 0;
   for (std::uint32_t i = 0; i < aSequence.mSize; i++)
   {
      const auto count =
         EvaluateRule(mLexicon.mRules[i], aInput.substr(charsRead));
      if (!count.has_value())
      {
         return std::nullopt;
      }
      charsRead += *count;
   }
   return charsRead;
}

std::optional<std::uint32_t>
rjcpt::lex::Lexer::EvaluateRuleP(const SequenceRule& aRule,
                                 std::string_view aInput) const
{
   switch (aRule.mType)
   {
   case SequenceRule::CharacterClass:
   {
      const auto& cc = mLexicon.mCharClasses[aRule.mData];
      return (aInput.size() > 0 && cc.Test(aInput.front())) ? std::make_optional(1) : std::nullopt;
   }
   case SequenceRule::RawCharacters:
   {
      const auto chars = mLexicon.mCharData.substr(aRule.mData, aRule.mSize);
      return aInput.starts_with(chars) ? std::make_optional(aRule.mSize)
         : std::nullopt;
   }
   case SequenceRule::Sequence:
      return EvaluateSequence(mLexicon.mSequences[aRule.mData], aInput);
   default:
      assert(false);
      return std::nullopt;
   }
}

bool rjcpt::lex::Lexer::ExtractToken(Token& aOut, std::uint32_t aKind,
                                     std::uint32_t aSize)
{
   const std::uint32_t end = mPosition + aSize;
   if (mPosition + aSize <= mInput.size())
   {
      aOut.mTypeName = mLexicon.mTokenTypes[aKind].mName;
      aOut.mTypeIndex = aKind;
      aOut.mPosition = mPosition;
      aOut.mSize = aSize;
      aOut.mRow = mRow;
      aOut.mColumn = mColumn;
      for (; mPosition < end; mPosition++)
      {
         ++mColumn;
         if (mInput[mPosition] == '\n')
         {
            ++mRow;
            mColumn = 1;
         }
      }

      return true;
   }
   return false;
}
