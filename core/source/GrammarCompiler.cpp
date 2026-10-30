#include "GrammarCompiler.hpp"

#include "IniReader.hpp"
#include "ProtoLexer.hpp"

#include <algorithm>
#include <format>

rjcpt::lex::GrammarInput::GrammarInput(std::string aRawInput)
{
   mRawData.mInput = std::move(aRawInput);
}

void rjcpt::lex::GrammarInput::PushRule(Token aToken, name_t aName)
{
   GrammarRule rule;
   rule.mName = aName;
   rule.mBegin = static_cast<std::uint32_t>(mRawData.mNodes.size());
   rule.mEnd = rule.mBegin;
   mRawData.mRules.push_back(rule);
   mRawData.mRuleTokens.push_back(aToken);
}
void rjcpt::lex::GrammarInput::PushRuleAlternate(Token aToken)
{
   assert(!mRawData.mRules.empty());
   PushRule(aToken, mRawData.mRules.back().mName);
}
void rjcpt::lex::GrammarInput::PushNode(Token aToken, name_t aName, RuleRef aRule)
{
   assert(mRawData.mNodes.size() == mRawData.mNodeTokens.size());
   assert(!mRawData.mRules.empty());
   mRawData.mNodes.push_back(GrammarNode{ .mName = aName, .mRule = aRule });
   ++mRawData.mRules.back().mEnd;
   mRawData.mNodeTokens.push_back(aToken);
}

void rjcpt::lex::GrammarInput::PushReadError(std::string_view aErrorMessage, Token aToken)
{
   std::string err = std::format("{0}\n   Near \"{1}\", row {2}, column {3}.",
                                 aErrorMessage, aToken.Get(mRawData.mInput), aToken.mRow, aToken.mColumn);
   mErrors.push_back(std::move(err));
}

///////////////////////////////////////////////////////////////////

namespace
{
   bool ReadStandaloneGrammarDeclaration(rjcpt::lex::GrammarInput& aInput, rjcpt::lex::Token aToken)
   {
      std::string_view text = aToken.Get(aInput.mRawData.mInput);
      if (!text.ends_with('=') || text == "=")
      {
         aInput.PushReadError("Expected declaration.", aToken);
      }
      else if (text.size() >= rjcpt::lex::cMAX_NAME)
      {
         aInput.PushReadError("Declared name too long.", aToken);
      }
      else if (text.starts_with('$') || text.starts_with('&'))
      {
         aInput.PushReadError("Rule cannot start with '$' or '&'.", aToken);
      }
      else
      {
         aInput.PushRule(aToken, text.substr(0, text.size() - 1));
         return true;
      }
      return false;
   }

