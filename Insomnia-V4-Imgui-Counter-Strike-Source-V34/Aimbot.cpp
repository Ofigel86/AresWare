// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r40 (2026-09-19): lag records - per-record anti-jitter resolve (phase prediction + miss brute).
// BUILD MARKER r37 (2026-09-18): rage audit fixes - mins/maxs [65] (OOB), Fallback Hitbox exclusive, hitgroup clamp.
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r23 (2026-09-18): exact-seed hitchance (256-seed census + ForceSeed-aware) + ForceSeed prefers hit seeds + Lua: on_shot, draw.get_screen_size, utils.latency/choke, ents.eye_angles/hitbox.
// BUILD MARKER r12 (2026-09-18): best-of-256 ForceSeed picker + AutoWall corner points + menu polish (rounding/banner gradient/section accents).
// BUILD MARKER r11 (2026-09-18): hitchance fire gate on live-shot commit + enemy fakelag sensor (EnemyChoke) + resolver-adaptive body hitbox.
// BUILD MARKER r10 (2026-09-18): server-aligned backtrack record pick + choke-aware punch decay (SDK cs_gamemovement DecayPunchAngle).
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"

Aimbot g_Aimbot;

static int EffectiveMinDamage( void ); // fwd

bool Aimbot::CheckVisible( Vector vecAbsStart, Vector vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer ) // r47: by value
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return false;

	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities TraceFilter( Target, 0 );

	// todo: proper vis check

	Ray.Init( vecAbsStart, vecAbsEnd );
	g_pEngineTrace->TraceRay( Ray, 0x46004003, ( ITraceFilter* )&TraceFilter, &Trace );
	return ( Trace.fraction == 1.f );

}

bool Aimbot::CheckVisibleAWallCheck( Vector vecAbsStart, Vector vecAbsEnd, BasePlayer* Target, BasePlayer* LocalPlayer ) // r47: by value
{
	trace_t Trace;
	Ray_t Ray;
	TraceFilterSkipTwoEntities traceFilter( Target, 0 );

	// FPS: cheap direct trace first - full penetration sim only when blocked
	Ray.Init( vecAbsStart, vecAbsEnd );
	g_pEngineTrace->TraceRay( Ray, 0x46004003, ( ITraceFilter* )&traceFilter, &Trace );

	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );

	// direct visible: honor MinDamage too (0 = accept anything, as before)
	if( Trace.fraction == 1.f )
	{
		if( ( EffectiveMinDamage( ) <= 0 && !g_CVars.Aimbot.BestDamage ) || !Weapon ) return true;
		BaseEntity* pHit = nullptr;
		int dmg = GetTotalDamage( LocalPlayer, Weapon, &pHit );
		if( dmg < EffectiveMinDamage( ) ) return false;
		if( g_CVars.Aimbot.BestDamage )
		{
			if( dmg < iBestDamage ) return false; // ties overwrite: priority order wins
			iBestDamage = dmg;
		}
		return true;
	}

	if( !g_CVars.Aimbot.AutoWall ) return false;
	if( !Weapon ) return false;

	BaseEntity* pPlayerHit = nullptr;
	int dmg = GetTotalDamage( LocalPlayer, Weapon, &pPlayerHit );
	if( dmg < EffectiveMinDamage( ) ) return false;
	if( g_CVars.Aimbot.BestDamage )
	{
		if( dmg < iBestDamage ) return false; // ties overwrite: priority order wins
		iBestDamage = dmg;
	}
	return true;
}

// damage of the shot along the current vecDirection (caller must set vecDirection first)
int Aimbot::PointDamage( BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon ) return 0;
	BaseEntity* pHit = nullptr;
	return GetTotalDamage( LocalPlayer, Weapon, &pHit );
}

// MinDamage floor + best-damage tracking for plain-visibility corner points
bool Aimbot::PassesMinDamage( BasePlayer* LocalPlayer )
{
	int dmg = 0;
	if( EffectiveMinDamage( ) > 0 || g_CVars.Aimbot.BestDamage )
		dmg = PointDamage( LocalPlayer );
	if( dmg < EffectiveMinDamage( ) ) return false;
	if( g_CVars.Aimbot.BestDamage )
	{
		if( dmg < iBestDamage ) return false; // ties overwrite: priority order wins
		iBestDamage = dmg;
	}
	return true;
}

