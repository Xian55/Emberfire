#pragma once
//
// Detours/detours.h  (reconstructed parent-level support header — MINIMAL STUB)
//
// The client uses only a handful of Microsoft Detours entry points, all from
// WindowsKeybindHack.h:
//
//   DetourTransactionBegin();
//   DetourUpdateThread(GetCurrentThread());
//   DetourAttach(&(PVOID&)RealFn, HookedFn);
//   DetourDetach(&(PVOID&)RealFn, HookedFn);
//   DetourTransactionCommit();
//
// These are declared here with their real Microsoft Detours signatures so the
// client compiles and links WITHOUT the Detours library. This is a NO-OP stub:
//   * DetourAttach/DetourDetach do NOT modify the target function pointer, so the
//     hooks are simply never installed (the "Real*" pointers keep their original
//     values and the original WinAPI functions run unhooked).
//   * The WindowsKeybindHack joystick-disable / middle-mouse->F13 remapping will
//     therefore be inactive until the real Detours library is linked in.
//
// To restore full functionality, replace this file with the genuine
// Microsoft Detours <detours.h> and link detours.lib.
//
// Signatures match Microsoft Detours (LONG return, PVOID pointers).
// Compilable standalone under MSVC x86, C++17.
//
#include <windows.h>

#ifndef NO_ERROR
#define NO_ERROR 0L
#endif

// Begin a new detour transaction. Returns NO_ERROR on success.
inline LONG DetourTransactionBegin(void)
{
    return NO_ERROR;
}

// Commit the pending detour transaction. Returns NO_ERROR on success.
inline LONG DetourTransactionCommit(void)
{
    return NO_ERROR;
}

// Abort the pending detour transaction. Returns NO_ERROR on success.
inline LONG DetourTransactionAbort(void)
{
    return NO_ERROR;
}

// Enlist a thread for update in the current transaction.
inline LONG DetourUpdateThread(HANDLE /*hThread*/)
{
    return NO_ERROR;
}

// Attach a detour to a target function pointer.
//   ppPointer : address of the pointer to the target function (also receives the
//               trampoline in the real library). STUB leaves it unchanged.
//   pDetour   : the detour (hook) function.
inline LONG DetourAttach(PVOID* /*ppPointer*/, PVOID /*pDetour*/)
{
    // STUB: real Detours rewrites *ppPointer to point at a trampoline.
    return NO_ERROR;
}

// Detach a previously-attached detour. STUB no-op.
inline LONG DetourDetach(PVOID* /*ppPointer*/, PVOID /*pDetour*/)
{
    return NO_ERROR;
}
