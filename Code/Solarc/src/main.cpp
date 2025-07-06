#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
 
int main(int argc, char** argv)
{
    InitilizeCompileTimeCode();

    try
    {
        SolarcApp::Initialize(".\\Data\\config.json");
        auto& app = SolarcApp::Get();

        app.Run();
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }
}
