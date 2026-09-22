#include "Main.h"
#include <limits>

DWORD dwReturnAddress = NULL;
DWORD dwCreateMove = 0x24087270;
static bool bSendPacket;
int sequence_number = 0;

float _clamp( float val, float minVal, float maxVal )
{
	if ( maxVal < minVal )
		return maxVal;
	else if( val < minVal )
		return minVal;
	else if( val > maxVal )
		return maxVal;
	else
		return val;
}

static bool pass = false;
static int queue = 0;
static bool angelfix = false;
static bool edgetwitch = false;
static bool edgetwitchfake = false;

// Static yaw styles are world locked by default. "Relative Yaw" keeps them attached
// to the view direction instead: the body may only lean +-90 degrees from the eye yaw
// (m_flMaxBodyYawDegrees in base_playeranimstate.cpp), so a fixed world angle is
// trivially told apart from a real one.
static inline float AntiAimStatic( float flBase, float flWorld )
{
	return ( g_CVars.Miscellaneous.AntiAim.RelativeYaw ) ? flBase + flWorld : flWorld;
}

void AntiAimPitch( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	// Улучшенные старые Pitch - 9 режимов, но максимально эффективные
	// В CSS pitch 89 вниз прячет голову лучше всего, lisp не работает (коммент в оригинале)
	switch( g_CVars.Miscellaneous.AntiAim.Pitch )
	{
		case 0: break; // Off
		case 1: pCmd->viewangles.x = 89.f; break; // Normal - улучшено: было 180, стало 89 вниз (прячет голову, лучший для CSS)
		case 2: pCmd->viewangles.x = -89.f; break; // Inverse Normal - было -180, стало -89 вверх (тоже прячет)
		case 3: pCmd->viewangles.x = 70.f; break; // Safe - оставляем 70 (безопасный от untrusted)
		case 4: pCmd->viewangles.x = ( bSendPacket ) ? 89.f : 0.f; break; // FakeDown - было -179.99, улучшено: real 0 / fake 89 (десинк)
		case 5: pCmd->viewangles.x = 89.f; break; // Down - было 697049 lisp (не работает в CSS), стало 89 вниз рабочий
		case 6: pCmd->viewangles.x = -89.f; break; // Up - было 696871 lisp (не работает), стало -89 вверх
		case 7: pCmd->viewangles.x = ( bSendPacket ) ? 89.f : 0.f; break; // Lag Down - было fake lisp, улучшено: real 0 fake 89 + 14 тиков чока
		case 8: pCmd->viewangles.x = ( bSendPacket ) ? 0.f : 89.f; break; // Lag Up - было fake lisp up, улучшено: real 89 fake 0
	}
}

