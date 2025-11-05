#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
#include "Utility/FileSystemUtil.h"

int main(int argc, char** argv) 
{
    try
    {
        std::string configPath = GetExeDir() + "\\Data\\config.json";
        SolarcApp::Initialize(configPath);
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }

	// Since Google Mock depends on Google Test, InitGoogleMock() is
	// also responsible for initializing Google Test.  Therefore there's
	// no need for calling testing::InitGoogleTest() separately.
	testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}