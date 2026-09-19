// BUILD MARKER r40 (2026-09-19): lag records - per-record anti-jitter resolve (phase prediction + miss brute).
// BUILD MARKER r37 (2026-09-18): rage audit fixes - resolver proxy arrays [65] (OOB at entindex 64).
// BUILD MARKER r21 (2026-09-18): Show Fake Pose toggle (pin off on demand) + AIC punished-combo ban + AI resolver recency/soft-ban.
// BUILD MARKER r20 (2026-09-18): removed Yaw 13/14/15 (Jitter Back / Random Back / Fake 0) - Yaw list back to 0-12 (Server Hold last).
// BUILD MARKER r19 (2026-09-18): Freeze Model (ModelZero) feature fully removed.
// BUILD MARKER r18 (2026-09-18): Fake 0 sends 0 deg on non-shot ticks (move-base rotated) + render pin disabled for Fake 0 (live pose visible).
// BUILD MARKER r16 (2026-09-18): Freeze Model (0 deg) - old-school local look: frozen body, free skeleton overlay.
// BUILD MARKER r11 (2026-09-18): hitchance fire gate on live-shot commit + enemy fakelag sensor (EnemyChoke) + resolver-adaptive body hitbox.
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"

typedef void( __thiscall* FrameStageNotify_t )( void*, ClientFrameStage_t );

//===============================================================================================
// Resolver: hit memory + jitter detection + shot tracking
//===============================================================================================

static void Resolver_ResetPlayer( int Index )
{
	if( Index < 1 || Index > 64 ) return;
	g_CVars.Aimbot.Resolver.Hits[ Index ] = 0;
	g_CVars.Aimbot.Resolver.Misses[ Index ] = 0;
	g_CVars.Aimbot.Resolver.Step[ Index ] = 0;
	g_CVars.Aimbot.Resolver.StepShots[ Index ] = 0;
	g_CVars.Aimbot.Resolver.ShotPending[ Index ] = false;
	g_CVars.Aimbot.Resolver.Jitter[ Index ] = false;
	g_CVars.Aimbot.Resolver.YawHistCount[ Index ] = 0;
	g_CVars.Aimbot.Resolver.YawHistPos[ Index ] = 0;
	g_CVars.Aimbot.Resolver.AnimInit[ Index ] = false;
	g_CVars.Aimbot.Resolver.AnimSpin[ Index ] = false;
	g_CVars.Aimbot.Resolver.AnimFeetAvg[ Index ] = 0.f;
	g_CVars.Aimbot.Resolver.AnimEyePrev[ Index ] = 0.f;
	g_CVars.Aimbot.Resolver.AnimFireYaw[ Index ] = 0.f;
	g_CVars.Aimbot.Resolver.AnimFireTick[ Index ] = -100000;
	g_CVars.Aimbot.Resolver.AIMem[ Index ] = false; // AI shots/hits tables persist across deaths
	g_CVars.Aimbot.Resolver.EnemyChoke[ Index ] = 0;
	g_CVars.Aimbot.Resolver.HonestYaw[ Index ] = 0.f;
	g_CVars.Aimbot.Resolver.HonestTick[ Index ] = -100000;
	// r40: lag records
	g_CVars.Aimbot.Resolver.LagSide[ Index ] = false;
	g_CVars.Aimbot.Resolver.LagPhaseBrute[ Index ] = false;
}

void Resolver_OnDeath( int Index )
{
	Resolver_ResetPlayer( Index );
}

void Resolver_OnNetYaw( int Index, float yaw )
{
	if( Index < 1 || Index > 64 ) return;
	if( !g_CVars.Aimbot.Resolver.Active ) return;

	g_CVars.Aimbot.Resolver.YawHist[ Index ][ g_CVars.Aimbot.Resolver.YawHistPos[ Index ] ] = yaw;
	g_CVars.Aimbot.Resolver.YawHistPos[ Index ] = ( g_CVars.Aimbot.Resolver.YawHistPos[ Index ] + 1 ) % 8;
	if( g_CVars.Aimbot.Resolver.YawHistCount[ Index ] < 8 ) g_CVars.Aimbot.Resolver.YawHistCount[ Index ]++;

	if( g_CVars.Aimbot.Resolver.YawHistCount[ Index ] < 6 )
	{
		g_CVars.Aimbot.Resolver.Jitter[ Index ] = false;
		return;
	}

	// Detect ABAB oscillation: evens cluster at A, odds at B (angle-wrap safe)
	float ref = yaw, sumE = 0.f, sumO = 0.f;
	for( int k = 0; k < 6; k++ )
	{
		int slot = ( g_CVars.Aimbot.Resolver.YawHistPos[ Index ] - 1 - k + 16 ) % 8;
		float d = g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.YawHist[ Index ][ slot ] - ref );
		if( k % 2 == 0 ) sumE += d; else sumO += d;
	}
	float avgE = sumE / 3.f, avgO = sumO / 3.f;
	float width = avgE - avgO; if( width < 0.f ) width = -width;

	float var = 0.f;
	for( int k = 0; k < 6; k++ )
	{
		int slot = ( g_CVars.Aimbot.Resolver.YawHistPos[ Index ] - 1 - k + 16 ) % 8;
		float d = g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.YawHist[ Index ][ slot ] - ref );
		float m = ( k % 2 == 0 ) ? avgE : avgO;
		var += ( d - m ) * ( d - m );
	}
	var /= 6.f;

	if( width > 20.f && var < 200.f )
	{
		g_CVars.Aimbot.Resolver.Jitter[ Index ] = true;
		g_CVars.Aimbot.Resolver.JitterA[ Index ] = ref + avgE;
		g_CVars.Aimbot.Resolver.JitterB[ Index ] = ref + avgO;
	}
	else g_CVars.Aimbot.Resolver.Jitter[ Index ] = false;
}