void AntiAimYaw( CUserCmd* pCmd, BasePlayer* LocalPlayer, bool fake, bool half )
{
	Vector Velocity = LocalPlayer->m_vecVelocity( );
	const float flBase = pCmd->viewangles.y;
	float velYaw = 0.f;
	if( Velocity.Length2D() > 1.f )
		velYaw = RAD2DEG( atan2f( Velocity.y, Velocity.x ) );

	if( fake )
	{
		switch( g_CVars.Miscellaneous.AntiAim.Yaw )
		{
			case 0: // Forwards - улучшено: теперь с десинком
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: break; // чистый forwards
					case 1: pCmd->viewangles.y += ( half ) ? 90.f : 1.f; break; // fake side 1
					case 2: pCmd->viewangles.y += ( half ) ? -90.f : -1.f; break; // fake side 2
					case 3: pCmd->viewangles.y += 180.f; break; // random backwards
				}
				break;
			}
			case 1: // Backwards - улучшено: сильный десинк
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y = AntiAimStatic( flBase, 180.f ); break;
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 90.f : 1.f ); break;
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? -90.f : 359.f ); break;
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, 0.f ); break; // fake forwards
				}
				break;
			}
			case 2: // Sideways - улучшено: 90/-90 с вариациями
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y = AntiAimStatic( flBase, 90.f ); break;
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 180.f : 91.f ); break;
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 0.f : 89.f ); break;
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, -90.f ); break;
				}
				break;
			}
			case 3: // Jitter - улучшено: быстрый джиттер real/fake
			{
				static bool twitchfake = false;
				twitchfake = !twitchfake;
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y = AntiAimStatic( flBase, twitchfake ? 90.f : -90.f ); break; // 90/-90 jitter
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, twitchfake ? 0.f : 180.f ); break; // 0/180 jitter
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, twitchfake ? 45.f : -45.f ); break; // small jitter
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, twitchfake ? 135.f : -135.f ); break; // wide jitter
				}
				break;
			}
			case 4: // Static - улучшено: world locked с десинком
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y = AntiAimStatic( flBase, 180.f ); break;
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 90.f : 1.f ); break;
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 270.f : 359.f ); break;
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, 360.f ); break;
				}
				break;
			}
			case 5: // Static Reversed
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y = AntiAimStatic( flBase, 0.f ); break;
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 270.f : 181.f ); break;
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, ( half ) ? 90.f : 179.f ); break;
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, 180.f ); break;
				}
				break;
			}
			case 6: // Lisp - в CSS lisp не работает, улучшено: заменено на spin (реально работает)
			{
				static bool twitchfake2 = false;
				twitchfake2 = !twitchfake2;
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: // медленный спин вместо lisp
					{
						float spin = fmodf( g_pGlobals->curtime * 90.f, 360.f );
						pCmd->viewangles.y = AntiAimStatic( flBase, spin );
						break;
					}
					case 1: // быстрый спин
					{
						float spin = fmodf( g_pGlobals->curtime * 360.f, 360.f );
						pCmd->viewangles.y = AntiAimStatic( flBase, spin );
						break;
					}
					case 2: pCmd->viewangles.y = AntiAimStatic( flBase, twitchfake2 ? 90.f : -90.f ); break;
					case 3: // рандом спин
					{
						float spin = RandomFloat( 0.f, 360.f );
						pCmd->viewangles.y = AntiAimStatic( flBase, spin );
						break;
					}
				}
				break;
			}
			case 7: // Custom - улучшено: fake value с десинком
			{
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y += g_CVars.Miscellaneous.AntiAim.FakeValue; break;
					case 1: pCmd->viewangles.y = AntiAimStatic( flBase, g_CVars.Miscellaneous.AntiAim.FakeValue ); break;
				}
				break;
			}
		}
	}
	else // real
	{
		switch( g_CVars.Miscellaneous.AntiAim.Yaw )
		{
			case 0: break; // Forwards
			case 1: pCmd->viewangles.y += 180.f; break; // Backwards
			case 2: pCmd->viewangles.y += 90.f; break; // Sideways - 90
			case 3: // Jitter - улучшено
			{
				static bool twitch = false;
				twitch = !twitch;
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0: pCmd->viewangles.y += twitch ? 90.f : -90.f; break;
					case 1: pCmd->viewangles.y += twitch ? 0.f : 180.f; break;
					case 2: pCmd->viewangles.y += twitch ? 45.f : -45.f; break;
					case 3: pCmd->viewangles.y += twitch ? 135.f : -135.f; break;
				}
				break;
			}
			case 4: pCmd->viewangles.y = AntiAimStatic( flBase, 180.f ); break; // Static
			case 5: pCmd->viewangles.y = AntiAimStatic( flBase, 0.f ); break; // Static Reversed
			case 6: // Lisp заменен на spin (реально работает в CSS)
			{
				static bool twitch = false;
				twitch = !twitch;
				switch( g_CVars.Miscellaneous.AntiAim.Variation )
				{
					case 0:
					{
						float spin = fmodf( g_pGlobals->curtime * 90.f, 360.f );
						pCmd->viewangles.y = AntiAimStatic( flBase, spin );
						break;
					}
					case 1:
					{
						float spin = fmodf( g_pGlobals->curtime * 360.f, 360.f );
						pCmd->viewangles.y = AntiAimStatic( flBase, spin );
						break;
					}
					case 2: pCmd->viewangles.y += twitch ? 90.f : -90.f; break;
					case 3: pCmd->viewangles.y = AntiAimStatic( flBase, RandomFloat( 0.f, 360.f ) ); break;
				}
				break;
			}
			case 7: pCmd->viewangles.y += g_CVars.Miscellaneous.AntiAim.RealValue; break; // Custom
		}
	}
}


 // Choke decision, split out of AntiAim(): Anti-SMAC zeroes the angles and skips
