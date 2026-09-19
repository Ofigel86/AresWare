// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"
#include <math.h>

cGameEvent g_GameEventManager;
std::vector< hit_s > hit;

static const char* HitgroupName( int hg )
{
	switch( hg )
	{
		case 1: return "head";
		case 2: return "chest";
		case 3: return "stomach";
		case 4: return "left arm";
		case 5: return "right arm";
		case 6: return "left leg";
		case 7: return "right leg";
		default: return "body";
	}
}

static void EventPlayerName( int index, char* out, int outSize )
{
	if( !out || outSize <= 0 ) return;
	out[ 0 ] = 0;
	const char* src = "World";
	player_info_t info;
	if( index >= 1 && index <= 64 && g_pEngineClient->GetPlayerInfo( index, &info ) ) src = info.name;
	strncpy( out, src, outSize - 1 );
	out[ outSize - 1 ] = 0;
}

// shot/miss tracking for the event log (independent from resolver).
// Every shot stores a snapshot; a shot with no hurt in 30 ticks = miss with reason.
struct ShotSnap
{
	bool active;
	int target;
	int tick;
	int shotNum;
	float spreadUnits; // spread offset at target distance, in game units
	int hitchance;     // hitchance % at shot time (-1 = HC off)
	float distance;
	int hp;
	int resolverType;
	bool resolverOn;
	bool jitter;
	bool spin;
	bool shotClear;  // path to target was clear at the exact shot moment
	int enemyChoke;  // enemy choked ticks (simtime frozen) at shot moment
};
static ShotSnap logSnap = { false, -1, 0, 0, 0.f, -1, 0.f, 0, 0, false, false, false, true, 0 };
static int logUnconfirmed = 0; // shots since last confirmed hit / miss log
static int logShotCount[ 65 ] = { 0 }; // sequential shot counter per target

static const char* ResolverTypeName( int type )
{
	switch( type )
	{
		case 0: return "Spin";
		case 1: return "Back Twitch";
		case 2: return "Alternative";
		case 3: return "2 bullets";
		case 4: return "Anim Test";
		case 5: return "AI Learn";
		case 6: return "Honest";
		default: return "?";
	}
}

