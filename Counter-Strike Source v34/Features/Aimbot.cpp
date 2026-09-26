#include "Main.h"

Aimbot g_Aimbot;

bool Aimbot::CheckVisible( Vector& vecAbsStart, Vector& vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities TraceFilter( Target, 0 );

	// todo: proper vis check

	Ray.Init( vecAbsStart, vecAbsEnd );
	// same mask the bullet uses: CS_MASK_SHOOT|CONTENTS_HITBOX (0x4600400B) —
	// the old 0x46004003 lacked CONTENTS_GRATE, so railings/fences read as
	// "visible" while the real shot stopped on them
	g_pEngineTrace->TraceRay( Ray, 0x4600400B, ( ITraceFilter* )&TraceFilter, &Trace );
	return ( Trace.fraction == 1.f );

}

bool Aimbot::CheckVisibleAWallCheck( Vector& vecAbsStart, Vector& vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities traceFilter( Target, 0 );

	// Cheap path first: one line trace with the bullet's own mask. This is
	// semantics-preserving — a visible point always returned true before too
	// (GetTotalDamage >= MinDamage, else the line trace) — but it skips the
	// full multi-bounce penetration simulation (~10+ traces) for every point
	// on an open path. That simulation was the main FPS sink of MultiSpot/
	// HitScan with Perfect Auto Wall on.
	Ray.Init( vecAbsStart, vecAbsEnd );
	g_pEngineTrace->TraceRay( Ray, 0x4600400B, ( ITraceFilter* )&traceFilter, &Trace );
	if( Trace.fraction == 1.f ) return true;

	if( g_CVars.Aimbot.AutoWall )
	{
		BaseEntity* pPlayerHit = nullptr;
		if( GetTotalDamage( LocalPlayer, Weapon, &pPlayerHit ) >= g_CVars.Aimbot.MinDamage ) return true;
	}

	return false;
}

