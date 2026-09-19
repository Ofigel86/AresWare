// r47: detours15_compat.cpp — Detours 1.5 API shim over Detours 3 (ClientMod/detours3.lib).
//
// The MSVC project links BOTH detours generations (detours.lib 1.5 for the base's
// DetourFunction/DetourFindFunction/DetourRemove and detours3.lib for the vendored
// ClientMod emulator). binutils cannot link the two archives together: both contain
// a CDetourDis disassembler with identical C++ symbols. This file keeps only Detours 3
// and reimplements the two legacy entry points on top of it, so a MinGW build links
// cleanly. MSVC builds do not compile this file.

#include <windows.h>
#include "ClientMod/detours3.h" // r47: Detours 3 prototypes

// v1.5: PBYTE WINAPI DetourFunction( PBYTE pTarget, PBYTE pDetour )
//   installs pDetour over pTarget, returns the trampoline used to call the original.
extern "C" void* __stdcall DetourFunction( void* pTarget, void* pDetour )
{
	if( !pTarget || !pDetour )
		return NULL;

	void* trampoline = pTarget;
	DetourTransactionBegin();
	DetourUpdateThread( GetCurrentThread() );

	if( DetourAttach( &trampoline, pDetour ) != NO_ERROR )
	{
		DetourTransactionAbort();
		return NULL;
	}
	if( DetourTransactionCommit() != NO_ERROR )
		return NULL;

	return trampoline;
}

// v1.5: BOOL WINAPI DetourRemove( PBYTE pbTrampoline, PBYTE pbTarget )
//   removes the hook originally created by DetourFunction.
extern "C" BOOL __stdcall DetourRemove( void* pTrampoline, void* pDetour )
{
	if( !pTrampoline )
		return FALSE;

	DetourTransactionBegin();
	DetourUpdateThread( GetCurrentThread() );
	DetourDetach( &pTrampoline, pDetour );
	return DetourTransactionCommit() == NO_ERROR;
}