void Log_OnShoot( int target, CUserCmd* pCmd )
{
	if( target < 1 || target > 64 || !pCmd ) return;
	BasePlayer* LocalPlayer = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( g_pEngineClient->GetLocalPlayer( ) );
	BasePlayer* Ent = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( target );
	if( !LocalPlayer || !Ent ) return;

	if( logSnap.target != target ) logUnconfirmed = 0;
	logUnconfirmed++;
	logShotCount[ target ]++;

	float dist = Ent->m_vecOrigin( ).DistTo( EyePosition );

	// spread of the exact seed this bullet uses
	float spreadUnits = 0.f;
	CSWeapon* Weapon = ( CSWeapon* ) LocalPlayer->GetActiveBaseCombatWeapon( );
	if( Weapon )
	{
		float flSpread = Weapon->GetSpread( );
		RandomSeed( ( pCmd->random_seed & 255 ) + 1 );
		float sx = ( RandomFloat( -0.5f, 0.5f ) + RandomFloat( -0.5f, 0.5f ) ) * flSpread;
		float sy = ( RandomFloat( -0.5f, 0.5f ) + RandomFloat( -0.5f, 0.5f ) ) * flSpread;
		spreadUnits = sqrtf( sx * sx + sy * sy ) * dist;
	}

	int hc = -1;
	if( g_CVars.Aimbot.HitChance && Weapon )
		hc = g_Aimbot.GetHitChance( pCmd, LocalPlayer, Ent, Weapon );

	logSnap.active = true;
	logSnap.target = target;
	logSnap.tick = g_iGameTicks;
	logSnap.shotNum = logShotCount[ target ];
	logSnap.spreadUnits = spreadUnits;
	logSnap.hitchance = hc;
	logSnap.distance = dist;
	logSnap.hp = Ent->m_iHealth( );
	logSnap.resolverType = g_CVars.Aimbot.Resolver.Type;
	logSnap.resolverOn = g_CVars.Aimbot.Resolver.Active;
	logSnap.jitter = g_CVars.Aimbot.Resolver.Jitter[ target ];
	logSnap.spin = g_CVars.Aimbot.Resolver.AnimSpin[ target ];
	logSnap.enemyChoke = g_CVars.Aimbot.Resolver.EnemyChoke[ target ];
	// was the path clear at the exact shot moment? (not 30 ticks later when we already hid)
	{
		Vector fwd, right, up;
		AngleVectors( pCmd->viewangles, &fwd, &right, &up );
		TraceFilterSkipTwoEntities shotFilter( LocalPlayer, 0 );
		Ray_t shotRay; shotRay.Init( EyePosition, EyePosition + fwd * 8192.f );
		trace_t shotTr;
		g_pEngineTrace->TraceRay( shotRay, 0x46004003, ( ITraceFilter* )&shotFilter, &shotTr );
		logSnap.shotClear = ( shotTr.m_pEnt == Ent );
	}

	if( g_CVars.Visuals.ShotLog )
	{
		char name[ 64 ]; EventPlayerName( target, name, sizeof( name ) );
		if( hc >= 0 )
			g_Drawing.AddLog( Color( 140, 200, 255, 255 ), "Shot %d at %s (hc %d%% | %s)", logSnap.shotNum, name, hc, logSnap.resolverOn ? ResolverTypeName( logSnap.resolverType ) : "no resolver" );
		else
			g_Drawing.AddLog( Color( 140, 200, 255, 255 ), "Shot %d at %s (%s)", logSnap.shotNum, name, logSnap.resolverOn ? ResolverTypeName( logSnap.resolverType ) : "no resolver" );
	}
}

void Log_OnHitConfirm( int victim )
{
	// detail line for the hitting shot (snapshot may already be a newer spray bullet - best effort)
	if( logSnap.active && logSnap.target == victim )
	{
		if( logSnap.hitchance >= 0 )
			g_Drawing.AddLog( Color( 150, 150, 150, 255 ), "  #%d hc %d%% | spread %.1fu | dist %.0fu", logSnap.shotNum, logSnap.hitchance, logSnap.spreadUnits, logSnap.distance );
		else
			g_Drawing.AddLog( Color( 150, 150, 150, 255 ), "  #%d spread %.1fu | dist %.0fu | hp %d", logSnap.shotNum, logSnap.spreadUnits, logSnap.distance, logSnap.hp );
	}
	// one hurt confirms one bullet; leftover spray bullets can still time out as misses
	if( logUnconfirmed > 0 ) logUnconfirmed--;
	if( logUnconfirmed <= 0 ) { logSnap.active = false; logSnap.target = -1; }
}

// figure out WHY the shot missed, from the snapshot taken at shot time
static char missReasonBuf[ 48 ];
static const char* Log_MissReason( ShotSnap& snap )
{
	BasePlayer* Ent = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( snap.target );
	if( !Ent || Ent->m_lifeState( ) != 0 ) return "target dead";
	if( Ent->IsDormant( ) ) return "target dormant";
	if( !snap.shotClear ) return "occluded";
	if( snap.spreadUnits > 15.f ) return "spread";
	if( snap.enemyChoke >= 8 )
	{
		snprintf( missReasonBuf, sizeof( missReasonBuf ), "enemy fakelag (%d)", snap.enemyChoke );
		return missReasonBuf;
	}
	if( snap.jitter ) return "resolver (jitter AA)";
	if( snap.spin ) return "resolver (spin)";
	if( snap.resolverOn ) return "resolver (unresolved)";
	return "unknown";
}

