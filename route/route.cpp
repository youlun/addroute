// route.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "DownloadProgress.h"
#include "Utils.h"

#include <bitset>

using std::wcout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;

std::wstring URL(L"https://ftp.apnic.net/stats/apnic/delegated-apnic-latest?");
uint32_t BestRouteDestAddr = 1920103026; // 114.114.114.114
vector<unique_ptr<MIB_IPFORWARDROW>> Routes;
wchar_t TempFile[MAX_PATH];
MIB_IPFORWARDROW BestRoute;

void init()
{
    wcout << L"Initializing..." << endl;

#ifndef _DEBUG
    std::random_device device;
    std::default_random_engine generator(device());
    std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);
    URL.append(std::to_wstring(distribution(generator)));
#endif

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    GetTempFileNameW(tempPath, L"", 0, TempFile);

    GetBestRoute(BestRouteDestAddr, NULL, &BestRoute);
}

void ProcessApnicLine(const string &strIP, const string &strCount)
{
    unique_ptr<MIB_IPFORWARDROW> forwardRow(std::make_unique<MIB_IPFORWARDROW>());

    uint32_t ip, mask;
    inet_pton(AF_INET, strIP.c_str(), &ip);
    mask = htonl(UINT32_MAX << static_cast<uint32_t>(log2(std::stoi(strCount))));

    *forwardRow = BestRoute;

    forwardRow->dwForwardDest = ip;
    forwardRow->dwForwardMask = mask;
    forwardRow->dwForwardPolicy = 0;
    forwardRow->dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
    forwardRow->dwForwardProto = MIB_IPPROTO_NETMGMT;
    forwardRow->dwForwardAge = 0;
    forwardRow->dwForwardNextHopAS = 0;

    Routes.push_back(std::move(forwardRow));
}

int main()
{
    init();

    unique_ptr<IBindStatusCallback> callback(std::make_unique<DownloadProgress>());
    HRESULT hr = URLDownloadToFileW(NULL, URL.c_str(), TempFile, 0, callback.get());

    if (hr != S_OK) {
        wcout << L"Download failed!" << endl;
        return -1;
    }
    wcout << endl << L"Downloaded" << endl;

    std::ifstream infile(TempFile);
    string line;
    vector<string> tokens;
    uint32_t total = 0, now = 0;
    while (std::getline(infile, line)) {
        tokens.clear();
        SplitString(&tokens, &line, "|");
        if (tokens.size() != 7
            || tokens[1].compare("CN") != 0
            || tokens[2].compare("ipv4") != 0) continue;
        ProcessApnicLine(tokens[3], tokens[4]);
        total++;
    }

    for (auto &it : Routes) {
        CreateIpForwardEntry(it.get());
        wprintf(L"\rAdding... %d/%d", ++now, total);
    }

    wcout << endl << L"OK" << endl;

    getchar();

    return 0;
}

