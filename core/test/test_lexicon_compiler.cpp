#include <gtest/gtest.h>

#include "LexiconCompiler.hpp"

struct char_range
{
  int mBegin = 0;
  int mEnd = 255;
};

template<typename... Ts>
rjcpt::lex::CharacterClass MakeCC(rjcpt::lex::name_t aName, Ts... aChars)
{
  rjcpt::lex::CharacterClass cc;
  cc.mName = aName;
  
  auto apply = [&cc]<typename T>(T x) {
    if constexpr (std::integral<T>) {
      cc.Add(static_cast<char>(x));
    } else if constexpr (std::same_as<T, const char *>) {
      for (char c : std::string_view(x)) {
        cc.Add(c);
      }
    } else if constexpr (std::same_as<T, char_range>) {
      for (int i = x.mBegin; i <= x.mEnd; i++) {
        cc.Add(static_cast<char>(i));
      }
    }
  };
  (apply(aChars), ...);
  return cc;
}

TEST(LexiconCompiler, Empty) {
  auto result = rjcpt::lex::CompileLexicon("");
  ASSERT_TRUE(result.has_value());

  ASSERT_EQ(result->mCharClasses.size(), 1);
  EXPECT_EQ(result->mCharClasses[0], MakeCC("$$", char_range(0, 255)));
  
  EXPECT_TRUE(result->mCharData.empty());
  EXPECT_TRUE(result->mRules.empty());
  EXPECT_TRUE(result->mSequences.empty());
  EXPECT_TRUE(result->mTokenTypes.empty());
}

TEST(LexiconCompiler, Example)
{
  auto result = rjcpt::lex::CompileLexicon(R"(
$top qwertyuiop
   $mid asdf %67 hjkl#Has Comment
$bot z x c v b n m          

# Line comment
$letters $top $mid $bot
$a aaaaAAAAaaaaAAAA
$hex %0a %0B %Cd %eF
$no-a $$ -$a
$no-vowels $letters -aeiouy y
$no-vowels-2 $no-vowels -AEIOU
$empty

.baba baba
.baby b $a b %7b
.abba a b+ a
.babies .baby+
.no-babies .baby~
.word $letters+

%00 foo .baby $a* .no-babies
bar $top? $mid+ $bot*
baz
%03 zap hello

)");
  ASSERT_TRUE(result.has_value());

  ASSERT_EQ(result->mCharClasses.size(), 11);
  EXPECT_EQ(result->mCharClasses[0], MakeCC("$$", char_range(0, 255)));
  EXPECT_EQ(result->mCharClasses[1], MakeCC("$top", "qwertyuiop"));
  EXPECT_EQ(result->mCharClasses[2], MakeCC("$mid", "asdfghjkl"));
  EXPECT_EQ(result->mCharClasses[3], MakeCC("$bot", "zxcvbnm"));
  EXPECT_EQ(result->mCharClasses[4], MakeCC("$letters", char_range('a', 'z')));
  EXPECT_EQ(result->mCharClasses[5], MakeCC("$a", 'a', 'A'));
  EXPECT_EQ(result->mCharClasses[6], MakeCC("$hex", 0x0a, 0x0b, 0xcd, 0xef));
  EXPECT_EQ(result->mCharClasses[7], MakeCC("$no-a", char_range(0, 'A' - 1), char_range('B', 'a' - 1), char_range('b', 255)));
  EXPECT_EQ(result->mCharClasses[8],
            MakeCC("$no-vowels", "bcdfghjklmnpqrstvwxyz"));
  EXPECT_EQ(result->mCharClasses[9],
            MakeCC("$no-vowels-2", "bcdfghjklmnpqrstvwxyz"));
  EXPECT_EQ(result->mCharClasses[10], MakeCC("$empty"));

  ASSERT_EQ(result->mSequences.size(), 6);
  {
    const auto &seq = result->mSequences[0];
    EXPECT_EQ(seq.mName, ".baba");
    EXPECT_EQ(seq.mSize, 1);
    ASSERT_LT(seq.mBegin + seq.mSize, result->mRules.size());
    const auto &rule = result->mRules[seq.mBegin];
    EXPECT_EQ(rule.mType, rjcpt::lex::SequenceRule::RawCharacters);
    EXPECT_EQ(rule.mQuantity, rjcpt::lex::SequenceRule::Quantity::Once);
    EXPECT_EQ(rule.mSize, 4);
    ASSERT_LT(rule.mData + rule.mSize, result->mCharData.size());
    EXPECT_EQ(result->mCharData.substr(rule.mData, rule.mSize), "baba");
    const auto &rule2 = result->mRules[seq.mBegin + 1];
    EXPECT_EQ(rule2.mType, rjcpt::lex::SequenceRule::EndOfSequence);
  }
  {
    const auto &seq = result->mSequences[1];
    EXPECT_EQ(seq.mName, ".baby");
    EXPECT_EQ(seq.mSize, 4);
    ASSERT_LT(seq.mBegin + seq.mSize, result->mRules.size());
    const auto &rule1 = result->mRules[seq.mBegin];
    EXPECT_EQ(rule1.mType, rjcpt::lex::SequenceRule::RawCharacters);
    EXPECT_EQ(rule1.mQuantity, rjcpt::lex::SequenceRule::Quantity::Once);
    EXPECT_EQ(rule1.mSize, 1);
    ASSERT_LT(rule1.mData + rule1.mSize, result->mCharData.size());
    EXPECT_EQ(result->mCharData.substr(rule1.mData, rule1.mSize), "b");
    const auto &rule2 = result->mRules[seq.mBegin + 1];
    EXPECT_EQ(rule2.mType, rjcpt::lex::SequenceRule::CharacterClass);
    EXPECT_EQ(rule2.mQuantity, rjcpt::lex::SequenceRule::Quantity::Once);
    ASSERT_LT(rule2.mData, result->mCharClasses.size());
    EXPECT_EQ(result->mCharClasses[rule2.mData], MakeCC("$a", "aA"));
  }
  EXPECT_EQ(result->mTokenTypes.size(), 4);
}

