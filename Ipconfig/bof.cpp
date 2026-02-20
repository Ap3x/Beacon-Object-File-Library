#include <Windows.h>
#include <iphlpapi.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "..\Core\base\mock.h"
#pragma comment(lib, "Iphlpapi.lib")
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_Ipconfig = bool(*)();

m_Ipconfig Ipconfig = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
    void SetUp() override {
        Ipconfig = []() -> bool {
            BeaconPrintf(CALLBACK_OUTPUT, "Adapter: Local Area Connection\n");
            BeaconPrintf(CALLBACK_OUTPUT, "  IP Address:      192.168.1.100\n");
            BeaconPrintf(CALLBACK_OUTPUT, "  Subnet Mask:     255.255.255.0\n");
            BeaconPrintf(CALLBACK_OUTPUT, "  Default Gateway: 192.168.1.1\n");
            return true;
            };
    }
    void TearDown() override {
        Ipconfig = nullptr;
    }
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
    DFR(IPHLPAPI, GetAdaptersInfo);
    DFR(KERNEL32, GetProcessHeap);
    DFR(KERNEL32, HeapAlloc);
    DFR(KERNEL32, HeapFree);
}
#undef GetAdaptersInfo
#define GetAdaptersInfo IPHLPAPI$GetAdaptersInfo
#define GetProcessHeap KERNEL32$GetProcessHeap
#define HeapAlloc KERNEL32$HeapAlloc
#define HeapFree KERNEL32$HeapFree

bool Ipconfig() {
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulOutBufLen);

    if (pAdapterInfo == NULL) {
        BeaconPrintf(CALLBACK_ERROR, "HeapAlloc failed\n");
        return false;
    }

    DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        HeapFree(GetProcessHeap(), 0, pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulOutBufLen);
        if (pAdapterInfo == NULL) {
            BeaconPrintf(CALLBACK_ERROR, "HeapAlloc failed\n");
            return false;
        }
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    }

    if (dwRetVal != NO_ERROR) {
        BeaconPrintf(CALLBACK_ERROR, "GetAdaptersInfo failed with error: %d\n", dwRetVal);
        HeapFree(GetProcessHeap(), 0, pAdapterInfo);
        return false;
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter) {
        BeaconPrintf(CALLBACK_OUTPUT, "Adapter: %s\n", pAdapter->Description);
        BeaconPrintf(CALLBACK_OUTPUT, "  IP Address:      %s\n", pAdapter->IpAddressList.IpAddress.String);
        BeaconPrintf(CALLBACK_OUTPUT, "  Subnet Mask:     %s\n", pAdapter->IpAddressList.IpMask.String);
        BeaconPrintf(CALLBACK_OUTPUT, "  Default Gateway: %s\n", pAdapter->GatewayList.IpAddress.String);
        BeaconPrintf(CALLBACK_OUTPUT, "\n");
        pAdapter = pAdapter->Next;
    }

    HeapFree(GetProcessHeap(), 0, pAdapterInfo);
    return true;
}
#endif

extern "C" {
    void go(char* args, int len) {
        if (!Ipconfig()) {
			BeaconPrintf(CALLBACK_ERROR, "Failed to retrieve adapter information.");
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
    TEST_F(MockedFunctions, IpconfigFails) {
        Ipconfig = []() -> bool {return false;};

        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);

        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
    }

    TEST_F(MockedFunctions, IpconfigSuccess) {
        std::vector<bof::output::OutputEntry> got = bof::runMocked<>(go);
        ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

        bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "Adapter: Local Area Connection\n" };
        ASSERT_EQ(expected, got[0]);
    }
}

#endif
