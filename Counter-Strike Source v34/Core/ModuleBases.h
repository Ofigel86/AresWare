#pragma once
#ifndef _MODULE_BASES_H_
#define _MODULE_BASES_H_

#include <Windows.h>

// Runtime module bases.
//
// This project was written for the classic no-ASLR v34 layout where the game
// DLLs always landed at fixed bases (engine 0x20000000, server 0x22000000,
// client 0x24000000). On current systems client.dll / server.dll get rebased
// almost every run, so every engine-relative offset MUST ride on the real
// module base - otherwise detours/read-writes hit dead memory and kill hl2.exe.
//
// The BASE_* names stay macros, but now expand to these globals, so all the
// existing `( DWORD ) BASE_CLIENT + 0xXXXXXX` code keeps compiling unchanged.
extern DWORD g_dwBaseEngineDll; // engine.dll
extern DWORD g_dwBaseServerDll; // server.dll - 0 until a (listen) server is up
extern DWORD g_dwBaseClientDll; // client.dll

#define BASE_ENGINE g_dwBaseEngineDll
#define BASE_SERVER g_dwBaseServerDll
#define BASE_CLIENT g_dwBaseClientDll

// Resolves engine/client/server.dll bases via GetModuleHandleA and logs them.
// Safe to call repeatedly; server.dll only exists after joining a game.
void ResolveModuleBases( void );

// true if the address is committed memory in an executable page
// (detour-target sanity check before CDetour touches the opcodes).
bool AddressInModule( DWORD address, DWORD moduleBase );

#endif // _MODULE_BASES_H_