void Aimbot::GetHitbox( int iHitbox, BasePlayer* Entity )
{
	// every early-out below must undo the backtrack record application,
	// otherwise the entity is left stuck in an old pose
	bool bAppliedRecord = false;

	if( g_CVars.Aimbot.Interpolation.LagPrediction )
	{
		g_Stuff.StoreTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		// same latency-correct record CorrectTickCount will send in tick_count —
		// bones we aim at == bones the server restores (sv_maxunlag window)
		g_Stuff.ApplyTickRecord( Entity, &pPlayerHistory[ Entity->entindex( ) ][ Resolver_PickRecord( Entity->entindex( ) ) ] );
		bAppliedRecord = true;

		int m_iAccumulatedBoneMask = *( int* )( ( DWORD ) Entity + 0x49C + 0x4 );
		int m_nReadableBones = *( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 );
		int m_nWritableBones = *( int* )( ( DWORD ) Entity + 0x4AC + 0x4 );
		int m_iPrevBoneMask = *( int* )( ( DWORD ) Entity + 0x498 + 0x4 );

		*( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 ) = 0;						// baseanimating + 0x4A8
		*( int* )( ( DWORD ) Entity + 0x4AC + 0x4 ) = 0;						// baseanimating + 0x4AC
		*( int* )( ( DWORD ) Entity + 0x498 + 0x4 ) = m_iAccumulatedBoneMask;	// baseanimating + 0x498
		*( int* )( ( DWORD ) Entity + 0x49C + 0x4 ) = 0;						// baseanimating + 0x49C
	}

	matrix3x4_t matrix[ 128 ];
	if( !( Entity->SetupBones( matrix, 128, 0x100, Entity->m_flSimulationTime( ) ) ) )
	{
		if( bAppliedRecord ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		return;
	}
	void* pModel = Entity->GetModel( );
	if( !pModel )
	{
		if( bAppliedRecord ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		return;
	}
	studiohdr_t* studiohdr = g_pModelInfo->GetStudiomodel( pModel );
	mstudiohitboxset_t* studiohitboxset = studiohdr ? studiohdr->pHitboxSet( Entity->m_nHitboxSet( ) ) : NULL;
	if( !studiohitboxset )
	{
		if( bAppliedRecord ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		return;
	}
	mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( iHitbox );
	if( !studiobbox )
	{
		if( bAppliedRecord ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		return;
	}

	mins[ Entity->entindex( ) ] = studiobbox->bbmin;
	maxs[ Entity->entindex( ) ] = studiobbox->bbmax;

	// PointScale semantics (restored): 0.0 = all points on the hitbox center,
	// 1.0 = exact AABB corners. The old formulas made PointScale 0 collapse
	// points[1..8] onto a degenerate line on the x axis (broken default) and
	// inset the x axis only half as far as y/z.
	Vector vCenter = ( studiobbox->bbmin + studiobbox->bbmax ) * .5f;
	Vector vCorners[ 8 ] = {
		Vector( studiobbox->bbmin.x, studiobbox->bbmin.y, studiobbox->bbmin.z ),
		Vector( studiobbox->bbmin.x, studiobbox->bbmin.y, studiobbox->bbmax.z ),
		Vector( studiobbox->bbmin.x, studiobbox->bbmax.y, studiobbox->bbmin.z ),
		Vector( studiobbox->bbmin.x, studiobbox->bbmax.y, studiobbox->bbmax.z ),
		Vector( studiobbox->bbmax.x, studiobbox->bbmin.y, studiobbox->bbmin.z ),
		Vector( studiobbox->bbmax.x, studiobbox->bbmin.y, studiobbox->bbmax.z ),
		Vector( studiobbox->bbmax.x, studiobbox->bbmax.y, studiobbox->bbmin.z ),
		Vector( studiobbox->bbmax.x, studiobbox->bbmax.y, studiobbox->bbmax.z ) };

	float flPointScale = g_CVars.Aimbot.PointScale;
	if( flPointScale < 0.f ) flPointScale = 0.f;
	if( flPointScale > 1.f ) flPointScale = 1.f;

	Vector points[ 9 ];
	points[ 0 ] = vCenter;
	for( int i = 0; i < 8; i++ )
		points[ i + 1 ] = vCenter + ( vCorners[ i ] - vCenter ) * flPointScale;

	float flPitch = Entity->m_angEyeAngles( ).x;

	if( iHitbox == 12 )
	{
		if( g_CVars.Aimbot.HitboxMode == 0 )
		{
			if( g_CVars.Aimbot.AutoHeightMode[ Entity->entindex( ) ] == 1 )
			{
				if( Entity->m_vecVelocity( ).Length2D( ) < 40.f && !( Entity->m_fFlags( ) & FL_DUCKING ) )
				{
					Vector a = ( ( points[ 3 ] + points[ 5 ] ) * .5f );

					if( ( flPitch > 50.f ) && ( flPitch < 91.f ) )
					{
						Vector b = ( ( ( a - points[ 0 ] ) / 3 ) * 4 );
						Vector c = ( points[ 0 ] + ( b * .7f ) );
						points[ 0 ] = c;
					}
					else if( ( flPitch >= -91.f ) && ( flPitch <= -50.f ) ) points[ 0 ].z -= 1.f;
				}
				else
				{
					if( ( flPitch > 50.f ) && ( flPitch < 91.f ) )
					{
						points[ 0 ].x = studiobbox->bbmin.x * .75f;
						points[ 0 ].y = studiobbox->bbmax.y * .75f; 
						points[ 0 ].z = ( studiobbox->bbmin.z + studiobbox->bbmax.z ) * .5f;
					}
					else if( ( flPitch >= -91.f ) && ( flPitch <= -50.f ) ) points[ 0 ].z -= 1.f;
				}
			}
		}
		else
		{
			if( g_CVars.Aimbot.HitboxMode == 1 ) // Origin: aim at the exact bone origin
			{
				points[ 0 ] = Vector( 0.f, 0.f, 0.f );
			}
			else if( g_CVars.Aimbot.HitboxMode == 2 || g_CVars.Aimbot.HitboxMode == 3 )
			{			
				if( ( flPitch > 50.f ) && ( flPitch < 91.f ) )
				{
					points[ 0 ].x = studiobbox->bbmin.x * .75f;
					points[ 0 ].y = studiobbox->bbmax.y * .75f; 
					points[ 0 ].z = ( studiobbox->bbmin.z + studiobbox->bbmax.z ) * .5f;

					if( g_CVars.Aimbot.HitboxMode == 2 ) points[ 0 ] += Vector( 0, .9f, .5f );
				}
				else if( ( flPitch >= -91.f ) && ( flPitch <= -50.f ) ) points[ 0 ].z -= 1.f;
			}
			else if( g_CVars.Aimbot.HitboxMode == 4 ) // Highest: Segregation aim-height fraction
			{
				// Segregation Interface_Aim_Height (default 0.9): Z = bbmin.z + (bbmax.z - bbmin.z) * h
				float h = g_CVars.Aimbot.AimHeight;
				if( h > 1.f ) h *= 0.01f; // tolerate % value in the ini
				if( h < 0.f ) h = 0.f;
				if( h > 1.f ) h = 1.f;
				points[ 0 ] = Vector( vCenter.x, vCenter.y,
					studiobbox->bbmin.z + ( studiobbox->bbmax.z - studiobbox->bbmin.z ) * h );
			}
		}
	}
	// other hitboxes keep the true local center: the old "points[0] += points[0]*.5"
	// pushed the aim point 50% away from the bone origin, potentially outside the box

	for( int index = 0; index <= 8; ++index ) VectorTransform( points[ index ], matrix[ studiobbox->bone ], vecCorners[ index ] );

	if( bAppliedRecord ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
}

bool BoneFilter( int bone )
{
	if( bone > 18 && bone < 28 ) return false; 
	else if( bone > 31 && bone < 41 ) return false;
	else return true; 
}

bool Aimbot::GetBone( int iBone, BasePlayer* Target, Vector& vBonePos )
{
	static matrix3x4_t matrix[ 128 ];

	if( !Target->SetupBones( matrix, 128, 0x100, Target->m_flSimulationTime( ) ) ) return false;

	vBonePos.x = matrix[ iBone ][ 0 ][ 3 ];
	vBonePos.y = matrix[ iBone ][ 1 ][ 3 ];
	vBonePos.z = matrix[ iBone ][ 2 ][ 3 ];

	return true;
}

int GetPlayerModifiedDamage( const float &constdamage, bool isHeadshot, bool isFriendly, bool hasHelmet, CSWeapon* Weapon, BaseEntity *targetEntity )
{
	WeaponInfo WeaponInfo = g_NoSpread.GetWeaponInfo( Weapon );

	float damage = constdamage;
	if( isFriendly ) damage *= .35f;

	int armor = targetEntity->m_ArmorValue( );
	if( ( armor > 0 ) && ( !isHeadshot || ( isHeadshot && hasHelmet ) ) )
	{
		float weaponArmorRatio = ( .5f * WeaponInfo.ArmorRatio );

		float newdamage = ( weaponArmorRatio * damage );
		float armordamage = ( ( damage - newdamage ) * .5f );

		if( armordamage <= ( float )armor )
		{
			armordamage = floor( armordamage );
		}
		else
		{
			newdamage = ( damage + ( ( float )armor * -2.f ) );
			armordamage = ( int )armor;
		}

		damage = floor( newdamage );
	}
	else damage = floor( damage );
	return ( int )damage;
}

float GetHitgroupModifiedDamage( float dmg, int hitgroup )
{
	static float hitgroupModifiers[ ] = { 1.f, 4.f, 1.f, 1.25f, 1.f, 1.f, .75f, .75f };
	return( dmg * hitgroupModifiers[ hitgroup ] );
}

int Aimbot::GetTotalDamage( BaseEntity* LocalPlayer, CSWeapon* Weapon, BaseEntity** ppPlayerHit )
{
	if( !LocalPlayer ) return -1;

	trace_t traceData, wallTraceData;
	Ray_t ray;

	WeaponInfo WeaponInfo = g_NoSpread.GetWeaponInfo( Weapon );

	int currentPenetration = WeaponInfo.Penetration;
	if( currentPenetration > 16 ) currentPenetration = 16;	// safety clamp on garbage weapon data
	float currentPenetrationPower = WeaponInfo.PenetrationPower;
	float currentDamage = ( float )WeaponInfo.Damage;
	float currentMaxRange = WeaponInfo.MaxRange;

	Vector start = EyePosition;
	Vector end;
	float tracedDistance = 0.f;
	BaseEntity *skipPlayer = 0;
	BaseEntity *tmp = 0;
	int totalDamage = 0;
	float wallThickness;
	bool isGrate;
	int material;

	float penetrationPowerModifier = 1.f;
	float damageModifier = .5f;		// engine default (cs_player_shared.cpp), overwritten per surface

	Vector clip;
	static ConVar* mp_friendlyfire = g_pCvar->FindVar( /*mp_friendlyfire*/XorStr<0x7A,16,0x24CED9A4>("\x17\x0B\x23\x1B\x0C\x16\xE5\xEF\xE6\xEF\xFD\xE3\xEF\xF5\xED"+0x24CED9A4).s );
	float tmpDistance;

	while( true )
	{
		// rebuilt every iteration with the last pierced player (engine passes
		// lastPlayerHit to each trace). The old filter was constructed ONCE
		// with a null second entity, so lined-up players were re-hit on the
		// next bounce instead of being skipped and the exit search could
		// stall inside their hull.
		TraceFilterSkipTwoEntities TraceFilter( LocalPlayer, skipPlayer );

		end = ( start + ( vecDirection * currentMaxRange ) );
		ray.Init( start, end );
		g_pEngineTrace->TraceRay( ray, 0x4600400B, ( ITraceFilter* )&TraceFilter, &traceData );
		clip = ( end + ( vecDirection * 40.0f ) );
		g_Stuff.ClipTraceToPlayers( start, clip, 0x4600400B, &TraceFilter, &traceData );
		tmp = traceData.m_pEnt;

		if( tmp && tmp->entindex( ) > 0 && tmp->entindex( ) <= g_pGlobals->maxClients ) 
			skipPlayer = tmp;
		else 
			skipPlayer = 0;

		if( traceData.fraction == 1.f ) break;
		
		tmpDistance = ( tracedDistance + ( currentMaxRange * traceData.fraction ) );
		currentDamage *= pow( WeaponInfo.RangeModifier, ( tmpDistance * .002f ) );

		if( skipPlayer )
		{
			if( ( g_CVars.Aimbot.FriendlyFire && ( skipPlayer->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) ) || ( skipPlayer->m_iTeamNum( ) != LocalPlayer->m_iTeamNum( ) ) )
			{
				totalDamage += GetPlayerModifiedDamage( GetHitgroupModifiedDamage( currentDamage, traceData.hitgroup ),
					( traceData.hitgroup == 1 ), 
					( skipPlayer->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ), 
					skipPlayer->m_bHasHelmet( ), 
					Weapon, 
					skipPlayer );

				if( ppPlayerHit && !*ppPlayerHit ) *ppPlayerHit = skipPlayer;
			}
		}

		isGrate = ( traceData.contents & CONTENTS_GRATE );
		material = ( int )g_pPhysicsSurfaceProps->GetSurfaceData( traceData.surface.surfaceProps )->game.material;

		// engine (cs_player_shared.cpp:440) ALWAYS refreshes both modifiers from
		// the ENTER material, then overrides them for grates (1.0 / 0.99). The old
		// code skipped the lookup on grates and kept the previous surface's values,
		// so a fence/railing after another material got a stale (too low) modifier
		// and walls that the engine pens read as unpennable.
		Weapon->GetMaterialParameters( material, penetrationPowerModifier, damageModifier );
		if( isGrate )
		{
			penetrationPowerModifier = 1.0f;
			damageModifier = 0.99f;
		}

		if( tmpDistance > WeaponInfo.PenetrationRange ) currentPenetration = ( currentPenetration <= 0 ) ? currentPenetration : 0;
		if( ( currentPenetration < 0 ) || ( ( currentPenetration == 0 ) && !isGrate ) ) break;

		// engine TraceToExit (step 24, candidates 24..144), restarted from THIS
		// surface every iteration. The old search reused a stale step counter
		// across outer iterations (and started at 48), so the exit of the 2nd+
		// surface or a thin wall followed by other geometry was found in the
		// wrong place -> thickness overestimated -> "some materials don't pen".
		Vector penetrationEnd;
		bool exitFound = false;
		float exitStep;
		for( exitStep = 24.f; exitStep <= 144.f; exitStep += 24.f )
		{
			penetrationEnd = traceData.endpos + ( vecDirection * exitStep );
			if( !( g_pEngineTrace->GetPointContents( penetrationEnd, 0 ) & 0x200400B ) )
			{
				exitFound = true;
				break;
			}
		}

		if( !exitFound ) break;

		ray.Init( penetrationEnd, traceData.endpos );
		g_pEngineTrace->TraceRay( ray, 0x4600400B, 0, &wallTraceData );

		if( wallTraceData.m_pEnt && ( wallTraceData.m_pEnt != traceData.m_pEnt ) )
		{
			g_Stuff.UTIL_TraceLine( penetrationEnd, traceData.endpos, 0x4600400B, wallTraceData.m_pEnt, 0, &wallTraceData );
		}

		wallThickness = ( wallTraceData.endpos - traceData.endpos ).Length( );

		// hollow wood/metal crate/barrel bonus (enter==exit && wood|metal -> x2)
		if( ( material == ( int )g_pPhysicsSurfaceProps->GetSurfaceData( wallTraceData.surface.surfaceProps )->game.material ) && ( ( material == 'W' ) || ( material == 'M' ) ) ) penetrationPowerModifier += penetrationPowerModifier;
		if( wallThickness > ( currentPenetrationPower * penetrationPowerModifier ) ) break;

		currentPenetrationPower -= ( wallThickness / penetrationPowerModifier );
		tracedDistance = ( tmpDistance + wallThickness );
		start = wallTraceData.endpos;
		currentMaxRange = ( ( currentMaxRange - tracedDistance ) * .5f );
		currentDamage *= damageModifier;
		currentPenetration--;
	}

	if( totalDamage == 0 ) totalDamage = -1;
	return totalDamage;
}

void Aimbot::Reset( )
{
	Distance = INT_MAX;
	Temp = INT_MAX;
	TargetIndex = -1;
	vecDirection.Init( 0, 0, 0 );
	for( int i = 0; i <= 8; ++i ) vecCorners[ i ].Init( 0, 0, 0 );
	IsAimbotting = false;
	qFinalAngle = QAngle( 0, 0, 0 );
}

int next_shot;
int Rate( BasePlayer* LocalPlayer, BasePlayer* Ent )
{
	int rate = 0;

	if( g_CVars.Aimbot.TargetSelection == 0 ) rate = LocalPlayer->GetAbsOrigin( ).DistTo( Ent->GetAbsOrigin( ) ); // distance
	if( g_CVars.Aimbot.TargetSelection == 1 ) rate = Ent->m_iHealth( ); // health
	if( g_CVars.Aimbot.TargetSelection == 2 || g_CVars.Aimbot.TargetSelection == 3 ) rate = Ent->entindex( ) > next_shot ? 0 : 1; // next shot, random

	return rate;
}

// lightweight profiler for the aimbot cost suspicion: accumulates an EMA and
// the worst tick, then emits one [perf] line every 15 s into AresWare.log
namespace
{
	struct AimbotPerfScope
	{
		LARGE_INTEGER t0;

		AimbotPerfScope( ) { QueryPerformanceCounter( &t0 ); }

		~AimbotPerfScope( )
		{
			LARGE_INTEGER t1, freq;
			QueryPerformanceCounter( &t1 );
			QueryPerformanceFrequency( &freq );

			const double ms = ( double )( t1.QuadPart - t0.QuadPart ) * 1000.0 / ( double )freq.QuadPart;

			static double ema = 0.0, worst = 0.0;
			static int calls = 0;
			static ULONGLONG lastLog = 0;

			ema = ( ema == 0.0 ) ? ms : ( ema * 0.95 + ms * 0.05 );
			if( ms > worst ) worst = ms;
			calls++;

			const ULONGLONG now = GetTickCount64( );
			if( !lastLog ) lastLog = now;
			if( now - lastLog > 15000 )
			{
				Logger::Write( "[perf] Aimbot::Main avg %.3f ms / worst %.3f ms over %d calls in 15 s",
					( float )ema, ( float )worst, calls );
				worst = 0.0;
				calls = 0;
				lastLog = now; // fixed: previously never updated -> [perf] line was written every call
			}
		}
	};
}

void Aimbot::Main( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	AimbotPerfScope _perfScope;

	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon || !Weapon->IsWeapon( ) ) return;

	WeaponInfo wpnInfo = g_NoSpread.GetWeaponInfo( Weapon );

	Reset( );

	int iSpot;

	int m_iWeaponID = Weapon->GetWeaponID( );
	( void )m_iWeaponID;

	if( Weapon->GetWeaponID( ) == 17 )
	{
		if( g_CVars.Aimbot.BodyAWP ) iSpot = 10;
		else iSpot = g_CVars.Aimbot.Hitbox;
	}
	else iSpot = g_CVars.Aimbot.Hitbox;

	// fixed hitbox priority: slot 0 is always the per-entity primary spot,
	// slots 1..18 are the fallback scan order (duplicates of the primary are
	// skipped during iteration — the old replace-with-itself dedup was a no-op)
	static const int Choose[ ] = { 12, 11, 5, 0, 1, 9, 10, 13, 14, 16, 17, 18, 8, 7, 6, 4, 3, 2, 15 };

	if( g_CVars.Aimbot.Key > 0 )
	{
		if( !GetAsyncKeyState( g_CVars.Aimbot.Key ) ) return;
	}

	for( auto i = g_pGlobals->maxClients; i >= 1; --i )
	{
		if( i == g_pEngineClient->GetLocalPlayer( ) ) continue;
		BasePlayer* Ent = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( i );
		if( Ent == 0 ) continue;
		if( Ent->IsDormant( ) ) continue;
		if( Ent->m_lifeState( ) != 0 ) continue;

		if( !g_CVars.Aimbot.FriendlyFire )
		{
			if( Ent->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
		}

		if( Ent->m_iHealth( ) > 500 ) continue;
		if( Ent->IsSpawnProtectedPlayer( ) ) continue;
		if( g_CVars.PlayerList.Friend[ i ] ) continue;
		if( Ent->m_vecOrigin( ).DistTo( EyePosition ) > wpnInfo.MaxRange ) continue;

		// whitelist forces hitbox 12 for THIS entity only — the old code wrote
		// the shared static iSpot, leaking the override onto later entities (AWP)
		// and being ignored entirely by the non-AWP branches
		int primarySpot = g_Whitelist.List( i ) ? 12 : iSpot;

		int rate = Rate( LocalPlayer, Ent );
		if( rate > Temp ) continue;

		if( Weapon->GetWeaponID( ) == 17 ) // awp
		{
			// todo: fix issue with awp not hitting shit while backtracking is on
			
			GetHitbox( primarySpot, Ent );
			
			if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
			{
				VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
				VectorNormalizeFast( vecDirection );

				if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
				{
					VectorAngles( vecDirection, pCmd->viewangles );
					IsAimbotting = true;
				}

				TargetIndex = i;
				Temp = rate;
			}
		}
		else
		{
			// Unified MultiSpot / HitScan scan:
			//   strict hitbox priority (primary -> fallbacks), inside each hitbox
			//   center first then the PointScale scale points, first visible point
			//   wins and the scan STOPS for this entity.
			// The old version ran two passes (all edges of all hitboxes, then all
			// centers), so the center of a low-priority hitbox beat a visible edge
			// of the preferred one, re-ran SetupBones for every hitbox, and never
			// broke out — up to 19x SetupBones + 150+ traces per player per tick.
			const bool bMulti = g_CVars.Aimbot.MultiSpot;
			const bool bScan = g_CVars.Aimbot.HitScan;
			const int maxSlot = bScan ? 18 : 0;
			bool found = false;

			for( int h = 0; h <= maxSlot && !found; h++ )
			{
				int hb = ( h == 0 ) ? primarySpot : Choose[ h ];

				bool dup = false;
				for( int k = 0; k < h; k++ )
				{
					int prev = ( k == 0 ) ? primarySpot : Choose[ k ];
					if( prev == hb ) { dup = true; break; }
				}
				if( dup ) continue;

				GetHitbox( hb, Ent );

				// center (0) always, scale points (1..8) only with MultiSpot
				const int lastPoint = bMulti ? 8 : 0;

				for( int c = 0; c <= lastPoint && !found; c++ )
				{
					VectorSubtract( vecCorners[ c ], EyePosition, vecDirection );
					VectorNormalizeFast( vecDirection );

					if( CheckVisibleAWallCheck( EyePosition, vecCorners[ c ], Ent, LocalPlayer ) )
					{
						if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
						{
							VectorAngles( vecDirection, pCmd->viewangles );
							IsAimbotting = true;
						}

						TargetIndex = i;
						Temp = rate;
						found = true;
					}
				}
			}
		}
	}

	if( IsAimbotting && TargetIndex != -1 )
	{
		if( g_CVars.Aimbot.AutoShoot ) pCmd->buttons |= IN_ATTACK;
	}

	if( g_CVars.Aimbot.TargetSelection == 3 ) next_shot = TargetIndex;

	if( IsAimbotting )
	{
		if( g_CVars.Aimbot.TargetSelection != 3 ) next_shot = TargetIndex;
	}
	else if( Weapon->ShouldReload( ) ) pCmd->buttons |= IN_RELOAD;

	if( !g_CVars.Aimbot.Silent ) g_pEngineClient->SetViewAngles( pCmd->viewangles );
}