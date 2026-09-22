#include "Main.h"

Aimbot g_Aimbot;

bool Aimbot::CheckVisible( Vector& vecAbsStart, Vector& vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities TraceFilter( Target, 0 );

	// direct visibility: the trace filter skips the target, so a fraction of 1.0
	// means nothing in the world blocks the line from the eye to the aim point
	Ray.Init( vecAbsStart, vecAbsEnd );
	g_pEngineTrace->TraceRay( Ray, 0x46004003, ( ITraceFilter* )&TraceFilter, &Trace );
	return ( Trace.fraction == 1.f );

}

bool Aimbot::CheckVisibleAWallCheck( Vector& vecAbsStart, Vector& vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer )
{
	// optimized: visibility first (cheap), autowall second (expensive)
	// direct visibility: the filter skips the target itself, so a fraction of 1.0
	// means nothing in the world blocks the line from the eye to the aim point
	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities TraceFilter( Target, 0 );

	Ray.Init( vecAbsStart, vecAbsEnd );
	g_pEngineTrace->TraceRay( Ray, 0x46004003, ( ITraceFilter* )&TraceFilter, &Trace );
	if( Trace.fraction == 1.f ) return true;

	// autowall: only if not directly visible and enabled
	if( !g_CVars.Aimbot.AutoWall ) return false;

	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	BaseEntity* pPlayerHit = nullptr;
	int dmg = GetTotalDamage( LocalPlayer, Weapon, &pPlayerHit );
	if( dmg >= g_CVars.Aimbot.MinDamage && pPlayerHit == Target ) return true;

	return false;
}

matrix3x4_t* Aimbot::GetBoneMatrix( BasePlayer* Entity )
{
	static bool bCacheInit = false;
	if( !bCacheInit )
	{
		for( int i = 0; i < 64; i++ ) { BoneCacheValid[i] = false; BoneCacheSimTime[i] = 0.f; }
		bCacheInit = true;
	}

	int idx = Entity->entindex( );
	if( idx < 0 || idx >= 64 ) return nullptr;

	float curSimTime = Entity->m_flSimulationTime( );

	// optimized cache: if sim time same and cache valid, reuse (saves SetupBones FPS)
	if( BoneCacheValid[ idx ] && BoneCacheSimTime[ idx ] == curSimTime )
	{
		// copy cached to BoneCache for compatibility with old code using BoneCache
		for( int i = 0; i < 128; i++ ) BoneCache[i] = BoneCacheArray[idx][i];
		BoneCacheIndex = idx;
		return BoneCache;
	}

	// one SetupBones per entity per tick, reused for every hitbox point (old single cache)
	if( BoneCacheIndex == idx && BoneCacheValid[ idx ] && BoneCacheSimTime[ idx ] == curSimTime )
		return BoneCache;

	if( g_CVars.Aimbot.Interpolation.LagPrediction )
	{
		g_Stuff.StoreTickRecord( Entity, &pBackupData[ idx ] );
		g_Stuff.ApplyTickRecord( Entity, &pPlayerHistory[ idx ][ 0 ] );

		int m_iAccumulatedBoneMask = *( int* )( ( DWORD ) Entity + 0x49C + 0x4 );
		int m_nReadableBones = *( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 );
		int m_nWritableBones = *( int* )( ( DWORD ) Entity + 0x4AC + 0x4 );
		int m_iPrevBoneMask = *( int* )( ( DWORD ) Entity + 0x498 + 0x4 );

		*( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 ) = 0;
		*( int* )( ( DWORD ) Entity + 0x4AC + 0x4 ) = 0;
		*( int* )( ( DWORD ) Entity + 0x498 + 0x4 ) = m_iAccumulatedBoneMask;
		*( int* )( ( DWORD ) Entity + 0x49C + 0x4 ) = 0;
	}

	bool bSuccess = Entity->SetupBones( BoneCache, 128, 0x100, curSimTime );

	if( g_CVars.Aimbot.Interpolation.LagPrediction ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ idx ] );

	if( !bSuccess ) return nullptr;

	// store to per-entity cache
	for( int i = 0; i < 128; i++ ) BoneCacheArray[idx][i] = BoneCache[i];
	BoneCacheValid[ idx ] = true;
	BoneCacheSimTime[ idx ] = curSimTime;
	BoneCacheIndex = idx;
	return BoneCache;
}

