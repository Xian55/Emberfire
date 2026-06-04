#pragma once
// Force-included (CMake /FI) to give Machinespecs.h a NTSTATUS type it uses without including <winternl.h>.
// Identical-typedef redefinition is legal in C++, so this is safe even if a TU also pulls in winternl.h.
typedef long NTSTATUS;
