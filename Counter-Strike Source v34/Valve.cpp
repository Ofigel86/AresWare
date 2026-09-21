#include "Valve.hpp"
#include "Source.hpp"
#include "Entity.hpp"
#include "inetmsghandler.hpp"

bool CGameTrace::DidHitWorld(void) const
{
	return m_pEnt == Source::m_pEntList->GetBaseEntity(0);
}

bool CGameTrace::DidHitNonWorldEntity(void) const
{
	return m_pEnt != NULL && !DidHitWorld();
}

KeyValues::KeyValues(const char* setName)
{
	typedef void(__thiscall* KeyValuesFn)(void*, const char*);

	static KeyValuesFn pKeyValues = nullptr;

	if (!pKeyValues)
	{
		pKeyValues = (KeyValuesFn)Memory::PatternScan(XorStr("client.dll"), XorStr("33 C0 56 8B F1 C7 06 ?? ?? ?? ?? 88 46 10"));

		if (!pKeyValues)
			return;
	}

	pKeyValues(this, setName);
}

bool KeyValues::LoadFromBuffer(const char* resourceName, const char* pBuffer, void* pFileSystem, const char* pPathID)
{
	typedef bool(__thiscall* LoadFromBufferFn)(void*, const char*, const char*, void*, const char*);

	static LoadFromBufferFn pLoadFromBuffer = nullptr;

	if (!pLoadFromBuffer)
	{
		pLoadFromBuffer = (LoadFromBufferFn)Memory::PatternScan(XorStr("client.dll"), XorStr("64 A1 ?? ?? ?? ?? 8B 54 24 08 6A FF"));

		if (!pLoadFromBuffer)
			return false;
	}

	return pLoadFromBuffer(this, resourceName, pBuffer, pFileSystem, pPathID);
}

TraceType_t CTraceFilter::GetTraceType(void) const
{
	return TRACE_EVERYTHING;
}

CTraceFilterSimple::CTraceFilterSimple(void)
	: m_pPassEnt(nullptr)
{

}

CTraceFilterSimple::CTraceFilterSimple(const C_BaseEntity* pPassEnt)
	: m_pPassEnt(pPassEnt)
{

}

bool CTraceFilterSimple::ShouldHitEntity(IHandleEntity* pEntity, int contentsMask)
{
	auto pEnt = Source::m_pEntList->GetBaseEntityFromHandle(pEntity->GetRefEHandle());

	return !(pEnt == m_pPassEnt);
}

CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities(const C_BaseEntity* pPassEnt, const C_BaseEntity* pPassEnt2)
	: m_pPassEnt(pPassEnt),
	m_pPassEnt2(pPassEnt2)
{

}

bool CTraceFilterSkipTwoEntities::ShouldHitEntity(IHandleEntity* pEntity, int contentsMask)
{
	auto pEnt = Source::m_pEntList->GetBaseEntityFromHandle(pEntity->GetRefEHandle());

	return !(pEnt == m_pPassEnt || pEnt == m_pPassEnt2);
}

bool CTraceFilterWorldOnly::ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask )
{
	auto pEnt = Source::m_pEntList->GetBaseEntityFromHandle(pServerEntity->GetRefEHandle());

	if( pEnt )
	{
		auto pBaseEntity = pEnt->GetBaseEntity();

		if( pBaseEntity )
			return ( pBaseEntity->GetIndex() == 0 );
	}

	return false;
}

TraceType_t CTraceFilterWorldOnly::GetTraceType() const
{
	return TRACE_WORLD_ONLY;
}
namespace Valve
{
	CCSWeaponInfo* GetCSWpnData(C_WeaponCSBaseGun* pWeapon)
	{
		typedef CCSWeaponInfo* (__thiscall* GetCSWpnDataFn)(void*);

		static GetCSWpnDataFn pGetCSWpnData = nullptr;

		if (!pGetCSWpnData)
		{
			pGetCSWpnData = (GetCSWpnDataFn)Memory::PatternScan(XorStr("client.dll"), XorStr("66 8B 81 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 83 C4 04"));

			if (!pGetCSWpnData)
			{
				DPRINT(XorStr("[Valve::GetCSWpnData] Can't find function address!"));
				return nullptr;
			}
		}

		if (!pWeapon)
		{
			DPRINT(XorStr("[Valve::GetCSWpnData] Weapon parameter is invalid!"));
			return nullptr;
		}

		return pGetCSWpnData(pWeapon);
	}

	D3DDeviceWrapper* Dx9Device()
	{
		typedef D3DDeviceWrapper* (*Dx9DeviceFn)();

		static auto hMod = ModuleD(XorStr("shaderapidx9.dll"));
		static auto pDx9Device = (Dx9DeviceFn)((std::uintptr_t)hMod + 0xBC40);

		return pDx9Device();
	}

