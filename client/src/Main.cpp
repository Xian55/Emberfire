// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Application.h"
#include "OpenGlCheck.h"

#include "..\CrashHandler.h"
#include "..\Shared\DmystVersion.h"
#include "..\Logger.h"

#include "Machinespecs.h"
#include "steam_api.h"

#pragma comment(lib, "winmm.lib")

#ifdef DMYST_INTERNAL
// internal uses main (Console subsystem in project settings)
int main()
#else
// user build uses WinMain (Windows subsystem in project settings)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
    GlCaps caps = probeGL();

    if (caps.renderer.find("GDI Generic") != std::string::npos ||
        caps.maxTexture <= 1024 ||
        !caps.shadersSupported)
    {
        showGlErrorMessage(caps);
        return EXIT_FAILURE;
    }

    ::timeBeginPeriod(1);
    ::SetCurrentDirectoryA("..\\");
    ::SetUnhandledExceptionFilter(CrashHandler::UnhandledExceptionHandler);

    MachineSpecs::logMachineSpecs();

#ifdef DMYST_INTERNAL
    blog(Logger::LOG_INFO, "DMYST INTERNAL BUILD %d", DYMST_VERSION);
#else
    CrashHandler::Initialize(
        L"https://7ef0a40bbc9d6ab99a2c9b269d4aa20a@o4510569281552384.ingest.us.sentry.io/4510569525411840",
        to_wstring(DYMST_VERSION),
        Logger::instance().getLogPath().c_str());
#endif

#ifndef DMYST_INTERNAL
    // Steam init after crash handler so any Steam failures get logged
    if (!SteamAPI_Init())
    {
        blog(Logger::LOG_ERROR, "Steam API failed to initialize");
        MessageBoxA(nullptr, "Please launch the game through Steam.", "Emberfire", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
#endif
    sApplication->begin();
    sApplication->clearNow();

    SteamAPI_Shutdown();
    return EXIT_SUCCESS;
}