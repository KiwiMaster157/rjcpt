#include <gtest/gtest.h>

#include <cstdlib>

int main(int aArgc, char** aArgv)
{
#if _WIN32
   _set_error_mode(_OUT_TO_MSGBOX);
#endif

   ::testing::InitGoogleTest(&aArgc, aArgv);
   return RUN_ALL_TESTS();
}