void Resolver_OnShot( int Index )
{
	if( Index < 1 || Index > 64 ) return;
	if( !g_CVars.Aimbot.Resolver.Active ) return;

	g_CVars.Aimbot.Resolver.Shots[ Index ]++;

	// remember which angle this shot used (hurt arrives later, angle may change)
	BasePlayer* Ent = ( BasePlayer* )g_pClientEntityList->GetClientEntity( Index );
	if( Ent ) g_CVars.Aimbot.Resolver.ShotYaw[ Index ] = Ent->m_angEyeAngles( ).y;

	g_CVars.Aimbot.Resolver.LastShotTick[ Index ] = g_iGameTicks;
	g_CVars.Aimbot.Resolver.ShotPending[ Index ] = true;

	// advance bruteforce step (shot-paced types)
	int type = g_CVars.Aimbot.Resolver.Type;
	if( type == 1 && g_CVars.Aimbot.Resolver.Jitter[ Index ] )
	{
		g_CVars.Aimbot.Resolver.Step[ Index ]++; // adaptive jitter: flip extreme every shot
	}
	else if( type == 3 )
	{
		// 2 bullets: 2 shots per angle, then next
		g_CVars.Aimbot.Resolver.StepShots[ Index ]++;
		if( g_CVars.Aimbot.Resolver.StepShots[ Index ] >= 2 )
		{
			g_CVars.Aimbot.Resolver.StepShots[ Index ] = 0;
			g_CVars.Aimbot.Resolver.Step[ Index ]++;
		}
	}
	else if( type == 4 )
	{
		// Anim Test: advance bruteforce step every shot
		g_CVars.Aimbot.Resolver.Step[ Index ]++;
	}
	else if( type == 6 )
	{
		g_CVars.Aimbot.Resolver.Step[ Index ]++; // Honest: next variant around the measured angle
	}
	if( type == 5 && !g_CVars.Aimbot.Resolver.AIMem[ Index ] )
		g_CVars.Aimbot.Resolver.AIShots[ Index ][ g_CVars.Aimbot.Resolver.AIChosen[ Index ] ]++;
}

void Resolver_OnHit( int Index )
{
	if( Index < 1 || Index > 64 ) return;
	if( !g_CVars.Aimbot.Resolver.Active ) return;

	g_CVars.Aimbot.Resolver.ShotPending[ Index ] = false;
	g_CVars.Aimbot.Resolver.Misses[ Index ] = 0;
	g_CVars.Aimbot.Resolver.Hits[ Index ]++;
	g_CVars.Aimbot.Resolver.HitYaw[ Index ] = g_CVars.Aimbot.Resolver.ShotYaw[ Index ];
	g_CVars.Aimbot.Resolver.HitTick[ Index ] = g_iGameTicks;
	if( g_CVars.Aimbot.Resolver.Type == 5 && !g_CVars.Aimbot.Resolver.AIMem[ Index ] )
	{
		g_CVars.Aimbot.Resolver.AIHits[ Index ][ g_CVars.Aimbot.Resolver.AIChosen[ Index ] ]++;
		g_CVars.Aimbot.Resolver.AIHitTick[ Index ][ g_CVars.Aimbot.Resolver.AIChosen[ Index ] ] = g_iGameTicks; // r21 recency
	}
}

