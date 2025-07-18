#include <gtest/gtest.h>

int main(int aArgc, char** aArgv)
{
   ::testing::InitGoogleTest(&aArgc, aArgv);
   return GTEST_RUN_ALL(aArgc, aArgv);
}
