#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
#include "Utility/FileSystemUtil.h"

#define SOLARC_TEST

int main(int argc, char** argv) 
{
	// Since Google Mock depends on Google Test, InitGoogleMock() is
	// also responsible for initializing Google Test.  Therefore there's
	// no need for calling testing::InitGoogleTest() separately.
	testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}