#pragma once
// Minimal NvAPI stub — only what Application::configureNvidia references. All no-ops returning OK,
// so configureNvidia() does nothing (returns false -> no restart). The real perf-profile tweak is skipped.
typedef unsigned int   NvU32;
typedef unsigned short NvU16;
typedef int            NvAPI_Status;
#define NVAPI_OK 0
#define NVAPI_UNICODE_STRING_MAX 2048
typedef NvU16 NvAPI_UnicodeString[NVAPI_UNICODE_STRING_MAX];
typedef void* NvDRSSessionHandle;
typedef void* NvDRSProfileHandle;

#define NVDRS_PROFILE_VER              0
#define NVDRS_APPLICATION_VER_V1       0
#define NVDRS_APPLICATION_VER          0
#define NVDRS_SETTING_VER              0
#define NVDRS_CURRENT_PROFILE_LOCATION 0
#define NVDRS_DWORD_TYPE               0

struct NVDRS_PROFILE {
    NvU32 version; NvU32 isPredefined; NvAPI_UnicodeString profileName; NvU32 gpuSupport;
};
struct NVDRS_APPLICATION {
    NvU32 version; NvU32 isPredefined;
    NvAPI_UnicodeString appName, userFriendlyName, launcher, fileInFolder;
};
struct NVDRS_SETTING {
    NvU32 version; NvAPI_UnicodeString settingName; NvU32 settingId; NvU32 settingType;
    NvU32 settingLocation; NvU32 isCurrentPredefined; NvU32 isPredefinedValid;
    NvU32 u32PredefinedValue; NvU32 u32CurrentValue;
};

// Return NOT-OK so Application::configureNvidia() early-returns false (skips the driver tweak + the
// one-time restart). Otherwise the no-op stub looks like "applied a new setting" -> restart loop -> instant exit.
inline NvAPI_Status NvAPI_Initialize() { return -1; }
inline NvAPI_Status NvAPI_DRS_CreateSession(NvDRSSessionHandle* h) { *h = nullptr; return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_DestroySession(NvDRSSessionHandle) { return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_LoadSettings(NvDRSSessionHandle) { return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_SaveSettings(NvDRSSessionHandle) { return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_CreateProfile(NvDRSSessionHandle, NVDRS_PROFILE*, NvDRSProfileHandle* p) { *p = nullptr; return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_CreateApplication(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_APPLICATION*) { return NVAPI_OK; }
inline NvAPI_Status NvAPI_DRS_SetSetting(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_SETTING*) { return NVAPI_OK; }
