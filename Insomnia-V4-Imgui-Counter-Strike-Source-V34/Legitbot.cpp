// BUILD MARKER r31 (2026-09-18): reverted r30 Humanize+ per user request - legit anti-snap back to r29 state. Awaiting user-supplied anti-detect material.
// BUILD MARKER r29 (2026-09-18): legit max - dual FOV (near/far by distance) + visible-only target lock + anti-snap on target switch + dual FOV circles.
// BUILD MARKER r14 (2026-09-18): counter-strafe AutoStop (rage) + sticky target bias + idle micro-sway (legit).
#include "Main.h"
#include "SDK/checksum_md5.h" // MD5_PseudoRandom decl (defined once via Stuff.cpp)

// ============================================================================
// LEGITBOT - own settings (g_CVars.Legit), shared aim engine.
//
// Legit_Begin swaps the legit-owned fields over the rage base right before
// the aim/weapon block in CreateMove, Legit_End restores everything after.
// Anything the legit tab does not own (resolver, multipoint, autowall, ...)
// is forced to legit-safe values, the rest inherits the rage config.
// Stage 2 (later): native legit smoothing/RCS here, engine stays untouched.
// ============================================================================

static CVars::AimbotSettings s_savedRage;
static bool s_legitActive = false;

void Legit_Begin( void )
{
	if( s_legitActive ) return;
	if( g_CVars.Aimbot.Active ) return; // rage wins when both are on
	if( !g_CVars.Legit.Active ) return;

	s_savedRage = g_CVars.Aimbot;

	// legit-owned fields:
	g_CVars.Aimbot.Active = true;
	g_CVars.Aimbot.AutoShoot = g_CVars.Legit.AutoShoot;
	g_CVars.Aimbot.Silent = g_CVars.Legit.Silent;
	g_CVars.Aimbot.SnapLimiter = g_CVars.Legit.SnapLimiter;
	g_CVars.Aimbot.AngleLimit = g_CVars.Legit.AngleLimit;
	g_CVars.Aimbot.AngleLimitTens = g_CVars.Legit.AngleLimitTens;
	g_CVars.Aimbot.Key = g_CVars.Legit.Key;
	g_CVars.Aimbot.AimFOV = g_CVars.Legit.AimFOV;
	g_CVars.Aimbot.Hitbox = g_CVars.Legit.Hitbox;
	g_CVars.Aimbot.AutoStop = g_CVars.Legit.AutoStop;
	g_CVars.Aimbot.BacktrackTicks = g_CVars.Legit.BacktrackTicks;
	g_CVars.Aimbot.TargetSelection = g_CVars.Legit.TargetSelection;
	for( int hg = 0; hg < 7; hg++ ) g_CVars.Aimbot.HitboxGroup[ hg ] = g_CVars.Legit.HitboxGroup[ hg ];

	// legit-safe forces (no rage-only behavior in legit mode):
	g_CVars.Aimbot.MultiSpot = false;
	g_CVars.Aimbot.HitScan = false;
	g_CVars.Aimbot.AutoWall = false;
	g_CVars.Aimbot.HitChance = false;
	g_CVars.Aimbot.PerfectSilent = false;
	g_CVars.Aimbot.Resolver.Active = false;
	g_CVars.Aimbot.StrictPrimary = false;
	g_CVars.Aimbot.BestDamage = false;
	g_CVars.Aimbot.Interpolation.LagPrediction = 1; // server-correct shot ticks (CorrectTickCount)

	s_legitActive = true;
}

void Legit_End( void )
{
	if( !s_legitActive ) return;
	g_CVars.Aimbot = s_savedRage;
	s_legitActive = false;
}

bool Legit_IsActive( void )
{
	return s_legitActive;
}

bool checkkey( )
{
	// completely not ghett0
	switch( g_CVars.Triggerbot.Key )
	{
		case 0: return true; break;
		case 1:
		{
			if( GetAsyncKeyState( VK_LBUTTON ) ) return true;
			break;
		}
		case 2:
		{
			if( GetAsyncKeyState( VK_RBUTTON ) ) return true;
			break;
		}
		case 3:
		{
			if( GetAsyncKeyState( VK_MBUTTON ) ) return true;
			break;
		}
		case 4:
		{
			if( GetAsyncKeyState( VK_XBUTTON1 ) ) return true;
			break;
		}
		case 5:
		{
			if( GetAsyncKeyState( VK_XBUTTON2 ) ) return true;
			break;
		}
	}
	
	return false;
}