	void SetPredictionSeed(CUserCmd* pCmd)
	{
		typedef int(__cdecl* SetPredictionSeedFn)(CUserCmd*);

		static SetPredictionSeedFn pSetPredictionSeed = nullptr;

		if (!pSetPredictionSeed)
		{
			pSetPredictionSeed = (SetPredictionSeedFn)Memory::PatternScan(XorStr("client.dll"), XorStr("8B 44 24 04 85 C0 75 0B"));

			if (!pSetPredictionSeed)
			{
				DPRINT(XorStr("[Valve::SetPredictionSeed] Can't find function address!"));
				return;
			}
		}

		pSetPredictionSeed(pCmd);
	}

	void SetPredictionPlayer(C_CSPlayer* pPlayer)
	{
		static std::uintptr_t uPredictionPlayer = 0;

		if (!uPredictionPlayer)
		{
			uPredictionPlayer = Memory::PatternScan(XorStr("client.dll"), XorStr("89 35 ?? ?? ?? ?? DB 86 ?? ?? ?? ?? 83 C4 04"));

			if (!uPredictionPlayer)
			{
				DPRINT(XorStr("[Valve::SetPredictionPlayer] Can't find address!"));
				return;
			}

			uPredictionPlayer = *(std::uintptr_t*)(uPredictionPlayer + 2);

			if (!uPredictionPlayer)
			{
				DPRINT(XorStr("[Valve::SetPredictionPlayer] Can't get address!"));
				return;
			}
		}

		*(C_CSPlayer**)uPredictionPlayer = pPlayer;
	}

	void RandomSeed(unsigned int nSeed)
	{
		typedef void(*RandomSeedFn)(unsigned int);

		static RandomSeedFn pRandomSeed = nullptr;

		if (!pRandomSeed)
		{
			auto hMod = ModuleD(XorStr("vstdlib.dll"));

			if (!hMod)
			{
				DPRINT(XorStr("[Valve::RandomSeed] Can't get 'vstdlib.dll' handle!"));
				return;
			}

			pRandomSeed = (RandomSeedFn)GetProcedure(hMod, XorStr("RandomSeed"));

			if (!pRandomSeed)
			{
				DPRINT(XorStr("[Valve::RandomSeed] Can't get function address!"));
				return;
			}
		}

		pRandomSeed(nSeed);
	}

	float RandomFloat(float flMin, float flMax)
	{
		typedef float(*RandomFloatFn)(float, float);

		static RandomFloatFn pRandomFloat = nullptr;

		if (!pRandomFloat)
		{
			auto hMod = ModuleD(XorStr("vstdlib.dll"));

			if (!hMod)
			{
				DPRINT(XorStr("[Valve::RandomFloat] Can't get 'vstdlib.dll' handle!"));
				return 0.0f;
			}

			pRandomFloat = (RandomFloatFn)GetProcedure(hMod, XorStr("RandomFloat"));

			if (!pRandomFloat)
			{
				DPRINT(XorStr("[Valve::RandomFloat] Can't get function address!"));
				return 0.0f;
			}
		}

		return pRandomFloat(flMin, flMax);
	}

	int RandomInt(int iMin, int iMax)
	{
		typedef int(*RandomIntFn)(int, int);

		static RandomIntFn pRandomInt = nullptr;

		if (!pRandomInt)
		{
			auto hMod = ModuleD(XorStr("vstdlib.dll"));

			if (!hMod)
			{
				DPRINT(XorStr("[Valve::RandomInt] Can't get 'vstdlib.dll' handle!"));
				return 0;
			}

			pRandomInt = (RandomIntFn)GetProcedure(hMod, XorStr("RandomInt"));

			if (!pRandomInt)
			{
				DPRINT(XorStr("[Valve::RandomInt] Can't get function address!"));
				return 0;
			}
		}

		return pRandomInt(iMin, iMax);
	}

	void GetBulletTypeParameters(int iBulletType, float& flPenetrationPower, float& flPenetrationDistance)
	{
		typedef void(__stdcall* GetBulletTypeParametersFn)(int, float&, float&);

		static GetBulletTypeParametersFn pGetBulletTypeParameters = nullptr;

		if (!pGetBulletTypeParameters)
		{
			pGetBulletTypeParameters = (GetBulletTypeParametersFn)Memory::PatternScan(XorStr("client.dll"), XorStr("56 8B 74 24 08 68 ?? ?? ?? ?? 56 E8 ?? ?? ?? ?? 83 C4 08 84 C0"));

			if (!pGetBulletTypeParameters)
			{
				DPRINT(XorStr("[Valve::GetBulletTypeParameters] Can't get function address!"));
				return;
			}
		}

		pGetBulletTypeParameters(iBulletType, flPenetrationPower, flPenetrationDistance);
	}

