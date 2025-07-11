#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
#include "Utility/FileSystemUtil.h"
 
int main(int argc, char** argv)
{
    InitilizeCompileTimeCode();

    try
    {
        std::string configPath = GetExeDir() + "\\Data\\config.json";

        SolarcApp::Initialize(configPath);
        auto& app = SolarcApp::Get();

        app.Run();
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
}