// FPS: lag-record apply + SetupBones run ONCE per enemy per tick.
// The returned hitbox set and matrix are reused for every hitbox/point scanned.
mstudiohitboxset_t* Aimbot::SetupAimBones( BasePlayer* Entity, matrix3x4_t* matrix, int recordIdx, bool storeBackup )
{
	if( g_CVars.Aimbot.Interpolation.LagPrediction )
	{
		if( storeBackup ) g_Stuff.StoreTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
		if( recordIdx < 0 ) recordIdx = 0; if( recordIdx > 31 ) recordIdx = 31;
		g_Stuff.ApplyTickRecord( Entity, &pPlayerHistory[ Entity->entindex( ) ][ recordIdx ] );

		int m_iAccumulatedBoneMask = *( int* )( ( DWORD ) Entity + 0x49C + 0x4 );
		int m_nReadableBones = *( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 );
		int m_nWritableBones = *( int* )( ( DWORD ) Entity + 0x4AC + 0x4 );
		int m_iPrevBoneMask = *( int* )( ( DWORD ) Entity + 0x498 + 0x4 );

		*( int* )( ( DWORD ) Entity + 0x4A8 + 0x4 ) = 0;							// baseanimating + 0x4A8
		*( int* )( ( DWORD ) Entity + 0x4AC + 0x4 ) = 0;							// baseanimating + 0x4AC
		*( int* )( ( DWORD ) Entity + 0x498 + 0x4 ) = m_iAccumulatedBoneMask;	// baseanimating + 0x498
		*( int* )( ( DWORD ) Entity + 0x49C + 0x4 ) = 0;							// baseanimating + 0x49C
	}

	// r40: lag records - per-record anti-jitter resolve. Tick records keep the pose
	// captured at network-update time; a per-tick jitterer flips server-side every
	// tick, so records from a choked burst carry the wrong flick phase. Predict the
	// cluster side (A/B) for the claimed server tick and rebuild bones on it.
	float lagSavedYaw = Entity->m_angEyeAngles( ).y;
	bool lagApplied = false;
	{
		int idxL = Entity->entindex( );
		if( g_CVars.Aimbot.Resolver.Active && g_CVars.Aimbot.Resolver.LagRecords
			&& idxL > 0 && idxL < 65 && g_CVars.Aimbot.Resolver.Jitter[ idxL ] )
		{
			float flickW = g_Stuff.GuwopNormalize( g_CVars.Aimbot.Resolver.JitterB[ idxL ] - g_CVars.Aimbot.Resolver.JitterA[ idxL ] );
			if( flickW < 0.f ) flickW = -flickW;
			if( flickW <= 100.f ) // wide flick: eyes are fake, feet aren't - keep capture pose
			{
				int tickLag;
				if( g_CVars.Aimbot.Interpolation.LagPrediction && recordIdx > 0 && pPlayerHistory[ idxL ][ 0 ].m_SimulationTime > 0.f )
					tickLag = TIME_TO_TICKS( pPlayerHistory[ idxL ][ 0 ].m_SimulationTime - Entity->m_flSimulationTime( ) );
				else
					tickLag = -TIME_TO_TICKS( g_pGlobals->curtime - Entity->m_flSimulationTime( ) );
				if( tickLag < 0 ) tickLag = -tickLag;
				bool side = g_CVars.Aimbot.Resolver.LagSide[ idxL ];
				if( tickLag & 1 ) side = !side; // per-tick flip: parity walks to the claimed tick
				if( g_CVars.Aimbot.Resolver.LagPhaseBrute[ idxL ] ) side = !side; // miss-driven phase brute
				float lagPredicted = side ? g_CVars.Aimbot.Resolver.JitterB[ idxL ] : g_CVars.Aimbot.Resolver.JitterA[ idxL ];
				if( lagPredicted == lagPredicted ) // NaN guard
				{
					Entity->m_angEyeAngles( ).y = lagPredicted;
					lagApplied = true;
				}
			}
		}
	}
	if( !( Entity->SetupBones( matrix, 128, 0x100, Entity->m_flSimulationTime( ) ) ) ) { RestoreAimBones( Entity ); return nullptr; }
	if( lagApplied ) Entity->m_angEyeAngles( ).y = lagSavedYaw; // r40: hand the fresh-resolve yaw back
	void* pModel = Entity->GetModel( );
	if( !pModel ) { RestoreAimBones( Entity ); return nullptr; }
	studiohdr_t* studiohdr = g_pModelInfo->GetStudiomodel( pModel );
	if( !studiohdr ) { RestoreAimBones( Entity ); return nullptr; }
	mstudiohitboxset_t* studiohitboxset = studiohdr->pHitboxSet( Entity->m_nHitboxSet( ) );
	if( !studiohitboxset ) { RestoreAimBones( Entity ); return nullptr; }
	return studiohitboxset;
}

