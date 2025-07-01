#include "FreqUsedSymbolsOfTesting.h"
#include "Utility/CompileTimeUtil.h"
#include "ENerfApp.h"

TEST(AENerfApp, HasASingleInstacne)
{
	InitilizeCompileTimeCode();

	ENerfApp::Initialize();

	ENerfApp& app1 = ENerfApp::Get();

	ENerfApp& app2 = ENerfApp::Get();

	ASSERT_EQ(&app1 , &app2);
}
