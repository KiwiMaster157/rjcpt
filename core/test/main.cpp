#include <gtest/gtest.h>

int main(int aArgc, char** aArgv)
{
   ::testing::InitGoogleTest(&aArgc, aArgv);
   return RUN_ALL_TESTS();
}