// convert timed-out pending shots to misses (called every frame per enemy)
static void Resolver_Update( int Index )
{
	if( Index < 1 || Index > 64 ) return;
	if( !g_CVars.Aimbot.Resolver.Active ) return;

	if( g_CVars.Aimbot.Resolver.ShotPending[ Index ] &&
		( g_iGameTicks - g_CVars.Aimbot.Resolver.LastShotTick[ Index ] ) > 20 )
	{
		g_CVars.Aimbot.Resolver.ShotPending[ Index ] = false;
		g_CVars.Aimbot.Resolver.Misses[ Index ]++;
		g_CVars.Aimbot.Resolver.AIMissTick[ Index ][ g_CVars.Aimbot.Resolver.AIChosen[ Index ] ] = g_iGameTicks; // r21 soft-ban stamp
		if( g_CVars.Aimbot.Resolver.Misses[ Index ] >= 2 )
			g_CVars.Aimbot.Resolver.Hits[ Index ] = 0; // drop stale memory
		// r40: lag records - a missed jitter shot flips the predicted phase (brute)
		if( g_CVars.Aimbot.Resolver.Jitter[ Index ] )
			g_CVars.Aimbot.Resolver.LagPhaseBrute[ Index ] = !g_CVars.Aimbot.Resolver.LagPhaseBrute[ Index ];
	}
}

// use remembered working angle if fresh and no misses
static bool Resolver_TryMemory( BasePlayer* Entity, int Index )
{
	if( Index < 1 || Index > 64 || !Entity ) return false;
	if( g_CVars.Aimbot.Resolver.Hits[ Index ] <= 0 ) return false;
	if( g_CVars.Aimbot.Resolver.Misses[ Index ] != 0 ) return false;
	if( ( g_iGameTicks - g_CVars.Aimbot.Resolver.HitTick[ Index ] ) > 300 ) return false;

	Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.HitYaw[ Index ];
	g_CVars.Aimbot.Resolver.AIMem[ Index ] = true; // don't train AI on memory shots
	return true;
}

//===============================================================================================
// Anim Test resolver (Type 4): uses server animation state instead of blind bruteforce.
// Engine facts (CMultiPlayerAnimState): server feet yaw (m_angRotation) converges toward the
// transmitted eye yaw - instantly when moving, in 45-degree steps at 720 deg/s when standing.
// Gesture slots are stored in the 15 anim layers with m_nOrder == slot:
//   0 = ATTACK_AND_RELOAD, 1 = GRENADE, 2 = JUMP, 3 = SWIM, 4 = FLINCH
//===============================================================================================

// true if enemy attack/reload gesture layer is playing (weight > 0, slot 0)
static bool Resolver_AnimEnemyFiring( BasePlayer* Entity )
{
	if( !Entity ) return false;
	DWORD_PTR pLayers = *( DWORD_PTR* )( ( DWORD_PTR ) Entity + 1712 );
	if( !pLayers ) return false;
	for( int i = 0; i < 15; i++ )
	{
		DWORD_PTR Layer = pLayers + 32 * i;
		if( *( int* )( Layer + 12 ) == 0 && *( float* )( Layer + 8 ) > 0.1f )
			return true;
	}
	return false;
}

static float Resolver_AnimAbs( float v )
{
	return ( v < 0.f ) ? -v : v;
}

// Shared animation sensing for Type 4/5: updates feet average, fire capture,
// spin flag and eye history. Returns smoothed server feet yaw.
static float Resolver_AnimSense( BasePlayer* Entity, int Index, float netYaw )
{
	float feetYaw = Entity->m_angRotation( ).y; // server feet yaw (lags eyes)
	Vector vel = Entity->GetVelocity( );
	float speedSq = vel.x * vel.x + vel.y * vel.y;

	// smoothed feet baseline (exponential moving average)
	if( !g_CVars.Aimbot.Resolver.AnimInit[ Index ] )
	{
		g_CVars.Aimbot.Resolver.AnimFeetAvg[ Index ] = feetYaw;
		g_CVars.Aimbot.Resolver.AnimInit[ Index ] = true;
	}
	float feetAvg = g_CVars.Aimbot.Resolver.AnimFeetAvg[ Index ];
	feetAvg += g_Stuff.GuwopNormalize( feetYaw - feetAvg ) * 0.35f;
	g_CVars.Aimbot.Resolver.AnimFeetAvg[ Index ] = feetAvg;

	// fire-gesture capture: while shooting, transmitted eyes are (almost always) real
	if( Resolver_AnimEnemyFiring( Entity ) )
	{
		g_CVars.Aimbot.Resolver.AnimFireYaw[ Index ] = netYaw;
		g_CVars.Aimbot.Resolver.AnimFireTick[ Index ] = g_iGameTicks;
	}

	// spin detect: standing + feet lag big + eyes jump fast between frames
	bool standing = ( speedSq < 40.f * 40.f );
	float eyeMove = Resolver_AnimAbs( g_Stuff.GuwopNormalize( netYaw - g_CVars.Aimbot.Resolver.AnimEyePrev[ Index ] ) );
	float feetDelta = Resolver_AnimAbs( g_Stuff.GuwopNormalize( netYaw - feetYaw ) );
	g_CVars.Aimbot.Resolver.AnimSpin[ Index ] = ( standing && feetDelta > 45.f && eyeMove > 25.f );
	g_CVars.Aimbot.Resolver.AnimEyePrev[ Index ] = netYaw;

	return feetAvg;
}