void Aimbot::RestoreAimBones( BasePlayer* Entity )
{
	if( g_CVars.Aimbot.Interpolation.LagPrediction ) g_Stuff.ApplyTickRecord( Entity, &pBackupData[ Entity->entindex( ) ] );
}

// multipoint corners for one hitbox from the cached bone matrix (no SetupBones here)
bool Aimbot::GetHitboxPoints( int iHitbox, BasePlayer* Entity, matrix3x4_t* matrix, mstudiohitboxset_t* studiohitboxset )
{
	if( !studiohitboxset || iHitbox < 0 || iHitbox > 18 ) return false;
	mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( iHitbox );
	if( !studiobbox ) return false;

	float scalecenter = g_pGlobals->interval_per_tick * g_CVars.Aimbot.PointScale;

	mins[ Entity->entindex( ) ] = studiobbox->bbmin;
	maxs[ Entity->entindex( ) ] = studiobbox->bbmax;

	Vector points[ ] = { ( ( studiobbox->bbmin + studiobbox->bbmax ) * .5f ),
		Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ) };

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
			if( g_CVars.Aimbot.HitboxMode == 2 || g_CVars.Aimbot.HitboxMode == 3 )
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
			else if( g_CVars.Aimbot.HitboxMode == 4 )
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
		}
	}
	else points[ 0 ] += points[ 0 ] * .5f;

	for( int index = 0; index <= 8; ++index ) VectorTransform( points[ index ], matrix[ studiobbox->bone ], vecCorners[ index ] );
	return true;
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
	if( hitgroup < 0 || hitgroup > 7 ) hitgroup = 0; // r37: HITGROUP_GEAR(8) would read OOB
	return( dmg * hitgroupModifiers[ hitgroup ] );
}

