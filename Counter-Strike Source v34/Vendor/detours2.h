//detours.h

class CDetour
{
public:
	CDetour() : iOpcodeLength(0), dwOrigAddress(0), bOldLocation(NULL), dwTrampoline(0) {}
	DWORD DetourFunction(DWORD dwAddress, void *pFunction);
	void RetourFunction();

private:
	int iOpcodeLength;
	DWORD dwOrigAddress;
	BYTE *bOldLocation;
	DWORD dwTrampoline;
};

extern CDetour g_Detour;