   rjcpt::lex::RuleRef ReadStandaloneGrammarTerminal(rjcpt::lex::GrammarInput& aInput, rjcpt::lex::Token aToken)
   {
      std::string_view text = aToken.Get(aInput.mRawData.mInput);

      assert(text.starts_with('$') || text.starts_with('&'));
      std::uint32_t x;
      auto result = std::from_chars(text.data() + 1, text.data() + text.size(), x);
      if (result.ec != std::errc() || result.ptr != text.data() + text.size())
      {
         aInput.PushReadError("Cannot read value.", aToken);
      }
      else if (x == 0)
      {
         aInput.PushReadError("Cannot have a terminal of value 0.", aToken);
      }
      else if (x > 0x4000'0000)
      {
         aInput.PushReadError("Value too large.", aToken);
      }
      else if (text.starts_with('$'))
      {
         return rjcpt::lex::RuleRef::MakeInputTerminal(x);
      }
      else if (text.starts_with('&'))
      {
         return rjcpt::lex::RuleRef::MakeOutputTerminal(x);
      }
      return rjcpt::lex::RuleRef();
   }
}

auto rjcpt::lex::grammar_compiler::ReadStandaloneInput(std::string aGrammar) -> GrammarInput
{
   GrammarInput grammar(std::move(aGrammar));
   const std::string& raw = grammar.mRawData.mInput;

   auto lexer = lex::ProtoLexer(grammar.mRawData.mInput);

   bool expectDeclaration = true;
   lex::Token tok;
   while (lexer.ReadToken(tok))
   {
      std::string_view text = tok.Get(raw);
      if (expectDeclaration)
      {
         expectDeclaration = !::ReadStandaloneGrammarDeclaration(grammar, tok);
      }
      else if (text.contains('='))
      {
         grammar.PushReadError("Unexpected declaration.", tok);
      }
      else if (text == ";")
      {
         expectDeclaration = true;
      }
      else if (text == "|")
      {
         grammar.PushRuleAlternate(tok);
      }
      else if (text.starts_with('$') || text.starts_with('&'))
      {
         grammar.PushNode(tok, text, ::ReadStandaloneGrammarTerminal(grammar, tok));
      }
      else if (text.size() == 3 && text[0] == '\'' && text[2] == '\'')
      {
         grammar.PushNode(tok, text, RuleRef::MakeInputTerminal(static_cast<unsigned char>(text[1])));
      }
      else if (text.size() >= lex::cMAX_NAME)
      {
         grammar.PushReadError("Name too long.", tok);
      }
      else
      {
         grammar.PushNode(tok, text, RuleRef());
      }
   }
   if (!expectDeclaration)
   {
      // May have to handle this one differently.
      grammar.PushReadError("Incomplete rule definition.", Token());
   }
}

namespace
{
   void ReadTwoPhaseGrammarCharacters(rjcpt::core::IniReader& aReader, rjcpt::lex::GrammarInput& aLexGrammar)
   {
      rjcpt::core::IniReader::value_type line;
      while (aReader.ReadValue(line) && line.section == "characters")
      {
         std::string_view name = line.key.Get(aReader.GetInput());
         if (name.size() >= rjcpt::lex::cMAX_NAME)
         {
            aLexGrammar.PushReadError("Character class name too long.", line.key);
            continue;
         }
         aLexGrammar.PushRule(line.key, rjcpt::lex::name_t(name));
         std::string_view chars = line.value.Get(aReader.GetInput());
         if (chars.size() > 0)
         {
            auto ref0 = rjcpt::lex::RuleRef::MakeInputTerminal(static_cast<unsigned char>(chars[0]));
            aLexGrammar.PushNode(line.value, {}, ref0);
            for (char c : chars.substr(1))
            {
               auto ref = rjcpt::lex::RuleRef::MakeInputTerminal(static_cast<unsigned char>(c));
               aLexGrammar.PushRuleAlternate(line.value);
               aLexGrammar.PushNode(line.value, {}, ref);
            }
         }
      }
      while (aReader.ReadValue(line) && line.section == "ascii")
      {
         std::string_view name = line.key.Get(aReader.GetInput());
         if (name.size() >= rjcpt::lex::cMAX_NAME)
         {
            aLexGrammar.PushReadError("Character class name too long.", line.key);
            continue;
         }
         aLexGrammar.PushRule(line.key, rjcpt::lex::name_t(name));
         std::string_view chars = line.value.Get(aReader.GetInput());
         std::uint8_t value = 0;
         const auto r = std::from_chars(chars.data(), chars.data() + chars.size(), value);
         if (r.ec != std::errc() || r.ptr != chars.data() + chars.size())
         {
            aLexGrammar.PushReadError("Unable to read ascii code.", line.value);
         }
         else
         {
            auto ref = rjcpt::lex::RuleRef::MakeInputTerminal(value);
            aLexGrammar.PushNode(line.value, {}, ref);
         }
      }
   }

   void ReadTwoPhaseGrammarTokens(rjcpt::core::IniReader&   aReader,
                                  rjcpt::lex::GrammarInput& aLexGrammar,
                                  rjcpt::lex::GrammarInput& aParserGrammar)
   {
      rjcpt::core::IniReader::value_type line;
      std::uint32_t counter = 1;
      while (aReader.ReadValue(line) && line.section == "tokens")
      {
         std::string_view name = line.key.Get(aReader.GetInput());
         if (name.size() + 1 >= rjcpt::lex::cMAX_NAME)
         {
            aLexGrammar.PushReadError("Token name too long.", line.key);
            aParserGrammar.PushReadError("Token name too long.", line.key);
            continue;
         }
         std::string ruleName = "}";
         ruleName += name;
         aLexGrammar.PushRule(line.key, rjcpt::lex::name_t(ruleName));
         auto ref = rjcpt::lex::RuleRef::MakeOutputTerminal(counter);
         aLexGrammar.PushNode(line.key, {}, ref);

         ruleName[0] = '.';
         aParserGrammar.PushRule(line.key, rjcpt::lex::name_t(ruleName));
         ref = rjcpt::lex::RuleRef::MakeInputTerminal(counter);
         aParserGrammar.PushNode(line.key, {}, ref);

         counter++;
      }

      aLexGrammar.PushRule({}, "{");
      aLexGrammar.PushNode({}, {}, rjcpt::lex::RuleRef::MakeOutputTerminal(1000));
   }