// zoom state for scoped check / autoscope (AWP): 1 scoped, 0 unscoped, -1 unknown (netvar missing -> fail-open)
static int Legit_ZoomState( BasePlayer* LocalPlayer )
{
	static int fovOff = 0, defOff = 0;
	static bool tried = false;
	if( !tried )
	{
		tried = true;
		fovOff = HackInterfaces::NetvarManager( ).GetOffset( "DT_BasePlayer", "m_iFOV" );
		defOff = HackInterfaces::NetvarManager( ).GetOffset( "DT_BasePlayer", "m_iDefaultFOV" );
	}
	if( fovOff <= 0 || defOff <= 0 || !LocalPlayer ) return -1;
	int fov = *( int* )( ( DWORD )LocalPlayer + fovOff );
	int def = *( int* )( ( DWORD )LocalPlayer + defOff );
	if( def <= 0 ) def = 90;
	if( fov <= 0 ) return -1;
	return ( fov < def ) ? 1 : 0;
}

void Legit_Trigger( CUserCmd* pCmd, BasePlayer* LocalPlayer, CSWeapon* Weapon )
{
	if( !Weapon || !Weapon->IsWeapon( ) ) return;
	g_CVars.Triggerbot.IsShooting = false;

	// scoped check / autoscope (AWP): unscoped trigger shots are random
	if( Weapon->GetWeaponID( ) == 17 && Legit_ZoomState( LocalPlayer ) == 0 )
	{
		if( g_CVars.Legit.AutoScope ) pCmd->buttons |= IN_ATTACK2;
		if( g_CVars.Legit.ScopedCheck ) return;
	}

    Vector vEyePosition = LocalPlayer->EyePosition( );
	QAngle qCurAngle = pCmd->viewangles;

	Vector vForward,vRight,vUp;
	AngleVectors( qCurAngle, &vForward, &vRight, &vUp );

	int iMyTeam = LocalPlayer->m_iTeamNum( );

	for( auto i = g_pGlobals->maxClients; i >= 1; --i )
	{
		if( i == g_pEngineClient->GetLocalPlayer( ) ) continue;
		BasePlayer* Ent = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( i );
		if( Ent == 0 ) continue;
		if( Ent->IsDormant( ) ) continue;
		if( Ent->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
		if( !( *( int* )( ( DWORD ) Ent + 0x87 ) == 0 ) ) continue;
		if( g_CVars.PlayerList.Friend[ i ] ) continue;

		PVOID pCollisionProperty = Ent->CollisionProperty( );

		PFLOAT pfvecMaxsZ = ( PFLOAT )( ( DWORD ) pCollisionProperty + 0x1C );
		PFLOAT vecSpecifiedSurroundingMaxsZ = ( PFLOAT )( ( DWORD ) pCollisionProperty + 0x40 );

		Vector vMins, vMaxs;
		Ent->GetRenderBounds( vMins, vMaxs );

		if( *pfvecMaxsZ == vMaxs.z && *vecSpecifiedSurroundingMaxsZ == vMaxs.z ) return;

		*pfvecMaxsZ = vMaxs.z;
		*vecSpecifiedSurroundingMaxsZ = vMaxs.z;

		Vector vecSize;
		VectorSubtract( vMaxs, vMins, vecSize );
		float fNewRadius = vecSize.Length( ) * 0.5f;

		*( PFLOAT )( ( DWORD ) pCollisionProperty + 0x42 ) = fNewRadius;

		Ent->AddEFlags( 0x4000 );
	}

	if( checkkey( ) )
	{
		// trigger delay: hold the crosshair on target for N ms before firing (human reaction)
		int delayTicks = ( g_CVars.Triggerbot.Delay * 66 ) / 1000;
		if( delayTicks > 0 )
		{
			static int s_trigSince = 0;
			if( s_trigSince == 0 ) s_trigSince = g_iGameTicks;
			if( !g_Stuff.CanHit( vEyePosition, qCurAngle, vForward, vRight, vUp, 8192.0f, iMyTeam, Weapon, LocalPlayer, pCmd->random_seed ) )
			{
				s_trigSince = g_iGameTicks; // off target: restart the reaction timer
				return;
			}
			if( ( g_iGameTicks - s_trigSince ) < delayTicks ) return; // still "reacting"
		}
		if( !g_CVars.Triggerbot.Seed )
		{
			if( g_Stuff.CanHit( vEyePosition, qCurAngle, vForward, vRight, vUp, 8192.0f, iMyTeam, Weapon, LocalPlayer, pCmd->random_seed ) )
			{
				g_CVars.Triggerbot.IsShooting = true;
				pCmd->buttons |= IN_ATTACK;
			}			
		}
		else
		{
			int iHitSeed = 1337;
			int iStrength = 256;

			if( g_CVars.Triggerbot.Strength == 0 ) iStrength = 32;
			else if( g_CVars.Triggerbot.Strength == 1 ) iStrength = 64;
			else if( g_CVars.Triggerbot.Strength == 2 ) iStrength = 128;
			else if( g_CVars.Triggerbot.Strength == 3 ) iStrength = 256;		

			for( int iSeed = 0; iSeed < iStrength; iSeed++ )
			{
				if( g_Stuff.CanHit( vEyePosition, qCurAngle, vForward, vRight, vUp, 8192.0f, iMyTeam, Weapon, LocalPlayer, iSeed ) )
				{
					iHitSeed = iSeed;
					break;
				}
			}

			if( iHitSeed != 1337 )
			{
				int iCurrentCommand = pCmd->command_number;
				int iSeedGuard = 0;
				for( ; ; iCurrentCommand++ )
				{
					if( ++iSeedGuard > 1024 ) break; // safety: never hang the tick
					int iRandomSeed = ( MD5_PseudoRandom( iCurrentCommand ) & 0x7fffffff ) & 255;
					if( iRandomSeed == iHitSeed )
					{
						pCmd->command_number = iCurrentCommand;
						g_CVars.Triggerbot.IsShooting = true;
						pCmd->buttons |= IN_ATTACK;
						break;
					}
				}
			}
		}
	}
}


// ============================================================================
// SMOOTH AIM (AimType 1/2) - native legit aim: gradual human-like movement,
// punch compensation, reaction time, aim lock, desync-aware bone choice.
// AimType 0 (Snap) keeps using the shared rage engine via Legit_Begin swap.
// ============================================================================

static int s_lockTarget = -1;
static int s_lastTarget = -1;
static int s_seenTick[ 65 ] = { 0 };
static float s_feetYaw[ 65 ] = { 0.f };
static bool s_feetInit[ 65 ] = { false };
static QAngle s_oldPunch = QAngle( 0, 0, 0 );
static int s_switchTick = -100000; // r29: when the lock last changed target
static int s_killTick = -100000;

void Legit_OnKill( void )
{
	s_killTick = g_iGameTicks;
}

// hitbox -> aim bone (raw bone indices, see Bones_t in Aimbot.h)
static int Legit_HitboxBone( int hb )
{
	if( hb == 12 ) return 14; // head
	if( hb == 11 ) return 13; // neck
	if( hb == 10 ) return 11; // chest (spine3)
	if( hb == 9 ) return 9; // stomach (spine)
	if( hb == 0 ) return 0; // pelvis
	if( hb >= 1 && hb <= 8 ) return 1; // legs (left thigh)
	return 16; // arms 13-18 (left upper arm)
}

// same hb -> group map as rage: 0 Head, 1 Neck, 2 Chest, 3 Stomach, 4 Pelvis, 5 Arms, 6 Legs
static int Legit_GroupMode( int hb )
{
	static const int hbGroup[ 19 ] = { 4, 6, 6, 6, 6, 6, 6, 6, 6, 3, 2, 1, 0, 5, 5, 5, 5, 5, 5 };
	if( hb < 0 || hb > 18 ) return 0;
	return g_CVars.Legit.HitboxGroup[ hbGroup[ hb ] ]; // 0 Off, 1 Scan, 2 Priority
}

static bool Legit_CheckKey( int key )
{
	if( key <= 0 ) return true; // auto
	static const int vk[ 6 ] = { 0, VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
	if( key > 5 ) return true;
	return ( GetAsyncKeyState( vk[ key ] ) & 0x8000 ) != 0;
}

void Legit_StandaloneRCS( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	QAngle punch = LocalPlayer->GetPunchAngle( );
	if( !g_CVars.Legit.RCSStandalone || !( pCmd->buttons & IN_ATTACK ) ) { s_oldPunch = punch; return; }
	float scale = ( float )g_CVars.Legit.RCS * 0.02f; // 100% = classic x2.0
	pCmd->viewangles.x -= ( punch.x - s_oldPunch.x ) * scale;
	pCmd->viewangles.y -= ( punch.y - s_oldPunch.y ) * scale;
	if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
	if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
	pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
	s_oldPunch = punch;
	if( !g_CVars.Legit.Silent ) g_pEngineClient->SetViewAngles( pCmd->viewangles );
}

void Legit_SmoothAim( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	g_Aimbot.IsAimbotting = false;
	if( !Legit_CheckKey( g_CVars.Legit.Key ) ) { s_lockTarget = -1; s_oldPunch = LocalPlayer->GetPunchAngle( ); return; }

	// flash check: frozen by flash = no aim (NoFlash zeroes the netvar, so this passes on clear screen)
	if( g_CVars.Legit.FlashCheck )
	{
		static int flashOff = 0;
		if( !flashOff ) flashOff = HackInterfaces::NetvarManager( ).GetOffset( "DT_CSPlayer", "m_flFlashDuration" );
		if( flashOff > 0 && *( float* )( ( DWORD )LocalPlayer + flashOff ) > 0.5f ) { s_lockTarget = -1; return; }
	}

	// scoped check / autoscope (AWP): never aim an unscoped shot (unknown zoom = allow)
	CSWeapon* ScopeWpn = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( ScopeWpn && ScopeWpn->GetWeaponID( ) == 17 && Legit_ZoomState( LocalPlayer ) == 0 )
	{
		if( g_CVars.Legit.AutoScope ) pCmd->buttons |= IN_ATTACK2;
		if( g_CVars.Legit.ScopedCheck ) { s_lockTarget = -1; s_oldPunch = LocalPlayer->GetPunchAngle( ); return; }
	}

	// kill delay: pause aim for a human moment after a kill
	int killTicks = ( g_CVars.Legit.KillDelayMs * 66 ) / 1000;
	if( killTicks > 0 && ( g_iGameTicks - s_killTick ) < killTicks ) { Legit_StandaloneRCS( pCmd, LocalPlayer ); return; }

	Vector eye = LocalPlayer->EyePosition( );

	// r29 DUAL FOV: wide cone for close targets (fast reacquire), tight for far
	// (precision) - the cone is picked per-candidate from its distance.
	int fovNear = g_CVars.Legit.FovNear, fovFar = g_CVars.Legit.FovFar;
	int fovSwitch = g_CVars.Legit.FovSwitchDist;
	if( fovNear < 1 ) fovNear = 1; if( fovNear > 30 ) fovNear = 30;
	if( fovFar < 1 ) fovFar = 1; if( fovFar > 30 ) fovFar = 30;
	if( fovSwitch < 0 ) fovSwitch = 0; if( fovSwitch > 3000 ) fovSwitch = 3000;
	Vector myOrigin = LocalPlayer->GetAbsOrigin( );

	// validate aim lock
	int target = -1;
	if( g_CVars.Legit.AimLock && s_lockTarget >= 1 && s_lockTarget <= 64 )
	{
		BasePlayer* e = ( BasePlayer* )g_pClientEntityList->GetClientEntity( s_lockTarget );
		if( e && !e->IsDormant( ) && e->m_lifeState( ) == 0 && e->m_iTeamNum( ) != LocalPlayer->m_iTeamNum( ) )
			target = s_lockTarget;
		else
			s_lockTarget = -1;
	}

	// select new target (same modes as rage: 0 dist, 1 hp, 2/3 next-random, 4 crosshair)
	if( target == -1 )
	{
		int bestRate = INT_MAX;
		QAngle viewAng = LocalPlayer->m_angEyeAngles( );
		for( int i = g_pGlobals->maxClients; i >= 1; --i )
		{
			if( i == g_pEngineClient->GetLocalPlayer( ) ) continue;
			BasePlayer* e = ( BasePlayer* )g_pClientEntityList->GetClientEntity( i );
			if( !e ) continue;
			if( e->IsDormant( ) ) continue;
			if( e->m_lifeState( ) != 0 ) continue;
			if( !g_CVars.Aimbot.FriendlyFire && e->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
			if( e->m_iHealth( ) > 500 ) continue;
			if( e->IsSpawnProtectedPlayer( ) ) continue;
			if( g_CVars.PlayerList.Friend[ i ] ) continue;
			int fov = ( myOrigin.DistTo( e->GetAbsOrigin( ) ) < ( float )fovSwitch ) ? fovNear : fovFar; // r29 dual cone
			Vector toEnt = e->EyePosition( ) - eye;
			QAngle angTo; VectorAngles( toEnt, angTo );
			float fdx = g_Stuff.GuwopNormalize( angTo.x - pCmd->viewangles.x ); if( fdx < 0.f ) fdx = -fdx;
			float fdy = g_Stuff.GuwopNormalize( angTo.y - pCmd->viewangles.y ); if( fdy < 0.f ) fdy = -fdy;
			if( ( fdx + fdy ) > ( float )fov ) continue;
			// r29: never lock an enemy we cannot see (legit = no wallhack aim)
			if( !g_Aimbot.CheckVisible( eye, e->EyePosition( ), e, LocalPlayer ) ) continue;
			int rate = 0;
			int sel = g_CVars.Legit.TargetSelection;
			if( sel == 0 ) rate = ( int )LocalPlayer->GetAbsOrigin( ).DistTo( e->GetAbsOrigin( ) );
			else if( sel == 1 ) rate = e->m_iHealth( );
			else if( sel == 2 || sel == 3 ) rate = rand( ) % 1000;
			else rate = ( int )( ( fdx + fdy ) * 100.f );
			// r14 sticky-target bias: the previous target keeps a 20% rating bonus,
			// so aim only switches when the new enemy is CLEARLY better - no more
			// tick-to-tag flipping between two equal enemies in the cone.
			if( i == s_lastTarget && s_lastTarget >= 1 && s_lastTarget <= 64 ) rate = rate * 80 / 100;
			if( rate < bestRate ) { bestRate = rate; target = i; }
		}
		if( target != -1 ) s_lockTarget = target;
	}

	if( target == -1 ) { Legit_StandaloneRCS( pCmd, LocalPlayer ); return; }
	BasePlayer* Ent = ( BasePlayer* )g_pClientEntityList->GetClientEntity( target );
	if( !Ent ) { s_lockTarget = -1; return; }

	// r29: the selected target's cone size (dual FOV by its distance)
	int selFov = ( myOrigin.DistTo( Ent->GetAbsOrigin( ) ) < ( float )fovSwitch ) ? fovNear : fovFar;

	// reaction time: hold fire on a fresh target for N ms
	if( target != s_lastTarget ) { s_seenTick[ target ] = g_iGameTicks; s_switchTick = g_iGameTicks; s_lastTarget = target; } // r29: remember the switch moment
	int reactTicks = ( g_CVars.Legit.ReactionMs * 66 ) / 1000;
	if( reactTicks > 0 && ( g_iGameTicks - s_seenTick[ target ] ) < reactTicks ) return;

	// --- smooth backtrack: newest valid record at-or-before wanted depth ---
	int btRec = 0;
	bool useBT = false;
	int wantRec = g_CVars.Legit.BacktrackTicks;
	if( wantRec < 0 ) wantRec = 0; if( wantRec > 31 ) wantRec = 31;
	for( int r = wantRec; r >= 1; r-- )
	{
		float st = pPlayerHistory[ target ][ r ].m_SimulationTime;
		if( st == 0.f ) continue;
		int rtick = TIME_TO_TICKS( st );
		if( rtick > pCmd->tick_count ) continue;
		if( ( pCmd->tick_count - rtick ) > 24 ) continue; // older than ~360ms: skip
		btRec = r; useBT = true; break;
	}
	Vector btShift( 0, 0, 0 );
	if( useBT )
	{
		btShift = Ent->GetAbsOrigin( ) - pPlayerHistory[ target ][ btRec ].m_Origin;
		if( btShift.Length( ) > 300.f ) { useBT = false; btRec = 0; btShift.Init( 0, 0, 0 ); } // teleport: ignore
	}

	// --- legit desync resolver: server feet don't lie, desynced eyes do ---
	// bones are real positions, so the legit counter is center-mass (chest),
	// not head: head hitbox shifts with fake angles, chest barely moves.
	float feet = Ent->m_angRotation( ).y;
	Vector feetVel = Ent->GetVelocity( );
	float feetSpeedSq = feetVel.x * feetVel.x + feetVel.y * feetVel.y;
	if( !s_feetInit[ target ] )
	{
		s_feetYaw[ target ] = feet;
		s_feetInit[ target ] = true;
	}
	else
	{
		float dFeet = g_Stuff.GuwopNormalize( feet - s_feetYaw[ target ] ); if( dFeet < 0.f ) dFeet = -dFeet;
		if( dFeet > 40.f )
		{
			// server snapped the feet (45-degree stand-step or move transition):
			// jump the baseline instantly instead of lagging a full turn behind
			s_feetYaw[ target ] = feet;
		}
		else
		{
			// adaptive smoothing: feet converge instantly while moving (fast track),
			// standing steps arrive quantized (smooth track)
			float rate = ( feetSpeedSq > 4096.f ) ? 0.65f : 0.25f;
			s_feetYaw[ target ] = g_Stuff.GuwopNormalize( s_feetYaw[ target ] + g_Stuff.GuwopNormalize( feet - s_feetYaw[ target ] ) * rate );
		}
	}
	float eyeYaw = Ent->m_angEyeAngles( ).y;
	float desyncD = g_Stuff.GuwopNormalize( eyeYaw - s_feetYaw[ target ] ); if( desyncD < 0.f ) desyncD = -desyncD;
	bool desync = desyncD > 35.f;
	bool forceChest = ( g_CVars.Legit.DesyncResolver && desync );

	// pick bone: primary -> priority groups -> scan groups, first visible in FOV wins
	Vector bonePos; bool haveBone = false;
	if( forceChest )
	{
		if( g_Aimbot.GetBone( 11, Ent, bonePos ) && g_Aimbot.CheckVisible( eye, bonePos, Ent, LocalPlayer ) )
			haveBone = true;
	}
	else
	{
		int primary = g_CVars.Legit.Hitbox;
		if( primary < 0 || primary > 18 ) primary = 12;
		for( int pass = 0; pass < 3 && !haveBone; pass++ ) // 0 primary, 1 priority, 2 scan
		{
			for( int hb = 0; hb < 19 && !haveBone; hb++ )
			{
				if( pass == 0 && hb != primary ) continue;
				if( pass == 1 && ( hb == primary || Legit_GroupMode( hb ) != 2 ) ) continue;
				if( pass == 2 && ( hb == primary || Legit_GroupMode( hb ) != 1 ) ) continue;
				Vector pos;
				if( !g_Aimbot.GetBone( Legit_HitboxBone( hb ), Ent, pos ) ) continue;
				if( !g_Aimbot.CheckVisible( eye, pos, Ent, LocalPlayer ) ) continue;
				Vector bd = pos - eye;
				QAngle ba; VectorAngles( bd, ba );
				float bx = g_Stuff.GuwopNormalize( ba.x - pCmd->viewangles.x ); if( bx < 0.f ) bx = -bx;
				float by = g_Stuff.GuwopNormalize( ba.y - pCmd->viewangles.y ); if( by < 0.f ) by = -by;
				if( ( bx + by ) > ( float )selFov * 1.5f ) continue; // bones get a slightly wider cone
				bonePos = pos; haveBone = true;
			}
		}
	}
	if( !haveBone )
	{
		// locked target lost all bones (died behind wall etc): drop lock, reselect next tick
		if( s_lockTarget == target ) s_lockTarget = -1;
		Legit_StandaloneRCS( pCmd, LocalPlayer );
		return;
	}

	// aim with punch compensation
	if( useBT ) bonePos -= btShift; // aim where the server saw them (tick rewound by CorrectTickCount)
	else if( g_CVars.Legit.Prediction )
	{
		// velocity lead: compensate latency + 1 tick so real-time aim lands on movers
		float lead = g_pGlobals->interval_per_tick;
		INetChannelInfo* nci = g_pEngineClient->GetNetChannelInfo( );
		if( nci ) lead += nci->GetLatency( 0 );
		Vector ev = Ent->m_vecVelocity( );
		Vector origBt = bonePos;
		bonePos.x += ev.x * lead; bonePos.y += ev.y * lead; // no Z lead (jumps are unpredictable)
		if( !g_Aimbot.CheckVisible( eye, bonePos, Ent, LocalPlayer ) ) bonePos = origBt;
	}

	Vector delta = bonePos - eye;
	QAngle angTo; VectorAngles( delta, angTo );
	QAngle punch = LocalPlayer->GetPunchAngle( );
	float scale = ( float )g_CVars.Legit.RCS * 0.02f;
	angTo.x -= ( punch.x - s_oldPunch.x ) * scale;
	angTo.y -= ( punch.y - s_oldPunch.y ) * scale;
	s_oldPunch = punch;

	QAngle cur = pCmd->viewangles;
	float dx = g_Stuff.GuwopNormalize( angTo.x - cur.x );
	float dy = g_Stuff.GuwopNormalize( angTo.y - cur.y );
	float len = sqrt( dx * dx + dy * dy );
	int smooth = g_CVars.Legit.Smoothing;
	if( smooth < 1 ) smooth = 1; if( smooth > 30 ) smooth = 30;
	float div = ( float )smooth;
	if( g_CVars.Legit.AimType == 2 ) // adaptive: fast when far, slow when close (most human)
	{
		div = len * 0.35f + 1.2f;
		if( div < 1.2f ) div = 1.2f;
		if( div > ( float )smooth ) div = ( float )smooth;
	}
	div *= 0.92f + ( float )( rand( ) % 17 ) / 100.f; // humanize: +-8% smoothing jitter
	div *= ( ( g_iGameTicks - s_switchTick ) < 8 ) ? 2.f : 1.f; // r29 anti-snap: glide to a NEW target, no bot-flick
	float stepX = dx / div, stepY = dy / div;
	if( stepX > 12.f ) stepX = 12.f; if( stepX < -12.f ) stepX = -12.f; // never snap in smooth modes
	if( stepY > 12.f ) stepY = 12.f; if( stepY < -12.f ) stepY = -12.f;
	pCmd->viewangles.x = cur.x + stepX;
	if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
	if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
	pCmd->viewangles.y = g_Stuff.GuwopNormalize( cur.y + stepY );

	// r14 idle micro-sway: once settled (<2 deg), the crosshair breathes with a
	// slow dual-frequency drift instead of freezing on the bone - looks human.
	if( len < 2.f )
	{
		float swayY = sin( ( float )g_iGameTicks * 0.55f ) * 0.10f;
		float swayX = sin( ( float )g_iGameTicks * 0.41f + 1.3f ) * 0.07f;
		pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y + swayY );
		pCmd->viewangles.x += swayX;
		if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
		if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;
	}

	if( !g_CVars.Legit.Silent ) g_pEngineClient->SetViewAngles( pCmd->viewangles );

	g_Aimbot.IsAimbotting = true;
	g_Aimbot.TargetIndex = target;
	g_Aimbot.BacktrackRecord[ target ] = btRec; // rewound tick (0 = real-time)
	if( g_CVars.Legit.AutoShoot && len < 1.0f ) pCmd->buttons |= IN_ATTACK; // settled on target
}

// ============================================================================
// LEGIT STRAFE - gentle air-strafe assist: alternating side-move with small
// yaw steps. Hold SPACE like the rage strafe. Small steps = looks human.
// ============================================================================
void Legit_Strafe( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	if( !( GetAsyncKeyState( VK_SPACE ) & 0x8000 ) ) return;
	if( LocalPlayer->m_fFlags( ) & FL_ONGROUND ) return;
	Vector vel = LocalPlayer->m_vecVelocity( );
	float speed = sqrt( vel.x * vel.x + vel.y * vel.y );
	if( speed < 80.f ) return;
	int power = g_CVars.Legit.StrafePower;
	if( power < 1 ) power = 1; if( power > 10 ) power = 10;
	float turn = ( float )power * 0.25f;
	bool side = ( g_iGameTicks % 2 ) == 0;
	pCmd->sidemove = side ? 450.f : -450.f;
	pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y + ( side ? -turn : turn ) );
}

// ============================================================================
// LEGIT AUTOPISTOL - re-taps attack so semi-auto pistols keep firing while held
// ============================================================================
void Legit_AutoPistol( CUserCmd* pCmd, CSWeapon* Weapon )
{
	if( !g_CVars.Legit.AutoPistol ) return;
	if( !Weapon || !Weapon->IsPistol( ) ) return;
	if( !( pCmd->buttons & IN_ATTACK ) ) return;
	if( ( g_iGameTicks % 2 ) == 0 ) pCmd->buttons &= ~IN_ATTACK; // pistols fire on click edge
}