// AntiAim() entirely, which used to silently disable fake lag along with it.
void FakeLag_Update( BasePlayer* LocalPlayer, CUserCmd* pCmd, int LagValue )
{
	Vector Velocity = LocalPlayer->m_vecVelocity( );

	bool ShouldChoke = false;

	bool inair = !( LocalPlayer->m_fFlags( ) & FL_ONGROUND );

	int tmpLagticks;
	if( g_CVars.Miscellaneous.Fakelag.AirOnly )
	{
		tmpLagticks = ( inair ) ? LagValue : 1;
	}
	else tmpLagticks = LagValue;

	// adaptive fake lag has to shrink the target BEFORE the delta is taken, otherwise
	// the reduced tick count gets computed and then thrown away
	if( g_CVars.Miscellaneous.Fakelag.Active && g_CVars.Miscellaneous.Fakelag.Mode == 2 ) // thx polak
	{
		float Velocity2D = Velocity.Length2D( ) * g_pGlobals->interval_per_tick;

		// keep the accumulated shift inside the 68 unit window. the old loop stepped
		// -2 -1 +1 +2 +5 and broke right after overshooting, so it always ended up at
		// 16+ ticks and the hard queue cap did all the work.
		while( tmpLagticks > 1 && ( tmpLagticks * Velocity2D ) > 68.f ) --tmpLagticks;
	}

	// creds to machete for giving me this brilliant idea lol
	int DeltaTicks = _clamp( abs( queue - tmpLagticks ), 0, 15 );

	if( g_CVars.Miscellaneous.Fakelag.Active )
	{
		if( g_CVars.Miscellaneous.Fakelag.Mode == 0 || g_CVars.Miscellaneous.Fakelag.Mode == 2 )
		{
			if( DeltaTicks > 0 ) ShouldChoke = true;
		}
		else if( g_CVars.Miscellaneous.Fakelag.Mode == 1 )
		{
			if( ( pCmd->command_number % 30 ) < 15 )
			{
				if( DeltaTicks > 0 ) ShouldChoke = true;
			}
		}
	}
	else
	{
		if( g_CVars.Miscellaneous.AntiAim.Active )
		{
			// improved AA fakelag: choke max ticks for best desync
			// when AA active without fakelag, we still want strong desync
			// choke 14 ticks, send 1 - gives max fake/real delta
			if( queue < 14 ) ShouldChoke = true;
			else ShouldChoke = false;
		}
	}

	if( pass ) ShouldChoke = false;

	bSendPacket = ( ShouldChoke ) ? false : true;

	if( !bSendPacket )
	{
		if( queue >= 14 )
		{
			bSendPacket = true;
			queue = 0;
		}
		else ++queue;
	}
	else queue = 0;

	pass = false;
}

