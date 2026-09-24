#include "main.h"

CDetour g_Detour;

DWORD CDetour::DetourFunction(DWORD dwAddress, void *pFunction)
{
    //Init
    iOpcodeLength = 0;
    bOldLocation = NULL;
    dwOrigAddress = 0;
    dwTrampoline = 0;

    if( !dwAddress || !pFunction )
        return 0;

    BYTE *bOpcodes = (BYTE*)dwAddress;

    //Collect whole instructions until we cover at least 5 bytes (for the JMP).
    //The old loop advanced bOpcodes by the running total instead of the last
    //instruction's length, and the ==5 branch added only 4 bytes - both could
    //produce a trampoline that split an instruction or under-copied one.
    while( iOpcodeLength < 5 )
    {
        int iOpcodeLen = oplen(bOpcodes);

        if( iOpcodeLen <= 0 )
            return 0;

        iOpcodeLength += iOpcodeLen;
        bOpcodes += iOpcodeLen;

        if( iOpcodeLength >= 5 )
            break;
    }

    //Allocate space and store original jmp location
    bOldLocation = new BYTE[iOpcodeLength];
    dwOrigAddress = dwAddress;

    //+5: after the copied bytes we append our own 5-byte JMP back into the original
    DWORD dwHookedAddr = (DWORD)VirtualAlloc(0, iOpcodeLength + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if(!dwHookedAddr)
    {
        delete[] bOldLocation;
        bOldLocation = NULL;
        dwOrigAddress = 0;
        iOpcodeLength = 0;
        return 0;
    }
    dwTrampoline = dwHookedAddr;

    //Copy content from function to our new allocated space for retouring and orig func
    memcpy(bOldLocation, (void*)dwAddress, iOpcodeLength);
    memcpy((void*)dwHookedAddr, bOldLocation, iOpcodeLength);

    //Setup jmp back to the original function after the stolen bytes
    BYTE bJmpAddress[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
    *(DWORD*)&bJmpAddress[1] = (DWORD)(dwAddress + iOpcodeLength) - (dwHookedAddr + iOpcodeLength + 5);
    memcpy((void*)(dwHookedAddr + iOpcodeLength), &bJmpAddress, sizeof(bJmpAddress));

    //Unprotect
    DWORD dwOld;
    VirtualProtect((void*)dwAddress, iOpcodeLength, PAGE_EXECUTE_READWRITE, &dwOld);

    //NOP whatever instruction bytes spill past the 5-byte JMP
    for(int i = 5; i < iOpcodeLength; ++i)
        *(BYTE*)(dwAddress + i) = 0x90;

    //Setup jmp to our hook at the start of the original function
    *(DWORD*)&bJmpAddress[1] = (DWORD)pFunction - dwAddress - 0x5;
    memcpy((void*)dwAddress, &bJmpAddress, sizeof(bJmpAddress));

    //Set protections back
    VirtualProtect((void*)dwAddress, iOpcodeLength, dwOld, &dwOld);

    return dwHookedAddr;
}

void CDetour::RetourFunction()
{
    if( !dwOrigAddress || !bOldLocation )
        return;

    //Unprotect
    DWORD dwOld;
    VirtualProtect((void*)dwOrigAddress, iOpcodeLength, PAGE_EXECUTE_READWRITE, &dwOld);

    //Copy old code back to function to avoid crashes on detach etc
    memcpy((void*)dwOrigAddress, bOldLocation, iOpcodeLength);

    //Set protections back
    VirtualProtect((void*)dwOrigAddress, iOpcodeLength, dwOld, &dwOld);

    delete[] bOldLocation;
    bOldLocation = NULL;

    if( dwTrampoline )
    {
        VirtualFree((void*)dwTrampoline, 0, MEM_RELEASE);
        dwTrampoline = 0;
    }

    dwOrigAddress = 0;
    iOpcodeLength = 0;
}
