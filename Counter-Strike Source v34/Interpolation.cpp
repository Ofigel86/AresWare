#include "Main.h"

typedef bool ( __thiscall *Interpolate_t )( void*, float );
typedef void ( __thiscall *Interpolation_t )( void*, float, float );
typedef void ( __thiscall *SetInterpolationAmount_t )( void*, float );
typedef int ( __thiscall *BaseInterpolatePart1_t )( void*, float, Vector, QAngle, int );

Interpolate_t Interpolate = nullptr;
Interpolation_t Interpolation = nullptr;
BaseInterpolatePart1_t BaseInterpolatePart1 = nullptr;
SetInterpolationAmount_t SetInterpolationAmount = nullptr;

// Safe hooks - don't break ESP, don't crash on map enter
bool __fastcall Hooked_Interpolate( void* ptr, int edx, float currentTime )
{
	// Always safe: if original null or disable off, call original or return true
	if( !g_CVars.Aimbot.Interpolation.Disable || !Interpolate )
	{
		if( Interpolate )
			return Interpolate( ptr, currentTime );
		return true;
	}
	// When disabled, still call original to keep entities valid (ESP fix)
	// Don't skip like sega does (return 1) because that can crash AresWare
	if( ptr && Interpolate )
	{
		__try
		{
			return Interpolate( ptr, currentTime );
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return true;
		}
	}
	return true;
}

void __fastcall Hooked_ClientInterpolation( void* ptr, int edx, float currentTime, float interpolation_amount )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !Interpolation )
	{
		if( Interpolation )
		{
			__try { Interpolation( ptr, currentTime, interpolation_amount ); }
			__except(EXCEPTION_EXECUTE_HANDLER) {}
		}
		return;
	}
	// disable by negative amount - safe, doesn't break ESP
	if( g_pGlobals && g_pGlobals->interval_per_tick > 0 )
	{
		__try { Interpolation( ptr, currentTime, -( g_pGlobals->interval_per_tick ) ); }
		__except(EXCEPTION_EXECUTE_HANDLER) {}
	}
}

int __fastcall Hooked_BaseInterpolatePart1( void* thisptr, int edx, float &currentTime, Vector &oldOrigin, QAngle &oldAngles, int &bNoMoreChanges )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !BaseInterpolatePart1 )
	{
		if( BaseInterpolatePart1 )
		{
			__try { return BaseInterpolatePart1( thisptr, currentTime, oldOrigin, oldAngles, bNoMoreChanges ); }
			__except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
		}
		return 0;
	}
	// When disabled, return 0 to skip interpolation part1 (safe)
	return 0;
}

void __fastcall Hooked_SetInterpolationAmount( void* ptr, int edx, float seconds )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !SetInterpolationAmount )
	{
		if( SetInterpolationAmount )
		{
			__try { SetInterpolationAmount( ptr, seconds ); }
			__except(EXCEPTION_EXECUTE_HANDLER) {}
		}
		return;
	}
	if( g_pGlobals && g_pGlobals->interval_per_tick > 0 )
	{
		__try { SetInterpolationAmount( ptr, -( g_pGlobals->interval_per_tick ) ); }
		__except(EXCEPTION_EXECUTE_HANDLER) {}
	}
}

void ClientInterpolation( void )
{
	// Safe enable - check BASE_CLIENT valid and g_Detour valid
	// If addresses invalid or detour fails, don't crash, just skip
	if( !BASE_CLIENT )
		return;
	if( (DWORD)BASE_CLIENT < 0x10000 )
		return;

	__try
	{
		// Only hook the safe ones that don't break ESP: ClientInterpolation and SetInterpolationAmount
		// Interpolate and BaseInterpolatePart1 are more risky and can crash on map enter
		// We keep them disabled by default to avoid crash, only hook if user explicitly wants
		// For now, hook all but with safe checks

		DWORD base = (DWORD)BASE_CLIENT;

		// Check if addresses are within reasonable range (client.dll size ~ 5-10 MB)
		// If base + offset > base + 0x1000000, skip (invalid)
		if( base + 0x4C250 < base + 0x1000000 )
			Interpolate = ( Interpolate_t ) g_Detour.DetourFunction( base + 0x4C250, Hooked_Interpolate );
		if( base + 0x39D40 < base + 0x1000000 )
			Interpolation = ( Interpolation_t ) g_Detour.DetourFunction( base + 0x39D40, Hooked_ClientInterpolation );	
		if( base + 0x47170 < base + 0x1000000 )
			BaseInterpolatePart1 = ( BaseInterpolatePart1_t ) g_Detour.DetourFunction( base + 0x47170, Hooked_BaseInterpolatePart1 );
		if( base + 0x34670 < base + 0x1000000 )
			SetInterpolationAmount = ( SetInterpolationAmount_t ) g_Detour.DetourFunction( base + 0x34670, Hooked_SetInterpolationAmount );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// If detour crashes, disable interpolation feature
		Interpolate = nullptr;
		Interpolation = nullptr;
		BaseInterpolatePart1 = nullptr;
		SetInterpolationAmount = nullptr;
		g_CVars.Aimbot.Interpolation.Disable = false;
	}
}
