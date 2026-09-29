#include <gtest/gtest.h>

#include "Grammar.hpp"

TEST(Grammar, Compile)
{
   rjcpt::lex::Grammar g;
   EXPECT_TRUE(g.Compile("").empty());
   EXPECT_TRUE(g.Compile("# comments").empty());
   EXPECT_TRUE(g.Compile("$abc=6 ;").empty());
   EXPECT_TRUE(g.Compile("$abc=6 ; $def=8 ;").empty());
   EXPECT_TRUE(g.Compile("&abc=5 ;").empty());

   EXPECT_FALSE(g.Compile("$abc=6").empty()) << "Missing semicolon.";
   EXPECT_FALSE(g.Compile("$abc").empty()) << "Missing declaration.";
   EXPECT_FALSE(g.Compile(";").empty());
   EXPECT_FALSE(g.Compile("$abc=6 ; $abc=6 ;").empty()) << "Duplicate input declaration.";
   EXPECT_FALSE(g.Compile("&abc=5").empty()) << "Missing semicolon.";
   EXPECT_FALSE(g.Compile("$abc=4 ; $def=4 ;").empty()) << "Duplicate input values.";
   EXPECT_FALSE(g.Compile("$abc= ;").empty()) << "Missing input value.";

   EXPECT_FALSE(g.Compile("$abc=1 $def=2").empty()) << "Missing semicolons.";
   EXPECT_FALSE(g.Compile("qwerty= ; $uiop=1 $uiop ;").empty()) << "Unexpected input reference.";
}
