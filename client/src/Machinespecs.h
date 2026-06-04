#pragma once

#ifndef MACHINE_SPECS_H
#define MACHINE_SPECS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <VersionHelpers.h>
#include <intrin.h>
#include <Psapi.h>
#include <powerbase.h>
#include <dxgi.h>
#include <d3d11.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "PowrProf.lib")

class MachineSpecs
{
public:
    static void logMachineSpecs()
    {
        blog(Logger::LOG_INFO, "================================================================================");
        blog(Logger::LOG_INFO, "                         SYSTEM DIAGNOSTICS REPORT                              ");
        blog(Logger::LOG_INFO, "================================================================================");

        logTimestamp();
        logOSInfo();
        logCPUInfo();
        logMemoryInfo();
        logGPUInfo();
        logDisplayInfo();
        logStorageInfo();
        logAudioInfo();
        logNetworkAdapterInfo();
        logLocaleInfo();
        logPowerInfo();
        logProcessInfo();
        logEnvironmentInfo();

        blog(Logger::LOG_INFO, "================================================================================");
        blog(Logger::LOG_INFO, "                       END SYSTEM DIAGNOSTICS REPORT                            ");
        blog(Logger::LOG_INFO, "================================================================================");
    }

private:
    // ==================== TIMESTAMP ====================
    static void logTimestamp()
    {
        blog(Logger::LOG_INFO, "--- Timestamp ---");

        std::time_t now = std::time(nullptr);
        std::tm localTime = {};
        localtime_s(&localTime, &now);

        char timeBuffer[64];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime);
        blog(Logger::LOG_INFO, "Local Time: %s", timeBuffer);

        TIME_ZONE_INFORMATION tzInfo;
        GetTimeZoneInformation(&tzInfo);
        int biasMins = -tzInfo.Bias;
        int biasHours = biasMins / 60;
        int biasRemMins = abs(biasMins % 60);

        if (biasRemMins > 0)
            blog(Logger::LOG_INFO, "Timezone: UTC%+d:%02d", biasHours, biasRemMins);
        else
            blog(Logger::LOG_INFO, "Timezone: UTC%+d", biasHours);

        ULONGLONG uptimeMs = GetTickCount64();
        ULONGLONG uptimeSec = uptimeMs / 1000;
        ULONGLONG days = uptimeSec / 86400;
        ULONGLONG hours = (uptimeSec % 86400) / 3600;
        ULONGLONG mins = (uptimeSec % 3600) / 60;
        blog(Logger::LOG_INFO, "System Uptime: %llud %lluh %llum", days, hours, mins);