// Type 4 entry: netYaw = transmitted eye yaw saved before resolve
static void Resolver_AnimTest( BasePlayer* Entity, int Index, float netYaw )
{
	if( Index < 1 || Index > 64 || !Entity ) return;

	float feetAvg = Resolver_AnimSense( Entity, Index, netYaw );
	Vector vel = Entity->GetVelocity( );
	float speedSq = vel.x * vel.x + vel.y * vel.y;
	bool spin = g_CVars.Aimbot.Resolver.AnimSpin[ Index ];

	// (B) fire-gesture lock: while shooting, transmitted eyes are (almost always) real
	if( ( g_iGameTicks - g_CVars.Aimbot.Resolver.AnimFireTick[ Index ] ) < 100 ) // ~1.5 s lock
	{
		Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.AnimFireYaw[ Index ];
		return;
	}

	int step = g_CVars.Aimbot.Resolver.Step[ Index ];

	// (C) move-aware base: running enemies have feet == eyes, trust networked yaw
	if( speedSq > 80.f * 80.f )
	{
		static const float moveOff[ 3 ] = { 0.f, 20.f, -20.f };
		Entity->m_angEyeAngles( ).y = netYaw + moveOff[ step % 3 ];
	}
	else if( spin )
	{
		// (A) spinning: stable feet base + small offsets
		static const float spinOff[ 5 ] = { 0.f, 15.f, -15.f, 30.f, -30.f };
		Entity->m_angEyeAngles( ).y = feetAvg + spinOff[ step % 5 ];
	}
	else
	{
		// standing, no spin: static AA bruteforce around networked yaw
		switch( step % 4 )
		{
			case 0: Entity->m_angEyeAngles( ).y = netYaw + 180.f; break;
			case 1: Entity->m_angEyeAngles( ).y = netYaw; break;
			case 2: Entity->m_angEyeAngles( ).y = netYaw + 90.f; break;
				case 3: Entity->m_angEyeAngles( ).y = netYaw + 270.f; break;
		}
	}
}

//===============================================================================================
// AI Learn resolver (Type 5): smart base + learned offset per enemy.
// Each of the 10 candidate offsets tracks its own hit-rate; the resolver plays the
// best one 88% of the time and explores the rest 12%. Tables persist across deaths
// so the AI keeps learning the enemy for the whole match.
//===============================================================================================
static const float AIResolver_Offsets[ 12 ] = { 0.f, 12.f, -12.f, 25.f, -25.f, 45.f, -45.f, 60.f, -60.f, 90.f, -90.f, 180.f };

void AIResolver_Reset( void )
{
	for( int i = 0; i < 65; i++ )
	{
		g_CVars.Aimbot.Resolver.AIChosen[ i ] = 0;
		g_CVars.Aimbot.Resolver.AIMem[ i ] = false;
		for( int c = 0; c < 12; c++ )
		{
			g_CVars.Aimbot.Resolver.AIShots[ i ][ c ] = 0;
			g_CVars.Aimbot.Resolver.AIHits[ i ][ c ] = 0;
			g_CVars.Aimbot.Resolver.AIHitTick[ i ][ c ] = -100000;
			g_CVars.Aimbot.Resolver.AIMissTick[ i ][ c ] = -100000;
		}
	}
}

// epsilon-greedy pick by Laplace-smoothed hit-rate.
// Explore rate decays as data grows (30/15/6%); a candidate that missed
// twice in a row is banned from the exploit pick until something else lands.
static int Resolver_AISelect( int Index, bool isFar )
{
	int poolN = isFar ? 5 : 12; // isFar: { 0, +-12, +-25 } only, wide offsets always miss at range
	int total = 0;
	for( int c = 0; c < poolN; c++ ) total += g_CVars.Aimbot.Resolver.AIShots[ Index ][ c ];
	int explorePct = ( total < 8 ) ? 30 : ( ( total < 25 ) ? 15 : 6 );
	if( isFar && explorePct > 6 ) explorePct = 6; // experiments are extra-punishing at range
	if( ( rand( ) % 100 ) < explorePct ) return rand( ) % poolN; // explore
	// r21: pass 0 skips candidates missed in the last 30 ticks (fresh miss = costly
	// experiment), pass 1 accepts everything if all were fresh-missed. Recently
	// confirmed candidates get a small bonus so a working resolve wins ties.
	int best = -1; float bestScore = -1.f;
	int banned = ( g_CVars.Aimbot.Resolver.Misses[ Index ] >= 2 ) ? g_CVars.Aimbot.Resolver.AIChosen[ Index ] : -1;
	for( int pass = 0; pass < 2 && best == -1; pass++ )
	for( int c = 0; c < poolN; c++ )
	{
		if( c == banned ) continue;
		if( pass == 0 && ( g_iGameTicks - g_CVars.Aimbot.Resolver.AIMissTick[ Index ][ c ] ) < 30 ) continue;
		float score = ( float )( g_CVars.Aimbot.Resolver.AIHits[ Index ][ c ] + 1 ) /
			( float )( g_CVars.Aimbot.Resolver.AIShots[ Index ][ c ] + 2 );
		if( ( g_iGameTicks - g_CVars.Aimbot.Resolver.AIHitTick[ Index ][ c ] ) < 300 ) score += 0.10f;
		if( score > bestScore ) { bestScore = score; best = c; }
	}
	return ( best != -1 ) ? best : 0;
}

