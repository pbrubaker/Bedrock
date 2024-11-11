// SPDX-License-Identifier: MPL-2.0
#include <Bedrock/Test.h>
#include <Bedrock/Debug.h>
#include <Bedrock/Vector.h>
#include <Bedrock/StringView.h>
#include <Bedrock/Trace.h>
#include <Bedrock/Ticks.h>

struct Test
{
	const char*  mName;
	TestFunction mFunction = nullptr;
};

static Vector<Test>& sGetAllTests()
{
	// Return a static variable to avoid issues with globals initialization order.
	static Vector<Test> sAllTests;
	return sAllTests;
}


void gRegisterTest(const char* inName, TestFunction inFunction)
{
	gAssert(inName != nullptr && inName[0] != 0);

	sGetAllTests().PushBack({ inName, inFunction });
}


static thread_local StringView sCurrentTestName;
static thread_local bool       sCurrentTestSuccess;

bool gIsRunningTest()
{
	return !sCurrentTestName.Empty();
}


TestResult gRunTests()
{
	gTrace("Running all tests.");
	bool all_success = true;

	for (const Test& test : sGetAllTests())
	{
		sCurrentTestName    = test.mName;
		sCurrentTestSuccess = true;
		gTrace(R"(Test "%s" starting.)", test.mName);
		Timer timer;

		test.mFunction();

		gTrace(R"(Test "%s" finished: %s (%.2f ms))", 
			test.mName, 
			sCurrentTestSuccess ? "Success" : "Failure",
			gTicksToMilliseconds(timer.GetTicks()));

		all_success = all_success && sCurrentTestSuccess;
		sCurrentTestName = "";
	}

	return all_success ? TestResult::Success : TestResult::Failure;
}


void gFailTest(const char* inMacro, const char* inCode, const char* inFile, int inLine)
{
	gTrace(R"(%s(%s) failed (%s:%d))", inMacro, inCode, inFile, inLine);

	sCurrentTestSuccess = false;

	// If a debugger is attached, break here.
	if (gIsDebuggerAttached())
		breakpoint;
}

