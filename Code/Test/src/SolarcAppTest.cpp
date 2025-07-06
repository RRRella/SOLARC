#include "FreqUsedSymbolsOfTesting.h"
#include "Utility/CompileTimeUtil.h"
#include "SolarcApp.h"

TEST(ASolarcApp, HasASingleInstacne)
{
	InitilizeCompileTimeCode();

	SolarcApp::Initialize();

	SolarcApp& app1 = SolarcApp::Get();

	SolarcApp& app2 = SolarcApp::Get();

	ASSERT_EQ(&app1 , &app2);
}
