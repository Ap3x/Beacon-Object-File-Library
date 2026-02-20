#include <Windows.h>
#include "..\Core\base\helpers.h"

/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the debug build.
 *
 * If using Win32 APIs that are NOT in kernel32.lib (e.g. Advapi32, Psapi, etc.)
 * add a #pragma comment(lib, "Library.lib") here so the debug build links.
 */
#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
// #pragma comment(lib, "Advapi32.lib")
#include "..\Core\base\mock.h"
#endif

/**
 * Unit test build: define mocked function pointers.
 * Replace __FUNCTION_NAME__ with your function name and update the signature.
 */
#if defined(_GTEST)
#include <gtest\gtest.h>
using m___FUNCTION_NAME__ = bool(*)();

m___FUNCTION_NAME__ __FUNCTION_NAME__ = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		__FUNCTION_NAME__ = []() -> bool {
			BeaconPrintf(CALLBACK_OUTPUT, "mocked output");
			return true;
			};
	}
	void TearDown() override {
		__FUNCTION_NAME__ = nullptr;
	}
};
#else
/**
 * Release/Debug build: actual implementation.
 *
 * Use DFR(MODULE, Function) at global scope inside extern "C" to resolve
 * Win32 APIs at runtime, then #define Function MODULE$Function to alias it.
 * This ensures symbols are not C++ name-mangled, which is required for
 * both Cobalt Strike and third-party COFF loaders (e.g. TrustedSec COFFLoader).
 *
 * Common modules: KERNEL32, ADVAPI32, NTDLL, PSAPI, USER32, etc.
 */
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

// TODO: Add DFR declarations for any Win32 APIs you need:
// extern "C" {
//     DFR(MODULE, WinApiFunction);
// }
// #define WinApiFunction MODULE$WinApiFunction

bool __FUNCTION_NAME__() {
	// TODO: Implement BOF logic here
	// Use BeaconPrintf(CALLBACK_OUTPUT, "...", ...) for output

	BeaconPrintf(CALLBACK_OUTPUT, "Hello from __PROJECT_NAME__");
	return true;
}
#endif

/**
 * BOF entry point. This is called by Beacon via inline-execute.
 *
 * To parse arguments from the operator, use:
 *   datap parser;
 *   BeaconDataParse(&parser, args, len);
 *   int val = BeaconDataInt(&parser);
 *   char* str = BeaconDataExtract(&parser, &len);
 *   short s = BeaconDataShort(&parser);
 */
extern "C" {
	void go(char* args, int len) {
		if (!__FUNCTION_NAME__()) {
			BeaconPrintf(CALLBACK_ERROR, "Failed.");
			return;
		}
	}
}

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
	bof::runMocked<>(go);

	return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

namespace UnitTest {
	TEST_F(MockedFunctions, __FUNCTION_NAME__Fails) {
		__FUNCTION_NAME__ = []() -> bool {return false; };

		std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);

		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
	}

	TEST_F(MockedFunctions, __FUNCTION_NAME__Success) {
		std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);
		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

		bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "mocked output" };
		ASSERT_EQ(expected, got[0]);
	}
}

#endif
