#include "Main.h"

// ---------------------------------------------------------------------------
// Adaptive resolver (SDK / source-2007 derived). See Resolver.h for the
// full list of engine facts this file is built on.
// ---------------------------------------------------------------------------

struct ResolverPlayerState
{
	bool	initialized;
	float	lastRawYaw;
	float	lastPitch;
	int	lastTheirShots;		// target's m_iShotsFired (networked, 8 bit)
	int	bfIndex;			// bruteforce table index (Segregation Shots_Fired)
	bool	legacyHalf;			// Type 1 side toggle
	bool	memoryActive;			// memorized offset in use (Memorized_Y)
	float	memorizedOffset;		// offset from networked eye that produced a hit
	float	memoryExpire;			// curtime when the memory becomes stale
	float	appliedOffset;			// offset written on the last Apply
	float	offsetAtLastShot;		// offset in use when we last fired at them
	float	stationarySince;		// curtime they went idle, -1 while moving
	int	yawMode;			// Smart aim-relative classification (AutoHeight)
	int	bodyHits;			// sequential non-head hits - current offset stalls
	int	headHits;			// head hits count (diag)
	int	jitterDetect;			// updates left for the jitter signature
	float	lastYawDelta;			// norm(rawYaw - prevRawYaw) captured last update
	float	jitterMag;			// smoothed alternating-flip magnitude in degrees
};

static ResolverPlayerState g_ResolverState[ 64 ];

// Segregation default table is "0, -90, 90"; the full inverse is appended,
// then the velocity-implied gait yaw as a dynamic 5th candidate.
static const float kResolverOffsets[ 4 ] = { 0.f, -90.f, 90.f, 180.f };
static const int   kResolverOffsetCount = 5;	// 4 fixed + 1 velocity candidate

static void Resolver_InitPlayer( ResolverPlayerState& st )
{
	st.initialized = false;
	st.lastRawYaw = 0.f;
	st.lastPitch = 0.f;
	st.lastTheirShots = -1;
	st.bfIndex = 0;
	st.legacyHalf = false;
	st.memoryActive = false;
	st.memorizedOffset = 0.f;
	st.memoryExpire = 0.f;
	st.appliedOffset = 0.f;
	st.offsetAtLastShot = 0.f;
	st.stationarySince = -1.f;
	st.yawMode = 0;
	st.bodyHits = 0;
	st.headHits = 0;
	st.jitterDetect = 0;
	st.lastYawDelta = 0.f;
	st.jitterMag = 0.f;
}

void Resolver_ResetPlayer( int idx )
{
	if( idx < 0 || idx > 63 ) return;
	Resolver_InitPlayer( g_ResolverState[ idx ] );
}

void Resolver_ResetAll( )
{
	for( int i = 0; i < 64; i++ ) Resolver_InitPlayer( g_ResolverState[ i ] );
}

float Resolver_Quant11( float deg )
{
	// decode(n) = n * 360 / 2047 on the wire — snap to the same grid so the
	// value we write into m_angEyeAngles looks exactly like a real netvar.
	return ( float )round( ( deg * 2047.f ) / 360.f ) * ( 360.f / 2047.f );
}

static float Resolver_Norm( float deg )
{
	return g_Stuff.GuwopNormalize( deg );
}

