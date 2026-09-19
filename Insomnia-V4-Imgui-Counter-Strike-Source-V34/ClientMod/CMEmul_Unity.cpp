// BUILD MARKER r32 (2026-09-18): ClientMod3 Emulator, unity translation unit.
// Vendored from Informant.WTF (clientmod by rusherr-c; ClientMod3Emulator by
// 0TheSpy; RevSpoofer by 2010kohtep). Answers the "client mod" checks of
// no-steam v34 servers: spoofed revemu connect ticket, cvar-update spoof,
// DispatchUserMessage/SetStringUserData emulation, "Disconnect by ClientMod".
// CMEMUL_STATIC renames the entry to CMEMUL_Main (no DllMain of its own).
// Hooks live for the whole process lifetime (r32: uninject hotkeys removed).
#define CMEMUL_STATIC 1
#pragma comment(lib, "ClientMod/detours3")
#include "dllmain.cpp"
#include "hash.cpp"
#include "Public/StrUtils.cpp"
#include "Public/RevSpoofer.cpp"
#include "Public/Encryption/CRijndael.cpp"
#include "Public/Encryption/DoubleBuffering.cpp"
#include "Public/Encryption/MessageDigest.cpp"
#include "Public/Encryption/SHA.cpp"