void AntiAim( BasePlayer* LocalPlayer, CUserCmd* pCmd )
{
	if( !g_CVars.Miscellaneous.AntiAim.Active ) return;

	int MoveType = LocalPlayer->m_MoveType( );
	Vector Velocity = LocalPlayer->m_vecVelocity( );

	bool WallDTC = false;
	bool ret = true;

	{
		for( int i = g_pGlobals->maxClients; i >= 1; i-- )
		{
			if( i == g_pEngineClient->GetLocalPlayer( ) ) continue;			
			BasePlayer* Ent = ( BasePlayer* )g_pClientEntityList->GetClientEntity( i );
			if( !Ent ) continue;
			if( Ent->IsDormant( ) ) continue;
			if( Ent->m_lifeState( ) != 0 ) continue;
			if( Ent->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;

			ret = false;
		}

		if( g_CVars.Miscellaneous.AntiAim.TurnOff )
		{
			if( ret ) return;
		}

		if( ( MoveType == Valve::MoveType_t::MOVETYPE_LADDER ) && ( pCmd->buttons & IN_DUCK ) )
		{
			if( !bSendPacket ) pCmd->buttons &= ~IN_DUCK;
		}

		if( g_CVars.Miscellaneous.AntiAim.AtTargets ) g_Stuff.AntiAim.AtTargets( LocalPlayer, pCmd );

		if( g_CVars.Miscellaneous.AntiAim.WallDetection && Velocity.Length( ) < 300.f )
		{
			if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 0 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, 0.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 1 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( bSendPacket ) ? 0.f : 180.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 2 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( bSendPacket ) ? 180.f : 0.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 3 )
			{
				if( bSendPacket )
				{
					edgetwitch = !edgetwitch;
					WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( edgetwitch ) ? 0.f : 180.f );
				}
				else
				{
					edgetwitchfake = !edgetwitchfake; // mirrored edgetwitch and was never read, so the fake side just repeated the real side's edge
					WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( edgetwitchfake ) ? 0.f : 180.f );
				}
			}
		}

		if( MoveType != Valve::MoveType_t::MOVETYPE_LADDER )
		{
			if( WallDTC ) pCmd->viewangles.x = 89.f;
			else
			{
				AntiAimPitch( pCmd, LocalPlayer );
				if( bSendPacket ) AntiAimYaw( pCmd, LocalPlayer, false, false );
				else
				{
					AntiAimYaw( pCmd, LocalPlayer, true, true );
					if( g_CVars.Miscellaneous.AntiAim.DuckInAir && LocalPlayer->GetVelocity( ).z > 0 ) pCmd->buttons |= IN_DUCK;
				}
			}
		}
	}
}

