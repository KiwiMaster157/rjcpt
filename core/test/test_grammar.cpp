#include <gtest/gtest.h>

#include "GrammarLL.hpp"

TEST(Grammar, SkipSpace)
{
   auto sp = [](const char* aStart)
      {
         return rjcpt::grammar_util::SkipSpace(aStart) - aStart;
      };
   EXPECT_EQ(sp(""), 0);
   EXPECT_EQ(sp(" "), 1);
   EXPECT_EQ(sp("\n"), 1);
   EXPECT_EQ(sp("1"), 0);
   EXPECT_EQ(sp("1 "), 0);
   EXPECT_EQ(sp(" 1"), 1);
   EXPECT_EQ(sp("     "), 5);
   EXPECT_EQ(sp("#"), 1);
   EXPECT_EQ(sp("#hello"), 6);
   EXPECT_EQ(sp("#hello\n"), 7);
   EXPECT_EQ(sp("  #"), 3);
   EXPECT_EQ(sp("1#"), 0);
   EXPECT_EQ(sp("#\n1"), 2);
   EXPECT_EQ(sp("#\n#\n"), 4);
   EXPECT_EQ(sp("#\n  1"), 4);
}

TEST(Grammar, ExtractWord)
{

}