// Type 5 entry: smart base + learned offset
static void Resolver_AILearn( BasePlayer* Entity, int Index, float netYaw )
{
	if( Index < 1 || Index > 64 || !Entity ) return;

	float feetAvg = Resolver_AnimSense( Entity, Index, netYaw );

	// long range: degrees cost units at 1000u+, use the most stable base/pool
	BasePlayer* pLocal = ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	float resDist = ( pLocal ) ? pLocal->m_vecOrigin( ).DistTo( Entity->m_vecOrigin( ) ) : 0.f;
	bool isFar = ( g_CVars.Aimbot.LongRangeDist > 0 && resDist > ( float )g_CVars.Aimbot.LongRangeDist );

	// smart base: fire-lock > feet (spin) > jitter center > networked
	// anti-defensive: enemy flicking between wide extremes -> eyes lie, feet don't
	float flickW = Resolver_AnimAbs( g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.JitterB[ Index ] - g_CVars.Aimbot.Resolver.JitterA[ Index ] ) );
	bool wideFlick = g_CVars.Aimbot.Resolver.Jitter[ Index ] && flickW > 100.f;
	float base = netYaw;
	if( ( g_iGameTicks - g_CVars.Aimbot.Resolver.AnimFireTick[ Index ] ) < 100 )
		base = g_CVars.Aimbot.Resolver.AnimFireYaw[ Index ];
	else if( g_CVars.Aimbot.Resolver.AnimSpin[ Index ] )
		base = feetAvg;
	else if( wideFlick )
		base = feetAvg;
	else if( !isFar && g_CVars.Aimbot.Resolver.Jitter[ Index ] )
		base = g_CVars.Aimbot.Resolver.JitterA[ Index ] +
			g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.JitterB[ Index ] - g_CVars.Aimbot.Resolver.JitterA[ Index ] ) * 0.5f;
	else if( isFar )
		base = feetAvg; // isFar + no better signal: feet yaw is the most stable

	int chosen = Resolver_AISelect( Index, isFar );
	Entity->m_angEyeAngles( ).y = base + AIResolver_Offsets[ chosen ];
	g_CVars.Aimbot.Resolver.AIChosen[ Index ] = chosen;
	g_CVars.Aimbot.Resolver.AIMem[ Index ] = false; // AI wrote the angles (not hit-memory)
}

// Type 6 entry: measured honest yaw from bullet_impact (captured in GameEventManager).
// bullet_impact gives the exact server shot yaw; impact residue is only weapon spread,
// so small +-45/90 offsets on the measured angle cover it. Stale measurement -> netYaw.
static void Resolver_Honest( BasePlayer* Entity, int Index, float netYaw )
{
	if( Index < 1 || Index > 64 || !Entity ) return;

	int age = g_iGameTicks - g_CVars.Aimbot.Resolver.HonestTick[ Index ];
	if( age < 0 || age > 128 )
	{
		Entity->m_angEyeAngles( ).y = netYaw; // no fresh measurement: trust networked yaw
		return;
	}

	static const float honestOff[ 5 ] = { 0.f, 45.f, -45.f, 90.f, -90.f };
	Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.HonestYaw[ Index ] + honestOff[ g_CVars.Aimbot.Resolver.Step[ Index ] % 5 ];
}

