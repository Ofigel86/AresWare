// BUILD MARKER r26 (2026-09-18): Disable Enemy Interpolation now also sets cl_interp/cl_interp_ratio 0 (restore on untick).
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
#include "Main.h"

// ============================================================================
// ENEMY INTERPOLATION BYPASS (Segregation css_nosteam port)
//
// Segregation's Interpolate.hpp: C_BaseEntity::Interpolate is hooked so that
// ONLY the local player is interpolated; every other entity returns 1 (skip).
// Enemy models then sit on their EXACT last server snapshot instead of the
// lerp window, which matches what server-side lag compensation restores and
// widens the valid hit window. Pair with cl_interp 0 / cl_interp_ratio 0.
//
// SAFETY: the hook address is Segregation's VERIFIED Interpolate VA for the
// css v34 no-steam client: 604201536 = 0x24000000 + 0x36200. The old dead-code
// offset 0x4C250 from this source was WRONG for this binary family - never use it.
// OFF by default (Aimbot > Disable Enemy Interpolation). If anything breaks with
// the box ticked - untick it; the detour only installs once per session.
// ============================================================================

typedef bool ( __thiscall *Interpolate_t )( void*, float );
typedef void ( __thiscall *Interpolation_t )( void*, float, float );
typedef void ( __thiscall *SetInterpolationAmount_t )( void*, float );
typedef int ( __thiscall *BaseInterpolatePart1_t )( void*, float, Vector, QAngle, int );

Interpolate_t Interpolate;
Interpolation_t Interpolation;
BaseInterpolatePart1_t BaseInterpolatePart1;
SetInterpolationAmount_t SetInterpolationAmount;

bool __fastcall Hooked_Interpolate( void* ptr, int edx, float currentTime )
{
	// local player keeps vanilla smoothing; enemies are NOT interpolated
	BasePlayer* LocalPlayer = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	if( ptr == ( void* ) LocalPlayer )
		return Interpolate( ptr, currentTime );

	return 1; // Segregation: enemies stay on the exact server snapshot
}

void __fastcall Hooked_ClientInterpolation( void* ptr, int edx, float currentTime, float interpolation_amount )
{
	Interpolation( ptr, currentTime, -( g_pGlobals->interval_per_tick ) );
}

int __fastcall Hooked_BaseInterpolatePart1( void* thisptr, int edx, float &currentTime, Vector &oldOrigin, QAngle &oldAngles, int &bNoMoreChanges )
{
	BaseEntity *thisent = ( BaseEntity* ) thisptr;
	return 0; // unused: Segregation only needs the per-entity Interpolate hook
}

void __fastcall Hooked_SetInterpolationAmount( void* ptr, int edx, float seconds )
{
	SetInterpolationAmount( ptr, -( g_pGlobals->interval_per_tick ) );
}

void ClientInterpolation( void )
{
	// idempotent: called every CreateMove, installs once when the box is ticked
	static bool s_interpInstalled = false;
	static bool s_cvarsZeroed = false;
	static float s_oldInterp = -1.f, s_oldRatio = -1.f;

	// r26: the toggle drives the NETWORK side too - cl_interp/cl_interp_ratio 0
	// (engine-legal: sv_maxunlag 1.0s window, the SDK-verified preset). Unticking
	// restores the values that were set before.
	if( !g_CVars.Aimbot.Interpolation.DisableInterp )
	{
		if( s_cvarsZeroed )
		{
			static ConVar* rInterp = g_pCvar->FindVar( "cl_interp" );
			static ConVar* rRatio = g_pCvar->FindVar( "cl_interp_ratio" );
			if( rInterp && s_oldInterp >= 0.f ) rInterp->SetValue( s_oldInterp );
			if( rRatio && s_oldRatio >= 0.f ) rRatio->SetValue( s_oldRatio );
			s_cvarsZeroed = false;
			printconsole( "[Awesware] interp restored (cl_interp %.5f, ratio %.1f)\n", s_oldInterp, s_oldRatio );
		}
		return;
	}

	if( !s_cvarsZeroed )
	{
		static ConVar* rInterp = g_pCvar->FindVar( "cl_interp" );
		static ConVar* rRatio = g_pCvar->FindVar( "cl_interp_ratio" );
		if( rInterp && rRatio )
		{
			s_oldInterp = rInterp->GetFloat( );
			s_oldRatio = rRatio->GetFloat( );
			if( s_oldRatio > 0.f && rRatio->GetInt( ) != 0 ) rRatio->SetValue( 0 );
			if( rInterp->GetFloat( ) > 0.f ) rInterp->SetValue( 0.f );
			s_cvarsZeroed = true;
			printconsole( "[Awesware] interp OFF: cl_interp 0 + cl_interp_ratio 0 (was %.5f / %.1f)\n", s_oldInterp, s_oldRatio );
		}
	}

	if( s_interpInstalled ) return;

	// only the per-entity Interpolate hook - same minimal set as Segregation
	Interpolate = ( Interpolate_t ) g_Detour.DetourFunction( ( ( DWORD ) BASE_CLIENT + 0x36200 ), ( PBYTE ) Hooked_Interpolate ); // Segregation-verified VA 604201536
	//Interpolation = ( Interpolation_t ) g_Detour.DetourFunction( ( ( DWORD ) BASE_CLIENT + 0x39D40 ), ( PBYTE ) Hooked_ClientInterpolation );
	//BaseInterpolatePart1 = ( BaseInterpolatePart1_t ) g_Detour.DetourFunction( ( ( DWORD ) BASE_CLIENT + 0x47170 ), ( PBYTE ) Hooked_BaseInterpolatePart1 );
	//SetInterpolationAmount = ( SetInterpolationAmount_t ) g_Detour.DetourFunction( ( ( DWORD ) BASE_CLIENT + 0x34670 ), ( PBYTE ) Hooked_SetInterpolationAmount );

	if( Interpolate )
	{
		s_interpInstalled = true;
		printconsole( "[Awesware] enemy interpolation bypass installed (enemies on server snapshots)\n" );
	}
}