// Same cl_interp math as CorrectTickCount, kept local so record picking works
// from both the aimbot and the shot path.
static float Resolver_GetInterp( )
{
	static bool init = false;
	static ConVar* cvar_cl_interp = nullptr;
	static ConVar* cvar_cl_updaterate = nullptr;
	static ConVar* cvar_cl_interp_ratio = nullptr;
	static ConVar* cvar_sv_minupdaterate = nullptr;
	static ConVar* cvar_sv_maxupdaterate = nullptr;
	static ConVar* cvar_sv_client_min_interp_ratio = nullptr;
	static ConVar* cvar_sv_client_max_interp_ratio = nullptr;

	if( !init )
	{
		if( !g_pCvar ) return 0.f;
		cvar_cl_interp = g_pCvar->FindVar( "cl_interp" );
		cvar_cl_updaterate = g_pCvar->FindVar( "cl_updaterate" );
		cvar_cl_interp_ratio = g_pCvar->FindVar( "cl_interp_ratio" );
		cvar_sv_minupdaterate = g_pCvar->FindVar( "sv_minupdaterate" );
		cvar_sv_maxupdaterate = g_pCvar->FindVar( "sv_maxupdaterate" );
		cvar_sv_client_min_interp_ratio = g_pCvar->FindVar( "sv_client_min_interp_ratio" );
		cvar_sv_client_max_interp_ratio = g_pCvar->FindVar( "sv_client_max_interp_ratio" );
		init = true;
	}

	if( !cvar_cl_interp || !cvar_cl_updaterate || !cvar_cl_interp_ratio ||
	    !cvar_sv_minupdaterate || !cvar_sv_maxupdaterate ||
	    !cvar_sv_client_min_interp_ratio || !cvar_sv_client_max_interp_ratio ) return 0.f;

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

	if( cl_updaterate <= 0 ) return cl_interp;

	float interp = ( float )cl_interp_ratio / ( float )cl_updaterate;
	if( interp > cl_interp ) cl_interp = interp;
	return cl_interp;
}

void Resolver_OnShot( int idx )
{
	if( idx < 1 || idx > 63 ) return;
	ResolverPlayerState& st = g_ResolverState[ idx ];
	if( !st.initialized ) Resolver_InitPlayer( st );

	// capture the offset the victim's bones were resolved with on this tick —
	// if this bullet connects, that offset is the one that worked
	st.offsetAtLastShot = st.appliedOffset;

	// Segregation Copy_Command: advance the table once per shot we take at them
	// while no memory is holding the answer
	if( !st.memoryActive )
		st.bfIndex = ( st.bfIndex + 1 ) % kResolverOffsetCount;
}

void Resolver_OnHit( int idx, int hitgroup, int damage )
{
	if( idx < 1 || idx > 63 ) return;
	ResolverPlayerState& st = g_ResolverState[ idx ];
	if( !st.initialized ) return;

	// hitgroup 1 == HITGROUP_HEAD (CSS hitgroup order: 0 generic, 1 head,
	// 2 chest, 3 stomach, 4/5 arms, 6/7 legs)
	if( hitgroup == 1 )
	{
		// Segregation Event_Processor: remember the offset that produced the
		// HEAD hit and keep using it until the behavior clearly changes
		st.memorizedOffset = st.offsetAtLastShot;
		st.memoryActive = true;
		st.memoryExpire = g_pGlobals->curtime + 6.f;
		st.headHits++;
		st.bodyHits = 0;
		Logger::Write( "[resolver] idx=%d HEAD %ddmg - offset %.1f memorized%s (head %d)",
			idx, damage, st.memorizedOffset, st.headHits - 1 ? "" : " (relearned)", st.headHits );
	}
	else
	{
		// A non-head hit proves the offset used for this bullet yields body
		// contact only (classic vs down-pitch AA: head lies inside the chest
		// volume). The old behavior memorized ANY hit's offset - a bodyshot
		// froze the body-producing offset in memory for 6 seconds and every
		// next bullet landed in the body again ("always body" loop). Now a
		// body hit releases stale memory and walks the bruteforce table on.
		st.bodyHits++;
		if( st.memoryActive )
		{
			st.memoryActive = false;
			Logger::Write( "[resolver] idx=%d body hg=%d %ddmg - stale memory released", idx, hitgroup, damage );
		}
		st.bfIndex = ( st.bfIndex + 1 ) % kResolverOffsetCount;
		Logger::Write( "[resolver] idx=%d body hg=%d %ddmg (streak %d) - bf advance to %d",
			idx, hitgroup, damage, st.bodyHits, st.bfIndex );
	}
}

