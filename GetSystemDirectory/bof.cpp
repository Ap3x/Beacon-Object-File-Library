#include <Windows.h>
#include <Psapi.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_GetSystemDirectoryCustom = char*(*)();

m_GetSystemDirectoryCustom GetSystemDirectoryCustom = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		GetSystemDirectoryCustom = []() -> char* {
			return "C:\\Windows\\system32"; // Mocked function always returns false
			};
	}
	void TearDown() override {
		GetSystemDirectoryCustom = nullptr;
	}
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
	DFR(KERNEL32, GetSystemDirectoryA);
}
#define GetSystemDirectoryA KERNEL32$GetSystemDirectoryA

LPSTR GetSystemDirectoryCustom() {
	char path[MAX_PATH + 1];

	UINT bytesCopied = GetSystemDirectoryA(path, MAX_PATH + 1);
	if (bytesCopied == 0 || bytesCopied > MAX_PATH) {
		BeaconPrintf(CALLBACK_ERROR, "Error retrieving System Directory: The bytes copied is zero or larger than 260");
		return NULL;
	}

	return path;

}
#endif

extern "C" {
	void go(char* args, int len) {
		LPSTR path = GetSystemDirectoryCustom();
		
		if (path == NULL || path == "") {
			BeaconPrintf(CALLBACK_ERROR, "Error attempting to retrieve System Directory: %d");
		}
		else {
			BeaconPrintf(CALLBACK_OUTPUT, "System Directory: %s", path);
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
	TEST_F(MockedFunctions, GetSystemDirFailsEmpty) {
		m_GetSystemDirectoryCustom GetSystemDirectoryCustom =
			[]() -> char* {return ""; };

		std::vector<bof::output::OutputEntry> got =
			bof::runMocked<>(go);
		bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "" };

		ASSERT_EQ(expected.output, got.back().output);
	}

	TEST_F(MockedFunctions, GetSystemDirFailsNull) {
		m_GetSystemDirectoryCustom GetSystemDirectoryCustom =
			[]() -> char* {return nullptr; };

		std::vector<bof::output::OutputEntry> got =
			bof::runMocked<>(go);
		bof::output::OutputEntry expected = { CALLBACK_OUTPUT, NULL };

		ASSERT_EQ(expected.output, got.back().output);
	}

	TEST_F(MockedFunctions, GetSystemDirSuccess) {
		std::vector<bof::output::OutputEntry> got =
			bof::runMocked<>(go);
		bof::output::OutputEntry expected =
		{ CALLBACK_OUTPUT, "System Directory: C:\\Windows\\system32" };

		ASSERT_EQ(expected.output, got.back().output);
	}
}

#endif