void sendcmd( const char* input, ... )
{
	va_list va_alist;
	char buf[ 256 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	g_pEngineClient->ExecuteClientCmd( buf );
}

void ForceFullUpdate( BasePlayer* Ent )
{
	typedef void( __thiscall* ForceFullUpdate_t )( void* );
	( ( ForceFullUpdate_t )( ( DWORD ) BASE_ENGINE + 0x9E0D0 ) )( Ent );
}

void CorrectTickCount( CUserCmd* pCmd )
{
	if( !g_CVars.Aimbot.Interpolation.LagPrediction ) return;

	static ConVar* cvar_cl_interp = g_pCvar->FindVar( /*cl_interp*/XorStr<0x24,10,0x7F9B18B0>("\x47\x49\x79\x4E\x46\x5D\x4F\x59\x5C"+0x7F9B18B0).s );
	static ConVar* cvar_cl_updaterate = g_pCvar->FindVar( /*cl_updaterate*/XorStr<0xC6,14,0xD9FFF99F>("\xA5\xAB\x97\xBC\xBA\xAF\xAD\xB9\xAB\xBD\xB1\xA5\xB7"+0xD9FFF99F).s );
	static ConVar* cvar_cl_interp_ratio = g_pCvar->FindVar( /*cl_interp_ratio*/XorStr<0xBF,16,0x298E884B>("\xDC\xAC\x9E\xAB\xAD\xB0\xA0\xB4\xB7\x97\xBB\xAB\xBF\xA5\xA2"+0x298E884B).s );
	static ConVar* cvar_sv_minupdaterate = g_pCvar->FindVar( /*sv_minupdaterate*/XorStr<0xF6,17,0x99ECD573>("\x85\x81\xA7\x94\x93\x95\x89\x8D\x9A\x9E\x74\x64\x70\x62\x70\x60"+0x99ECD573).s );
	static ConVar* cvar_sv_maxupdaterate = g_pCvar->FindVar( /*sv_maxupdaterate*/XorStr<0x6A,17,0x6C62999F>("\x19\x1D\x33\x00\x0F\x17\x05\x01\x16\x12\x00\x10\x04\x16\x0C\x1C"+0x6C62999F).s );
	static ConVar* cvar_sv_client_min_interp_ratio = g_pCvar->FindVar( /*sv_client_min_interp_ratio*/XorStr<0x9B,27,0x783554E3>("\xE8\xEA\xC2\xFD\xF3\xC9\xC4\xCC\xD7\xFB\xC8\xCF\xC9\xF7\xC0\xC4\xDF\xC9\xDF\xDE\xF0\xC2\xD0\xC6\xDA\xDB"+0x783554E3).s );
	static ConVar* cvar_sv_client_max_interp_ratio = g_pCvar->FindVar( /*sv_client_max_interp_ratio*/XorStr<0xAF,27,0xED44D950>("\xDC\xC6\xEE\xD1\xDF\xDD\xD0\xD8\xC3\xE7\xD4\xDB\xC3\xE3\xD4\xD0\xCB\xA5\xB3\xB2\x9C\xB6\xA4\xB2\xAE\xA7"+0xED44D950).s );

	float cl_interp = cvar_cl_interp->GetFloat( );
	int cl_updaterate = cvar_cl_updaterate->GetInt( ),
	sv_maxupdaterate = cvar_sv_maxupdaterate->GetInt( ),
	sv_minupdaterate = cvar_sv_minupdaterate->GetInt( ),
	cl_interp_ratio = cvar_cl_interp_ratio->GetInt( ),
	sv_client_min_interp_ratio = cvar_sv_client_min_interp_ratio->GetInt( ),
	sv_client_max_interp_ratio = cvar_sv_client_max_interp_ratio->GetInt( );

	if( sv_client_min_interp_ratio > cl_interp_ratio ) cl_interp_ratio = sv_client_min_interp_ratio;
	if( cl_interp_ratio > sv_client_max_interp_ratio ) cl_interp_ratio = sv_client_max_interp_ratio;
	if( sv_maxupdaterate <= cl_updaterate ) cl_updaterate = sv_maxupdaterate;
	if( sv_minupdaterate > cl_updaterate ) cl_updaterate = sv_minupdaterate;

	float interp = cl_interp_ratio / cl_updaterate;
	if( interp > cl_interp ) cl_interp = interp;

	BasePlayer* LocalPlayer = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );

	int tick;

	if( g_Aimbot.TargetIndex != -1 )
	{
		tick = TIME_TO_TICKS( pPlayerHistory[ g_Aimbot.TargetIndex ][ 0 ].m_SimulationTime );
		bool timeout = ( tick < ( pCmd->tick_count - 50 ) );
		if( !timeout ) pCmd->tick_count = tick;
	}
}

