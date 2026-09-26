#include "Main.h"

typedef void( __thiscall* FrameStageNotify_t )( void*, ClientFrameStage_t );
void __fastcall Hooked_FrameStageNotify( void* ecx, void* edx, ClientFrameStage_t curStage )
{
	BasePlayer* LocalPlayer = ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	QAngle *pPunchAngle;
	QAngle PunchAngle, PunchAngle_old;

	if( g_pEngineClient->IsInGame( ) )
	{
		if( curStage == FRAME_UNDEFINED ) return;
	}

	static bool once;
	
	if( LocalPlayer && LocalPlayer->m_lifeState( ) == 0 )
	{
		if( g_CVars.Miscellaneous.CheatsBypass && g_CVars.Miscellaneous.ThirdPerson )
		{
			*( float* )( ( DWORD ) LocalPlayer + 0xD14 ) = g_qThirdPerson.y; // pl + deadflag + 0x4

			if( once )
			{
				g_pEngineClient->ExecuteClientCmd( /*thirdperson*/XorStr<0x20,12,0x7A808928>("\x54\x49\x4B\x51\x40\x55\x43\x55\x5B\x46\x44"+0x7A808928).s );
				once = false;
			}
			else once = true;
		}
	}

	CreateMoveVMT->Function< FrameStageNotify_t >( 32 )( ecx, curStage );

	if( !g_pEngineClient->IsInGame( ) ) return;
	if( !LocalPlayer ) return;

	if( curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START )
	{
		for( auto Index = g_pGlobals->maxClients; Index >= 1; --Index )
		{
			BasePlayer* Entity = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( Index );
			if( Entity == 0 ) continue;
			if( Index == g_pEngineClient->GetLocalPlayer( ) ) continue;
			if( Entity->m_lifeState( ) != 0 ) continue;
			if( Entity->m_iHealth( ) <= 0 || Entity->m_iHealth( ) >= 500 ) continue;
			if( !g_CVars.Aimbot.FriendlyFire )
			{
				if( Entity->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
			}
			if( Entity->IsDormant( ) ) continue;

			// who may have their yaw overwritten this update:
			// - not whitelisted, resolver on, and either everyone or marked in the playerlist
			bool doResolve = !g_Whitelist.List( Index ) && g_CVars.Aimbot.Resolver.Active;
			if( g_CVars.Aimbot.Resolver.Mode == 1 && g_CVars.PlayerList.Yaw[ Index ] != 1 ) doResolve = false;

			// SDK-derived adaptive resolver (Type 4) + legacy types.
			// Signals are tracked even when doResolve is false so switching the
			// resolver on mid-round starts with fresh state.
			Resolver_Apply( Index, Entity, doResolve );

			// store AFTER Apply: history keeps the resolved body facing, so
			// backtrack bone setup aims with the same yaw we validated against
			if( pPlayerHistory[ Index ][ 0 ].m_SimulationTime != Entity->m_flSimulationTime( ) )
			{
				for( int tick = 31; tick > 0; tick-- ) pPlayerHistory[ Index ][ tick ] = pPlayerHistory[ Index ][ tick - 1 ];
				g_Stuff.StoreTickRecord( Entity, &pPlayerHistory[ Index ][ 0 ] );
			}
		}
	}
}