void Log_Tick( void )
{
	if( !logSnap.active || logSnap.target < 1 || logUnconfirmed <= 0 ) return;
	if( ( g_iGameTicks - logSnap.tick ) < 30 ) return; // hurt arrives ~10 ticks after the shot

	char name[ 64 ]; EventPlayerName( logSnap.target, name, sizeof( name ) );
	const char* reason = Log_MissReason( logSnap );
	if( logUnconfirmed > 1 )
		g_Drawing.AddLog( Color( 255, 200, 120, 255 ), "Missed %s x%d: %s", name, logUnconfirmed, reason );
	else
		g_Drawing.AddLog( Color( 255, 200, 120, 255 ), "Missed %s [#%d]: %s", name, logSnap.shotNum, reason );

	if( logSnap.hitchance >= 0 )
		g_Drawing.AddLog( Color( 150, 150, 150, 255 ), "  hc %d%% | spread %.1fu | dist %.0fu | hp %d | %s", logSnap.hitchance, logSnap.spreadUnits, logSnap.distance, logSnap.hp, logSnap.resolverOn ? ResolverTypeName( logSnap.resolverType ) : "no resolver" );
	else
		g_Drawing.AddLog( Color( 150, 150, 150, 255 ), "  spread %.1fu | dist %.0fu | hp %d | %s", logSnap.spreadUnits, logSnap.distance, logSnap.hp, logSnap.resolverOn ? ResolverTypeName( logSnap.resolverType ) : "no resolver" );

	logSnap.active = false;
	logSnap.target = -1;
	logUnconfirmed = 0;
}

