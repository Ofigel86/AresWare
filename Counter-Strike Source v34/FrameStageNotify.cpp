#include "Main.h"

// Improved resolver for CSS v34
// Groups: Head, Neck, Chest, Stomach, Arms, Legs are already handled in aimbot
// Resolver now handles anti-aim yaw/pitch with pattern detection, bruteforce, velocity, LBY

static float GetVelocityYaw( BasePlayer* ent )
{
	Vector vel = ent->m_vecVelocity();
	if( vel.Length2D() < 1.f ) return 0.f;
	return RAD2DEG( atan2f( vel.y, vel.x ) );
}

static float NormalizeYaw( float yaw )
{
	yaw = g_Stuff.GuwopNormalize( yaw );
	return yaw;
}

// Improved yaw guess with 9 types
static float ResolveYawGuess( int nType, int nIndex, float flBaseYaw, BasePlayer* ent )
{
	// access per-player resolver state
	auto& res = g_CVars.Aimbot.Resolver;
	float velYaw = GetVelocityYaw( ent );
	bool isMoving = ent->m_vecVelocity().Length2D() > 1.f;

	// update moving tracking
	if( isMoving )
	{
		res.LastMovingYaw[nIndex] = flBaseYaw;
		res.LastVelocityYaw[nIndex] = velYaw;
		res.IsMoving[nIndex] = true;
	}
	else
	{
		res.IsMoving[nIndex] = false;
	}

	// spin detection
	float lastYaw = res.LastYaw[nIndex];
	float delta = NormalizeYaw( flBaseYaw - lastYaw );
	res.SpinRate[nIndex] = delta;
	res.LastYaw[nIndex] = flBaseYaw;

	// jitter detection: if delta flips sign rapidly
	if( fabs(delta) > 30.f )
	{
		res.JitterSide[nIndex] = (delta > 0) ? 1 : -1;
	}

	switch( nType )
	{
		case 0: // off - use original
			return flBaseYaw;

		case 1: // bruteforce classic 4-way
		{
			switch( g_iGameTicks % 4 )
			{
				case 0: return flBaseYaw;
				case 1: return flBaseYaw + 90.f;
				case 2: return flBaseYaw + 180.f;
				default: return flBaseYaw - 90.f;
			}
		}

		case 2: // bruteforce 8-way with miss tracking
		{
			int idx = res.BruteforceIndex[nIndex] % 8;
			// if we recently missed, advance
			// g_iBulletsFired is per target, we can use MissedShots to advance
			switch( idx )
			{
				case 0: return flBaseYaw;
				case 1: return flBaseYaw + 90.f;
				case 2: return flBaseYaw + 180.f;
				case 3: return flBaseYaw - 90.f;
				case 4: return flBaseYaw + 45.f;
				case 5: return flBaseYaw - 45.f;
				case 6: return flBaseYaw + 135.f;
				case 7: return flBaseYaw - 135.f;
			}
			return flBaseYaw;
		}

		case 3: // velocity - real yaw follows velocity when moving
		{
			if( isMoving )
				return velYaw;
			else
			{
				// if not moving, use last moving yaw
				if( res.LastMovingYaw[nIndex] != 0.f )
					return res.LastMovingYaw[nIndex];
				return flBaseYaw;
			}
		}

		case 4: // LBY - last moving / lower body
		{
			if( res.LastMovingYaw[nIndex] != 0.f )
				return res.LastMovingYaw[nIndex];
			if( res.LastVelocityYaw[nIndex] != 0.f )
				return res.LastVelocityYaw[nIndex];
			return flBaseYaw;
		}

		case 5: // smart v2 - combined logic
		{
			// if moving, use velocity (most reliable)
			if( isMoving && fabs(velYaw) > 0.1f )
				return velYaw;

			// if jitter detected, resolve to opposite side
			if( fabs(delta) > 35.f )
			{
				// jitter: flip to opposite of last jitter side
				if( res.JitterSide[nIndex] == 1 )
					return flBaseYaw - 90.f;
				else
					return flBaseYaw + 90.f;
			}

			// if spin detected (consistent delta), counter it
			if( fabs(res.SpinRate[nIndex]) > 20.f && fabs(res.SpinRate[nIndex]) < 80.f )
			{
				// spin: guess opposite direction
				return flBaseYaw + res.SpinRate[nIndex] * 2.f;
			}

			// if standing, use LBY or bruteforce based on missed shots
			if( res.LastMovingYaw[nIndex] != 0.f && res.MissedShots[nIndex] < 2 )
				return res.LastMovingYaw[nIndex];

			// bruteforce fallback with 8-way
			int idx = (res.BruteforceIndex[nIndex] + res.MissedShots[nIndex]) % 8;
			switch( idx )
			{
				case 0: return flBaseYaw;
				case 1: return flBaseYaw + 180.f;
				case 2: return flBaseYaw + 90.f;
				case 3: return flBaseYaw - 90.f;
				case 4: return flBaseYaw + 45.f;
				case 5: return flBaseYaw - 45.f;
				case 6: return flBaseYaw + 135.f;
				case 7: return flBaseYaw - 135.f;
			}
			return flBaseYaw;
		}

		case 6: // jitter resolver - tries to catch jitter
		{
			// alternate between 90 and -90 each tick, with detection of side
			if( g_iGameTicks % 2 == 0 )
				return flBaseYaw + 90.f;
			else
				return flBaseYaw - 90.f;
		}

		case 7: // 180 - always backwards
			return flBaseYaw + 180.f;

		case 8: // 90 left / 90 right based on velocity or random
		{
			if( isMoving )
			{
				// if moving left, guess right, etc. Use velocity vs base yaw delta
				float vDelta = NormalizeYaw( velYaw - flBaseYaw );
				if( vDelta > 0 )
					return flBaseYaw - 90.f;
				else
					return flBaseYaw + 90.f;
			}
			return (g_iGameTicks % 2 == 0) ? flBaseYaw + 90.f : flBaseYaw - 90.f;
		}
	}

	return flBaseYaw;
}

