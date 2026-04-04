#include <gtest/gtest.h>

#include "Lexer.hpp"
#include "LexiconCompiler.hpp"

namespace
{
   rjcpt::lex::Lexicon GetLexicon()
   {
      auto result = rjcpt::lex::CompileLexicon(R"(
# Character classes

$space %20 %0a %0d
$alpha abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_
$lead-digit 123456789
$digit 0123456789
$alnum $alpha $digit

# Named characters

$newline   %0a %0d
$exclam    %21
$hash      %23
$dollar    %24
$percent   %25
$ampersand %26
$asterisk  %2a
$plus      %2b
$hyphen    %2d
$dot       %2e
$question  %3f
$caret     %5e
$tilde     %7e

# Character utilities

$e eE
$pm $plus $hyphen
$line-char $$ -$newline

.pos-int $lead-digit $digit*
.int .pos-int? 0?
.exponent $e $pm? .int

word $alpha $alnum*
int  .int
number .int? $dot $digit+ .exponent?

!whitespace $space+
!comment    $hash $line-char* $newline
)");
      if (!result)
      {
         throw std::runtime_error(result.error());
      }
      return result.value();
   }
}

TEST(Lexer, EmptyLex)
{
   const auto result = rjcpt::lex::Lexer::LexInput(rjcpt::lex::Lexicon{}, "");
   EXPECT_TRUE(result.IsValid());
   EXPECT_TRUE(result.mTokens.empty());
}

TEST(Lexer, EmptyLexicon)
{
   const std::string_view input = "Hello World";
   const auto result = rjcpt::lex::Lexer::LexInput(rjcpt::lex::Lexicon{}, input);
   EXPECT_FALSE(result.IsValid());
   EXPECT_TRUE(result.mTokens.empty());
}

TEST(Lexer, EmptyInput)
{
   const auto result = rjcpt::lex::Lexer::LexInput(GetLexicon(), "");
   EXPECT_TRUE(result.IsValid());
   EXPECT_TRUE(result.mTokens.empty());
}

TEST(Lexer, HelloWorld)
{
   const std::string_view input = "Hello World";
   const auto result = rjcpt::lex::Lexer::LexInput(GetLexicon(), input);
   EXPECT_TRUE(result.IsValid());
   ASSERT_EQ(result.mTokens.size(), 2);
   EXPECT_EQ(result.mTokens[0].Get(input), "Hello");
   EXPECT_EQ(result.mTokens[1].Get(input), "World");
}

TEST(Lexer, HelloWorld2)
{
   const std::string_view input = "Hello World";
   const auto result = rjcpt::lex::Lexer::LexInput(GetLexicon(), input, rjcpt::lex::Option::KeepIgnored);
   EXPECT_TRUE(result.IsValid());
   ASSERT_EQ(result.mTokens.size(), 3);
   EXPECT_EQ(result.mTokens[0].Get(input), "Hello");
   EXPECT_EQ(result.mTokens[1].Get(input), " ");
   EXPECT_EQ(result.mTokens[2].Get(input), "World");
}