TEST(LexiconCompiler, EmptyCcName)
{
  auto result = rjcpt::lex::CompileLexicon("$ abc");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, EmptyCcCcRef)
{
  auto result = rjcpt::lex::CompileLexicon("$abc $");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, InvalidCcRef)
{
  auto result = rjcpt::lex::CompileLexicon("$abc $def");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, SelfCcRef) {
  auto result = rjcpt::lex::CompileLexicon("$abc $abc");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, EmptyNegate) {
  auto result = rjcpt::lex::CompileLexicon("$abc -");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, EmptySeqName) {
  auto result = rjcpt::lex::CompileLexicon(". abc");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, EmptySeqCcRef)
{
  auto result = rjcpt::lex::CompileLexicon(".abc $");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, InvalidSeqCcRef)
{
  auto result = rjcpt::lex::CompileLexicon(".abc $abc\n$abc abc");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, EmptySeqSeqRef)
{
   auto result = rjcpt::lex::CompileLexicon(".abc .");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, InvalidSeqRef) {
  auto result = rjcpt::lex::CompileLexicon(".abc .abc");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, QuantityMarkers) {
  auto result = rjcpt::lex::CompileLexicon("abc qwe rty+ uiop* asdf? xyz~");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->mTokenTypes.size(), 1);
  const auto &tt = result->mTokenTypes[0];
  EXPECT_EQ(tt.mName, "abc");
  EXPECT_EQ(tt.mSize, 5);
  ASSERT_LT(tt.mBegin + tt.mSize, result->mRules.size());
  using Quantity = rjcpt::lex::SequenceRule::Quantity;
  EXPECT_EQ(result->mRules[tt.mBegin + 0].mQuantity, Quantity::Once);
  EXPECT_EQ(result->mRules[tt.mBegin + 1].mQuantity, Quantity::OnePlus);
  EXPECT_EQ(result->mRules[tt.mBegin + 2].mQuantity, Quantity::ZeroPlus);
  EXPECT_EQ(result->mRules[tt.mBegin + 3].mQuantity, Quantity::Optional);
  EXPECT_EQ(result->mRules[tt.mBegin + 4].mQuantity, Quantity::Never);
}

TEST(LexiconCompiler, EmptyQuantity) {
  auto result = rjcpt::lex::CompileLexicon("abc +");
  EXPECT_FALSE(result.has_value());
}

TEST(LexiconCompiler, HexQuantityEquivalent) {
  auto result = rjcpt::lex::CompileLexicon("abc %2b"); // %2b is the "+" character
  EXPECT_TRUE(result.has_value());
  // TODO: Test contents
}

TEST(LexiconCompiler, HexQuantity) {
  auto result = rjcpt::lex::CompileLexicon("abc %20*");
  EXPECT_TRUE(result.has_value());
  // TODO: Test contents
}

TEST(LexiconCompiler, DuplicatedQuantity) {
  auto result = rjcpt::lex::CompileLexicon("abc ++");
  EXPECT_TRUE(result.has_value());
  // TODO: Test contents
}

TEST(LexiconCompiler, BadTokenEnum) {
  auto result = rjcpt::lex::CompileLexicon("%01 abc xyz");
  EXPECT_FALSE(result.has_value());
  result = rjcpt::lex::CompileLexicon("%00 abc xyz");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->mTokenTypes.size(), 1);
  const auto &tt = result->mTokenTypes[0];
  EXPECT_EQ(tt.mName, "abc");
  EXPECT_EQ(tt.mSize, 1);
  ASSERT_LT(tt.mBegin + tt.mSize, result->mRules.size());
  const auto &rule = result->mRules[tt.mBegin];
  EXPECT_EQ(rule.mType, rjcpt::lex::SequenceRule::RawCharacters);
}

TEST(LexiconCompiler, HexChars)
{
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter(""));
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter("%"));
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter("%a"));
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter("%0"));
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter("%000"));
  EXPECT_EQ(std::nullopt, rjcpt::lex::ReadHexCharacter("%0g"));

  EXPECT_EQ(0, rjcpt::lex::ReadHexCharacter("%00"));
  EXPECT_EQ(1, rjcpt::lex::ReadHexCharacter("%01"));
  EXPECT_EQ(0x4f, rjcpt::lex::ReadHexCharacter("%4f"));
  EXPECT_EQ(0x4f, rjcpt::lex::ReadHexCharacter("%4F"));
}
