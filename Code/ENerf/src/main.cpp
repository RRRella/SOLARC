#include "ENerfApp.h"
#include "Utility/CompileTimeUtil.h"
 
int main(int argc, char** argv)
{
    InitilizeCompileTimeCode();

    try
    {
        ENerfApp::Initialize(".\\Data\\config.json");
        auto& app = ENerfApp::Get();

        app.Run();
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }
}
