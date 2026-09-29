#include <gtest/gtest.h>

#include "ProtoLexer.hpp"

TEST(ProtoLexer, Empty)
{
   const auto result = rjcpt::lex::ProtoLexer::LexInput("");
   EXPECT_TRUE(result.empty());
}

TEST(ProtoLexer, OnlySpace)
{
   const auto result = rjcpt::lex::ProtoLexer::LexInput(" \n ");
   EXPECT_TRUE(result.empty());
}

TEST(ProtoLexer, Words)
{
   const std::string_view text = "abc def\n\nghi\tjklmn";
   const auto result = rjcpt::lex::ProtoLexer::LexInput(text);
   ASSERT_EQ(result.size(), 4);
   
   EXPECT_EQ(result[0].Get(text), "abc");
   EXPECT_EQ(result[0].mRow, 1);
   EXPECT_EQ(result[0].mColumn, 1);
   
   EXPECT_EQ(result[1].Get(text), "def");
   EXPECT_EQ(result[1].mRow, 1);
   EXPECT_EQ(result[1].mColumn, 5);

   EXPECT_EQ(result[2].Get(text), "ghi");
   EXPECT_EQ(result[2].mRow, 3);
   EXPECT_EQ(result[2].mColumn, 1);

   EXPECT_EQ(result[3].Get(text), "jklmn");
   EXPECT_EQ(result[3].mRow, 3);
   EXPECT_EQ(result[3].mColumn, 5);
}

TEST(ProtoLexer, CommentOnly)
{
   const auto result = rjcpt::lex::ProtoLexer::LexInput("# this is a comment\n");
   EXPECT_TRUE(result.empty());
}

TEST(ProtoLexer, IncompleteCommentOnly)
{
   const auto result = rjcpt::lex::ProtoLexer::LexInput("# incomplet");
   EXPECT_TRUE(result.empty());
}

TEST(ProtoLexer, WhitespaceBeforeComment)
{
   const auto result = rjcpt::lex::ProtoLexer::LexInput("    #comment");
   EXPECT_TRUE(result.empty());
}

TEST(ProtoLexer, BackToBackComments)
{
   const std::string_view text = "#abc\n#def\n   #ghi\n  jkl  ";
   const auto result = rjcpt::lex::ProtoLexer::LexInput(text);
   ASSERT_EQ(result.size(), 1);

   EXPECT_EQ(result[0].Get(text), "jkl");
   EXPECT_EQ(result[0].mRow, 4);
   EXPECT_EQ(result[0].mColumn, 3);
}

TEST(ProtoLexer, WordStopsAtComment)
{
   const std::string_view text = "abc#d e f\nghi";
   const auto result = rjcpt::lex::ProtoLexer::LexInput(text);
   ASSERT_EQ(result.size(), 2);

   EXPECT_EQ(result[0].Get(text), "abc");
   EXPECT_EQ(result[0].mRow, 1);
   EXPECT_EQ(result[0].mColumn, 1);

   EXPECT_EQ(result[1].Get(text), "ghi");
   EXPECT_EQ(result[1].mRow, 2);
   EXPECT_EQ(result[1].mColumn, 1);
}
