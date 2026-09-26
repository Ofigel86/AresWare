#include "Main.h"

typedef void( __cdecl* FX_FireBullets_t )( int, const Vector&, const QAngle&, int, int, int, float );

FX_FireBullets_t FX_FireBulletsServer;
FX_FireBullets_t FX_FireBulletsClient;

//One CDetour instance per hooked function: the old code ran both detours
//through the single g_Detour object, so the second call overwrote the first
//one's saved bytes and only the last detour could ever be restored.
static CDetour Detour_FX_FireBulletsServer;
static CDetour Detour_FX_FireBulletsClient;

void __cdecl Hooked_FX_FireBulletsServer( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, int iWeaponID, int iMode, int iSeed, float flSpread )
{
	FX_FireBulletsServer( iPlayerIndex, vOrigin, vAngles, iWeaponID, iMode, iSeed, flSpread );
}

void __cdecl Hooked_FX_FireBulletsClient( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, int iWeaponID, int iMode, int iSeed, float flSpread )
{
	FX_FireBulletsClient( iPlayerIndex, vOrigin, vAngles, iWeaponID, iMode, iSeed, flSpread );
}

void FX_FireBullets( void )
{
	DWORD dwServerTarget = ( DWORD ) BASE_SERVER + 0x2FD320;
	DWORD dwClientTarget = ( DWORD ) BASE_CLIENT + 0x1D5060;

	if( !g_dwBaseServerDll )
	{
		// main-menu injection: server.dll appears only in-game
		Logger::Write( "FX_FireBullets server detour SKIPPED: server.dll not loaded" );
	}
	else if( !AddressInModule( dwServerTarget, ( DWORD ) BASE_SERVER ) )
	{
		Logger::Write( "FX_FireBullets server detour SKIPPED: target 0x%08X not in server.dll code", dwServerTarget );
	}
	else
	{
		FX_FireBulletsServer = ( FX_FireBullets_t ) Detour_FX_FireBulletsServer.DetourFunction( dwServerTarget, ( PBYTE ) Hooked_FX_FireBulletsServer );
		Logger::Write( "FX_FireBullets[server] detoured @ 0x%08X -> trampoline 0x%08X", dwServerTarget, ( DWORD ) FX_FireBulletsServer );
	}

	if( !AddressInModule( dwClientTarget, ( DWORD ) BASE_CLIENT ) )
	{
		Logger::Write( "FX_FireBullets client detour SKIPPED: target 0x%08X not in client.dll code", dwClientTarget );
	}
	else
	{
		FX_FireBulletsClient = ( FX_FireBullets_t ) Detour_FX_FireBulletsClient.DetourFunction( dwClientTarget, ( PBYTE ) Hooked_FX_FireBulletsClient );
		Logger::Write( "FX_FireBullets[client] detoured @ 0x%08X -> trampoline 0x%08X", dwClientTarget, ( DWORD ) FX_FireBulletsClient );
	}
}

void UnFX_FireBullets( void )
{
	Detour_FX_FireBulletsServer.RetourFunction( );
	Detour_FX_FireBulletsClient.RetourFunction( );
	FX_FireBulletsServer = NULL;
	FX_FireBulletsClient = NULL;
}
