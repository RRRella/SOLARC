#include "FreqUsedSymbolsOfTesting.h"

#include "ENerfApp.h"

TEST(AENerfApp, HasASingleInstacne)
{
	ENerfApp& app1 = ENerfApp::Get();

	ENerfApp& app2 = ENerfApp::Get();

	ASSERT_EQ(&app1 , &app2);
}