int Aimbot::GetTotalDamage( BaseEntity* LocalPlayer, CSWeapon* Weapon, BaseEntity** ppPlayerHit )
{
	if( !LocalPlayer ) return -1;

	trace_t traceData, wallTraceData;
	Ray_t ray;

	WeaponInfo WeaponInfo = g_NoSpread.GetWeaponInfo( Weapon );

	int currentPenetration = WeaponInfo.Penetration;
	float currentPenetrationPower = WeaponInfo.PenetrationPower;
	float currentDamage = ( float )WeaponInfo.Damage;
	float currentMaxRange = WeaponInfo.MaxRange;

	Vector start = EyePosition, wall;
	Vector end;
	float tracedDistance = 0.f;
	float multiplier = 24.f;
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

		if( !isGrate ) Weapon->GetMaterialParameters( material, penetrationPowerModifier, damageModifier );
		if( tmpDistance > WeaponInfo.PenetrationRange ) currentPenetration = ( currentPenetration <= 0 ) ? currentPenetration : 0;
		if( ( currentPenetration < 0 ) || ( ( currentPenetration == 0 ) && !isGrate ) ) break;

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

// HitChance: simulate 64 spread seeds along the final aim angles,
// return % of seeds whose bullet path reaches the target entity.
int Aimbot::GetHitChance( CUserCmd* pCmd, BasePlayer* LocalPlayer, BasePlayer* Target, CSWeapon* Weapon )
{
	if( !pCmd || !LocalPlayer || !Target || !Weapon ) return 0;

	float flSpread = Weapon->GetSpread( );
	if( flSpread <= 0.0001f )
	{
		// effectively no spread: visibility decides
		Vector end = EyePosition + vecDirection * 8192.f;
		return CheckVisible( EyePosition, end, Target, LocalPlayer ) ? 100 : 0;
	}

	Vector forward, right, up;
	AngleVectors( pCmd->viewangles, &forward, &right, &up );
	TraceFilterSkipTwoEntities traceFilter( LocalPlayer, 0 );

	// r23: full census over the 256 seeds the engine actually uses
	// (MD5(command_number) & 255). With ForceSeed enabled the pick is
	// deterministic, so if ANY seed can hit, the forced shot lands: 100.
	int hits = 0;
	for( int seed = 0; seed < 256; seed++ )
	{
		RandomSeed( seed + 1 );
		float sx = ( RandomFloat( -0.5f, 0.5f ) + RandomFloat( -0.5f, 0.5f ) ) * flSpread;
		float sy = ( RandomFloat( -0.5f, 0.5f ) + RandomFloat( -0.5f, 0.5f ) ) * flSpread;
		Vector dir = forward + sx * right + sy * up;
		VectorNormalizeFast( dir );
		Vector end = EyePosition + dir * 8192.f;

		Ray_t ray; ray.Init( EyePosition, end );
		trace_t tr;
		g_pEngineTrace->TraceRay( ray, 0x46004003, ( ITraceFilter* )&traceFilter, &tr );
		if( tr.m_pEnt == Target ) hits++;
	}
	if( g_CVars.Accuracy.ForceSeed ) return ( hits > 0 ) ? 100 : 0; // r23 deterministic
	return hits * 100 / 256; // r23 exact probability (was 32-sample estimate)
}

void Aimbot::Reset( )
{
	Distance = INT_MAX;
	Temp = INT_MAX;
	TargetIndex = -1;
	for( int i = 0; i < 65; i++ ) BacktrackRecord[ i ] = 0;
	vecDirection.Init( 0, 0, 0 );
	for( int i = 0; i <= 8; ++i ) vecCorners[ i ].Init( 0, 0, 0 );
	IsAimbotting = false;
	qFinalAngle = QAngle( 0, 0, 0 );
}

// hitbox -> group map (from player skeleton):
// 0 pelvis, 1-8 legs, 9 stomach, 10 chest, 11 neck, 12 head, 13-18 arms.
// Groups: 0 Head, 1 Neck, 2 Chest, 3 Stomach, 4 Pelvis, 5 Arms, 6 Legs.
// Mode: 0 Off, 1 Scan, 2 Priority. All-zero (fresh config) = Scan everything.
static int AimGroupMode( int hb )
{
	static const int hbGroup[ 19 ] = { 4, 6, 6, 6, 6, 6, 6, 6, 6, 3, 2, 1, 0, 5, 5, 5, 5, 5, 5 };
	if( hb < 0 || hb > 18 ) return 0;
	bool anyCfg = false;
	for( int g = 0; g < 7; g++ ) if( g_CVars.Aimbot.HitboxGroup[ g ] != 0 ) { anyCfg = true; break; }
	if( !anyCfg ) return 1;
	return g_CVars.Aimbot.HitboxGroup[ hbGroup[ hb ] ];
}

// rage force-keys (hold): body-aim override + min-damage override value
static bool ForceBodyDown( void ) { return ComboKeyDown( g_CVars.Aimbot.ForceBodyKey ); }
static int EffectiveMinDamage( void )
{
	if( ComboKeyDown( g_CVars.Aimbot.ForceMinDmgKey ) ) return g_CVars.Aimbot.ForceMinDmgValue;
	return g_CVars.Aimbot.MinDamage;
}

int next_shot;
int Rate( BasePlayer* LocalPlayer, BasePlayer* Ent )
{
	int rate = 0;

	if( g_CVars.Aimbot.TargetSelection == 0 ) rate = LocalPlayer->GetAbsOrigin( ).DistTo( Ent->GetAbsOrigin( ) ); // distance
	if( g_CVars.Aimbot.TargetSelection == 1 ) rate = Ent->m_iHealth( ); // health
	if( g_CVars.Aimbot.TargetSelection == 2 || g_CVars.Aimbot.TargetSelection == 3 ) rate = Ent->entindex( ) > next_shot ? 0 : 1; // next shot, random
	if( g_CVars.Aimbot.TargetSelection == 4 ) // crosshair: angular distance from view
	{
		Vector delta = Ent->EyePosition( ) - EyePosition;
		QAngle aimAng, viewAng = LocalPlayer->m_angEyeAngles( );
		VectorAngles( delta, aimAng );
		float dx = g_Stuff.GuwopNormalize( aimAng.x - viewAng.x ); if( dx < 0.f ) dx = -dx;
		float dy = g_Stuff.GuwopNormalize( aimAng.y - viewAng.y ); if( dy < 0.f ) dy = -dy;
		rate = ( int )( ( dx + dy ) * 100.f );
	}

	return rate;
}

// r45: v34 weapon id -> rage group index (WeaponIDsCSS from NoSpread.h). -1 = global.
int Aimbot_RageGroupForWeapon( BasePlayer* LocalPlayer )
{
	if( !g_CVars.Aimbot.RageGroups || !LocalPlayer ) return -1;
	CSWeapon* rgWpn = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !rgWpn ) return -1;
	switch( rgWpn->GetWeaponID( ) )
	{
		case WEAPON_P228: case WEAPON_GLOCK: case WEAPON_ELITES: case WEAPON_FIVESEVEN: case WEAPON_USP: case WEAPON_DEAGLE:
			return 0; // pistols
		case WEAPON_MAC10: case WEAPON_UMP: case WEAPON_MP5: case WEAPON_TMP: case WEAPON_P90:
			return 1; // smg
		case WEAPON_AUG: case WEAPON_GALIL: case WEAPON_FAMAS: case WEAPON_M4A1: case WEAPON_SG552: case WEAPON_AK47: case WEAPON_M249:
			return 2; // rifles (+ mg)
		case WEAPON_XM1014: case WEAPON_M3:
			return 3; // shotguns
		case WEAPON_SCOUT: case WEAPON_AWP: case WEAPON_SG550: case WEAPON_G3SG1:
			return 4; // snipers
		default:
			return -1; // knife / grenades / c4 -> global settings
	}
}

