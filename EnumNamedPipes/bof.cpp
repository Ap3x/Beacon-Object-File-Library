#include <Windows.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_EnumNamedPipes = bool(*)();

m_EnumNamedPipes EnumNamedPipes = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
    void SetUp() override {
        EnumNamedPipes = []() -> bool {
            BeaconPrintf(CALLBACK_OUTPUT, "\\\\.\\ pipe\\testpipe\n");
            return true;
            };
    }
    void TearDown() override {
        EnumNamedPipes = nullptr;
    }
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
    DFR(KERNEL32, FindFirstFileA);
    DFR(KERNEL32, FindNextFileA);
    DFR(KERNEL32, FindClose);
    DFR(KERNEL32, GetLastError);
}
#undef FindFirstFileA
#undef FindNextFileA
#undef FindClose
#undef GetLastError
#define FindFirstFileA KERNEL32$FindFirstFileA
#define FindNextFileA KERNEL32$FindNextFileA
#define FindClose KERNEL32$FindClose
#define GetLastError KERNEL32$GetLastError

bool EnumNamedPipes() {
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    int count = 0;

    hFind = FindFirstFileA("\\\\.\\pipe\\*", &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        BeaconPrintf(CALLBACK_ERROR, "FindFirstFileA failed (error %lu)\n", GetLastError());
        return false;
    }

    do {
        BeaconPrintf(CALLBACK_OUTPUT, "\\\\.\\pipe\\%s\n", findData.cFileName);
        count++;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    BeaconPrintf(CALLBACK_OUTPUT, "\nTotal pipes: %d\n", count);
    return true;
}
#endif

extern "C" {
    void go(char* args, int len) {
        if (!EnumNamedPipes()) {
            BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate named pipes.");
            return;
        }
    }
}

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
    // Run BOF's entrypoint
    bof::runMocked<>(go);

    return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

namespace UnitTest {
    TEST_F(MockedFunctions, EnumNamedPipesFails) {
        EnumNamedPipes = []() -> bool {return false; };

        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);

        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
    }

    TEST_F(MockedFunctions, EnumNamedPipesSuccess) {
        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);
        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

        bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "\\\\.\\ pipe\\testpipe\n" };
        ASSERT_EQ(expected, got[0]);
    }
}

#endif
