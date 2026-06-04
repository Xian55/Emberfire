#pragma once
//
// CrashHandler.h  (reconstructed parent-level support header)
//
// Reconstructed from Main.cpp call sites:
//
//   ::SetUnhandledExceptionFilter(CrashHandler::UnhandledExceptionHandler);
//   CrashHandler::Initialize(dsnWide, versionWide, logPathWide);
//
// UnhandledExceptionHandler must match LPTOP_LEVEL_EXCEPTION_FILTER:
//   LONG WINAPI (PEXCEPTION_POINTERS)
//
// The original wired this to Sentry crash reporting. This reconstruction is a
// compilable no-op stub (pass-through filter + Initialize that just records the
// log path) so the client builds without the crash-reporting backend. Replace
// with the real Sentry-backed implementation when available.
//
// Compilable standalone under MSVC x86, C++17.
//
#include <windows.h>
#include <string>

class CrashHandler
{
public:
    // Top-level exception filter. Returning EXCEPTION_CONTINUE_SEARCH lets the
    // default OS handling proceed (matches a no-op crash reporter).
    static LONG WINAPI UnhandledExceptionHandler(PEXCEPTION_POINTERS /*exceptionInfo*/)
    {
        // STUB: real implementation captured a minidump and uploaded to Sentry.
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Initialize the crash reporter.
    //   dsn      : Sentry DSN (wide)
    //   version  : build version (wide)
    //   logPath  : path to the active log file (wide)
    static void Initialize(const wchar_t* /*dsn*/,
                           const std::wstring& /*version*/,
                           const wchar_t* /*logPath*/)
    {
        // STUB: real implementation initialised the Sentry native SDK.
    }
};
