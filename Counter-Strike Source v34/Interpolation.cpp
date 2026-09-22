#include "Main.h"

typedef bool ( __thiscall *Interpolate_t )( void*, float );
typedef void ( __thiscall *Interpolation_t )( void*, float, float );
typedef void ( __thiscall *SetInterpolationAmount_t )( void*, float );
typedef int ( __thiscall *BaseInterpolatePart1_t )( void*, float, Vector, QAngle, int );

Interpolate_t Interpolate = nullptr;
Interpolation_t Interpolation = nullptr;
BaseInterpolatePart1_t BaseInterpolatePart1 = nullptr;
SetInterpolationAmount_t SetInterpolationAmount = nullptr;

// Safe hooks - no __try to avoid C2712, use null checks only
bool __fastcall Hooked_Interpolate( void* ptr, int edx, float currentTime )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !Interpolate || !ptr )
	{
		if( Interpolate && ptr )
			return Interpolate( ptr, currentTime );
		return true;
	}
	// When disabled, still call original to keep ESP working
	if( Interpolate )
		return Interpolate( ptr, currentTime );
	return true;
}

void __fastcall Hooked_ClientInterpolation( void* ptr, int edx, float currentTime, float interpolation_amount )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !Interpolation )
	{
		if( Interpolation && ptr )
			Interpolation( ptr, currentTime, interpolation_amount );
		return;
	}
	if( g_pGlobals && g_pGlobals->interval_per_tick > 0 && ptr )
		Interpolation( ptr, currentTime, -( g_pGlobals->interval_per_tick ) );
}

int __fastcall Hooked_BaseInterpolatePart1( void* thisptr, int edx, float &currentTime, Vector &oldOrigin, QAngle &oldAngles, int &bNoMoreChanges )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !BaseInterpolatePart1 || !thisptr )
	{
		if( BaseInterpolatePart1 && thisptr )
			return BaseInterpolatePart1( thisptr, currentTime, oldOrigin, oldAngles, bNoMoreChanges );
		return 0;
	}
	return 0;
}

void __fastcall Hooked_SetInterpolationAmount( void* ptr, int edx, float seconds )
{
	if( !g_CVars.Aimbot.Interpolation.Disable || !SetInterpolationAmount )
	{
		if( SetInterpolationAmount && ptr )
			SetInterpolationAmount( ptr, seconds );
		return;
	}
	if( g_pGlobals && g_pGlobals->interval_per_tick > 0 && ptr )
		SetInterpolationAmount( ptr, -( g_pGlobals->interval_per_tick ) );
}

void ClientInterpolation( void )
{
	if( !BASE_CLIENT )
		return;
	if( (DWORD)BASE_CLIENT < 0x10000 )
		return;

	DWORD base = (DWORD)BASE_CLIENT;

	// Only hook if within reasonable range
	if( base + 0x4C250 < base + 0x1000000 )
		Interpolate = ( Interpolate_t ) g_Detour.DetourFunction( base + 0x4C250, Hooked_Interpolate );
	if( base + 0x39D40 < base + 0x1000000 )
		Interpolation = ( Interpolation_t ) g_Detour.DetourFunction( base + 0x39D40, Hooked_ClientInterpolation );	
	if( base + 0x47170 < base + 0x1000000 )
		BaseInterpolatePart1 = ( BaseInterpolatePart1_t ) g_Detour.DetourFunction( base + 0x47170, Hooked_BaseInterpolatePart1 );
	if( base + 0x34670 < base + 0x1000000 )
		SetInterpolationAmount = ( SetInterpolationAmount_t ) g_Detour.DetourFunction( base + 0x34670, Hooked_SetInterpolationAmount );
}