void Aimbot::Main( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( !Weapon || !Weapon->IsWeapon( ) ) return;

	WeaponInfo wpnInfo = g_NoSpread.GetWeaponInfo( Weapon );

	Reset( );

	static int Choose[ ] = { 12, 11, 5, 0, 1, 9, 10, 13, 14, 16, 17, 18, 8, 7, 6, 4, 3, 2, 15 };

	int m_iWeaponID = Weapon->GetWeaponID( );

	int iSpot;
	if( m_iWeaponID == 17 )
	{
		if( g_CVars.Aimbot.BodyAWP ) iSpot = 10;
		else iSpot = g_CVars.Aimbot.Hitbox;
	}
	else iSpot = g_CVars.Aimbot.Hitbox;
	// FIX r8: raw ids are 9=stomach 10=chest 11=neck 12=head. Stale configs load
	// Hitbox=0 (menu still shows "Head") which aimed a body hitbox - "always body".
	if( iSpot < 9 || iSpot > 12 ) iSpot = 12;

	if( g_CVars.Aimbot.Key > 0 && !ComboKeyDown( g_CVars.Aimbot.Key ) ) return; // VK map fix: raw index broke Mouse 3+

	// hitscan extras come from hitbox groups (AimGroupMode); primary is always scanned

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
		int rate = Rate( LocalPlayer, Ent );
		if( rate > Temp ) continue;

		// FPS: FOV cone pre-check - skip enemies outside the cone before any bones/traces
		int aimFov = g_CVars.Aimbot.AimFOV;
		if( aimFov < 0 ) aimFov = 0; if( aimFov > 180 ) aimFov = 180;
		if( aimFov > 0 )
		{
			Vector toEnt = Ent->EyePosition( ) - EyePosition;
			QAngle angTo; VectorAngles( toEnt, angTo );
			float fdx = g_Stuff.GuwopNormalize( angTo.x - pCmd->viewangles.x ); if( fdx < 0.f ) fdx = -fdx;
			float fdy = g_Stuff.GuwopNormalize( angTo.y - pCmd->viewangles.y ); if( fdy < 0.f ) fdy = -fdy;
			if( ( fdx + fdy ) > ( float )aimFov ) continue;
		}

		// per-enemy primary hitbox (whitelist head / body vs jumping)
		int primary = iSpot;
		if( g_Whitelist.List( i ) ) primary = 12;
		if( !g_CVars.Aimbot.StrictPrimary && g_CVars.Aimbot.BodyVsJump && !( Ent->m_fFlags( ) & FL_ONGROUND ) && AimGroupMode( 9 ) != 0 ) primary = 9;
		if( ForceBodyDown( ) ) primary = 10; // force-key: chest over everything (scan fallback unchanged)

		// r11 adaptive body: a desyncing enemy (yaw jitter / spin / own fakelag sensor)
		// makes the thin head a lottery - fall back to chest. Respects StrictPrimary
		// and the per-player whitelist above (head stays forced for whitelisted).
		if( !g_CVars.Aimbot.StrictPrimary && !g_Whitelist.List( i ) &&
			g_CVars.Aimbot.Resolver.Active &&
			( g_CVars.Aimbot.Resolver.Jitter[ i ] || g_CVars.Aimbot.Resolver.AnimSpin[ i ] ||
			  g_CVars.Aimbot.Resolver.EnemyChoke[ i ] >= 3 ) )
			primary = 10;

		// long range: big forgiving target (head is ~20u wide at 1300u, chest 3x that)
		float rangeDist = Ent->m_vecOrigin( ).DistTo( EyePosition );
		bool longRange = ( g_CVars.Aimbot.LongRangeDist > 0 && rangeDist > ( float )g_CVars.Aimbot.LongRangeDist );
		if( longRange && !g_CVars.Aimbot.StrictPrimary && AimGroupMode( 10 ) != 0 ) primary = 10; // chest

		// FPS: bones once per enemy, reused for every hitbox scanned
		matrix3x4_t matrix[ 128 ];
		mstudiohitboxset_t* hitboxSet = SetupAimBones( Ent, matrix, 0, true );
		if( !hitboxSet ) continue;

		bool enemyDone = false;
		iBestDamage = -1;

		if( m_iWeaponID == 17 ) // awp: single point
		{
			if( GetHitboxPoints( primary, Ent, matrix, hitboxSet ) &&
				CheckVisibleAWallCheck( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
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
				enemyDone = true;
			}
		}
		else
		{
			// Phase 1: primary hitbox center. A full scan would end with these
			// exact angles when the primary is hittable, so skip the scan.
			// r25: with BestDamage the shortcut would take the center WITHOUT
			// comparing corner damage - run the full scan instead so BestDamage
			// really picks the best point. StrictPrimary keeps the shortcut.
			if( ( !g_CVars.Aimbot.BestDamage || g_CVars.Aimbot.StrictPrimary ) && GetHitboxPoints( primary, Ent, matrix, hitboxSet ) )
			{
				VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
				VectorNormalizeFast( vecDirection );

				if( CheckVisibleAWallCheck( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
				{
					if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
					{
						VectorAngles( vecDirection, pCmd->viewangles );
						IsAimbotting = true;
					}

					TargetIndex = i;
					Temp = rate;
					enemyDone = true;
				}
			}

			// Phase 2: primary blocked - scan the rest (priority order; last wins, or best damage)
			if( !enemyDone && !g_CVars.Aimbot.StrictPrimary )
			{
				// scan order: Scan-group hitboxes first, Priority-group last (later overwrites = wins)
				int scanList[ 20 ]; int scanCount = 0;
				// r25: explicit fallback hitbox overrides the group scan set
				{
					int fb = g_CVars.Aimbot.FallbackHitbox;
					if( fb < 9 || fb > 12 ) fb = 0;
					if( fb ) { scanList[ 0 ] = fb; scanCount = 1; }
				}
				for( int pass = 1; pass <= 2 && scanCount == 0; pass++ ) // r37: explicit fallback replaces the group scan (r25 spec)
				{
					for( int idx = 18; idx >= 0; idx-- )
					{
						int shb = Choose[ idx ];
						if( shb == primary ) { if( pass == 2 ) scanList[ scanCount++ ] = shb; continue; }
						if( AimGroupMode( shb ) == pass ) scanList[ scanCount++ ] = shb;
					}
				}
				if( g_CVars.Aimbot.MultiSpot && !longRange ) // far: centers only
				{
					if( g_CVars.Aimbot.HitScan )
					{
						// multipoint corners over enabled hitboxes (primary corners included)
						for( int sl = 0; sl < scanCount; sl++ )
						{
							int hb = scanList[ sl ]; // primary corners included (primary is always in Choose)
							if( !GetHitboxPoints( hb, Ent, matrix, hitboxSet ) ) continue;

							for( int m_iCorners = 8; m_iCorners > 0; m_iCorners-- )
							{
								VectorSubtract( vecCorners[ m_iCorners ], EyePosition, vecDirection );
								VectorNormalizeFast( vecDirection );

								if( CheckVisibleAWallCheck( EyePosition, vecCorners[ m_iCorners ], Ent, LocalPlayer ) ) // r12: corners go through the full damage eval too (AutoWall bangs through corners, not only centers)
								{
									if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
									{
										VectorAngles( vecDirection, pCmd->viewangles );
										IsAimbotting = true;
									}

									TargetIndex = i;
									Temp = rate;
								}
							}
						}

						// centers over enabled hitboxes (primary center already failed)
						for( int sl = 0; sl < scanCount; sl++ )
						{
							int hb = scanList[ sl ];
							if( hb == primary ) continue;
							if( !GetHitboxPoints( hb, Ent, matrix, hitboxSet ) ) continue;

							VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
							VectorNormalizeFast( vecDirection );

							if( CheckVisibleAWallCheck( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
							{
								if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
								{
									VectorAngles( vecDirection, pCmd->viewangles );
									IsAimbotting = true;
								}

								TargetIndex = i;
								Temp = rate;
							}
						}
					}
					else
					{
						// multipoint corners of the primary hitbox (center already failed)
						if( GetHitboxPoints( primary, Ent, matrix, hitboxSet ) )
						{
							for( int m_iCorners = 8; m_iCorners > 0; m_iCorners-- )
							{
								VectorSubtract( vecCorners[ m_iCorners ], EyePosition, vecDirection );
								VectorNormalizeFast( vecDirection );

								if( CheckVisibleAWallCheck( EyePosition, vecCorners[ m_iCorners ], Ent, LocalPlayer ) ) // r12: corners go through the full damage eval too (AutoWall bangs through corners, not only centers)
								{
									if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
									{
										VectorAngles( vecDirection, pCmd->viewangles );
										IsAimbotting = true;
									}

									TargetIndex = i;
									Temp = rate;
								}
							}
						}
					}
				}
				else
				{
					if( g_CVars.Aimbot.HitScan )
					{
						// centers over enabled hitboxes (primary center already failed)
						for( int sl = 0; sl < scanCount; sl++ )
						{
							int hb = scanList[ sl ];
							if( hb == primary ) continue;
							if( !GetHitboxPoints( hb, Ent, matrix, hitboxSet ) ) continue;

							VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
							VectorNormalizeFast( vecDirection );

							if( CheckVisibleAWallCheck( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
							{
								if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
								{
									VectorAngles( vecDirection, pCmd->viewangles );
									IsAimbotting = true;
									}

								TargetIndex = i;
								Temp = rate;
							}
						}
					}
				}
			}
		}

		// Backtrack: primary blocked on the newest record - scan older records, newest first.
		// Only the primary center is tested per record (cheap); CorrectTickCount rewinds the server.
		if( !enemyDone && g_CVars.Aimbot.Interpolation.LagPrediction )
		{
			int maxTicks = g_CVars.Aimbot.BacktrackTicks;
			if( maxTicks < 0 ) maxTicks = 0; if( maxTicks > 12 ) maxTicks = 12;
			if( maxTicks > 0 )
			{
				RestoreAimBones( Ent ); // back to current pose before scanning history
				// r10 SERVER-ALIGNED ORDER: the server rewinds to correct_time =
				// outgoing latency + lerp (player_lagcompensation.cpp). A too-fresh
				// record (rec 1) gets snapped server-side to an older pose while our
				// bones/aim came from rec 1 -> misses on movers. Scan from the record
				// the server will ACTUALLY use (closest to correct time), outward.
				int order[ 13 ]; int orderN = 0;
				float lerpTime = 0.f;
				static ConVar* btClInterp = g_pCvar->FindVar( "cl_interp" );
				if( btClInterp ) lerpTime = btClInterp->GetFloat( );
				INetChannelInfo* btNci = g_pEngineClient->GetNetChannelInfo( );
				float btCorrect = ( btNci ? btNci->GetLatency( 0 ) : 0.f ) + lerpTime;
				int centerRec = ( g_pGlobals->interval_per_tick > 0.f ) ? ( int )( btCorrect / g_pGlobals->interval_per_tick + 0.5f ) : 1;
				if( centerRec < 1 ) centerRec = 1;
				if( centerRec > maxTicks ) centerRec = maxTicks;
				order[ orderN++ ] = centerRec;
				for( int off = 1; off < maxTicks && orderN < 13; off++ )
				{
					if( centerRec - off >= 1 ) order[ orderN++ ] = centerRec - off;
					if( centerRec + off <= maxTicks && orderN < 13 ) order[ orderN++ ] = centerRec + off;
				}
				for( int oi = 0; oi < orderN && !enemyDone; oi++ )
				{
					int rec = order[ oi ];
					if( pPlayerHistory[ i ][ rec ].m_SimulationTime == 0.f ) continue; // gap in history: try the next candidate
					// FPS: test the record eye (autowall) BEFORE the expensive SetupBones
					Vector recEye = pPlayerHistory[ i ][ rec ].m_Origin;
					recEye.z += ( pPlayerHistory[ i ][ rec ].m_Flags & FL_DUCKING ) ? 45.f : 62.f;
					int savedBest = iBestDamage; // pretest is a gate, not a candidate
					if( !CheckVisibleAWallCheck( EyePosition, recEye, Ent, LocalPlayer ) ) { iBestDamage = savedBest; continue; }
					iBestDamage = savedBest;
					mstudiohitboxset_t* btSet = SetupAimBones( Ent, matrix, rec, false );
					if( !btSet ) { RestoreAimBones( Ent ); continue; }
					if( GetHitboxPoints( primary, Ent, matrix, btSet ) )
					{
						VectorSubtract( vecCorners[ 0 ], EyePosition, vecDirection );
						VectorNormalizeFast( vecDirection );
						if( CheckVisibleAWallCheck( EyePosition, vecCorners[ 0 ], Ent, LocalPlayer ) )
						{
							if( g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )
							{
								VectorAngles( vecDirection, pCmd->viewangles );
								IsAimbotting = true;
							}
							TargetIndex = i;
							Temp = rate;
							BacktrackRecord[ i ] = rec;
							enemyDone = true;
						}
					}
					RestoreAimBones( Ent );
				}
			}
		}

		RestoreAimBones( Ent );
	}

	if( IsAimbotting && TargetIndex != -1 )
	{
		if( g_CVars.Aimbot.AutoShoot )
		{
			bool canShoot = true;
			if( g_CVars.Aimbot.HitChance )
			{
				BasePlayer* Target = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( TargetIndex );
				if( Target ) canShoot = ( GetHitChance( pCmd, LocalPlayer, Target, Weapon ) >= g_CVars.Aimbot.HitChanceValue );
			}
			if( canShoot ) pCmd->buttons |= IN_ATTACK;
		}
	}

	if( g_CVars.Aimbot.TargetSelection == 3 ) next_shot = TargetIndex;

	if( IsAimbotting )
	{
		if( g_CVars.Aimbot.TargetSelection != 3 ) next_shot = TargetIndex;
	}
	else if( Weapon->ShouldReload( ) ) pCmd->buttons |= IN_RELOAD;

	if( !g_CVars.Aimbot.Silent ) g_pEngineClient->SetViewAngles( pCmd->viewangles );
}