	void ClipTraceToPlayers(const Vector3& vStart, const Vector3& vEnd, unsigned int iMask, ITraceFilter* pTraceFilter, trace_t* pTrace)
	{
		typedef void(*ClipTraceToPlayersFn)(const Vector3&, const Vector3&, unsigned int, ITraceFilter*, trace_t*);

		static ClipTraceToPlayersFn pClipTraceToPlayers = nullptr;

		if (!pClipTraceToPlayers)
		{
			pClipTraceToPlayers = (ClipTraceToPlayersFn)Memory::PatternScan(XorStr("client.dll"), XorStr("81 EC ?? ?? ?? ?? 8B 84 24 ?? ?? ?? ?? 8B 48 2C"));

			if (!pClipTraceToPlayers)
			{
				DPRINT(XorStr("[Valve::ClipTraceToPlayers] Can't get function address!"));
				return;
			}
		}

		pClipTraceToPlayers(vStart, vEnd, iMask, pTraceFilter, pTrace);
	}

	void GetMaterialParameters(int iMaterial, float& flPenetrationModifier, float& flDamageModifier)
	{
		switch (iMaterial)
		{
		case CHAR_TEX_METAL:
			flPenetrationModifier = 0.5f;
			flDamageModifier = 0.3f;
			break;
		case CHAR_TEX_DIRT:
			flPenetrationModifier = 0.5f;
			flDamageModifier = 0.3f;
			break;
		case CHAR_TEX_CONCRETE:
			flPenetrationModifier = 0.4f;
			flDamageModifier = 0.25f;
			break;
		case CHAR_TEX_GRATE:
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.99f;
			break;
		case CHAR_TEX_VENT:
			flPenetrationModifier = 0.5f;
			flDamageModifier = 0.45f;
			break;
		case CHAR_TEX_TILE:
			flPenetrationModifier = 0.65f;
			flDamageModifier = 0.3f;
			break;
		case CHAR_TEX_COMPUTER:
			flPenetrationModifier = 0.4f;
			flDamageModifier = 0.45f;
			break;
		case CHAR_TEX_WOOD:
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.6f;
			break;
		default:
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.5f;
			break;
		}
	}

	bool TraceToExit(Vector3& vStart, Vector3& vDirection, Vector3& vEnd, float flStepSize, float flMaxDistance)
	{
		float flDistance = 0.0f;

		Vector3 vLast = vStart;

		while (flDistance <= flMaxDistance)
		{
			flDistance += flStepSize;

			vEnd = vStart + flDistance * vDirection;

			// SDK MASK_SOLID (0x204400B): original missed CONTENTS_MOVEABLE.
			if ((Source::m_pEngineTrace->GetPointContents(vEnd) & 0x204400B) == 0)
				return true;
		}

		return false;
	}

	int GetPlayerModDamage(float flDamage, int iArmorValue, float flArmorRatio, int iHitgroup, bool bIsFriendly, bool bHasHelmet)
	{
		if (bIsFriendly)
			flDamage *= 0.35f;

		flArmorRatio *= 0.5f;

		// SDK CCSPlayer::IsArmored: generic/chest/stomach/arms take armor when
		// ArmorValue > 0, head only with a helmet, legs never take armor.
		// (Original applied armor to legs too, underestimating leg damage.)
		bool bApplyArmor = false;

		if (iArmorValue > 0)
		{
			switch (iHitgroup)
			{
			case 0: // HITGROUP_GENERIC
			case 2: // HITGROUP_CHEST
			case 3: // HITGROUP_STOMACH
			case 4: // HITGROUP_LEFTARM
			case 5: // HITGROUP_RIGHTARM
				bApplyArmor = true;
				break;
			case 1: // HITGROUP_HEAD
				bApplyArmor = bHasHelmet;
				break;
			default: // HITGROUP_LEFTLEG / HITGROUP_RIGHTLEG: no armor
				break;
			}
		}

		if (bApplyArmor)
		{
			float flNew = flDamage * flArmorRatio;
			float flArmor = (flDamage - flNew) * 0.5f;

			if (flArmor <= iArmorValue)
			{
				flArmor = floor(flArmor);
			}
			else
			{
				flNew = flDamage + (iArmorValue * -2.0f);
				flArmor = (float)iArmorValue;
			}

			flDamage = floor(flNew);
		}
		else
		{
			flDamage = floor(flDamage);
		}

		return (int)flDamage;
	}

	float GetHitgroupModDamage(float flDamage, int iHitgroup)
	{
		static float flHitgroupDamageMod[] = { 1.0f, 4.0f, 1.0f, 1.25f, 1.0f, 1.0f, 0.75f, 0.75f };

		return (flDamage * flHitgroupDamageMod[iHitgroup]);
	}
}

bool compare(const std::string& a, const std::string& b)
{
	auto size = a.size();

	if (b.size() != size)
		return false;

	for (auto i = 0; i < size; i++)
	{
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	}

	return true;
}