        blog(Logger::LOG_INFO, "");
    }

    // ==================== OS INFO ====================
    static void logOSInfo()
    {
        blog(Logger::LOG_INFO, "--- Operating System ---");

        using RtlGetVersionPtr = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll)
        {
            auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
            if (rtlGetVersion)
            {
                RTL_OSVERSIONINFOW osInfo = {};
                osInfo.dwOSVersionInfoSize = sizeof(osInfo);
                if (rtlGetVersion(&osInfo) == 0)
                {
                    blog(Logger::LOG_INFO, "Version: Windows %lu.%lu (Build %lu)",
                        osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
                }
            }
        }

        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t productName[256] = {};
            DWORD size = sizeof(productName);
            if (RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, reinterpret_cast<LPBYTE>(productName), &size) == ERROR_SUCCESS)
            {
                blog(Logger::LOG_INFO, "Product: %s", wstringToString(productName).c_str());
            }

            wchar_t displayVersion[64] = {};
            size = sizeof(displayVersion);
            if (RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr, reinterpret_cast<LPBYTE>(displayVersion), &size) == ERROR_SUCCESS)
            {
                blog(Logger::LOG_INFO, "Display Version: %s", wstringToString(displayVersion).c_str());
            }

            wchar_t editionId[64] = {};
            size = sizeof(editionId);
            if (RegQueryValueExW(hKey, L"EditionID", nullptr, nullptr, reinterpret_cast<LPBYTE>(editionId), &size) == ERROR_SUCCESS)
            {
                blog(Logger::LOG_INFO, "Edition: %s", wstringToString(editionId).c_str());
            }

            RegCloseKey(hKey);
        }

        SYSTEM_INFO sysInfo;
        GetNativeSystemInfo(&sysInfo);
        const char* arch = "Unknown";
        switch (sysInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64: arch = "x64 (AMD64)"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: arch = "x86 (Intel)"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: arch = "ARM64"; break;
        case PROCESSOR_ARCHITECTURE_ARM:   arch = "ARM"; break;
        }
        blog(Logger::LOG_INFO, "Architecture: %s", arch);

        wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
        DWORD compNameSize = sizeof(computerName) / sizeof(wchar_t);
        if (GetComputerNameW(computerName, &compNameSize))
        {
            blog(Logger::LOG_INFO, "Computer Name: %s", wstringToString(computerName).c_str());
        }

        wchar_t userName[256] = {};
        DWORD userNameSize = sizeof(userName) / sizeof(wchar_t);
        if (GetUserNameW(userName, &userNameSize))
        {
            blog(Logger::LOG_INFO, "User Name: %s", wstringToString(userName).c_str());
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== CPU INFO ====================
    static void logCPUInfo()
    {
        blog(Logger::LOG_INFO, "--- CPU ---");

        int cpuInfo[4] = { -1 };
        char cpuBrand[49] = {};

        __cpuid(cpuInfo, 0x80000000);
        unsigned int maxExtFunc = cpuInfo[0];

        if (maxExtFunc >= 0x80000004)
        {
            __cpuid(cpuInfo, 0x80000002);
            memcpy(cpuBrand, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000003);
            memcpy(cpuBrand + 16, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000004);
            memcpy(cpuBrand + 32, cpuInfo, sizeof(cpuInfo));
            cpuBrand[48] = '\0';

            // Trim leading spaces
            const char* brand = cpuBrand;
            while (*brand == ' ') brand++;
            blog(Logger::LOG_INFO, "Model: %s", brand);
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        blog(Logger::LOG_INFO, "Logical Processors: %lu", sysInfo.dwNumberOfProcessors);

        DWORD length = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::vector<char> buffer(length);
            if (GetLogicalProcessorInformationEx(RelationProcessorCore,
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length))
            {
                int coreCount = 0;
                char* ptr = buffer.data();
                while (ptr < buffer.data() + length)
                {
                    auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
                    if (info->Relationship == RelationProcessorCore)
                        coreCount++;
                    ptr += info->Size;
                }
                blog(Logger::LOG_INFO, "Physical Cores: %d", coreCount);
            }
        }

        std::vector<const char*> features;
        __cpuid(cpuInfo, 1);
        int ecx = cpuInfo[2];
        int edx = cpuInfo[3];

        if (edx & (1 << 25)) features.push_back("SSE");
        if (edx & (1 << 26)) features.push_back("SSE2");
        if (ecx & (1 << 0))  features.push_back("SSE3");
        if (ecx & (1 << 9))  features.push_back("SSSE3");
        if (ecx & (1 << 19)) features.push_back("SSE4.1");
        if (ecx & (1 << 20)) features.push_back("SSE4.2");
        if (ecx & (1 << 28)) features.push_back("AVX");
        if (ecx & (1 << 25)) features.push_back("AES-NI");
        if (ecx & (1 << 12)) features.push_back("FMA3");

        __cpuid(cpuInfo, 7);
        int ebx7 = cpuInfo[1];
        if (ebx7 & (1 << 5))  features.push_back("AVX2");
        if (ebx7 & (1 << 16)) features.push_back("AVX-512F");

        if (!features.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < features.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << features[i];
            }
            blog(Logger::LOG_INFO, "Features: %s", oss.str().c_str());
        }

        __cpuid(cpuInfo, 0x80000006);
        int l2CacheKB = (cpuInfo[2] >> 16) & 0xFFFF;
        if (l2CacheKB > 0)
        {
            blog(Logger::LOG_INFO, "L2 Cache: %d KB", l2CacheKB);
        }

        if (maxExtFunc >= 0x80000006)
        {
            int l3CacheKB = ((cpuInfo[3] >> 18) & 0x3FFF) * 512;
            if (l3CacheKB > 0)
            {
                blog(Logger::LOG_INFO, "L3 Cache: %d MB", l3CacheKB / 1024);
            }
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== MEMORY INFO ====================
    static void logMemoryInfo()
    {
        blog(Logger::LOG_INFO, "--- Memory ---");

        MEMORYSTATUSEX memStatus = {};
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            double totalGB = static_cast<double>(memStatus.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
            double availGB = static_cast<double>(memStatus.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
            double usedGB = totalGB - availGB;

            blog(Logger::LOG_INFO, "Total Physical: %.2f GB", totalGB);
            blog(Logger::LOG_INFO, "Available Physical: %.2f GB", availGB);
            blog(Logger::LOG_INFO, "Used Physical: %.2f GB (%lu%% used)", usedGB, memStatus.dwMemoryLoad);

            double totalVirtualGB = static_cast<double>(memStatus.ullTotalVirtual) / (1024.0 * 1024.0 * 1024.0);
            double availVirtualGB = static_cast<double>(memStatus.ullAvailVirtual) / (1024.0 * 1024.0 * 1024.0);

            blog(Logger::LOG_INFO, "Total Virtual: %.2f GB", totalVirtualGB);
            blog(Logger::LOG_INFO, "Available Virtual: %.2f GB", availVirtualGB);

            double totalPageGB = static_cast<double>(memStatus.ullTotalPageFile) / (1024.0 * 1024.0 * 1024.0);
            double availPageGB = static_cast<double>(memStatus.ullAvailPageFile) / (1024.0 * 1024.0 * 1024.0);

            blog(Logger::LOG_INFO, "Total Page File: %.2f GB", totalPageGB);
            blog(Logger::LOG_INFO, "Available Page File: %.2f GB", availPageGB);
        }
        else
        {
            blog(Logger::LOG_ERROR, "Failed to retrieve memory information.");
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== GPU INFO ====================
    static void logGPUInfo()
    {
        blog(Logger::LOG_INFO, "--- GPU (via DXGI) ---");

        IDXGIFactory1* pFactory = nullptr;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));
        if (FAILED(hr))
        {
            blog(Logger::LOG_ERROR, "Failed to create DXGI Factory.");
            blog(Logger::LOG_INFO, "");
            return;
        }

        IDXGIAdapter1* pAdapter = nullptr;
        UINT adapterIndex = 0;

        while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(pAdapter->GetDesc1(&desc)))
            {
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    pAdapter->Release();
                    adapterIndex++;
                    continue;
                }

                blog(Logger::LOG_INFO, "[Adapter %u]", adapterIndex);
                blog(Logger::LOG_INFO, "  Name: %s", wstringToString(desc.Description).c_str());
                blog(Logger::LOG_INFO, "  Vendor ID: 0x%04X", desc.VendorId);
                blog(Logger::LOG_INFO, "  Device ID: 0x%04X", desc.DeviceId);

                double vramGB = static_cast<double>(desc.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0);
                blog(Logger::LOG_INFO, "  Dedicated VRAM: %.2f GB", vramGB);

                double sharedGB = static_cast<double>(desc.SharedSystemMemory) / (1024.0 * 1024.0 * 1024.0);
                blog(Logger::LOG_INFO, "  Shared System Memory: %.2f GB", sharedGB);

                D3D_FEATURE_LEVEL featureLevels[] = {
                    D3D_FEATURE_LEVEL_12_1,
                    D3D_FEATURE_LEVEL_12_0,
                    D3D_FEATURE_LEVEL_11_1,
                    D3D_FEATURE_LEVEL_11_0,
                    D3D_FEATURE_LEVEL_10_1,
                    D3D_FEATURE_LEVEL_10_0,
                    D3D_FEATURE_LEVEL_9_3
                };
                D3D_FEATURE_LEVEL achievedLevel;
                ID3D11Device* pDevice = nullptr;
                if (SUCCEEDED(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                    featureLevels, _countof(featureLevels), D3D11_SDK_VERSION,
                    &pDevice, &achievedLevel, nullptr)))
                {
                    blog(Logger::LOG_INFO, "  Max Feature Level: %s", featureLevelToString(achievedLevel));
                    pDevice->Release();
                }

                IDXGIOutput* pOutput = nullptr;
                UINT outputIndex = 0;
                while (pAdapter->EnumOutputs(outputIndex, &pOutput) != DXGI_ERROR_NOT_FOUND)
                {
                    DXGI_OUTPUT_DESC outputDesc;
                    if (SUCCEEDED(pOutput->GetDesc(&outputDesc)))
                    {
                        blog(Logger::LOG_INFO, "  Output %u: %s", outputIndex,
                            wstringToString(outputDesc.DeviceName).c_str());
                    }
                    pOutput->Release();
                    outputIndex++;
                }
            }
            pAdapter->Release();
            adapterIndex++;
        }

        pFactory->Release();

        if (adapterIndex == 0)
        {
            blog(Logger::LOG_ERROR, "No display adapters found.");
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== DISPLAY INFO ====================
    static void logDisplayInfo()
    {
        blog(Logger::LOG_INFO, "--- Display ---");

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        blog(Logger::LOG_INFO, "Primary Resolution: %dx%d", screenWidth, screenHeight);

        int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        blog(Logger::LOG_INFO, "Virtual Screen: %dx%d", virtualWidth, virtualHeight);

        int monitorCount = GetSystemMetrics(SM_CMONITORS);
        blog(Logger::LOG_INFO, "Monitor Count: %d", monitorCount);

        s_monitorIndex = 0;
        EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, 0);

        DEVMODEW devMode = {};
        devMode.dmSize = sizeof(devMode);
        if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &devMode))
        {
            blog(Logger::LOG_INFO, "Primary Refresh Rate: %lu Hz", devMode.dmDisplayFrequency);
            blog(Logger::LOG_INFO, "Primary Color Depth: %lu bits", devMode.dmBitsPerPel);
        }

        HDC hdc = GetDC(nullptr);
        if (hdc)
        {
            int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            blog(Logger::LOG_INFO, "System DPI: %dx%d", dpiX, dpiY);
            ReleaseDC(nullptr, hdc);
        }

        blog(Logger::LOG_INFO, "");
    }

    static inline int s_monitorIndex = 0;

    static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
    {
        MONITORINFOEXW monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (GetMonitorInfoW(hMonitor, &monitorInfo))
        {
            int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

            const char* primary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) ? " (Primary)" : "";
            blog(Logger::LOG_INFO, "Monitor %d: %dx%d%s", s_monitorIndex, width, height, primary);
        }
        s_monitorIndex++;
        return TRUE;
    }

    // ==================== STORAGE INFO ====================
    static void logStorageInfo()
    {
        blog(Logger::LOG_INFO, "--- Storage ---");

        DWORD driveMask = GetLogicalDrives();
        for (char drive = 'A'; drive <= 'Z'; ++drive)
        {
            if (driveMask & (1 << (drive - 'A')))
            {
                char rootPath[4] = { drive, ':', '\\', '\0' };
                UINT driveType = GetDriveTypeA(rootPath);

                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE)
                {
                    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
                    if (GetDiskFreeSpaceExA(rootPath, &freeBytesAvailable, &totalBytes, &totalFreeBytes))
                    {
                        double totalGB = static_cast<double>(totalBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
                        double freeGB = static_cast<double>(freeBytesAvailable.QuadPart) / (1024.0 * 1024.0 * 1024.0);

                        const char* typeStr = (driveType == DRIVE_FIXED) ? "Fixed" : "Removable";
                        blog(Logger::LOG_INFO, "%c: %.2f GB free / %.2f GB total [%s]",
                            drive, freeGB, totalGB, typeStr);
                    }
                }
            }
        }

        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exePath, MAX_PATH))
        {
            blog(Logger::LOG_INFO, "Executable Path: %s", wstringToString(exePath).c_str());
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== AUDIO INFO ====================
    static void logAudioInfo()
    {
        blog(Logger::LOG_INFO, "--- Audio ---");

        CoInitialize(nullptr);

        IMMDeviceEnumerator* pEnumerator = nullptr;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&pEnumerator));

        if (SUCCEEDED(hr) && pEnumerator)
        {
            IMMDevice* pDevice = nullptr;
            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
            if (SUCCEEDED(hr) && pDevice)
            {
                IPropertyStore* pProps = nullptr;
                hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
                if (SUCCEEDED(hr) && pProps)
                {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);
                    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                    if (SUCCEEDED(hr))
                    {
                        blog(Logger::LOG_INFO, "Default Output: %s", wstringToString(varName.pwszVal).c_str());
                        PropVariantClear(&varName);
                    }
                    pProps->Release();
                }
                pDevice->Release();
            }

            hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &pDevice);
            if (SUCCEEDED(hr) && pDevice)
            {
                IPropertyStore* pProps = nullptr;
                hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
                if (SUCCEEDED(hr) && pProps)
                {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);
                    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                    if (SUCCEEDED(hr))
                    {
                        blog(Logger::LOG_INFO, "Default Input: %s", wstringToString(varName.pwszVal).c_str());
                        PropVariantClear(&varName);
                    }
                    pProps->Release();
                }
                pDevice->Release();
            }

            pEnumerator->Release();
        }
        else
        {
            blog(Logger::LOG_ERROR, "Failed to enumerate audio devices.");
        }

        CoUninitialize();
        blog(Logger::LOG_INFO, "");
    }

    // ==================== NETWORK ADAPTER INFO ====================
    static void logNetworkAdapterInfo()
    {
        blog(Logger::LOG_INFO, "--- Network Adapters (via WMI) ---");

        CoInitialize(nullptr);
        CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

        IWbemLocator* pLocator = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, reinterpret_cast<void**>(&pLocator));

        if (SUCCEEDED(hr) && pLocator)
        {
            IWbemServices* pServices = nullptr;
            hr = pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0,
                nullptr, nullptr, &pServices);

            if (SUCCEEDED(hr) && pServices)
            {
                IEnumWbemClassObject* pEnumerator = nullptr;
                hr = pServices->ExecQuery(_bstr_t(L"WQL"),
                    _bstr_t(L"SELECT * FROM Win32_NetworkAdapter WHERE NetEnabled = TRUE"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);

                if (SUCCEEDED(hr) && pEnumerator)
                {
                    IWbemClassObject* pClassObject = nullptr;
                    ULONG returned = 0;
                    int adapterIndex = 0;

                    while (pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &returned) == S_OK)
                    {
                        VARIANT vtName, vtMAC;
                        VariantInit(&vtName);
                        VariantInit(&vtMAC);

                        if (SUCCEEDED(pClassObject->Get(L"Name", 0, &vtName, nullptr, nullptr)) &&
                            vtName.vt == VT_BSTR)
                        {
                            blog(Logger::LOG_INFO, "Adapter %d: %s", adapterIndex,
                                wstringToString(vtName.bstrVal).c_str());
                        }

                        if (SUCCEEDED(pClassObject->Get(L"MACAddress", 0, &vtMAC, nullptr, nullptr)) &&
                            vtMAC.vt == VT_BSTR)
                        {
                            blog(Logger::LOG_INFO, "  MAC: %s", wstringToString(vtMAC.bstrVal).c_str());
                        }

                        VariantClear(&vtName);
                        VariantClear(&vtMAC);
                        pClassObject->Release();
                        adapterIndex++;
                    }
                    pEnumerator->Release();
                }
                pServices->Release();
            }
            pLocator->Release();
        }
        else
        {
            blog(Logger::LOG_ERROR, "Failed to initialize WMI for network info.");
        }

        CoUninitialize();
        blog(Logger::LOG_INFO, "");
    }

    // ==================== LOCALE INFO ====================
    static void logLocaleInfo()
    {
        blog(Logger::LOG_INFO, "--- Locale & Language ---");

        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        if (GetSystemDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
        {
            blog(Logger::LOG_INFO, "System Locale: %s", wstringToString(localeName).c_str());
        }

        if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
        {
            blog(Logger::LOG_INFO, "User Locale: %s", wstringToString(localeName).c_str());
        }

        HKL hkl = GetKeyboardLayout(0);
        LANGID langId = LOWORD(reinterpret_cast<uintptr_t>(hkl));
        wchar_t langName[256];
        if (GetLocaleInfoW(MAKELCID(langId, SORT_DEFAULT), LOCALE_SLANGUAGE, langName, 256))
        {
            blog(Logger::LOG_INFO, "Keyboard Language: %s", wstringToString(langName).c_str());
        }

        UINT acp = GetACP();
        blog(Logger::LOG_INFO, "ANSI Code Page: %u", acp);

        blog(Logger::LOG_INFO, "");
    }

    // ==================== POWER INFO ====================
    static void logPowerInfo()
    {
        blog(Logger::LOG_INFO, "--- Power ---");

        SYSTEM_POWER_STATUS powerStatus;
        if (GetSystemPowerStatus(&powerStatus))
        {
            const char* acStatus = "Unknown";
            switch (powerStatus.ACLineStatus)
            {
            case 0: acStatus = "Offline (Battery)"; break;
            case 1: acStatus = "Online (AC)"; break;
            }
            blog(Logger::LOG_INFO, "AC Status: %s", acStatus);

            if (powerStatus.BatteryFlag != 128 && powerStatus.BatteryFlag != 255)
            {
                const char* batteryStatus = "Normal";
                if (powerStatus.BatteryFlag & 8) batteryStatus = "Charging";
                else if (powerStatus.BatteryFlag & 4) batteryStatus = "Critical";
                else if (powerStatus.BatteryFlag & 2) batteryStatus = "Low";
                else if (powerStatus.BatteryFlag & 1) batteryStatus = "High";

                blog(Logger::LOG_INFO, "Battery Status: %s", batteryStatus);

                if (powerStatus.BatteryLifePercent != 255)
                {
                    blog(Logger::LOG_INFO, "Battery Level: %d%%", powerStatus.BatteryLifePercent);
                }
            }
            else
            {
                blog(Logger::LOG_INFO, "Battery: Not present");
            }
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== PROCESS INFO ====================
    static void logProcessInfo()
    {
        blog(Logger::LOG_INFO, "--- Process ---");

#ifdef _WIN64
        blog(Logger::LOG_INFO, "Process Architecture: 64-bit");
#else
        BOOL isWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &isWow64);
        blog(Logger::LOG_INFO, "Process Architecture: 32-bit%s", isWow64 ? " (WoW64)" : "");
#endif

        BOOL isElevated = FALSE;
        HANDLE hToken = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_ELEVATION elevation;
            DWORD size;
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
            {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
        blog(Logger::LOG_INFO, "Elevated (Admin): %s", isElevated ? "Yes" : "No");

        blog(Logger::LOG_INFO, "Process ID: %lu", GetCurrentProcessId());

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            double workingSetMB = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
            blog(Logger::LOG_INFO, "Working Set: %.2f MB", workingSetMB);
        }

        LPWSTR cmdLine = GetCommandLineW();
        if (cmdLine)
        {
            blog(Logger::LOG_INFO, "Command Line: %s", wstringToString(cmdLine).c_str());
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== ENVIRONMENT INFO ====================
    static void logEnvironmentInfo()
    {
        blog(Logger::LOG_INFO, "--- Environment ---");

        const char* envVars[] = {
            "PATH",
            "TEMP",
            "APPDATA",
            "LOCALAPPDATA",
            "PROGRAMFILES",
            "PROCESSOR_ARCHITECTURE"
        };

        for (const char* var : envVars)
        {
            char* value = nullptr;
            size_t len = 0;
            if (_dupenv_s(&value, &len, var) == 0 && value != nullptr)
            {
                std::string strValue(value);
                if (strValue.length() > 200)
                {
                    strValue = strValue.substr(0, 200) + "...";
                }
                blog(Logger::LOG_INFO, "%s: %s", var, strValue.c_str());
                free(value);
            }
        }

        blog(Logger::LOG_INFO, "");
    }

    // ==================== UTILITY FUNCTIONS ====================
    static std::string wstringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";

        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
            nullptr, 0, nullptr, nullptr);
        std::string result(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
            &result[0], size, nullptr, nullptr);
        return result;
    }

    static const char* featureLevelToString(D3D_FEATURE_LEVEL level)
    {
        switch (level)
        {
        case D3D_FEATURE_LEVEL_12_1: return "12.1";
        case D3D_FEATURE_LEVEL_12_0: return "12.0";
        case D3D_FEATURE_LEVEL_11_1: return "11.1";
        case D3D_FEATURE_LEVEL_11_0: return "11.0";
        case D3D_FEATURE_LEVEL_10_1: return "10.1";
        case D3D_FEATURE_LEVEL_10_0: return "10.0";
        case D3D_FEATURE_LEVEL_9_3:  return "9.3";
        case D3D_FEATURE_LEVEL_9_2:  return "9.2";
        case D3D_FEATURE_LEVEL_9_1:  return "9.1";
        default: return "Unknown";
        }
    }
};

#endif // MACHINE_SPECS_H