void Aimbot::GetHitbox( int iHitbox, BasePlayer* Entity )
{
	matrix3x4_t* matrix = GetBoneMatrix( Entity );
	if( !matrix ) return;
	void* pModel = Entity->GetModel( );
	if( !pModel ) return;
	studiohdr_t* studiohdr = g_pModelInfo->GetStudiomodel( pModel );
	if( !studiohdr ) return;
	mstudiohitboxset_t* studiohitboxset = studiohdr->pHitboxSet( Entity->m_nHitboxSet( ) );	
	if( !studiohitboxset ) return;
	mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( iHitbox );
	if( !studiobbox ) return;

	mins[ Entity->entindex( ) ] = studiobbox->bbmin;
	maxs[ Entity->entindex( ) ] = studiobbox->bbmax;

	float ps = g_CVars.Aimbot.PointScale;
	if( ps < 0.f ) ps = 0.f;
	if( ps > 1.f ) ps = 1.f;
	float invPS = 1.f - ps;
	Vector points[ 9 ];
	// center is always accurate
	points[0] = ( studiobbox->bbmin + studiobbox->bbmax ) * 0.5f;

	// determine how many points we need (FPS + accuracy)
	int pointsNeeded = 1;
	if( g_CVars.Aimbot.MultiSpot )
	{
		if( iHitbox == 12 ) pointsNeeded = 9; // head full multispot for max accuracy
		else if( iHitbox == 11 || iHitbox == 5 || iHitbox == 9 || iHitbox == 10 ) pointsNeeded = 5; // chest/neck limited
	}

	if( pointsNeeded > 1 )
	{
		// proper scaled points: interpolate between min and max with PointScale
		// PointScale 0 = center only, 1 = full hitbox corners
		Vector center = points[0];
		Vector ext = ( studiobbox->bbmax - studiobbox->bbmin ) * 0.5f * invPS;
		// 8 corners around center scaled by PointScale
		points[1] = center + Vector( -ext.x, -ext.y, -ext.z );
		points[2] = center + Vector( -ext.x,  ext.y, -ext.z );
		points[3] = center + Vector(  ext.x,  ext.y, -ext.z );
		points[4] = center + Vector(  ext.x, -ext.y, -ext.z );
		points[5] = center + Vector(  ext.x,  ext.y,  ext.z );
		points[6] = center + Vector( -ext.x,  ext.y,  ext.z );
		points[7] = center + Vector( -ext.x, -ext.y,  ext.z );
		points[8] = center + Vector(  ext.x, -ext.y,  ext.z );
	}

	// head height correction based on HitboxMode, but fixed to not break head
	// Old code had buggy AutoHeightMode that moved head to feet. Now we do proper height modes.
	float flPitch = Entity->m_angEyeAngles( ).x;
	if( iHitbox == 12 )
	{
		// HitboxMode: 0 Auto, 1 Origin, 2 Center, 3 Center Fixed, 4 Highest
		// For head, we want to ensure we aim at head even when enemy looks up/down
		// Auto mode: adjust slightly based on pitch, but keep within head bounds
		if( g_CVars.Aimbot.HitboxMode == 0 )
		{
			// auto: if looking down (pitch >50), aim slightly lower in head to avoid miss due to model tilt
			// but keep within hitbox, not outside
			if( flPitch > 50.f && flPitch < 90.f )
			{
				// looking down, head tilts forward, aim a bit lower (0.5 units down)
				points[0].z -= 0.5f;
			}
			else if( flPitch < -50.f && flPitch > -90.f )
			{
				// looking up, aim a bit higher
				points[0].z += 0.5f;
			}
			// AutoHeightMode from resolver: if set, it means enemy is sideways, we should not adjust head height much
			// Previously it set head to bbmin*0.75 which was buggy (head at feet). Now we ignore AutoHeightMode for head center
		}
		else if( g_CVars.Aimbot.HitboxMode == 1 ) // Origin
		{
			points[0] = studiobbox->bbmin * 0.3f + studiobbox->bbmax * 0.7f; // slightly upper
		}
		else if( g_CVars.Aimbot.HitboxMode == 2 ) // Center
		{
			// already center
		}
		else if( g_CVars.Aimbot.HitboxMode == 3 ) // Center Fixed
		{
			points[0] = ( studiobbox->bbmin + studiobbox->bbmax ) * 0.5f;
		}
		else if( g_CVars.Aimbot.HitboxMode == 4 ) // Highest - aim at top of head for HS
		{
			points[0].z = studiobbox->bbmax.z - 1.f;
		}
	}
	else
	{
		// for body hitboxes, don't do buggy points[0] += points[0]*0.5
		// keep center as is, or slightly upper for chest
		if( iHitbox == 5 || iHitbox == 9 || iHitbox == 10 ) // chest
		{
			points[0].z += 1.f; // aim slightly upper chest
		}
	}

	for( int index = 0; index < pointsNeeded; ++index ) VectorTransform( points[ index ], matrix[ studiobbox->bone ], vecCorners[ index ] );
	for( int index = pointsNeeded; index <= 8; ++index ) vecCorners[ index ] = vecCorners[ 0 ];
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

	// FPS optimization: early out if min damage is 0 or weapon can't penetrate
	if( g_CVars.Aimbot.MinDamage <= 0 ) return 0;

	trace_t traceData, wallTraceData;
	Ray_t ray;

	WeaponInfo WeaponInfo = g_NoSpread.GetWeaponInfo( Weapon );

	int currentPenetration = WeaponInfo.Penetration;
	float currentPenetrationPower = WeaponInfo.PenetrationPower;
	float currentDamage = ( float )WeaponInfo.Damage;
	float currentMaxRange = WeaponInfo.MaxRange;

	// early out: if base damage already below min, no need to trace
	if( currentDamage < g_CVars.Aimbot.MinDamage ) return -1;

	Vector start = EyePosition, wall;
	Vector end;
	float tracedDistance = 0.f;
	float multiplier = 0.f;
	BaseEntity *skipPlayer = 0;
	BaseEntity *tmp = 0;
	TraceFilterSkipTwoEntities TraceFilter( LocalPlayer, skipPlayer );
	int totalDamage = 0;
	float wallThickness;
	bool isGrate;
	int material;

	float penetrationPowerModifier = 1.f;
	float damageModifier = .99f;

	Vector clip;
	static ConVar* mp_friendlyfire = g_pCvar->FindVar( /*mp_friendlyfire*/XorStr<0x7A,16,0x24CED9A4>("\x17\x0B\x23\x1B\x0C\x16\xE5\xEF\xE6\xEF\xFD\xE3\xEF\xF5\xED"+0x24CED9A4).s );
	float tmpDistance;

	while( true )
	{
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

		// FPS: if damage already below min, stop
		if( currentDamage < g_CVars.Aimbot.MinDamage && totalDamage == 0 ) break;

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

		// official behavior: hitting a grate forces full penetration power and near-full
		// damage; without this the modifiers from the previous wall carry over
		if( isGrate )
		{
			penetrationPowerModifier = 1.f;
			damageModifier = .99f;
		}
		else Weapon->GetMaterialParameters( material, penetrationPowerModifier, damageModifier );
		if( tmpDistance > WeaponInfo.PenetrationRange ) currentPenetration = ( currentPenetration <= 0 ) ? currentPenetration : 0;
		if( ( currentPenetration < 0 ) || ( ( currentPenetration == 0 ) && !isGrate ) ) break;

		multiplier = 0.f; // fresh TraceToExit for every wall
		while( true )
		{
			multiplier += 24.f;
			wall = ( traceData.endpos + ( vecDirection * multiplier ) );
			if( !( g_pEngineTrace->GetPointContents( ( traceData.endpos + ( vecDirection * multiplier ) ), 0 ) & 0x200400B ) ) break;
			if( multiplier > 128.f )
			{
				multiplier = -1.f;
				break;
			}
		}

		if( multiplier == -1.f ) break;

		ray.Init( wall, traceData.endpos );
		g_pEngineTrace->TraceRay( ray, 0x4600400B, 0, &wallTraceData );

		if( wallTraceData.m_pEnt && ( wallTraceData.m_pEnt != traceData.m_pEnt ) )
		{
			g_Stuff.UTIL_TraceLine( wall, traceData.endpos, 0x4600400B, wallTraceData.m_pEnt, 0, &wallTraceData );
		}

		wallThickness = ( wallTraceData.endpos - traceData.endpos ).Length( );

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
	Deviation = 0.f;
	BoneCacheIndex = -1;
	// don't clear per-entity cache here to keep FPS benefit across ticks
	// only clear if entity dormant will be handled in Main loop via sim time check
}

float Aimbot::AngleDeviation( const Vector& vecPoint, CUserCmd* pCmd )
{
	Vector vecDelta = vecPoint - EyePosition;
	VectorNormalizeFast( vecDelta );

	QAngle angTo;
	VectorAngles( vecDelta, angTo );

	float dx = g_Stuff.GuwopNormalize( angTo.x - pCmd->viewangles.x );
	float dy = g_Stuff.GuwopNormalize( angTo.y - pCmd->viewangles.y );

	return Vector( dx, dy, 0.f ).Length( );
}

// optimized scan: FPS friendly
// - visibility first, autowall second
// - multispot only for head (12) to save 6x traces
// - center point preferred, corners only if needed
// - early exit on first visible point
bool Aimbot::ScanTarget( BasePlayer* LocalPlayer, BasePlayer* Ent, const int* pHitboxOrder, int nHitboxes, bool bMultiSpot, Vector& vecBestPoint )
{
	// adaptive: far targets -> only head/chest, close -> more
	int maxHitboxes = nHitboxes;
	if( nHitboxes > 3 )
	{
		float dist = Ent->m_vecOrigin( ).DistTo( EyePosition );
		if( dist > 1200.f ) maxHitboxes = 3; // head, neck, chest only for far
		else if( dist > 600.f ) maxHitboxes = 6;
	}

	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	for( int h = 0; h < maxHitboxes; h++ )
	{
		int hitboxId = pHitboxOrder[ h ];
		GetHitbox( hitboxId, Ent );

		// first try center (most reliable)
		VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
		VectorNormalizeFast( vecDirection );

		// check visibility first (fast)
		if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
		{
			vecBestPoint = vecCorners[ 0 ];
			return true;
		}
		// autowall with head priority: head needs less damage to shoot
		if( g_CVars.Aimbot.AutoWall )
		{
			BaseEntity* pHit = nullptr;
			int dmg = GetTotalDamage( LocalPlayer, Weapon, &pHit );
			int required = g_CVars.Aimbot.MinDamage;
			if( hitboxId == 12 ) required = max( 1, required / 2 ); // head needs half damage (important for AWP, wallbang)
			else if( hitboxId == 11 || hitboxId == 9 ) required = max( 1, required * 3 / 4 ); // neck/chest 75%
			if( dmg >= required && pHit == Ent )
			{
				vecBestPoint = vecCorners[ 0 ];
				return true;
			}
		}

		// multispot only for head (12) and optionally chest (5) to keep FPS
		// head 9 points, chest limited 4 points
		if( bMultiSpot && ( hitboxId == 12 || hitboxId == 11 || hitboxId == 5 ) )
		{
			int nLastCorner = 8;
			if( hitboxId != 12 ) nLastCorner = 4;
			for( int c = 1; c <= nLastCorner; c++ )
			{
				VectorSubtract( vecCorners[ c ], EyePosition, vecDirection );
				VectorNormalizeFast( vecDirection );

				if( CheckVisible( EyePosition, vecCorners[ c ], Ent, LocalPlayer ) )
				{
					vecBestPoint = vecCorners[ c ];
					return true;
				}
				if( g_CVars.Aimbot.AutoWall )
				{
					BaseEntity* pHit2 = nullptr;
					int dmg = GetTotalDamage( LocalPlayer, Weapon, &pHit2 );
					int required = g_CVars.Aimbot.MinDamage;
					if( hitboxId == 12 ) required = max( 1, required / 2 );
					if( dmg >= required && pHit2 == Ent )
					{
						vecBestPoint = vecCorners[ c ];
						return true;
					}
				}
			}
		}
	}

	return false;
}

void Aimbot::ApplyAngles( CUserCmd* pCmd )
{
	// legit improvements: RCS, humanize, reaction, etc.
	bool bIsLegit = ( g_CVars.AimbotProfile == 0 );

	// silent aim and classic snap go straight to the target
	if( g_CVars.Aimbot.Silent || g_CVars.Aimbot.Smooth <= 1.f )
	{
		pCmd->viewangles = qFinalAngle;

		// RCS for rage/silent - instant compensation
		if( g_CVars.Aimbot.RCS && bIsLegit == false )
		{
			BasePlayer* LocalPlayer = ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
			if( LocalPlayer )
			{
				QAngle punch = LocalPlayer->GetPunchAngle( );
				pCmd->viewangles.x -= punch.x * g_CVars.Aimbot.RCSAmountX * g_CVars.Aimbot.RCSScale;
				pCmd->viewangles.y -= punch.y * g_CVars.Aimbot.RCSAmountY * g_CVars.Aimbot.RCSScale;
			}
		}
		return;
	}

	// smoothed aim: glide towards the target over multiple ticks
	float flSmooth = g_CVars.Aimbot.Smooth;
	if( flSmooth < 1.f ) flSmooth = 1.f;

	// humanize: randomize smooth a bit for legit
	if( bIsLegit && g_CVars.Aimbot.Humanize )
	{
		float randFactor = g_CVars.Aimbot.HumanizeRandom;
		if( randFactor < 0.f ) randFactor = 0.f;
		if( randFactor > 1.f ) randFactor = 1.f;
		// RandomFloat is engine's random, use it for humanization
		float rnd = RandomFloat( -randFactor, randFactor );
		flSmooth += flSmooth * rnd;

		// curve: ease out - slower when close to target
		float dist = Deviation;
		if( dist < 10.f )
			flSmooth *= ( 1.f + ( 10.f - dist ) * 0.05f );
	}

	float dx = g_Stuff.GuwopNormalize( qFinalAngle.x - pCmd->viewangles.x );
	float dy = g_Stuff.GuwopNormalize( qFinalAngle.y - pCmd->viewangles.y );

	// RCS for legit - gradual compensation
	if( g_CVars.Aimbot.RCS )
	{
		BasePlayer* LocalPlayer = ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
		if( LocalPlayer )
		{
			QAngle punch = LocalPlayer->GetPunchAngle( );
			// only compensate while shooting or always depending on mode
			bool bShouldRCS = true;
			if( g_CVars.Aimbot.RCSMode == 1 )
			{
				// while shooting only
				bShouldRCS = ( pCmd->buttons & IN_ATTACK ) != 0;
			}

			if( bShouldRCS )
			{
				float rcsX = punch.x * g_CVars.Aimbot.RCSAmountX * g_CVars.Aimbot.RCSScale;
				float rcsY = punch.y * g_CVars.Aimbot.RCSAmountY * g_CVars.Aimbot.RCSScale;

				// humanize RCS a bit
				if( bIsLegit && g_CVars.Aimbot.Humanize )
				{
					rcsX += RandomFloat( -0.2f, 0.2f ) * g_CVars.Aimbot.HumanizeRandom;
					rcsY += RandomFloat( -0.2f, 0.2f ) * g_CVars.Aimbot.HumanizeRandom;
				}

				dx -= rcsX;
				dy -= rcsY;
			}
		}
	}

	pCmd->viewangles.x += dx / flSmooth;
	pCmd->viewangles.y += dy / flSmooth;

	if( pCmd->viewangles.x > 89.f ) pCmd->viewangles.x = 89.f;
	else if( pCmd->viewangles.x < -89.f ) pCmd->viewangles.x = -89.f;

	pCmd->viewangles.y = g_Stuff.GuwopNormalize( pCmd->viewangles.y );
	pCmd->viewangles.z = 0.f;
}

int next_shot;
int Rate( BasePlayer* LocalPlayer, BasePlayer* Ent )
{
	int rate = 0;

	if( g_CVars.Aimbot.TargetSelection == 0 ) rate = LocalPlayer->GetAbsOrigin( ).DistTo( Ent->GetAbsOrigin( ) ); // distance
	if( g_CVars.Aimbot.TargetSelection == 1 ) rate = Ent->m_iHealth( ); // health
	if( g_CVars.Aimbot.TargetSelection == 2 || g_CVars.Aimbot.TargetSelection == 3 ) rate = Ent->entindex( ) > next_shot ? 0 : 1; // next shot, random

	// improved: lethal body aim - prioritize low health targets if lethal body enabled
	if( g_CVars.Aimbot.Interpolation.LethalBody )
	{
		int health = Ent->m_iHealth( );
		if( health <= 30 ) rate -= 1000; // very low health = highest priority
		else if( health <= 50 ) rate -= 500; // low health = high priority
	}

	return rate;
}

void Aimbot::Main( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon || !Weapon->IsWeapon( ) ) return;

	WeaponInfo wpnInfo = g_NoSpread.GetWeaponInfo( Weapon );

	Reset( );

	// hitbox groups: user selectable groups, not single hitbox
	// 0=Head(12),1=Neck(11),2=Chest(9,10,5),3=Stomach(0,1),4=Arms(13,14,16,17),5=Legs(2,3,4,15,6,7,8,18)
	static const int Group_Head[] = { 12 };
	static const int Group_Neck[] = { 11 };
	static const int Group_Chest[] = { 9, 10, 5 };
	static const int Group_Stomach[] = { 0, 1 };
	static const int Group_Arms[] = { 13, 14, 16, 17 };
	static const int Group_Legs[] = { 2, 3, 4, 15, 6, 7, 8, 18 };
	static const int* Groups[] = { Group_Head, Group_Neck, Group_Chest, Group_Stomach, Group_Arms, Group_Legs };
	static const int GroupSizes[] = { 1, 1, 3, 2, 4, 8 };
	static const int Choose[ ] = { 12, 11, 5, 0, 1, 9, 10, 13, 14, 16, 17, 18, 8, 7, 6, 4, 3, 2, 15 }; // fallback

	int iSpot = g_CVars.Aimbot.Hitbox;
	if( Weapon->GetWeaponID( ) == 17 && g_CVars.Aimbot.BodyAWP ) iSpot = 10; // awp prefers body

	// build hitbox order from groups with user-defined priority
	int HitboxOrder[ 19 ];
	int nHitboxes = 0;

	// check if any group enabled, otherwise fallback to old single-hitbox logic
	bool bHasGroups = false;
	for( int g=0; g<6; g++ ) if( g_CVars.Aimbot.HitboxGroup[g] ) { bHasGroups=true; break; }
	if( g_CVars.Aimbot.HitboxGroupsMask == 0 ) bHasGroups=false; // backward compat: mask 0 = old config

	if( bHasGroups )
	{
		bool bAdded[19] = {false};

		// primary override: if HitboxPriorityGroup set and enabled, put it first
		int primary = g_CVars.Aimbot.HitboxPriorityGroup;
		if( primary>=0 && primary<6 && g_CVars.Aimbot.HitboxGroup[primary] )
		{
			for( int j=0;j<GroupSizes[primary];j++ )
			{
				int hb = Groups[primary][j];
				if( hb<0||hb>=19 ) continue;
				if( bAdded[hb] ) continue;
				HitboxOrder[nHitboxes++]=hb;
				bAdded[hb]=true;
			}
		}

		// then add selected iSpot if its group enabled and not already added (keeps old priority hitbox behavior)
		for( int g=0; g<6; g++ )
		{
			if( !g_CVars.Aimbot.HitboxGroup[g] ) continue;
			for( int j=0;j<GroupSizes[g];j++ )
			{
				if( Groups[g][j]==iSpot && !bAdded[iSpot] )
				{
					HitboxOrder[nHitboxes++]=iSpot;
					bAdded[iSpot]=true;
				}
			}
		}

		// then add rest in user-defined group order (HitboxGroupOrder)
		for( int orderIdx=0; orderIdx<6; orderIdx++ )
		{
			int g = g_CVars.Aimbot.HitboxGroupOrder[orderIdx];
			if( g<0||g>=6 ) continue;
			if( !g_CVars.Aimbot.HitboxGroup[g] ) continue;
			if( g==primary ) continue; // already added
			for( int j=0;j<GroupSizes[g];j++ )
			{
				int hb = Groups[g][j];
				if( hb<0||hb>=19 ) continue;
				if( bAdded[hb] ) continue;
				bool dup=false;
				for(int k=0;k<nHitboxes;k++) if(HitboxOrder[k]==hb) {dup=true;break;}
				if(dup) continue;
				HitboxOrder[nHitboxes++]=hb;
				bAdded[hb]=true;
			}
		}

		// fallback to default order if still empty
		if( nHitboxes==0 )
		{
			HitboxOrder[nHitboxes++]=iSpot;
			for( int i=1;i<19;i++ ) { if( Choose[i]==iSpot ) continue; HitboxOrder[nHitboxes++]=Choose[i]; }
		}
	}
	else
	{
		// old logic: single hitbox first, then rest
		HitboxOrder[ nHitboxes++ ] = iSpot;
		for( int i = 1; i < 19; i++ )
		{
			if( Choose[ i ] == iSpot ) continue;
			HitboxOrder[ nHitboxes++ ] = Choose[ i ];
		}
	}

	bool bIsLegit = ( g_CVars.AimbotProfile == 0 );

	// legit: OnKey - only aim when key is held, if enabled
	if( bIsLegit && g_CVars.Aimbot.OnKey )
	{
		if( g_CVars.Aimbot.Key > 0 )
		{
			if( !GetAsyncKeyState( g_CVars.Aimbot.Key ) ) return;
		}
	}
	else
	{
		if( g_CVars.Aimbot.Key > 0 )
		{
			if( !GetAsyncKeyState( g_CVars.Aimbot.Key ) ) return;
		}
	}

	float flFov = g_CVars.Aimbot.FOV;
	if( flFov <= 0.f || flFov > 180.f ) flFov = 180.f;

	// legit: distance-scaled FOV - far targets need smaller FOV to look legit
	float flSmooth = g_CVars.Aimbot.Smooth;
	if( flSmooth < 1.f ) flSmooth = 1.f;

	// legit reaction time tracking
	static int lastTarget = -1;
	static float firstSeenTime = 0.f;
	static float lastAimTime = 0.f;
	float curTime = g_pGlobals->curtime;

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

		float distToTarget = Ent->m_vecOrigin( ).DistTo( EyePosition );

		// legit: distance-scaled FOV
		float effectiveFov = flFov;
		if( bIsLegit && flFov < 90.f )
		{
			// far targets -> smaller FOV, close -> slightly larger
			if( distToTarget > 500.f )
				effectiveFov *= ( 500.f / distToTarget ) * 1.2f;
			if( effectiveFov < 2.f ) effectiveFov = 2.f;
		}

		// cheap FOV precheck against the body center, so entities far outside the
		// aim cone never pay for the expensive bone setup (+15 deg body radius margin)
		if( effectiveFov < 180.f )
		{
			Vector vecCenter = Ent->GetAbsOrigin( );
			vecCenter.z += 40.f;
			if( AngleDeviation( vecCenter, pCmd ) > effectiveFov + 15.f ) continue;
		}

		if( g_Whitelist.List( i ) ) iSpot = 12;
		int rate = Rate( LocalPlayer, Ent );
		if( rate > Temp ) continue;

		bool bFound = false;
		Vector vecPoint;

		if( Weapon->GetWeaponID( ) == 17 ) // awp
		{
			// awp prefers body but respect groups if enabled
			if( bIsLegit && g_CVars.Aimbot.VisOnly )
			{
				// legit vis only: scan groups with visibility only (no autowall)
				for( int h=0; h<nHitboxes && !bFound; h++ )
				{
					GetHitbox( HitboxOrder[h], Ent );
					VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
					VectorNormalizeFast( vecDirection );
					if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
					{
						vecPoint = vecCorners[ 0 ];
						bFound = true;
					}
				}
			}
			else
			{
				GetHitbox( iSpot, Ent );
				VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
				VectorNormalizeFast( vecDirection );
				if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
				{
					vecPoint = vecCorners[ 0 ];
					bFound = true;
				}
			}
		}
		else
		{
			int nSpots = ( g_CVars.Aimbot.HitScan ) ? nHitboxes : 1;

			// lethal body aim - from sega inspiration, if body shot is lethal, aim body for higher hit chance
			if( g_CVars.Aimbot.Interpolation.LethalBody && !bFound )
			{
				// if enemy health low or body damage lethal, prioritize body
				int health = Ent->m_iHealth( );
				bool bLowHealth = ( health <= 50 ); // low health = lethal body
				// also check if any body hitbox does lethal damage
				static const int lethalBodyHitboxes[] = { 5, 9, 10, 0, 1, 11 }; // chest, stomach, neck
				for( int lb=0; lb<6 && !bFound; lb++ )
				{
					int hb = lethalBodyHitboxes[lb];
					GetHitbox( hb, Ent );
					VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
					VectorNormalizeFast( vecDirection );
					int dmg = 0;
					BaseEntity* pHit = nullptr;
					if( g_CVars.Aimbot.AutoWall )
					{
						dmg = GetTotalDamage( LocalPlayer, Weapon, &pHit );
						if( pHit != Ent ) dmg = 0;
					}
					else
					{
						if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
							dmg = 100; // visible = assume lethal if low health
					}
					if( dmg >= health || ( bLowHealth && dmg >= 30 ) )
					{
						// lethal body found, use it
						if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
						{
							vecPoint = vecCorners[ 0 ];
							bFound = true;
							break;
						}
						if( g_CVars.Aimbot.AutoWall && dmg >= g_CVars.Aimbot.MinDamage )
						{
							vecPoint = vecCorners[ 0 ];
							bFound = true;
							break;
						}
					}
				}
			}

			// legit: no autowall, only visible, but respect groups
			if( bIsLegit && g_CVars.Aimbot.VisOnly )
			{
				// for legit, scan only enabled groups, visibility only
				for( int h=0; h<nHitboxes && !bFound; h++ )
				{
					GetHitbox( HitboxOrder[h], Ent );
					VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
					VectorNormalizeFast( vecDirection );
					if( CheckVisible( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
					{
						vecPoint = vecCorners[ 0 ];
						bFound = true;
					}
					// for legit with multispot, also check extra points for head only
					if( !bFound && g_CVars.Aimbot.MultiSpot && HitboxOrder[h]==12 )
					{
						for( int c=1;c<=4;c++ )
						{
							VectorSubtract( vecCorners[c], EyePosition, vecDirection );
							VectorNormalizeFast( vecDirection );
							if( CheckVisible( EyePosition, vecCorners[c], Ent, LocalPlayer ) )
							{
								vecPoint = vecCorners[c];
								bFound = true;
								break;
							}
						}
					}
				}
			}
			else
			{
				if( !bFound )
					bFound = ScanTarget( LocalPlayer, Ent, HitboxOrder, nSpots, g_CVars.Aimbot.MultiSpot, vecPoint );
			}
		}

		if( !bFound ) continue;

		// exact FOV check against the actual aim point
		if( effectiveFov < 180.f && AngleDeviation( vecPoint, pCmd ) > effectiveFov ) continue;

		// legit reaction time: don't snap instantly to new target
		if( bIsLegit && g_CVars.Aimbot.ReactionTime > 0.f )
		{
			if( lastTarget != i )
			{
				firstSeenTime = curTime;
				lastTarget = i;
				continue; // wait a bit before aiming at new target
			}
			float reactionSec = g_CVars.Aimbot.ReactionTime / 1000.f;
			if( curTime - firstSeenTime < reactionSec )
				continue;
		}

		TargetIndex = i;
		Temp = rate;

		if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
		{
			Vector vecDelta;
			VectorSubtract( vecPoint, EyePosition, vecDelta );
			VectorNormalizeFast( vecDelta );
			VectorAngles( vecDelta, qFinalAngle );

			Deviation = AngleDeviation( vecPoint, pCmd );
			IsAimbotting = true;
		}
	}

	if( IsAimbotting && TargetIndex != -1 )
	{
		ApplyAngles( pCmd );

		// while smoothing we are not instantly on target, so only shoot once
		// the crosshair is close enough to the aim point
		float onTargetThresh = bIsLegit ? 1.5f : 2.f;
		if( bIsLegit && g_CVars.Aimbot.Humanize )
			onTargetThresh += RandomFloat( 0.f, 0.8f );

		bool bOnTarget = ( g_CVars.Aimbot.Silent || flSmooth <= 1.f || Deviation <= onTargetThresh );

		// legit auto delay
		if( bIsLegit && g_CVars.Aimbot.AutoDelay && bOnTarget )
		{
			float delaySec = g_CVars.Aimbot.AutoDelayTime / 1000.f;
			if( curTime - firstSeenTime < ( g_CVars.Aimbot.ReactionTime/1000.f + delaySec ) )
				bOnTarget = false;
		}

		if( g_CVars.Aimbot.AutoShoot && bOnTarget )
		{
			pCmd->buttons |= IN_ATTACK;
			lastAimTime = curTime;

			// improved aimbot: auto stop for better accuracy when shooting (rage)
			if( !bIsLegit )
			{
				// stop movement for better accuracy
				pCmd->forwardmove = 0.f;
				pCmd->sidemove = 0.f;
				pCmd->upmove = 0.f;
			}
		}
	}

	if( g_CVars.Aimbot.TargetSelection == 3 ) next_shot = TargetIndex;

	if( IsAimbotting )
	{
		if( g_CVars.Aimbot.TargetSelection != 3 ) next_shot = TargetIndex;
	}
	else
	{
		// reset reaction timer when not aimbotting
		if( bIsLegit )
		{
			if( curTime - lastAimTime > 0.5f )
				lastTarget = -1;
		}
		if( Weapon->ShouldReload( ) ) pCmd->buttons |= IN_RELOAD;
	}

	if( !g_CVars.Aimbot.Silent ) g_pEngineClient->SetViewAngles( pCmd->viewangles );
}