typedef void( __thiscall* CreateMove_t )( void*, int, float, bool );
void __fastcall CreateMove( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	CreateMoveVMT->Function< CreateMove_t >( 18 )( edx, sequence_number, input_sample_frametime, active );

	BasePlayer* LocalPlayer = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !LocalPlayer ) return;

	if( !g_pInput ) return;

	bSendPacket = true;
	CUserCmd* pCmd = g_pInput->GetUserCmd( sequence_number );

	g_TickCount = pCmd->tick_count;

	if( g_GUI.ShouldDisableInput( ) )
	{
		pCmd->buttons &= ~IN_ATTACK;
		pCmd->buttons &= ~IN_ATTACK2;
		pCmd->buttons &= ~IN_FORWARD;
		pCmd->buttons &= ~IN_BACK;
		pCmd->buttons &= ~IN_MOVELEFT;
		pCmd->buttons &= ~IN_MOVERIGHT;
	}

	g_Stuff.sidemove_old = pCmd->sidemove;
	g_Stuff.forwardmove_old = pCmd->forwardmove;
	g_Stuff.radarangles = pCmd->viewangles;
	g_Stuff.viewangles_old = pCmd->viewangles;
	g_Stuff.ForceCVars( );

	g_iGameTicks++;

	if( g_CVars.MovementRecorder.Active ) // todo: fix
	{
		if( GetAsyncKeyState( VK_F6 ) ) MovementRecorder.State = RECORDING; // record
		if( GetAsyncKeyState( VK_F7 ) ) // save
		{
			g_Macro.CurrentName = /*demo_1*/XorStr<0x15,7,0xEF9CCFF8>("\x71\x73\x7A\x77\x46\x2B"+0xEF9CCFF8).s;
			g_Macro.Save = true;
			g_Macro.Load = false;
		}
		if( GetAsyncKeyState( VK_F8 ) ) // load
		{
			g_Macro.CurrentName = /*demo_1*/XorStr<0x3E,7,0x37290FEA>("\x5A\x5A\x2D\x2E\x1D\x72"+0x37290FEA).s;
			g_Macro.Load = true;
			g_Macro.Save = false;
		}
		if( GetAsyncKeyState( VK_F9 ) ) MovementRecorder.State = PLAYING; // play
		if( GetAsyncKeyState( VK_F10 ) ) MovementRecorder.State = NOTHING; // stop playing/recording
		if( GetAsyncKeyState( VK_F11 ) ) MovementRecorder.State = STARTPOS; // find startposition

		MovementRecorder.RecordMovement( pCmd, LocalPlayer, pCmd->viewangles );
	}

	if( LocalPlayer->m_lifeState( ) != 0 ) return;

	// switch between legit / rage aimbot profiles on a key tap
	if( g_CVars.AimbotProfileKey > 0 )
	{
		if( GetAsyncKeyState( g_CVars.AimbotProfileKey ) & 1 )
			g_Stuff.SwitchAimbotProfile( ( g_CVars.AimbotProfile == 0 ) ? 1 : 0 );
	}

	if( g_CVars.Miscellaneous.BunnyHop ) g_Stuff.BunnyHop( pCmd, LocalPlayer );
	//if( g_CVars.Miscellaneous.EdgeJump ) g_Stuff.EdgeJump( pCmd, LocalPlayer );

	g_Prediction.Start( pCmd, LocalPlayer );
	EyePosition = LocalPlayer->EyePosition( );

	if( g_CVars.Miscellaneous.AutoKnife ) g_Stuff.Knifebot.Main( pCmd, LocalPlayer, Weapon );

	if( Weapon && Weapon->IsWeapon( ) )
	{
		if( g_CVars.Triggerbot.Active ) g_Stuff.SeedTrigger( pCmd, LocalPlayer, Weapon );
		if( g_CVars.Aimbot.Active )
		{
			QAngle tmp = pCmd->viewangles;
			g_Aimbot.Main( pCmd, LocalPlayer );

			if( g_CVars.Aimbot.SnapLimiter )
			{
				float delta_x, delta_y, limit;
				delta_x = g_Stuff.GuwopNormalize( pCmd->viewangles.x - tmp.x );
				delta_y = g_Stuff.GuwopNormalize( pCmd->viewangles.y - tmp.y );

				if( g_CVars.Aimbot.AngleLimit >= 180 ) limit = 180;
				else limit = float( g_CVars.Aimbot.AngleLimit ) + g_CVars.Aimbot.AngleLimitTens; // eks dee

				if( !( ( delta_x < limit && delta_x > -limit ) && ( delta_y < limit && delta_y > -limit ) ) ) pCmd->viewangles = tmp;
			}
		}

		if( pCmd->buttons & IN_ATTACK )
		{
			if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )	
			{
				if( g_Aimbot.TargetIndex != -1 ) g_iBulletsFired[ g_Aimbot.TargetIndex ]++;
				angelfix = false;
				pass = true;
				queue = 0;

				// improved trigger logic: only skip ForceSeed/NoSpread when triggerbot did its own seed search
				bool bTriggerSeedActive = ( g_CVars.Triggerbot.Active && g_CVars.Triggerbot.IsShooting && g_CVars.Triggerbot.Seed );
				if( !bTriggerSeedActive )
				{
					if( g_CVars.Accuracy.ForceSeed ) g_Stuff.ForceSeed( pCmd );
					if( g_CVars.Accuracy.PerfectAccuracy )
					{
						switch( g_CVars.Accuracy.NoSpreadMode )
						{
							case 1: g_NoSpread.Main( pCmd, pCmd->viewangles, LocalPlayer, Weapon, g_CVars.Miscellaneous.AntiAim.Static ); break;
							case 2: g_NoSpread.Iterative( pCmd, pCmd->viewangles, LocalPlayer, Weapon, g_CVars.Miscellaneous.AntiAim.Static ); break;
							case 3: g_NoSpread.CoolNospreee( pCmd, pCmd->viewangles, LocalPlayer, Weapon, g_CVars.Miscellaneous.AntiAim.Static ); break;
							default: g_NoSpread.Main( pCmd, pCmd->viewangles, LocalPlayer, Weapon, g_CVars.Miscellaneous.AntiAim.Static ); break;
						}
						g_Stuff.NoRecoil( pCmd, LocalPlayer, g_CVars.Miscellaneous.AntiAim.Static );
					}
				}
				else
				{
					// triggerbot with seed already forced command_number to hit seed
					// still apply NoRecoil for accuracy
					if( g_CVars.Accuracy.PerfectAccuracy )
						g_Stuff.NoRecoil( pCmd, LocalPlayer, g_CVars.Miscellaneous.AntiAim.Static );
				}

				if( g_CVars.Aimbot.PerfectSilent ) bSendPacket = false;

				CorrectTickCount( pCmd );
			}
			else
			{
				FakeLag_Update( LocalPlayer, pCmd, ( g_CVars.Miscellaneous.Fakelag.InAttack ) ? g_CVars.Miscellaneous.Fakelag.Value : 1 );

				// improved AntiSMAC - moved to Miscellaneous, works for all
				bool bAntiSMAC = g_CVars.Miscellaneous.AntiSMAC || g_CVars.Aimbot.AntiSMAC;
				int iMode = g_CVars.Miscellaneous.AntiSMACMode;
				if( bAntiSMAC )
				{
					// mode 0 = clamp only, keep AntiAim but clamped
					// mode 1 = clamp + hide AntiAim (no AA)
					// mode 2 = full - clamp + no snap + hide AA
					if( iMode == 0 )
					{
						AntiAim( LocalPlayer, pCmd );
						// clamp to valid SMAC range
						if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
						if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
						pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
						pCmd->viewangles.z = 0.f;
					}
					else
					{
						// hide anti-aim, keep legit angles
						// clamp current angles instead of zeroing
						if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
						if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
						pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
						pCmd->viewangles.z = 0.f;
					}
				}
				else
				{
					AntiAim( LocalPlayer, pCmd );
				}

				pCmd->buttons &= ~IN_ATTACK;
			}
		}
		else
		{
			FakeLag_Update( LocalPlayer, pCmd, g_CVars.Miscellaneous.Fakelag.Value );

			bool bAntiSMAC = g_CVars.Miscellaneous.AntiSMAC || g_CVars.Aimbot.AntiSMAC;
			int iMode = g_CVars.Miscellaneous.AntiSMACMode;
			if( bAntiSMAC )
			{
				if( iMode == 0 )
				{
					AntiAim( LocalPlayer, pCmd );
					if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
					if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
					pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
					pCmd->viewangles.z = 0.f;
				}
				else
				{
					if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
					if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
					pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
					pCmd->viewangles.z = 0.f;
				}
			}
			else
			{
				AntiAim( LocalPlayer, pCmd );
			}
		}
	}

	// global AntiSMAC clamp - always ensure angles are SMAC-safe if enabled (works for legit + rage)
	// improved for legit bot testing
	if( g_CVars.Miscellaneous.AntiSMAC || g_CVars.Aimbot.AntiSMAC )
	{
		// SMAC checks: pitch [-89,89], yaw [-180,180] normalized, roll 0, no NaN/Inf, no uninitialized
		if( pCmd->viewangles.x != pCmd->viewangles.x || pCmd->viewangles.y != pCmd->viewangles.y || pCmd->viewangles.z != pCmd->viewangles.z ||
			fabs( pCmd->viewangles.x ) > 360.f || fabs( pCmd->viewangles.y ) > 360.f || fabs( pCmd->viewangles.z ) > 360.f )
		{
			pCmd->viewangles = QAngle( 0, 0, 0 );
		}
		if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
		if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
		pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
		pCmd->viewangles.z = 0.f;

		bool bIsLegit = ( g_CVars.AimbotProfile == 0 );

		// full mode: also limit snap speed to avoid SMAC eye test - improved for legit
		if( g_CVars.Miscellaneous.AntiSMACMode == 2 || bIsLegit )
		{
			QAngle old = g_Stuff.viewangles_old;
			float dx = g_Stuff.GuwopNormalize( pCmd->viewangles.x - old.x );
			float dy = g_Stuff.GuwopNormalize( pCmd->viewangles.y - old.y );
			// legit = 10 deg max snap (very humanized), rage = 35 deg
			float maxSnap = bIsLegit ? 10.f : 35.f;
			// if legit with humanize, even stricter 5-10 deg
			if( bIsLegit && g_CVars.Aimbot.Humanize )
				maxSnap = 8.f;
			if( fabs( dx ) > maxSnap || fabs( dy ) > maxSnap )
			{
				// if snap too fast, use old angles + limited delta to stay legit
				// preserve movement via MovementFix later
				if( dx > maxSnap ) dx = maxSnap;
				if( dx < -maxSnap ) dx = -maxSnap;
				if( dy > maxSnap ) dy = maxSnap;
				if( dy < -maxSnap ) dy = -maxSnap;
				pCmd->viewangles.x = old.x + dx;
				pCmd->viewangles.y = old.y + dy;
				if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
				if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
				pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
			}
		}

		// legit extra: ensure pitch not at extremes (SMAC flags 89/-89 spam), clamp to 70 for legit
		if( bIsLegit && g_CVars.Aimbot.Active )
		{
			// for legit, don't allow 89 pitch (looks sus), clamp to 70 max
			if( pCmd->viewangles.x > 70.f ) pCmd->viewangles.x = 70.f;
			if( pCmd->viewangles.x < -70.f ) pCmd->viewangles.x = -70.f;
			// also ensure no AA when legit active (hide AA)
			// AA already hidden in mode 1/2, but force for legit
		}
	}

	if( g_CVars.Miscellaneous.AutoStrafe ) g_Stuff.AutoStrafe( pCmd, LocalPlayer );
	g_Stuff.MovementFix.FixMove( LocalPlayer, pCmd, angelfix );

	g_Prediction.End( pCmd, LocalPlayer );

	if( g_CVars.Miscellaneous.AirStuck )
	{
		// todo: fix local pos so the shots while stuck are accurate
		if( GetAsyncKeyState( 'F' ) & 1 ) g_CVars.Miscellaneous.AirStuckPress = !g_CVars.Miscellaneous.AirStuckPress;

		if( g_CVars.Miscellaneous.AirStuckPress )
		{
			if( !( pCmd->buttons & IN_ATTACK ) ) pCmd->tick_count = INT_MAX;
		}
	}
	
	if( bSendPacket ) g_qThirdPerson = pCmd->viewangles;
}

void __declspec( naked ) __fastcall Hooked_CreateMove( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	__asm
	{
		push ebp
		mov ebp, esp
		mov bSendPacket, bl			   
		movzx eax, active
		push eax
		mov eax, input_sample_frametime
		push eax
		mov eax, sequence_number
		push eax
		call CreateMove			   
		mov bl, bSendPacket			   
		mov esp, ebp
		pop ebp			   
		retn 0xC
	}
}