static float ResolvePitch( BasePlayer* ent, int nIndex )
{
	float pitch = ent->m_angEyeAngles().x;
	// if pitch is anti-aim (89/-89), try to resolve to 0 or last moving pitch
	if( pitch == 89.f || pitch == -89.f || pitch > 89.f || pitch < -89.f )
	{
		// if moving, pitch is usually 0
		if( ent->m_vecVelocity().Length2D() > 1.f )
			return 0.f;
		// otherwise keep last pitch if valid
		float lastPitch = g_CVars.Aimbot.Resolver.LastPitch[nIndex];
		if( lastPitch != 89.f && lastPitch != -89.f && lastPitch >= -89.f && lastPitch <= 89.f && lastPitch != 0.f )
			return lastPitch;
		return 0.f;
	}
	return pitch;
}

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
			*( float* )( ( DWORD ) LocalPlayer + 0xD14 ) = g_qThirdPerson.y;

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

	static float tempYaw[ 64 ];
	static float tempPitch[ 64 ];

	if( curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START )
	{
		for( auto Index = g_pGlobals->maxClients; Index >= 1; --Index )
		{
			BasePlayer* Entity = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( Index );
			if( Entity == 0 ) continue;
			if( Index == g_pEngineClient->GetLocalPlayer( ) ) continue;
			if( Entity->m_lifeState( ) != 0 ) continue;
			if( Entity->m_iHealth( ) > 0 && Entity->m_iHealth( ) < 500 );
			if( !g_CVars.Aimbot.FriendlyFire )
			{
				if( Entity->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;
			}
			if( Entity->IsDormant( ) ) continue;

			// track shots fired/hit for resolver bruteforce
			if( g_iBulletsFired[ Index ] > g_CVars.Aimbot.Resolver.ShotsFired[ Index ] )
			{
				// new shot fired, check if we hit? For now assume miss if health didn't change? Simplified: increment missed and rotate bruteforce
				// If we have hitmarker or damage, we could check, but use simple logic: after each shot, advance bruteforce if not hit in last 0.5s
				int diff = g_iBulletsFired[ Index ] - g_CVars.Aimbot.Resolver.ShotsFired[ Index ];
				if( diff > 0 )
				{
					// if we fired and didn't get a hit (we don't have hit detection here, so use time-based)
					// For improved resolver, we advance bruteforce every 2 shots missed
					g_CVars.Aimbot.Resolver.MissedShots[ Index ] += diff;
					if( g_CVars.Aimbot.Resolver.MissedShots[ Index ] >= 2 )
					{
						g_CVars.Aimbot.Resolver.BruteforceIndex[ Index ]++;
						g_CVars.Aimbot.Resolver.MissedShots[ Index ] = 0;
					}
					g_CVars.Aimbot.Resolver.ShotsFired[ Index ] = g_iBulletsFired[ Index ];
				}
			}

			if( !g_Whitelist.List( Index ) && g_CVars.Aimbot.Resolver.Active )
			{
				tempYaw[ Index ] = g_CVars.PlayerList.ViewAngles[ Index ].y;
				tempPitch[ Index ] = g_CVars.PlayerList.ViewAngles[ Index ].x;

				// store last pitch for pitch resolver
				if( tempPitch[Index] != 89.f && tempPitch[Index] != -89.f )
					g_CVars.Aimbot.Resolver.LastPitch[ Index ] = tempPitch[ Index ];

				bool ret = true;
				if( g_CVars.Aimbot.Resolver.Mode == 1 && g_CVars.PlayerList.Yaw[ Index ] != 1 ) ret = false;
				
				if( ret )
				{
					if( g_CVars.Aimbot.Resolver.Smart )
					{
						Vector resultLocal = EyePosition;
						Vector resultentity = Entity->EyePosition( );	
						Vector m_vTraceVector = Vector( resultLocal - resultentity );
						QAngle m_vAimAngles;
						static float yawDelta[ 64 ];
						static int yawMode[ 64 ];
	
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

						// improved smart: only resolve if pitch is AA or yawMode indicates AA, or if type is bruteforce/velocity
						bool shouldResolve = false;
						if( g_CVars.PlayerList.ViewAngles[ Index ].x == 89.f || g_CVars.PlayerList.ViewAngles[ Index ].x == -89.f )
							shouldResolve = true;
						else if( yawMode[ Index ] != 2 )
							shouldResolve = true;
						// for velocity/LBY types, always resolve when standing
						if( g_CVars.Aimbot.Resolver.Type == 3 || g_CVars.Aimbot.Resolver.Type == 4 || g_CVars.Aimbot.Resolver.Type == 5 )
							shouldResolve = true;

						if( shouldResolve )
						{
							float resolvedYaw = ResolveYawGuess( g_CVars.Aimbot.Resolver.Type, Index, tempYaw[ Index ], Entity );
							Entity->m_angEyeAngles( ).y = NormalizeYaw( resolvedYaw );
							g_CVars.Aimbot.Resolver.LastResolvedYaw[ Index ] = resolvedYaw;

							// pitch resolver
							float resolvedPitch = ResolvePitch( Entity, Index );
							Entity->m_angEyeAngles( ).x = resolvedPitch;
						}
					}
					else
					{
						g_CVars.Aimbot.AutoHeightMode[ Index ] = 0;
						float resolvedYaw = ResolveYawGuess( g_CVars.Aimbot.Resolver.Type, Index, tempYaw[ Index ], Entity );
						Entity->m_angEyeAngles( ).y = NormalizeYaw( resolvedYaw );
						g_CVars.Aimbot.Resolver.LastResolvedYaw[ Index ] = resolvedYaw;

						// pitch resolver even when not smart
						float resolvedPitch = ResolvePitch( Entity, Index );
						Entity->m_angEyeAngles( ).x = resolvedPitch;
					}
				}
			}

			if( pPlayerHistory[ Index ][ 0 ].m_SimulationTime != Entity->m_flSimulationTime( ) )
			{
				for( int tick = 31; tick > 0; tick-- ) pPlayerHistory[ Index ][ tick ] = pPlayerHistory[ Index ][ tick - 1 ];
				g_Stuff.StoreTickRecord( Entity, &pPlayerHistory[ Index ][ 0 ] );
			}
		}
	}
}
