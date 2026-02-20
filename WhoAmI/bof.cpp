#include <Windows.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#pragma comment(lib, "Advapi32.lib")
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_WhoAmI = bool(*)();

m_WhoAmI WhoAmI = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		WhoAmI = []() -> bool {
			BeaconPrintf(CALLBACK_OUTPUT, "DOMAIN\\User");
			return true;
			};
	}
	void TearDown() override {
		WhoAmI = nullptr;
	}
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
	DFR(ADVAPI32, GetUserNameA);
	DFR(KERNEL32, GetLastError);
}
#define GetUserNameA ADVAPI32$GetUserNameA
#define GetLastError KERNEL32$GetLastError

bool WhoAmI() {
	char username[256];
	DWORD usernameLen = sizeof(username);

	if (!GetUserNameA(username, &usernameLen)) {
		BeaconPrintf(CALLBACK_ERROR, "GetUserNameA failed: %d", GetLastError());
		return false;
	}

	BeaconPrintf(CALLBACK_OUTPUT, "%s", username);
	return true;
}
#endif

extern "C" {
	void go(char* args, int len) {
		if (!WhoAmI()) {
			BeaconPrintf(CALLBACK_ERROR, "Failed to get username.");
			return;
		}
	}
}

// Define a main function for the bebug build
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
	TEST_F(MockedFunctions, WhoAmIFails) {
		WhoAmI = []() -> bool {return false; };

		std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);

		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
	}

	TEST_F(MockedFunctions, WhoAmISuccess) {
		std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);
		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

		bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "DOMAIN\\User" };
		ASSERT_EQ(expected, got[0]);
	}
}

#endif
