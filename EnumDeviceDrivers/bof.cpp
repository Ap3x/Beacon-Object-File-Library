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
using m_EnumDrivers = bool(*)();

m_EnumDrivers EnumDrivers = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
    void SetUp() override {
        EnumDrivers = []() -> bool {
            BeaconPrintf(CALLBACK_OUTPUT, "1: ntoskrnl.exe\n");
            return true; // Mocked function always returns false
            };
    }
    void TearDown() override {
        EnumDrivers = nullptr;
    }
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"
#define ARRAY_SIZE 256

extern "C" {
    DFR(PSAPI, EnumDeviceDrivers);
    DFR(PSAPI, GetDeviceDriverBaseNameA);
}
#undef EnumDeviceDrivers
#undef GetDeviceDriverBaseNameA
#define EnumDeviceDrivers PSAPI$EnumDeviceDrivers
#define GetDeviceDriverBaseNameA PSAPI$GetDeviceDriverBaseNameA

bool EnumDrivers() {
    LPVOID drivers[ARRAY_SIZE];
    DWORD cbNeeded;
    int cDrivers, i;

    if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
    {
        char szDriver[ARRAY_SIZE];
        cDrivers = cbNeeded / sizeof(drivers[0]);

        BeaconPrintf(CALLBACK_OUTPUT, "There are %d drivers\n", cDrivers);

        for (i = 0; i < cDrivers; i++)
        {
            if (GetDeviceDriverBaseNameA(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
            {
                BeaconPrintf(CALLBACK_OUTPUT, "%d: %s\n", i + 1, szDriver);
            }
        }
        return true;
    }
    else
    {
        BeaconPrintf(CALLBACK_ERROR, "EnumDeviceDrivers failed; array size needed is %d\n", cbNeeded / sizeof(LPVOID));
        return false;
    }
}
#endif

extern "C" {
    void go(char* args, int len) {
        if (!EnumDrivers()) {
			BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate drivers.");
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
    TEST_F(MockedFunctions, EnumDriversFails) {
        EnumDrivers = []() -> bool {return false;};

        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);

        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
    }

    TEST_F(MockedFunctions, EnumDriversSuccess) {
        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);
        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

        bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "1: ntoskrnl.exe\n" };
        ASSERT_EQ(expected, got[0]);
    }
}

#endif