#include <psapi.h>

HMODULE ModuleD(const char* target)
{
	MEMORY_BASIC_INFORMATION mem;

	ZeroMemory(&mem, sizeof(MEMORY_BASIC_INFORMATION));

	mem.RegionSize = 1;

	for (auto i = 0; i < 0xFFFFFFFF; i += mem.RegionSize)
	{
		if (!VirtualQuery((LPCVOID)i, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
			continue;

		char temp[256] = { 0 };

		if (!GetMappedFileName(GetCurrentProcess(), mem.BaseAddress, temp, 256))
			continue;

		std::string name(temp);

		auto found = name.find_last_of("\\");

		if (!found)
			continue;

		name = name.substr(found + 1, name.length());

		if (compare(target, name))
			return (HMODULE)mem.BaseAddress;
	}

	return nullptr;
}

FARPROC GetProcedure(HMODULE hMod, const char* name)
{
	unsigned char* lpImage;

	IMAGE_DOS_HEADER* dosHdr;
	IMAGE_NT_HEADERS* ntHdr;
	IMAGE_EXPORT_DIRECTORY* pExports;
	char* lpName, **lppNameList;

	unsigned short wOrdinal, *lpwOrdinals;

	unsigned int i;

	FARPROC* lpMethods;

	if (!(lpImage = (unsigned char*)hMod))
		return 0;

	if (!(dosHdr = (IMAGE_DOS_HEADER*)lpImage))
		return 0;

	if (!(ntHdr = (IMAGE_NT_HEADERS*)(lpImage + dosHdr->e_lfanew)))
		return 0;

	pExports = (IMAGE_EXPORT_DIRECTORY*)(lpImage + ntHdr->OptionalHeader.DataDirectory[0].VirtualAddress);
	lpwOrdinals = (unsigned short*)(lpImage + pExports->AddressOfNameOrdinals);
	lpMethods = (FARPROC*)(lpImage + pExports->AddressOfFunctions);
	lppNameList = (char**)(lpImage + pExports->AddressOfNames);

	for (i = 0; i < pExports->NumberOfNames; i++)
	{
		lpName = (char*)(lpImage + (uintptr_t)lppNameList[i]);

		if (compare(lpName, name))
			break;
	}

	if (i >= pExports->NumberOfNames)
		return 0;

	wOrdinal = lpwOrdinals[i];

	return (FARPROC)((uintptr_t)hMod + (uintptr_t)lpMethods[wOrdinal]);
}


void old_bf_write::WriteWord(int val)
{
	WriteUBitLong(val, sizeof(unsigned short) << 3);
}

#define BITS_PER_INT		32

inline int GetBitForBitnum(int bitNum)
{
	static int bitsForBitnum[] =
	{
		(1 << 0),
		(1 << 1),
		(1 << 2),
		(1 << 3),
		(1 << 4),
		(1 << 5),
		(1 << 6),
		(1 << 7),
		(1 << 8),
		(1 << 9),
		(1 << 10),
		(1 << 11),
		(1 << 12),
		(1 << 13),
		(1 << 14),
		(1 << 15),
		(1 << 16),
		(1 << 17),
		(1 << 18),
		(1 << 19),
		(1 << 20),
		(1 << 21),
		(1 << 22),
		(1 << 23),
		(1 << 24),
		(1 << 25),
		(1 << 26),
		(1 << 27),
		(1 << 28),
		(1 << 29),
		(1 << 30),
		(1 << 31),
	};

	return bitsForBitnum[(bitNum) & (BITS_PER_INT - 1)];
}

inline int BitForBitnum(int bitnum)
{
	return GetBitForBitnum(bitnum);
}

// Precalculated bit masks for WriteUBitLong. Using these tables instead of 
// doing the calculations gives a 33% speedup in WriteUBitLong.
unsigned long g_BitWriteMasks[32][33];

// (1 << i) - 1
unsigned long g_ExtraMasks[32];

class CBitWriteMasksInit
{
public:
	CBitWriteMasksInit()
	{
		for (unsigned int startbit = 0; startbit < 32; startbit++)
		{
			for (unsigned int nBitsLeft = 0; nBitsLeft < 33; nBitsLeft++)
			{
				unsigned int endbit = startbit + nBitsLeft;
				g_BitWriteMasks[startbit][nBitsLeft] = BitForBitnum(startbit) - 1;
				if (endbit < 32)
					g_BitWriteMasks[startbit][nBitsLeft] |= ~(BitForBitnum(endbit) - 1);
			}
		}

		for (unsigned int maskBit = 0; maskBit < 32; maskBit++)
			g_ExtraMasks[maskBit] = BitForBitnum(maskBit) - 1;
	}
};
CBitWriteMasksInit g_BitWriteMasksInit;