void __fastcall Hooked_FrameStageNotify( void* ecx, void* edx, ClientFrameStage_t curStage )
{
	LuaAPI::FrameStage( ( int )curStage ); // lua on_frame_stage callbacks

	BasePlayer* LocalPlayer = ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	QAngle *pPunchAngle;
	QAngle PunchAngle, PunchAngle_old;

	if( g_pEngineClient->IsInGame( ) )
	{
		if( curStage == FRAME_UNDEFINED ) return;
	}

	static bool tpApplied = false;
	static int tpLastDist = -1;
	if( LocalPlayer && LocalPlayer->m_lifeState( ) == 0 && g_CVars.Miscellaneous.CheatsBypass )
	{
		// NOTE: removed ancient "local+0xD14 = TP yaw" write (eye-angles lore from an old
		// build). Verified in client.dll: +0xD14 is the Y of a stored Vector (a getter/
		// setter pair copies 0xD10/0xD14/0xD18 as x/y/z) - writing yaw there corrupted
		// it every frame in TP. TP yaw is now handled by the RENDER_START pin.
		// execute the camera command only when the state changes (bind or checkbox)
		if( g_CVars.Miscellaneous.ThirdPerson != tpApplied )
		{
			tpApplied = g_CVars.Miscellaneous.ThirdPerson;
			g_pEngineClient->ExecuteClientCmd( tpApplied ? "thirdperson" : "firstperson" );
		}
		if( tpApplied )
		{
			int wantDist = g_CVars.Miscellaneous.ThirdPersonDist;
			if( wantDist < 50 ) wantDist = 50; if( wantDist > 250 ) wantDist = 250;
			if( wantDist != tpLastDist )
			{
				tpLastDist = wantDist;
				static ConVar* camDist = g_pCvar->FindVar( "cam_idealdist" );
				if( camDist ) camDist->SetValue( ( float ) wantDist );
			}
		}
	}
	else if( tpApplied ) // bypass off while in TP: get the camera back
	{
		tpApplied = false;
		g_pEngineClient->ExecuteClientCmd( "firstperson" );
	}

	// FAKE FIX: pin the local model to the last REAL angles for the whole render.
	// Client anims run on the latest cmd angles, so without this the thirdperson
	// body flickers real/fake every tick and FakeAngleViz reads a mixed pose.
	static QAngle savedLocalEye; static bool localEyeSaved = false;
	if( curStage == FRAME_RENDER_START && LocalPlayer && LocalPlayer->m_lifeState( ) == 0 && g_CVars.Miscellaneous.AntiAim.Active
		&& !g_CVars.Visuals.ESP.ShowFake ) // r21: Show Fake Pose disables the pin -> the live real/fake pose is visible
	{
		savedLocalEye = LocalPlayer->m_angEyeAngles( );
		LocalPlayer->m_angEyeAngles( ) = g_qThirdPerson;
		localEyeSaved = true;
	}

	CreateMoveVMT->Function< FrameStageNotify_t >( 32 )( ecx, curStage );

	if( curStage == FRAME_RENDER_END && localEyeSaved )
	{
		if( LocalPlayer ) LocalPlayer->m_angEyeAngles( ) = savedLocalEye;
		localEyeSaved = false;
	}

	if( !g_pEngineClient->IsInGame( ) ) return;

	static float tempYaw[ 65 ]; // r37: indexed by entindex 1..64

	if( curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START )
	{
		for( auto Index = g_pGlobals->maxClients; Index >= 1; --Index )
		{
			BasePlayer* Entity = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( Index );
			if( Entity == 0 ) continue;
			if( Index == g_pEngineClient->GetLocalPlayer( ) ) continue;
			if( Entity->m_lifeState( ) != 0 ) continue;
			if( Entity->m_iHealth( ) > 0 && Entity->m_iHealth( ) < 500 );

			// r11: enemy fakelag sensor (fills the previously-dead EnemyChoke field):
			// simulation time frozen across net updates = the enemy is choking commands.
			{
				static int lastSimTick[ 65 ] = { 0 };
				static int chokeRun[ 65 ] = { 0 };
				int simTick = TIME_TO_TICKS( Entity->m_flSimulationTime( ) );
				if( lastSimTick[ Index ] != 0 && simTick == lastSimTick[ Index ] )
				{
					if( chokeRun[ Index ] < 64 ) chokeRun[ Index ]++;
				}
				else chokeRun[ Index ] = 0;
				lastSimTick[ Index ] = simTick;
				g_CVars.Aimbot.Resolver.EnemyChoke[ Index ] = chokeRun[ Index ];
			}
			if( !g_CVars.Aimbot.FriendlyFire )
			{
				if( Entity->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
			}
			if( Entity->IsDormant( ) ) continue;

			if( !g_Whitelist.List( Index ) && g_CVars.Aimbot.Resolver.Active )
			{
				tempYaw[ Index ] = g_CVars.PlayerList.ViewAngles[ Index ].y;

				bool ret = true;
				if( g_CVars.Aimbot.Resolver.Mode == 1 && g_CVars.PlayerList.Yaw[ Index ] != 1 ) ret = false;

				Resolver_Update( Index ); // pending shot -> miss conversion
				bool memUsed = Resolver_TryMemory( Entity, Index ); // hit memory

				// adaptive jitter + hit memory integrated below

				if( ret && !memUsed )
				{
					if( g_CVars.Aimbot.Resolver.Smart )
					{
						Vector resultLocal = EyePosition;
						Vector resultentity = Entity->EyePosition( );	
						Vector m_vTraceVector = Vector( resultLocal - resultentity );
						QAngle m_vAimAngles;
						static float yawDelta[ 65 ]; // r37
						static int yawMode[ 65 ]; // r37
	
						if( !resultLocal.IsValid( ) || !resultentity.IsValid( ) || !m_vTraceVector.IsValid( ) ) continue;

						VectorAngles( m_vTraceVector, m_vAimAngles );
						m_vAimAngles.x *= -1;

						if( !m_vAimAngles.IsValid( ) ) continue;

						yawDelta[ Index ] = m_vAimAngles.y - tempYaw[ Index ];
						yawDelta[ Index ] = g_Stuff.GuwopNormalize( yawDelta[ Index ] );
						if( yawDelta[ Index ] < 0.f ) yawDelta[ Index ] += 360.f;

						if( yawDelta[ Index ] <= 20.f || yawDelta[ Index ] >= 340.f ) yawMode[ Index ] = 1;
						else if( ( yawDelta[ Index ] >= 70.f && yawDelta[ Index ] <= 110.f ) || ( yawDelta[ Index ] >= 250.f && yawDelta[ Index ] <= 290.f ) ) yawMode[ Index ] = 2;
						else if( yawDelta[ Index ] >= 160.f && yawDelta[ Index ] <= 200.f ) yawMode[ Index ] = 3;
						else yawMode[ Index ] = 0;

						if( yawMode[ Index ] == 2 ) g_CVars.Aimbot.AutoHeightMode[ Index ] = 1;
						else g_CVars.Aimbot.AutoHeightMode[ Index ] = 0;

						// Type 4/5 (Anim/AI) run regardless of pitch - they use feet/gestures, not fakelag pitch
					if( ( ( g_CVars.PlayerList.ViewAngles[ Index ].x == 89.f || g_CVars.PlayerList.ViewAngles[ Index ].x == -89.f ) && yawMode[ Index ] != 2 ) || g_CVars.Aimbot.Resolver.Type >= 4 )
						{
							if( g_CVars.Aimbot.Resolver.Type == 0 )
							{
								int lol = ( g_iGameTicks % 4 );
								switch ( lol ) 
								{
									case 0: Entity->m_angEyeAngles( ).y = 0.f; break;
									case 1: Entity->m_angEyeAngles( ).y = 90.f; break;
									case 2: Entity->m_angEyeAngles( ).y = 180.f; break;
									case 3: Entity->m_angEyeAngles( ).y = 270.f; break;
								}
							}
							else if( g_CVars.Aimbot.Resolver.Type == 1 ) // Back Twitch: adaptive jitter
							{
								if( g_CVars.Aimbot.Resolver.Jitter[ Index ] )
								{
									// enemy jitters: alternate extremes + center per shot (covers jitter+fake)
									int jitPick = g_CVars.Aimbot.Resolver.Step[ Index ] % 3;
									if( jitPick == 0 ) Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterA[ Index ];
									else if( jitPick == 1 ) Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterB[ Index ];
									else Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterA[ Index ] + g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.JitterB[ Index ] - g_CVars.Aimbot.Resolver.JitterA[ Index ] ) * 0.5f;
								}
								else
								{
									int lol = ( g_iGameTicks % 4 );
									static bool half[ 65 ]; // r37

									switch ( lol )
									{
										case 0: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 0.f : 180.f; break;
										case 1: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 45.f : 225.f; break;
										case 2: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 90.f : 270.f; break;
										case 3: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 135.f : 315.f; half[ Index ] = !half[ Index ]; break;
									}
								}
							}
							else if( g_CVars.Aimbot.Resolver.Type == 2 )
							{
								Entity->m_angEyeAngles( ).y = ( g_iGameTicks % 2 == 0 ) ? 90.f : -90.f;
								if( g_iGameTicks % 4 == 0 ) Entity->m_angEyeAngles( ).y += 0.087936f;
							}
							else if( g_CVars.Aimbot.Resolver.Type == 3 ) // 2 bullets: shot-paced bruteforce around networked yaw
							{
								switch( g_CVars.Aimbot.Resolver.Step[ Index ] % 4 )
								{
									case 0: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 180.f; break;
									case 1: Entity->m_angEyeAngles( ).y = tempYaw[ Index ]; break;
									case 2: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 90.f; break;
									case 3: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 270.f; break;
								}
							}
							else if( g_CVars.Aimbot.Resolver.Type == 4 ) // Anim Test: feet-yaw + fire-gesture + move-aware
							{
								Resolver_AnimTest( Entity, Index, tempYaw[ Index ] );
							}
							else if( g_CVars.Aimbot.Resolver.Type == 5 ) // AI Learn: smart base + learned offset
							{
								Resolver_AILearn( Entity, Index, tempYaw[ Index ] );
							}
							else if( g_CVars.Aimbot.Resolver.Type == 6 ) // Honest Shot: measured bullet_impact yaw
							{
								Resolver_Honest( Entity, Index, tempYaw[ Index ] );
							}
						}
					}
					else
					{
						g_CVars.Aimbot.AutoHeightMode[ Index ] = 0;

						if( g_CVars.Aimbot.Resolver.Type == 0 )
						{
							int lol = ( g_iGameTicks % 4 );
							switch ( lol ) 
							{
								case 0: Entity->m_angEyeAngles( ).y = 0.f; break;
								case 1: Entity->m_angEyeAngles( ).y = 90.f; break;
								case 2: Entity->m_angEyeAngles( ).y = 180.f; break;
								case 3: Entity->m_angEyeAngles( ).y = 270.f; break;
							}
						}
						else if( g_CVars.Aimbot.Resolver.Type == 1 ) // Back Twitch: adaptive jitter
						{
							if( g_CVars.Aimbot.Resolver.Jitter[ Index ] )
							{
								// enemy jitters: alternate extremes + center per shot (covers jitter+fake)
								int jitPick = g_CVars.Aimbot.Resolver.Step[ Index ] % 3;
								if( jitPick == 0 ) Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterA[ Index ];
								else if( jitPick == 1 ) Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterB[ Index ];
								else Entity->m_angEyeAngles( ).y = g_CVars.Aimbot.Resolver.JitterA[ Index ] + g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.JitterB[ Index ] - g_CVars.Aimbot.Resolver.JitterA[ Index ] ) * 0.5f;
							}
							else
							{
								int lol = ( g_iGameTicks % 4 );
								static bool half[ 65 ]; // r37

								switch ( lol )
								{
									case 0: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 0.f : 180.f; break;
									case 1: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 45.f : 225.f; break;
									case 2: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 90.f : 270.f; break;
									case 3: Entity->m_angEyeAngles( ).y = ( half[ Index ] ) ? 135.f : 315.f; half[ Index ] = !half[ Index ]; break;
								}
							}
						}
						else if( g_CVars.Aimbot.Resolver.Type == 2 )
						{
							Entity->m_angEyeAngles( ).y = ( g_iGameTicks % 2 == 0 ) ? 90.f : -90.f;
							if( g_iGameTicks % 4 == 0 ) Entity->m_angEyeAngles( ).y += 0.087936f;
						}
						else if( g_CVars.Aimbot.Resolver.Type == 3 ) // 2 bullets: shot-paced bruteforce around networked yaw
						{
							switch( g_CVars.Aimbot.Resolver.Step[ Index ] % 4 )
							{
								case 0: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 180.f; break;
								case 1: Entity->m_angEyeAngles( ).y = tempYaw[ Index ]; break;
								case 2: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 90.f; break;
								case 3: Entity->m_angEyeAngles( ).y = tempYaw[ Index ] + 270.f; break;
							}
						}
						else if( g_CVars.Aimbot.Resolver.Type == 4 ) // Anim Test: feet-yaw + fire-gesture + move-aware
						{
							Resolver_AnimTest( Entity, Index, tempYaw[ Index ] );
						}
						else if( g_CVars.Aimbot.Resolver.Type == 5 ) // AI Learn: smart base + learned offset
						{
							Resolver_AILearn( Entity, Index, tempYaw[ Index ] );
						}
						else if( g_CVars.Aimbot.Resolver.Type == 6 ) // Honest Shot: measured bullet_impact yaw
						{
							Resolver_Honest( Entity, Index, tempYaw[ Index ] );
						}
					}
				}
			}

			if( pPlayerHistory[ Index ][ 0 ].m_SimulationTime != Entity->m_flSimulationTime( ) )
			{
				for( int tick = 31; tick > 0; tick-- ) pPlayerHistory[ Index ][ tick ] = pPlayerHistory[ Index ][ tick - 1 ];
				g_Stuff.StoreTickRecord( Entity, &pPlayerHistory[ Index ][ 0 ] );
				// r40: lag-record capture - remember the raw SENT yaw + its simtime so the
				// aimbot can phase-predict a per-tick jitterer into choked/backtrack records
				if( g_CVars.Aimbot.Resolver.Active && g_CVars.Aimbot.Resolver.Jitter[ Index ] )
				{
					g_CVars.Aimbot.Resolver.LagSide[ Index ] = Resolver_AnimAbs( g_Stuff.GuwopNormalize( tempYaw[ Index ] - g_CVars.Aimbot.Resolver.JitterB[ Index ] ) )
						< Resolver_AnimAbs( g_Stuff.GuwopNormalize( tempYaw[ Index ] - g_CVars.Aimbot.Resolver.JitterA[ Index ] ) );
				}
				g_CVars.Aimbot.Resolver.EnemyChoke[ Index ] = 0;
			}
			else g_CVars.Aimbot.Resolver.EnemyChoke[ Index ]++; // simtime frozen = enemy choking packets
		}
	}
}