   void ReadTwoPhaseGrammarLexicon(rjcpt::core::IniReader& aReader, rjcpt::lex::GrammarInput& aLexGrammar)
   {
      rjcpt::core::IniReader::value_type line;
      while (aReader.ReadValue(line) && line.section == "lexicon")
      {
         std::string_view name = line.key.Get(aReader.GetInput());
         if (name.size() >= rjcpt::lex::cMAX_NAME)
         {
            aLexGrammar.PushReadError("Token name too long.", line.key);
            continue;
         }
         aLexGrammar.PushRule(line.key, rjcpt::lex::name_t(name));

         // TODO: Read line token-by-token.
         // All tokens should be "|" or a rule reference.
      }
   }
}

auto rjcpt::lex::grammar_compiler::ReadTwoPhaseInput(std::string aGrammar) -> std::pair<GrammarInput, GrammarInput>
{
   GrammarInput lexerGrammar(aGrammar);
   GrammarInput parserGrammar(aGrammar);
   rjcpt::core::IniReader reader(aGrammar);

   ::ReadTwoPhaseGrammarCharacters(reader, lexerGrammar);
   ::ReadTwoPhaseGrammarTokens(reader, lexerGrammar, parserGrammar);


   return { lexerGrammar, parserGrammar };
}

///////////////////////////////////////////////////////////////////

rjcpt::lex::grammar_compiler::JumpTable::JumpTable(std::uint32_t aNumRules, std::vector<InType> aInputs)
   : mInputTypes(aInputs)
   , mJumpTableRows(aNumRules)
{
   mJumpIndices.resize(aNumRules * aInputs.size(), rjcpt::lex::npos);
}

std::uint32_t rjcpt::lex::grammar_compiler::JumpTable::GetNumRules() const { return mJumpTableRows; }
std::uint32_t rjcpt::lex::grammar_compiler::JumpTable::GetNumInputs() const { return mInputTypes.size(); }

//! Returns the column index associated with the given input type.
std::uint32_t rjcpt::lex::grammar_compiler::JumpTable::GetInputTypeIndex(InType aType) const
{
   auto it = std::ranges::lower_bound(mInputTypes, aType);
   if (it != mInputTypes.end() && *it == aType)
   {
      return static_cast<std::uint32_t>(it - mInputTypes.begin());
   }
   return rjcpt::lex::npos;
}

//! Returns the index (of mRules) that the parser should jump to for the given top rule type and current input type.
//! Returns npos on failure.
std::uint32_t rjcpt::lex::grammar_compiler::JumpTable::GetJumpIndex(RuleRef aRuleType, InType aInputType) const
{
   const std::uint32_t inputIndex = GetInputTypeIndex(aInputType);
   return GetJumpIndexP(aRuleType, inputIndex);
}

std::uint32_t rjcpt::lex::grammar_compiler::JumpTable::GetJumpIndexP(RuleRef aRuleType, std::uint32_t aInputIndex) const
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();

   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex < GetNumRules());
   assert(aInputIndex < GetNumInputs());
   const std::uint32_t tableIndex = ruleIndex * GetNumInputs() + aInputIndex;
   assert(tableIndex < mJumpIndices.size());
   return mJumpIndices[tableIndex];
}

bool rjcpt::lex::grammar_compiler::JumpTable::SetJump(RuleRef aRuleType, InType aInputType, std::uint32_t aJump)
{
   const std::uint32_t index = GetInputTypeIndex(aInputType);
   return SetJumpAt(aRuleType, index, aJump);
}

bool rjcpt::lex::grammar_compiler::JumpTable::SetJumpAt(RuleRef aRuleType, std::uint32_t aInputIndex, std::uint32_t aJump)
{
   const std::uint32_t ruleIndex = aRuleType.GetValue();

   assert(aRuleType.IsNonTerminal());
   assert(ruleIndex < GetNumRules());
   assert(aInputIndex < GetNumInputs());
   const std::uint32_t tableIndex = ruleIndex * GetNumInputs() + aInputIndex;

   if (mJumpIndices[tableIndex] == rjcpt::lex::npos)
   {
      mJumpIndices[tableIndex] = aJump;
      return true;
   }
   return false;
}