int Resolver_PickRecord( int idx )
{
	if( idx < 1 || idx > 63 ) return 0;
	if( !g_pGlobals ) return 0;

	// how far in the past the server will restore this player when our
	// usercmd lands: round trip + interp, clamped by sv_maxunlag (1.0s)
	float shift = 0.f;
	if( g_pEngineClient )
	{
		INetChannelInfo* nci = g_pEngineClient->GetNetChannelInfo( );
		if( nci ) shift = nci->GetLatency( 0 ) + nci->GetLatency( 1 );
	}
	shift += Resolver_GetInterp( );
	if( shift > 1.f ) shift = 1.f;
	if( shift < 0.f ) shift = 0.f;

	// jittering players: resolve works on the FRESHEST snapshot phase, so the
	// record we aim at must be the newest too — a latency-matched old record
	// restores a body pose that no longer matches the yaw we just resolved
	ResolverPlayerState& st = g_ResolverState[ idx ];
	if( st.initialized && st.jitterDetect > 0
	    && pPlayerHistory[ idx ][ 0 ].m_SimulationTime > 0.f )
		return 0;

	float target = g_pGlobals->curtime - shift;

	int best = 0;
	float bestDist = 1e9f;

	for( int i = 0; i < 32; i++ )
	{
		float sim = pPlayerHistory[ idx ][ i ].m_SimulationTime;
		if( sim <= 0.f ) continue;

		float age = g_pGlobals->curtime - sim;
		if( age < 0.f || age > 1.f ) continue;	// outside sv_maxunlag

		float dist = fabsf( sim - target );
		if( dist < bestDist )
		{
			bestDist = dist;
			best = i;
		}
	}

	return best;
}

// Resolve the yaw of bruteforce candidate `bfIndex` for the current signals.
static float Resolver_CandidateYaw( ResolverPlayerState& st, int bfIndex, float rawYaw, bool velValid, float velYaw )
{
	// jitter-compensated table: while the signature is fresh their plausible
	// offsets narrow to the measured flip magnitude instead of generic +-90
	bool j = st.jitterDetect > 0 && st.jitterMag >= 15.f;
	float d = st.jitterMag;
	if( d < 15.f ) d = 15.f;
	if( d > 105.f ) d = 105.f;

	switch( bfIndex % kResolverOffsetCount )
	{
		case 0: return Resolver_Norm( rawYaw - kResolverOffsets[ 0 ] );	// trust networked
		case 1: return Resolver_Norm( rawYaw - ( j ? -d : kResolverOffsets[ 1 ] ) );	// eye + flip-mag / + 90
		case 2: return Resolver_Norm( rawYaw - ( j ?  d : kResolverOffsets[ 2 ] ) );	// eye - flip-mag / - 90
		case 3: return Resolver_Norm( rawYaw - kResolverOffsets[ 3 ] );	// full inverse
		default:
			// velocity-implied gait yaw: independent networked signal (m_vecVelocity)
			if( velValid ) return Resolver_Norm( velYaw );
			return Resolver_Norm( rawYaw - kResolverOffsets[ 3 ] );
	}
}