void cGameEvent::FireGameEvent( IGameEvent* event )
{
	const char* eventName = event->GetName( );
	if( !eventName ) return;

	LuaAPI::Event( event ); // lua on_event callbacks

	if( strcmp( eventName, /*player_connect*/XorStr<0x1F,15,0xE6504059>("\x6F\x4C\x40\x5B\x46\x56\x7A\x45\x48\x46\x47\x4F\x48\x58"+0xE6504059).s ) == 0 )
	{
		std::string playerinfo = /*echo */XorStr<0xB3,6,0x7C8193A2>("\xD6\xD7\xDD\xD9\x97"+0x7C8193A2).s;
		playerinfo += event->GetString( /*name*/XorStr<0x98,5,0x86D9D33B>("\xF6\xF8\xF7\xFE"+0x86D9D33B).s, "" );
		playerinfo += ", ";
		playerinfo += event->GetString( /*address*/XorStr<0x50,8,0xA5C98DB9>("\x31\x35\x36\x21\x31\x26\x25"+0xA5C98DB9).s, "" );
		playerinfo += ", ";
		playerinfo += event->GetString( /*networkid*/XorStr<0xBB,10,0xB3ECAF1C>("\xD5\xD9\xC9\xC9\xD0\xB2\xAA\xAB\xA7"+0xB3ECAF1C).s, "" );

		g_pEngineClient->ExecuteClientCmd( playerinfo.c_str( ) );
		g_pEngineClient->ExecuteClientCmd( /*echo */XorStr<0xE9,6,0x93668CDD>("\x8C\x89\x83\x83\xCD"+0x93668CDD).s );
	}	

	if( strcmp( eventName, /*round_start*/XorStr<0x2F,12,0x8913A370>("\x5D\x5F\x44\x5C\x57\x6B\x46\x42\x56\x4A\x4D"+0x8913A370).s ) == 0 )
	{
		if( g_CVars.Miscellaneous.RoundSay ) g_pEngineClient->ClientCmd( "say twojstary 4.0 vip" );
	}

	if( strcmp( eventName, "bullet_impact" ) == 0 )
	{
		// Type 6 "Honest Shot": impact point + shooter eye = EXACT server shot yaw
		// (SDK ground truth: cs_player_shared.cpp fires bullet_impact with userid + x/y/z).
		int iShooter = g_pEngineClient->GetPlayerForUserID( event->GetInt( "userid", 0 ) );
		if( iShooter >= 1 && iShooter <= 64 && iShooter != g_pEngineClient->GetLocalPlayer( ) )
		{
			BasePlayer* pShooter = ( BasePlayer* )g_pClientEntityList->GetClientEntity( iShooter );
			if( pShooter )
			{
				Vector vImpact( event->GetFloat( "x", 0.f ), event->GetFloat( "y", 0.f ), event->GetFloat( "z", 0.f ) );
				Vector vDelta = vImpact - pShooter->EyePosition( );
				if( vDelta.Length2D( ) > 1.f )
				{
					QAngle aShot;
					VectorAngles( vDelta, aShot );
					g_CVars.Aimbot.Resolver.HonestYaw[ iShooter ] = aShot.y;
					g_CVars.Aimbot.Resolver.HonestTick[ iShooter ] = g_iGameTicks;
				}
			}
		}
	}

	if( strcmp( eventName, /*player_hurt*/XorStr<0xC5,12,0x41F135CF>("\xB5\xAA\xA6\xB1\xAC\xB8\x94\xA4\xB8\xBC\xBB"+0x41F135CF).s ) == 0 )
	{
		int iKiller = g_pEngineClient->GetPlayerForUserID( event->GetInt( /*attacker*/XorStr<0xD5,9,0x71615992>("\xB4\xA2\xA3\xB9\xBA\xB1\xBE\xAE"+0x71615992).s, false ) );
		int iVictim = g_pEngineClient->GetPlayerForUserID( event->GetInt( /*userid*/XorStr<0x20,7,0x8EFF66DE>("\x55\x52\x47\x51\x4D\x41"+0x8EFF66DE).s, false ) );
		int iDamage = event->GetInt( /*dmg_health*/XorStr<0x72,11,0x9F8B268B>("\x16\x1E\x13\x2A\x1E\x12\x19\x15\x0E\x13"+0x9F8B268B).s, false );

		if( iKiller == g_pEngineClient->GetLocalPlayer( ) && iVictim != g_pEngineClient->GetLocalPlayer( ) )
		{
			BasePlayer* Ent = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( iVictim );

			if( Ent )
			{
				hit_s kek;
				kek.idx = iVictim;
				kek.time = g_pGlobals->curtime + TICKS_TO_TIME( 60 );
				kek.dmg = iDamage;
				kek.pos = Ent->GetAbsOrigin( );
				hit.push_back( kek );
				Resolver_OnHit( iVictim ); // resolver: hit memory
				int iHitgroup = event->GetInt( "hitgroup", 0 );
				char victimName[ 64 ]; EventPlayerName( iVictim, victimName, sizeof( victimName ) );
				g_Drawing.AddLog( Color( 140, 255, 140, 255 ), "Hit %s for %d dmg (%s)", victimName, iDamage, HitgroupName( iHitgroup ) );
				Log_OnHitConfirm( iVictim );
				if( Ent->m_iHealth( ) <= 0 ) Legit_OnKill( ); // legit: kill delay
			}
		}
		else if( iVictim == g_pEngineClient->GetLocalPlayer( ) && iKiller != g_pEngineClient->GetLocalPlayer( ) )
		{
			AIAA_OnLocalHurt( iDamage ); // AI AntiAim: learn from damage taken
			char killerName[ 64 ]; EventPlayerName( iKiller, killerName, sizeof( killerName ) );
			g_Drawing.AddLog( Color( 255, 120, 120, 255 ), "Took %d dmg from %s", iDamage, killerName );
		}
	}

	if( strcmp( eventName, /*player_death*/XorStr<0x8A,13,0x411D0F9E>("\xFA\xE7\xED\xF4\xEB\xFD\xCF\xF5\xF7\xF2\xE0\xFD"+0x411D0F9E).s ) == 0 )
	{
		int iKiller = g_pEngineClient->GetPlayerForUserID( event->GetInt( /*attacker*/XorStr<0x31,9,0xAC07C770>("\x50\x46\x47\x55\x56\x5D\x52\x4A"+0xAC07C770).s, false ) );
		int iVictim = g_pEngineClient->GetPlayerForUserID( event->GetInt( /*userid*/XorStr<0xAA,7,0xAC29B4E9>("\xDF\xD8\xC9\xDF\xC7\xCB"+0xAC29B4E9).s, false ) );

		if( iKiller == g_pEngineClient->GetLocalPlayer( ) && iVictim != g_pEngineClient->GetLocalPlayer( ) )
		{
			// g_pEngineClient->ClientCmd( "say $$$ 1 TAP LAFF $$$" );
			Resolver_OnDeath( iVictim ); // resolver: reset memory
			char deadName[ 64 ]; EventPlayerName( iVictim, deadName, sizeof( deadName ) );
			g_Drawing.AddLog( Color( 120, 255, 120, 255 ), "Killed %s", deadName );
			logSnap.active = false; logSnap.target = -1; logUnconfirmed = 0; // killed: reset spray counter silently
		}
		else if( iVictim == g_pEngineClient->GetLocalPlayer( ) && iKiller != g_pEngineClient->GetLocalPlayer( ) )
		{
			char myKiller[ 64 ]; EventPlayerName( iKiller, myKiller, sizeof( myKiller ) );
			g_Drawing.AddLog( Color( 255, 100, 100, 255 ), "Died to %s", myKiller );
			{
				static const char* aaYaw[] = { "Forwards", "Backwards", "Sideways", "Jitter", "Static", "StaticRev", "Lisp", "Custom", "JitterX", "AI" };
				static const char* aaPitch[] = { "Off", "Normal", "InvNormal", "Safe", "FakeDown", "Down", "Up", "LagDown", "LagUp" };
				static const char* lagMode[] = { "Factor", "Switch", "Adaptive", "AISmart" };
				int y = g_CVars.Miscellaneous.AntiAim.Yaw, p = g_CVars.Miscellaneous.AntiAim.Pitch, lm = g_CVars.Miscellaneous.Fakelag.Mode;
				const char* yN = ( y >= 0 && y < 10 ) ? aaYaw[ y ] : "?";
				const char* pN = ( p >= 0 && p < 9 ) ? aaPitch[ p ] : "?";
				const char* lN = ( lm >= 0 && lm < 4 ) ? lagMode[ lm ] : "?";
				g_Drawing.AddLog( Color( 150, 150, 150, 255 ), "  my AA: %s%s v%d, pitch %s, lag %s(%d)%s", yN, g_CVars.Miscellaneous.AntiAim.Active ? "" : " OFF", g_CVars.Miscellaneous.AntiAim.Variation, pN, lN, g_CVars.Miscellaneous.Fakelag.Value, g_CVars.Miscellaneous.Fakelag.Active ? "" : " OFF" );
			}
		}
	}
}

void cGameEvent::RegisterSelf( )
{
	g_pGameEventManager->AddListener( this, /*round_start*/XorStr<0x4A,12,0x00451BD2>("\x38\x24\x39\x23\x2A\x10\x23\x25\x33\x21\x20"+0x00451BD2).s, false );
	g_pGameEventManager->AddListener( this, /*player_hurt*/XorStr<0x40,12,0xF561EE05>("\x30\x2D\x23\x3A\x21\x37\x19\x2F\x3D\x3B\x3E"+0xF561EE05).s, false );
	g_pGameEventManager->AddListener( this, /*player_death*/XorStr<0x82,13,0x7FA6859F>("\xF2\xEF\xE5\xFC\xE3\xF5\xD7\xED\xEF\xEA\xF8\xE5"+0x7FA6859F).s, false );
	g_pGameEventManager->AddListener( this, /*player_connect*/XorStr<0x1F,15,0xEF243F3D>("\x6F\x4C\x40\x5B\x46\x56\x7A\x45\x48\x46\x47\x4F\x48\x58"+0xEF243F3D).s, false );
	g_pGameEventManager->AddListener( this, "bullet_impact", false ); // honest shot measurements
}