void Resolver_Apply( int idx, BasePlayer* ent, bool doResolve )
{
	if( idx < 1 || idx > 63 || !ent ) return;
	if( !g_pGlobals ) return;

	ResolverPlayerState& st = g_ResolverState[ idx ];
	bool first = !st.initialized;
	if( first ) Resolver_InitPlayer( st );

	// ---- signals (always tracked, even while resolution is gated off) ----
	float rawYaw = Resolver_Norm( ent->m_angEyeAngles( ).y );
	float pitch = ent->m_angEyeAngles( ).x;

	Vector vel = ent->m_vecVelocity( );
	float speed = vel.Length2D( );
	bool moving = speed >= 50.f;			// above MOVING_MINIMUM_SPEED with margin
	bool velValid = speed > 10.f;
	float velYaw = velValid ? RAD2DEG( atan2f( vel.y, vel.x ) ) : 0.f;

	// 11-bit quantization: a networked +-89 AA pitch decodes to ~+-88.96 —
	// the old exact == 89 check almost never fired on raw netvar data
	bool pitchAA = fabsf( pitch ) >= 85.f;

	int theirShots = ent->m_iShotsFired( );
	bool theyShot = false;

	if( !first )
	{
		if( theirShots > st.lastTheirShots ) theyShot = true;	// they fired this update
		if( theirShots < st.lastTheirShots )			// their spray reset
			if( !st.memoryActive ) st.bfIndex = 0;
	}

	// ---- jitter signature: consecutive yaw deltas flip sign with real
	// magnitude (>=15 deg). Tracked even while resolution is gated off so the
	// detection is warm by the time we start resolving them.
	float dNow = first ? 0.f : Resolver_Norm( rawYaw - st.lastRawYaw );
	if( !first && dNow * st.lastYawDelta < -1.f
	    && fabsf( dNow ) >= 15.f && fabsf( st.lastYawDelta ) >= 15.f )
	{
		float m = ( fabsf( dNow ) + fabsf( st.lastYawDelta ) ) * 0.5f;
		st.jitterMag = ( st.jitterMag <= 0.f ) ? m : ( st.jitterMag * 0.7f + m * 0.3f );
		st.jitterDetect = 32;			// keep compensated candidates ~32 updates
	}
	if( st.jitterDetect > 0 ) st.jitterDetect--;
	st.lastYawDelta = dNow;

	st.lastTheirShots = theirShots;
	st.lastRawYaw = rawYaw;
	st.lastPitch = pitch;
	st.initialized = true;

	// idle tracking for the mp_facefronttime feet-snap heuristic
	if( moving ) st.stationarySince = -1.f;
	else if( st.stationarySince < 0.f ) st.stationarySince = g_pGlobals->curtime;

	if( !doResolve ) return;

	// ---- Smart: aim-relative mode classification (legacy AutoHeight hint) ----
	int yawMode = 0;
	if( g_CVars.Aimbot.Resolver.Smart )
	{
		Vector resultLocal = EyePosition;
		Vector resultEntity = ent->EyePosition( );
		Vector traceVec = resultLocal - resultEntity;

		if( resultLocal.IsValid( ) && resultEntity.IsValid( ) && traceVec.IsValid( ) )
		{
			QAngle aim;
			VectorAngles( traceVec, aim );
			aim.x *= -1;

			if( aim.IsValid( ) )
			{
				float d = Resolver_Norm( aim.y - rawYaw );
				if( d < 0.f ) d += 360.f;

				if( d <= 20.f || d >= 340.f ) yawMode = 1;
				else if( ( d >= 70.f && d <= 110.f ) || ( d >= 250.f && d <= 290.f ) ) yawMode = 2;
				else if( d >= 160.f && d <= 200.f ) yawMode = 3;
				else yawMode = 0;
			}
		}

		st.yawMode = yawMode;
		g_CVars.Aimbot.AutoHeightMode[ idx ] = ( yawMode == 2 ) ? 1 : 0;
	}
	else g_CVars.Aimbot.AutoHeightMode[ idx ] = 0;

	const int type = g_CVars.Aimbot.Resolver.Type;

	if( type == 4 )
	{
		// ================= ADAPTIVE (new) =================
		if( st.memoryActive && g_pGlobals->curtime > st.memoryExpire ) st.memoryActive = false;

		// 9WAY: while the target moves, feet track the networked eye yaw, so a
		// memorized stationary offset is stale — drop it and relearn
		if( st.memoryActive && moving )
		{
			st.memoryActive = false;
			if( !theyShot ) st.bfIndex = 0;
		}

		float offset = 0.f;

		if( st.memoryActive )
		{
			offset = st.memorizedOffset;		// eye - Memorized_Y (Segregation)
		}
		else if( moving )
		{
			offset = 0.f;				// feet = networked eye -> trust
		}
		else if( theyShot )
		{
			offset = 0.f;				// real-on-shot: their fire direction
		}
		else
		{
			static bool faceFrontInit = false;
			static ConVar* cvar_facefront = nullptr;
			if( !faceFrontInit )
			{
				if( g_pCvar )
				{
					cvar_facefront = g_pCvar->FindVar( "mp_facefronttime" );
					faceFrontInit = true;
				}
			}
			float faceFront = cvar_facefront ? cvar_facefront->GetFloat( ) : 3.f;

			float idle = ( st.stationarySince >= 0.f ) ? ( g_pGlobals->curtime - st.stationarySince ) : 0.f;

			if( faceFront > 0.f && idle >= faceFront )
				offset = 0.f;			// gait snapped feet back to the eye
			else
				offset = Resolver_Norm( rawYaw - Resolver_CandidateYaw( st, st.bfIndex, rawYaw, velValid, velYaw ) );
		}

		float resolved = Resolver_Norm( Resolver_Norm( rawYaw - offset ) );
		resolved = Resolver_Norm( Resolver_Quant11( resolved ) );

		ent->m_angEyeAngles( ).y = resolved;
		st.appliedOffset = offset;
	}
	else
	{
		st.appliedOffset = 0.f;

		// legacy types: Smart requires an AA-pitch (quantization-safe check)
		// and skips pure side-on targets — same intent as the old == +-89 gate
		if( g_CVars.Aimbot.Resolver.Smart )
		{
			if( !pitchAA || yawMode == 2 ) return;
		}

		if( type == 0 )
		{
			// Spin
			int lol = ( g_iGameTicks % 4 );
			switch( lol )
			{
				case 0: ent->m_angEyeAngles( ).y = 0.f; break;
				case 1: ent->m_angEyeAngles( ).y = 90.f; break;
				case 2: ent->m_angEyeAngles( ).y = 180.f; break;
				case 3: ent->m_angEyeAngles( ).y = 270.f; break;
			}
		}
		else if( type == 1 )
		{
			// Back Twitch
			int lol = ( g_iGameTicks % 4 );
			switch( lol )
			{
				case 0: ent->m_angEyeAngles( ).y = ( st.legacyHalf ) ? 0.f : 180.f; break;
				case 1: ent->m_angEyeAngles( ).y = ( st.legacyHalf ) ? 45.f : 225.f; break;
				case 2: ent->m_angEyeAngles( ).y = ( st.legacyHalf ) ? 90.f : 270.f; break;
				case 3: ent->m_angEyeAngles( ).y = ( st.legacyHalf ) ? 135.f : 315.f; st.legacyHalf = !st.legacyHalf; break;
			}
		}
		else if( type == 2 )
		{
			// Alternative — 0.087936 is exactly half of the 11-bit LSB (360/2047)
			ent->m_angEyeAngles( ).y = ( g_iGameTicks % 2 == 0 ) ? 90.f : -90.f;
			if( g_iGameTicks % 4 == 0 ) ent->m_angEyeAngles( ).y += 0.087936f;
		}
		else if( type == 3 )
		{
			// "2 bullets": hold one side for two of our shots, alternate +-90;
			// trust the networked yaw on the tick they fire (real on shot)
			if( theyShot ) ent->m_angEyeAngles( ).y = rawYaw;
			else
			{
				float side = ( ( st.bfIndex / 2 ) % 2 ) ? 90.f : -90.f;
				if( g_iGameTicks % 4 == 0 ) side += 0.087936f;
				ent->m_angEyeAngles( ).y = side;
			}
		}
	}
}
