// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r43 (2026-09-19): legit AA fake is now actually visible - auto-fakelag (Fake Choke Ticks, def 6) + fake goes out on the flush tick too.
// BUILD MARKER r28 (2026-09-18): AIAA (Yaw 9) cold-start fix - unsampled styles neutral-bad (35dmg prior), back style holds.
// BUILD MARKER r27 (2026-09-18): AIC cold-start fix - unsampled combos are neutral-bad (35dmg prior), so the AI stops drifting to forward/0-deg AA; default combo starts back+side.
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r24 (2026-09-18): ported Fakeduck (deep 12/3 slow cycle) + Micromoves (zero-net micro-jitter, position pinned).
// BUILD MARKER r23 (2026-09-18): exact-seed hitchance (256-seed census + ForceSeed-aware) + ForceSeed prefers hit seeds + Lua: on_shot, draw.get_screen_size, utils.latency/choke, ents.eye_angles/hitbox.
// BUILD MARKER r22 (2026-09-18): no-shoot fix - DT hold disabled while Speedhack is on (Speedhack+DT used to hold shots forever).
// BUILD MARKER r21 (2026-09-18): Show Fake Pose toggle (pin off on demand) + AIC punished-combo ban + AI resolver recency/soft-ban.
// BUILD MARKER r20 (2026-09-18): removed Yaw 13/14/15 (Jitter Back / Random Back / Fake 0) - Yaw list back to 0-12 (Server Hold last).
// BUILD MARKER r18 (2026-09-18): Fake 0 sends 0 deg on non-shot ticks (move-base rotated) + render pin disabled for Fake 0 (live pose visible).
// BUILD MARKER r17 (2026-09-18): rage Yaw 15 "Fake 0" - fake (choked) always 0 deg, real untouched.
// BUILD MARKER r15 (2026-09-18): legit AA (real view kept, fake yaw 0 on choked ticks) + skeleton drawn per-segment (no more vanish).
// BUILD MARKER r14 (2026-09-18): counter-strafe AutoStop (rage) + sticky target bias + idle micro-sway (legit).
// BUILD MARKER r11 (2026-09-18): hitchance fire gate on live-shot commit + enemy fakelag sensor (EnemyChoke) + resolver-adaptive body hitbox.
// BUILD MARKER r9 (2026-09-18): NATIVE CHOKE - CanPacket(slot 52) gate; engine skips CL_SendMove itself (SetChoked) so fakelag/DT backlog never loses CLC_Move.
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"
#include <limits>

DWORD dwReturnAddress = NULL;
DWORD dwCreateMove = 0x24087270;
extern "C" { bool g_bSendPacket; } // r47: definition (was declaration-only)
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
static int lastAttackTick = -100000; // game tick of last fired bullet (AI fakelag)
float g_flAARealYaw = 0.f, g_flAAFakeYaw = 0.f; // AA viz: last sent (real) and choked (fake) yaw

static void AIC_ApplyPitch( CUserCmd* pCmd ); // fwd: defined below (AI Custom)

void AntiAimPitch( CUserCmd* pCmd, BasePlayer* LocalPlayer )
{
	// note: lisp doesnt do shit in css, needs max float

	if( g_CVars.Miscellaneous.AntiAim.Yaw == 11 ) { AIC_ApplyPitch( pCmd ); return; } // AI Custom owns the pitch
	switch( g_CVars.Miscellaneous.AntiAim.Pitch )
	{
		case 0: break;
		case 1: pCmd->viewangles.x = 180.f; break;																// normal
		case 2: pCmd->viewangles.x = -180.f; break; 															// inverse normal
		case 3: pCmd->viewangles.x = 70.f; break;																// safe
		case 4: pCmd->viewangles.x = -179.990005f; break; 														// fakedown
		case 5: pCmd->viewangles.x = 697049.f; break;															// lisp down
		case 6: pCmd->viewangles.x = 696871.f; break; 															// lisp up
		case 7: pCmd->viewangles.x = ( g_bSendPacket ) ? 697049.f : 696871.f; break; 								// fake lisp down
		case 8: pCmd->viewangles.x = ( g_bSendPacket ) ? 696871.f : 697049.f; break; 								// fake lisp up
		case 9: pCmd->viewangles.x = 89.f; break; // down 89 (safe)
		case 10: pCmd->viewangles.x = -89.f; break; // up -89 (safe)
	}
}

static bool twitch, twitchfake, edgetwitch, edgetwitchfake;

// AI AntiAim learning: contextual bandit - separate damage stats per move-state
// (0 stand / 1 move / 2 air) x 8 styles. Re-picks epsilon-greedy every 120 ticks
// or instantly on state change; old damage decays so it tracks the enemy.
static float AIAA_Dmg[ 3 ][ 8 ];
static int AIAA_Ticks[ 3 ][ 8 ];
static int AIAA_Style = 0;
static int AIAA_State = 0;
static int AIAA_LastState = -1;
static int AIAA_StyleHist[ 64 ]; // packed state*8+style per tick (ring) for delayed attribution
static int AIAA_LastEval = 0;
static int AIAA_LastHurtTick = -100000; // AI fakelag reads this (reactive defense)
void AIC_OnLocalHurt( int dmg ); // fwd: defined below (AI Custom)

static int AIAA_GetMoveState( BasePlayer* LocalPlayer )
{
	if( !( LocalPlayer->m_fFlags( ) & FL_ONGROUND ) ) return 2;
	if( LocalPlayer->m_vecVelocity( ).Length2D( ) > 30.f ) return 1;
	return 0;
}

int AIAA_GetStyle( void ) { return AIAA_Style; }
int AIAA_GetState( void ) { return AIAA_State; }

// laggy angle: holds a random yaw for 8 ticks, then jumps (AI style 7 + Defensive)
static float AIAA_LaggyAngle( void )
{
	static float held = 180.f;
	static int heldSlot = -1;
	int slot = g_iGameTicks / 8;
	if( slot != heldSlot ) { heldSlot = slot; held = ( float )( rand( ) % 360 ); }
	return held;
}

void AIAA_Reset( void )
{
	for( int st = 0; st < 3; st++ ) for( int i = 0; i < 8; i++ ) { AIAA_Dmg[ st ][ i ] = 0.f; AIAA_Ticks[ st ][ i ] = 0; }
	AIAA_Style = 0;
	AIAA_State = 0;
	AIAA_LastState = -1;
}

void AIAA_OnLocalHurt( int dmg )
{
	AIAA_LastHurtTick = g_iGameTicks;
	AIC_OnLocalHurt( dmg ); // AI Custom learns from the same damage
	// attribute damage to the style active ~15 ticks ago (bullet flight time)
	int slot = ( g_iGameTicks - 15 ) % 64; if( slot < 0 ) slot += 64;
	int packed = AIAA_StyleHist[ slot ];
	int st = packed / 8, style = packed % 8;
	if( st >= 0 && st < 3 && style >= 0 && style < 8 ) AIAA_Dmg[ st ][ style ] += ( float ) dmg;
}

// epsilon-greedy pick from one state's table (lowest damage per active time)
// r28 cold-start fix: an untried style scored 0 - "better" than battle-tested
// back, so the bandit kept drifting into spin styles that sweep through 0 deg
// (front-facing AA). The prior makes untried = neutral-BAD; only real taken
// damage justifies leaving the working style.
static int AIAA_Pick( int state )
{
	if( ( rand( ) % 100 ) < 15 ) return rand( ) % 8; // explore
	int best = 0; float bestScore = 1e30f;
	for( int i = 0; i < 8; i++ )
	{
		float score = ( AIAA_Dmg[ state ][ i ] + 35.f ) / ( float )( AIAA_Ticks[ state ][ i ] + 50 );
		if( score < bestScore ) { bestScore = score; best = i; }
	}
	return best;
}

// AI Custom AA (Yaw 11): generative anti-aim. Real yaw (6) x fake yaw (4) x
// pitch (4) are learned as three independent contextual bandits (stand/move/air),
// so the AI composes its own combos instead of picking presets.
static float AIC_DmgR[ 3 ][ 6 ], AIC_DmgF[ 3 ][ 4 ], AIC_DmgP[ 3 ][ 4 ];
static int AIC_TicksR[ 3 ][ 6 ], AIC_TicksF[ 3 ][ 4 ], AIC_TicksP[ 3 ][ 4 ];
static int AIC_Real = 0, AIC_Fake = 0, AIC_Pitch = 0, AIC_State = 0, AIC_LastState = -1;
static int AIC_Hist[ 64 ]; // packed state*96 + pitch*24 + fake*6 + real
static int AIC_LastEval = 0;
static bool AIC_ForceRepick = false; // anti-bruteforce: reshuffle the combo now
static int AIC_BanUntil[ 3 ][ 6 ] = { { 0 } }; // r21: a punished real is banned from picks for a while

int AIC_GetInfo( void ) { return AIC_State * 96 + AIC_Pitch * 24 + AIC_Fake * 6 + AIC_Real; }

void AIC_Reset( void )
{
	for( int st = 0; st < 3; st++ )
	{
		for( int i = 0; i < 6; i++ ) { AIC_DmgR[ st ][ i ] = 0.f; AIC_TicksR[ st ][ i ] = 0; }
		for( int i = 0; i < 4; i++ ) { AIC_DmgF[ st ][ i ] = 0.f; AIC_TicksF[ st ][ i ] = 0; AIC_DmgP[ st ][ i ] = 0.f; AIC_TicksP[ st ][ i ] = 0; }
	}
	AIC_Real = 0; AIC_Fake = 1; AIC_Pitch = 0; AIC_State = 0; // r27: fresh session starts BACK + SIDE fake (never forward/0)
	AIC_LastState = -1;
	AIC_ForceRepick = false;
	for( int st = 0; st < 3; st++ ) AIC_TicksP[ st ][ 0 ] = 200; // down favored from the start
for( int st2 = 0; st2 < 3; st2++ ) for( int i2 = 0; i2 < 6; i2++ ) AIC_BanUntil[ st2 ][ i2 ] = 0; // r21
}

void AIC_OnLocalHurt( int dmg )
{
	if( dmg >= 20 ) AIC_ForceRepick = true; // anti-bruteforce: they hit us, change everything
	int slot = ( g_iGameTicks - 15 ) % 64; if( slot < 0 ) slot += 64;
	int packed = AIC_Hist[ slot ];
	int st = packed / 96, p = ( packed / 24 ) % 4, f = ( packed / 6 ) % 4, r = packed % 6;
	if( st < 0 || st > 2 ) return;
	if( r >= 0 && r < 6 ) AIC_DmgR[ st ][ r ] += ( float ) dmg;
	if( f >= 0 && f < 4 ) AIC_DmgF[ st ][ f ] += ( float ) dmg;
	if( p >= 0 && p < 4 ) AIC_DmgP[ st ][ p ] += ( float ) dmg;
}

static int AIC_Pick( float* dmg, int* ticks, int n, int* banUntil = 0, int now = 0 )
{
	if( ( rand( ) % 100 ) < 15 ) return rand( ) % n; // explore
	int best = -1; float bestScore = 1e30f;
	int fallbackBest = 0; float fallbackScore = 1e30f;
	for( int pass = 0; pass < 2 && best == -1; pass++ ) // pass 0 honors the ban, pass 1 ignores it
	for( int i = 0; i < n; i++ )
	{
		// r27 cold-start fix: with a raw dmg/(ticks+50) score an UNSAMPLED variant
		// scores 0 - "better" than any battle-tested combo, so the bandit kept
		// drifting off working backwards AA into untried forward/0-deg ones.
		// The prior makes untried options neutral-BAD: only real taken damage
		// justifies leaving the current combo.
		float score = ( dmg[ i ] + 35.f ) / ( float )( ticks[ i ] + 50 );
		if( score < fallbackScore ) { fallbackScore = score; fallbackBest = i; }
		if( pass == 0 && banUntil && banUntil[ i ] > now ) continue; // r21: skip recently punished
		if( score < bestScore ) { bestScore = score; best = i; }
	}
	return ( best != -1 ) ? best : fallbackBest;
}

static void AIC_Tick( BasePlayer* LocalPlayer )
{
	static bool aicInit = false;
	if( !aicInit ) { aicInit = true; AIC_Reset( ); } // priors even before any manual reset
	int state = AIAA_GetMoveState( LocalPlayer );
	AIC_State = state;
	AIC_TicksR[ state ][ AIC_Real ]++;
	AIC_TicksF[ state ][ AIC_Fake ]++;
	AIC_TicksP[ state ][ AIC_Pitch ]++;
	AIC_Hist[ g_iGameTicks % 64 ] = state * 96 + AIC_Pitch * 24 + AIC_Fake * 6 + AIC_Real;
	if( state != AIC_LastState || ( g_iGameTicks - AIC_LastEval ) > 120 || AIC_ForceRepick )
	{
		AIC_LastState = state;
		AIC_LastEval = g_iGameTicks;
		for( int i = 0; i < 6; i++ ) AIC_DmgR[ state ][ i ] *= 0.85f;
		for( int i = 0; i < 4; i++ ) { AIC_DmgF[ state ][ i ] *= 0.85f; AIC_DmgP[ state ][ i ] *= 0.85f; }
		if( AIC_ForceRepick ) // anti-bruteforce: reshuffle to a DIFFERENT combo
		{
			AIC_ForceRepick = false;
			AIC_BanUntil[ state ][ AIC_Real ] = g_iGameTicks + 300; // r21: the punished real is out for ~4.5s
			int oR = AIC_Real, oF = AIC_Fake, oP = AIC_Pitch;
			for( int t = 0; t < 8; t++ ) { AIC_Real = AIC_Pick( AIC_DmgR[ state ], AIC_TicksR[ state ], 6 ); if( AIC_Real != oR ) break; }
			for( int t = 0; t < 8; t++ ) { AIC_Fake = AIC_Pick( AIC_DmgF[ state ], AIC_TicksF[ state ], 4 ); if( AIC_Fake != oF ) break; }
			for( int t = 0; t < 8; t++ ) { AIC_Pitch = AIC_Pick( AIC_DmgP[ state ], AIC_TicksP[ state ], 4 ); if( AIC_Pitch != oP ) break; }
		}
		else
		{
			AIC_Real = AIC_Pick( AIC_DmgR[ state ], AIC_TicksR[ state ], 6, AIC_BanUntil[ state ], g_iGameTicks ); // r21
			AIC_Fake = AIC_Pick( AIC_DmgF[ state ], AIC_TicksF[ state ], 4 );
			AIC_Pitch = AIC_Pick( AIC_DmgP[ state ], AIC_TicksP[ state ], 4 );
		}
	}
}

static void AIC_ApplyReal( CUserCmd* pCmd )
{
	static bool tw = false; tw = !tw;
	switch( AIC_Real )
	{
		case 0: pCmd->viewangles.y += 180.f; break; // back
		case 1: pCmd->viewangles.y += ( tw ) ? 90.f : 270.f; break; // side jitter
		case 2: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 5 ) % 360 ); break; // slow spin
		case 3: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 13 ) % 360 ) + ( ( ( g_iGameTicks / 17 ) % 2 ) ? 180.f : 0.f ); break; // spin+flick
		case 4: pCmd->viewangles.y += ( ( ( g_iGameTicks / 2 ) % 2 ) ? 155.f : -155.f ); break; // flicker
		default: pCmd->viewangles.y += AIAA_LaggyAngle( ); break; // laggy
	}
}

static void AIC_ApplyFake( CUserCmd* pCmd )
{
	switch( AIC_Fake )
	{
		case 0: break; // forward (desync vs back-ish reals)
		case 1: pCmd->viewangles.y += 90.f; break;
		case 2: pCmd->viewangles.y += -90.f; break;
		default: pCmd->viewangles.y += ( float )( -( ( g_iGameTicks * 9 ) % 360 ) ); break; // reverse spin
	}
}

static void AIC_ApplyPitch( CUserCmd* pCmd )
{
	switch( AIC_Pitch )
	{
		case 0: pCmd->viewangles.x = 89.f; break; // down - the money pitch
		case 1: pCmd->viewangles.x = -179.990005f; break; // fake down
		case 2: pCmd->viewangles.x = -89.f; break; // up
		default: pCmd->viewangles.x = ( g_bSendPacket ) ? 697049.f : 696871.f; break; // lisp flick
	}
}

// deterministic stateless hash for flick scheduling: real/fake passes must agree on the
// same schedule within one tick, so randomness comes from (tick-index, salt), never from state
static int AwFlickHash( int idx, int salt )
{
	unsigned int h = ( unsigned int )idx * 0x9E3779B9u ^ ( unsigned int )salt * 0x85EBCA6Bu;
	h ^= h >> 15;
	h *= 0x2C1B3C6Du;
	h ^= h >> 12;
	return ( int )h;
}

void AntiAimYaw( CUserCmd* pCmd, BasePlayer* LocalPlayer, bool fake, bool half )
{
	Vector Velocity = LocalPlayer->m_vecVelocity( );
	if( g_CVars.Miscellaneous.AntiAim.Yaw == 11 ) AIC_Tick( LocalPlayer ); // AI Custom learning tick
	
	// FAKE FIX: yaw 0/1/2/4/5 + Variation 0 ("Normal") = fake side IDENTICAL to the
	// real side (zero desync on the default!). Remap 0<->3 so Normal = max desync.
	// The yawMode gate keeps the real-side switches (yaw 3/6/7/8/10) on the true
	// Variation, and the fake side of the other yaws is unaffected too.
	int fakeVar = g_CVars.Miscellaneous.AntiAim.Variation;
	int yawMode = g_CVars.Miscellaneous.AntiAim.Yaw;
	if( yawMode == 0 || yawMode == 1 || yawMode == 2 || yawMode == 4 || yawMode == 5 )
	{
		if( fakeVar == 0 ) fakeVar = 3;
		else if( fakeVar == 3 ) fakeVar = 0;
	}

	if( fake )
	{
		switch( g_CVars.Miscellaneous.AntiAim.Yaw )
		{
			case 0: 
			{
				switch( fakeVar )
				{
					case 0: break;
					case 1: pCmd->viewangles.y += ( half ) ? 270.f : 181.f; break;
					case 2: pCmd->viewangles.y += ( half ) ? 90.f : 179.f; break;
					case 3: pCmd->viewangles.y += 180.f; break;
				}
				break;
			}
			case 1:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += 180.f; break;
					case 1: pCmd->viewangles.y += ( half ) ? 90.f : 1.f; break;
					case 2: pCmd->viewangles.y += ( half ) ? 270.f : 359.f; break;
					case 3: break;
				}
				break;
			}
			case 2:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += 270.f; break;
					case 1: pCmd->viewangles.y += ( half ) ? 180.f : 91.f; break;
					case 2: pCmd->viewangles.y += ( half ) ? 360.f : 89.f; break;
					case 3: pCmd->viewangles.y += 90.f; break;
				}
				break;
			}
			case 3:
			{
				twitchfake = !twitch; // FIXED: was double-negated (fake == real, zero desync)
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += ( twitchfake ) ? 180.f : 0.f; break;
					case 1: pCmd->viewangles.y += 180.f + ( ( twitchfake ) ? -179.990005f : 0.f ); break;
					case 2: pCmd->viewangles.y = ( twitchfake ) ? 90.f : -90.f; break;
					case 3: pCmd->viewangles.y = 180.f + ( ( twitchfake ) ? 90.f : -89.990005f ); break;
				}
				break;
			}
			case 4:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y = 180.f; break;
					case 1: pCmd->viewangles.y = ( half ) ? 90.f : 1.f; break;
					case 2: pCmd->viewangles.y = ( half ) ? 280.f : 359.f; break;
					case 3: pCmd->viewangles.y = 360.f; break;
				}
				break;
			}
			case 5:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y = 360.f; break;
					case 1: pCmd->viewangles.y = ( half ) ? 270.f : 181.f; break;
					case 2: pCmd->viewangles.y = ( half ) ? 90.f : 179.f; break;
					case 3: pCmd->viewangles.y = 180.f; break;
				}
				break;
			}
			case 6:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += 697075.087936f; break;
					case 1: pCmd->viewangles.y += 697018.087936f; break;
					case 2: twitchfake = !twitch; pCmd->viewangles.y += ( twitchfake ) ? 696960.f : 697140.f; break; // FIXED: toggle was dead code above, twitch was stale
					case 3:
					{
						int value = ( g_iGameTicks % 4 );
						switch ( value ) 
						{					
							case 0: pCmd->viewangles.y = 697140.f; break;
							case 1: pCmd->viewangles.y = 697230.f; break;
							case 2: pCmd->viewangles.y = 696960.f; break;
							case 3: pCmd->viewangles.y = 697050.f; break;
						}
					}
				}
				break;
			}
			case 7:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += g_CVars.Miscellaneous.AntiAim.FakeValue; break;
					case 1: pCmd->viewangles.y = g_CVars.Miscellaneous.AntiAim.FakeValue; break;
				}
				break;
			}
			case 8: // Jitter X fake: mirrored against the real angles
			{
				twitchfake = !twitch;
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += ( twitchfake ) ? -120.f : 120.f; break; // wide
					case 1: // sway, phase-shifted 180 deg vs real
					{
						float ph = ( float )( ( g_iGameTicks + 60 ) % 120 );
						if( ph > 60.f ) ph = 120.f - ph;
						pCmd->viewangles.y += ( ph / 60.f * 2.f - 1.f ) * 140.f;
						break;
					}
					case 2: pCmd->viewangles.y += ( float )( ( rand( ) % 240 ) - 120 ); break; // random
					case 3: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 7 ) % 360 ) + ( ( twitchfake ) ? -60.f : 60.f ); break; // spin+jitter
				}
				break;
			}
			case 9: // AI fake: max desync against the learned real style
			{
				twitchfake = !twitch;
				switch( AIAA_Style )
				{
					case 0: break; // real back, fake forward
					case 1: pCmd->viewangles.y += ( twitchfake ) ? 270.f : 90.f; break;
					case 2: pCmd->viewangles.y += ( twitchfake ) ? -120.f : 120.f; break;
					case 3: pCmd->viewangles.y += ( float )( ( ( g_iGameTicks * 5 ) % 360 ) + 180 ); break;
					case 4: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 11 ) % 360 ) + ( ( ( g_iGameTicks / 19 ) % 2 ) ? 0.f : 180.f ); break;
					case 5: pCmd->viewangles.y += ( ( ( g_iGameTicks / 2 ) % 2 ) ? -155.f : 155.f ); break;
					case 6:
					{
						int dir = ( ( g_iGameTicks / 45 ) % 2 ) ? -1 : 1;
						pCmd->viewangles.y += ( float )( -( dir * ( ( g_iGameTicks * 9 ) % 360 ) ) );
						break;
					}
					default: pCmd->viewangles.y += AIAA_LaggyAngle( ) + 180.f; break;
				}
				break;
			}
			case 10: // Defensive fake: mirrored extremes, max desync
			{
				twitchfake = !twitch;
				switch( fakeVar )
				{
					case 0:
						pCmd->viewangles.y += ( float )( ( g_iGameTicks * 13 ) % 360 ) + ( ( ( g_iGameTicks / 17 ) % 2 ) ? 0.f : 180.f );
						break;
					case 1:
						pCmd->viewangles.y += ( ( ( g_iGameTicks / 2 ) % 2 ) ? -155.f : 155.f );
						break;
					case 2:
					{
						int dir = ( ( g_iGameTicks / 45 ) % 2 ) ? -1 : 1;
						pCmd->viewangles.y += ( float )( -( dir * ( ( g_iGameTicks * 9 ) % 360 ) ) );
						break;
					}
					case 4: // max desync: oppose the last real angle
						pCmd->viewangles.y = g_flAARealYaw + 180.f;
						break;
					default:
						pCmd->viewangles.y += AIAA_LaggyAngle( ) + 180.f;
						break;
				}
				break;
			}
			case 11: // AI Custom fake
			{
				AIC_ApplyFake( pCmd );
				break;
			}
			case 12: // Server Hold fake: fast spin while choked (visual chaos)
			{
				pCmd->viewangles.y += ( float )( ( g_iGameTicks * 25 ) % 360 );
				break;
			}
		}
	}
	else
	{
		switch( g_CVars.Miscellaneous.AntiAim.Yaw )
		{
			case 0: break;
			case 1: pCmd->viewangles.y += 180.f + ( ( g_iGameTicks % 4 < 2 ) ? 12.f : -12.f ); break; // send-volatility: static sent streams let feet converge (no desync)
			case 2: pCmd->viewangles.y += 270.f + ( ( g_iGameTicks % 4 < 2 ) ? 12.f : -12.f ); break; // send-volatility (see case 1)
			case 3:
			{
				twitch = !twitch;
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += ( twitch ) ? 180.f : 0.f; break;
					case 1: pCmd->viewangles.y += 180.f + ( ( twitch ) ? -179.990005f : 0.f ); break;
					case 2: pCmd->viewangles.y = ( twitch ) ? 90.f : -90.f; break;
					case 3: pCmd->viewangles.y = 180.f + ( ( twitch ) ? 90.f : -89.990005f ); break;
				}
				break;
			}
			case 4: pCmd->viewangles.y = 180.f + ( ( g_iGameTicks % 4 < 2 ) ? 12.f : -12.f ); break; // send-volatility (see case 1)
			case 5: pCmd->viewangles.y = 0.f + ( ( g_iGameTicks % 4 < 2 ) ? 12.f : -12.f ); break; // send-volatility (see case 1)
			case 6:
			{
				twitch = !twitch;
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y -= 696805.f; break;
					case 1: pCmd->viewangles.y -= 696625.f; break;
					case 2: pCmd->viewangles.y += ( twitch ) ? 696960.f : 697140.f; break; 
					case 3: // fake 4-step spin
					{
						int value = ( g_iGameTicks % 4 );
						switch ( value ) 
						{
							case 0: pCmd->viewangles.y = 696960.f; break;
							case 1: pCmd->viewangles.y = 697050.f; break;
							case 2: pCmd->viewangles.y = 697140.f; break;
							case 3: pCmd->viewangles.y = 697230.f; break;
						}
					}
				}
				break;
			}
			case 7:
			{
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += g_CVars.Miscellaneous.AntiAim.RealValue; break;
					case 1: pCmd->viewangles.y = g_CVars.Miscellaneous.AntiAim.RealValue; break;
				}
				break;
			}
			case 8: // Jitter X
			{
				twitch = !twitch;
				switch( fakeVar )
				{
					case 0: pCmd->viewangles.y += ( twitch ) ? 120.f : -120.f; break; // wide +-120
					case 1: // smooth sway +-140
					{
						float ph = ( float )( g_iGameTicks % 120 );
						if( ph > 60.f ) ph = 120.f - ph;
						pCmd->viewangles.y += ( ph / 60.f * 2.f - 1.f ) * 140.f;
						break;
					}
					case 2: pCmd->viewangles.y += ( float )( ( rand( ) % 240 ) - 120 ); break; // random
					case 3: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 7 ) % 360 ) + ( ( twitch ) ? 60.f : -60.f ); break; // spin+jitter
				}
				break;
			}
			case 9: // AI: contextual self-tuning style, learns from damage taken
			{
				int aiState = AIAA_GetMoveState( LocalPlayer );
				AIAA_State = aiState;
				AIAA_Ticks[ aiState ][ AIAA_Style ]++;
				AIAA_StyleHist[ g_iGameTicks % 64 ] = aiState * 8 + AIAA_Style;
				if( aiState != AIAA_LastState || ( g_iGameTicks - AIAA_LastEval ) > 120 )
				{
					AIAA_LastState = aiState;
					AIAA_LastEval = g_iGameTicks;
					for( int i = 0; i < 8; i++ ) AIAA_Dmg[ aiState ][ i ] *= 0.85f; // decay stale data
					AIAA_Style = AIAA_Pick( aiState );
				}
				twitch = !twitch;
				switch( AIAA_Style )
				{
					case 0: pCmd->viewangles.y += 180.f; break; // back
					case 1: pCmd->viewangles.y += ( twitch ) ? 90.f : 270.f; break; // side jitter
					case 2: pCmd->viewangles.y += ( twitch ) ? 120.f : -120.f; break; // wide jitter
					case 3: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 5 ) % 360 ); break; // slow spin
					case 4: pCmd->viewangles.y += ( float )( ( g_iGameTicks * 11 ) % 360 ) + ( ( ( g_iGameTicks / 19 ) % 2 ) ? 180.f : 0.f ); break; // spin+flick
					case 5: pCmd->viewangles.y += ( ( ( g_iGameTicks / 2 ) % 2 ) ? 155.f : -155.f ); break; // flicker
					case 6: // sway spin: direction flips every 45 ticks
					{
						int dir = ( ( g_iGameTicks / 45 ) % 2 ) ? -1 : 1;
						pCmd->viewangles.y += ( float )( dir * ( ( g_iGameTicks * 9 ) % 360 ) );
						break;
					}
					default: pCmd->viewangles.y += AIAA_LaggyAngle( ); break; // laggy jumps
				}
				break;
			}
			case 10: // Defensive: spin/flick chaos + desync, breaks resolvers and lagcomp
			{
				twitch = !twitch;
				switch( fakeVar )
				{
					case 0: // spin + flick
						pCmd->viewangles.y += ( float )( ( g_iGameTicks * 13 ) % 360 ) + ( ( ( g_iGameTicks / 17 ) % 2 ) ? 180.f : 0.f );
						break;
					case 1: // flicker: hard extremes every 2 ticks
						pCmd->viewangles.y += ( ( ( g_iGameTicks / 2 ) % 2 ) ? 155.f : -155.f );
						break;
					case 2: // sway spin: direction flips every 45 ticks
					{
						int dir = ( ( g_iGameTicks / 45 ) % 2 ) ? -1 : 1;
						pCmd->viewangles.y += ( float )( dir * ( ( g_iGameTicks * 9 ) % 360 ) );
						break;
					}
					case 4: // max desync: oppose the last fake angle
						pCmd->viewangles.y = g_flAAFakeYaw + 180.f;
						break;
					default: // laggy: random jumps every 8 ticks
						pCmd->viewangles.y += AIAA_LaggyAngle( );
						break;
				}
				break;
			}
			case 11: // AI Custom real (combo learned, see AIC_Tick)
			{
				AIC_ApplyReal( pCmd );
				break;
			}
			case 12: // Server Hold real: hard backwards on every SENT packet
			{
				pCmd->viewangles.y += 180.f;
				break;
			}
		}
	}

	// timed flick layer v2: alternating snaps on top of any yaw base.
	// hash-jittered period/angle/side (no metronome pattern), optional 2-tick hold,
	// optional 3x faster flicks while shooting to dodge crosshair tracking.
	// with FlickRandom off the old exact behavior is preserved 1:1.
	if( g_CVars.Miscellaneous.AntiAim.FlickEnable )
	{
		int fSide = g_CVars.Miscellaneous.AntiAim.FlickSide;
		if( ( !fake && ( fSide == 0 || fSide == 2 ) ) || ( fake && ( fSide == 1 || fSide == 2 ) ) )
		{
			int ft = g_CVars.Miscellaneous.AntiAim.FlickTicks;
			if( ft < 2 ) ft = 2;

			bool rnd = g_CVars.Miscellaneous.AntiAim.FlickRandom;
			bool fast = g_CVars.Miscellaneous.AntiAim.FlickOnShot && ( pCmd->buttons & IN_ATTACK ) != 0;
			int per = fast ? ( ft >= 6 ? ft / 3 : 2 ) : ft; // compressed period while shooting

			int idx = g_iGameTicks / per;
			int slot = g_iGameTicks % per;
			int jit = rnd ? ( ( AwFlickHash( idx, 1 ) & 0x7FFFFFFF ) % ( per / 3 + 1 ) ) : 0;

			if( slot == jit || ( rnd && per >= 4 && slot == jit + 1 ) ) // flick tick (+ optional hold tick)
			{
				float fAng = g_CVars.Miscellaneous.AntiAim.FlickAngle;
				if( rnd )
				{
					fAng *= 0.85f + 0.30f * ( float )( ( AwFlickHash( idx, 2 ) >> 8 ) & 255 ) / 255.0f; // 85%..115%
					if( slot != jit ) fAng *= 0.6f; // hold tick: partial angle
				}
				int salt = fast ? 9 : 0;
				bool plus = rnd ? ( ( AwFlickHash( idx, 3 + salt ) >> 5 ) & 1 ) != 0 : ( ( idx % 2 ) == 0 );
				if( plus ) pCmd->viewangles.y += fAng;
				else pCmd->viewangles.y -= fAng;
			}
		}
	}
}

void AntiAim( BasePlayer* LocalPlayer, CUserCmd* pCmd, int LagValue )
{
	int MoveType = LocalPlayer->m_MoveType( );
	Vector Velocity = LocalPlayer->m_vecVelocity( );

	bool WallDTC = false;
	bool ret = true;
	bool ShouldChoke = false;

	bool inair = !( LocalPlayer->m_fFlags( ) & FL_ONGROUND );

	int tmpLagticks;
	if( g_CVars.Miscellaneous.Fakelag.AirOnly )
	{
		tmpLagticks = ( inair ) ? LagValue : 1;
	}
	else tmpLagticks = LagValue;

	// creds to machete for giving me this brilliant idea lol
	int DeltaTicks = _clamp( abs( queue - tmpLagticks ), 0, 15 );

	if( g_CVars.Miscellaneous.Fakelag.Active )
	{
		if( g_CVars.Miscellaneous.Fakelag.Mode == 0 )
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
		else if( g_CVars.Miscellaneous.Fakelag.Mode == 2 ) // thx polak
		{
			float Velocity2D = Velocity.Length2D( ) * g_pGlobals->interval_per_tick;

			while( tmpLagticks - 2 <= 14 )
			{
				tmpLagticks -= 2;
				if( ( tmpLagticks * Velocity2D ) > 68.f ) break;

				tmpLagticks -= 1;
				if( ( tmpLagticks * Velocity2D ) > 68.f ) break;

				if( ( tmpLagticks * Velocity2D ) > 68.f ) break;

				tmpLagticks += 1;
				if( ( tmpLagticks * Velocity2D ) > 68.f ) break;

				tmpLagticks += 2;
				if( ( tmpLagticks * Velocity2D ) > 68.f ) break;

				tmpLagticks += 5;
			};

			if( DeltaTicks > 0 ) ShouldChoke = true;
		}
		else if( g_CVars.Miscellaneous.Fakelag.Mode == 3 ) // AI Smart: dynamic choke, slider Value = max cap
		{
			float speed = Velocity.Length2D( );
			int smart;
			if( inair ) smart = 12;
			else if( speed > 250.f ) smart = 12;
			else if( speed > 120.f ) smart = 9;
			else if( speed > 30.f ) smart = 6;
			else smart = 3;
			// recently fired: send fresh packets for accurate follow-up shots
			// recently fired + slow: fresh packets for accurate follow-ups.
			// Fast peekers keep choking (teleport) instead of dropping cover.
			if( ( g_iGameTicks - lastAttackTick ) < 12 && speed <= 200.f && smart > 2 ) smart = 2;
			// reactive defense: recently hurt -> thicker choke until it cools off
			if( ( g_iGameTicks - AIAA_LastHurtTick ) < 90 ) smart += 4;
			// jitter the choke so enemy lagcomp can't lock onto a pattern
			smart += ( rand( ) % 5 ) - 2;
			if( smart > g_CVars.Miscellaneous.Fakelag.Value ) smart = g_CVars.Miscellaneous.Fakelag.Value;
			if( smart < 1 ) smart = 1;
			tmpLagticks = smart;
			DeltaTicks = _clamp( abs( queue - tmpLagticks ), 0, 15 );
			if( DeltaTicks > 0 ) ShouldChoke = true;
		}
	}
	else
	{
		if( g_CVars.Miscellaneous.AntiAim.Active )
		{
			static bool flip;
			flip = !flip;
			if( flip ) ShouldChoke = true;
		}
	}

	if( pass ) ShouldChoke = false;

	// defensive doubletap: choke everything while not attacking to build a command
	// backlog, released all at once on the shot tick. [!pass] keeps the post-shot
	// fresh packet, queue>=14 below still caps the choke so we never time out.
	if( !pass && g_CVars.Miscellaneous.DoubleTap && g_CVars.Miscellaneous.DoubleTapMode == 1
		&& LocalPlayer->m_lifeState( ) == 0 && !( pCmd->buttons & IN_ATTACK ) ) ShouldChoke = true;

	// doubletap: hold the whole burst - shot + shift leave in ONE packet next frame.
	// Separate packets = server corrects every ahead-jump (snapback, 1 bullet).
	if( g_DTInBurst ) ShouldChoke = true;
	// r24 fakeduck (tuned port): hold bind -> choke the stream and duck-spam queued
	// commands. 12 ducked / 3 standing per 15-tick cycle: duck time climbs to FULL
	// crouch (deep, unlike the old 4/2 shallow bob) and the cycle is 2.5x slower.
	// Skipped while shooting (accuracy first) and during DT bursts.
	{
		static const int fdVK[ 6 ] = { 0, VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
		if( g_CVars.Miscellaneous.Fakeduck && g_CVars.Miscellaneous.FakeduckKey > 0 && g_CVars.Miscellaneous.FakeduckKey < 6
			&& ( GetAsyncKeyState( fdVK[ g_CVars.Miscellaneous.FakeduckKey ] ) & 0x8000 )
			&& !( pCmd->buttons & IN_ATTACK ) && !g_DTInBurst
			&& MoveType != Valve::MoveType_t::MOVETYPE_LADDER
			&& LocalPlayer && LocalPlayer->m_lifeState( ) == 0 )
		{
			ShouldChoke = true;
			if( ( g_iGameTicks % 15 ) < 12 ) pCmd->buttons |= IN_DUCK;
		}
	}

	// r43: LEGIT AA FAKE needs a live burst to be seen - auto-fakelag while the
	// fake is armed (Fake Choke Ticks, default 6). A manual fakelag with a bigger
	// value still wins. Skipped while shooting (real angle + shot go out at once)
	// and during DT bursts (the engine queue cap below still applies).
	if( Legit_IsActive( ) && g_CVars.Legit.DesyncAA && !g_DTInBurst
		&& !( pCmd->buttons & IN_ATTACK ) )
	{
		int wantChoke = g_CVars.Legit.DesyncChoke;
		if( wantChoke < 1 ) wantChoke = 1;
		if( wantChoke > 14 ) wantChoke = 14;
		if( g_CVars.Miscellaneous.Fakelag.Active && g_CVars.Miscellaneous.Fakelag.Value > wantChoke )
			wantChoke = g_CVars.Miscellaneous.Fakelag.Value;
		if( queue < wantChoke ) ShouldChoke = true;
	}

	g_bSendPacket = ( ShouldChoke ) ? false : true;

	if( !g_bSendPacket )
	{
		if( queue >= 14 && !g_DTInBurst ) // DT burst must stay in one packet
		{
			g_bSendPacket = true;
			queue = 0;
		}
		else ++queue;
	}
	else queue = 0;

	if( g_CVars.Miscellaneous.AntiAim.Active )
	{
		for( int i = g_pGlobals->maxClients; i >= 1; i-- )
		{
			if( i == g_pEngineClient->GetLocalPlayer( ) ) continue;			
			BasePlayer* Ent = ( BasePlayer* )g_pClientEntityList->GetClientEntity( i );
			if( !Ent ) continue;
			if( !( *( int* )( ( DWORD ) Ent + 0x87 ) == 0 ) ) continue;
			if( Ent->m_iTeamNum( ) == LocalPlayer->m_iTeamNum( ) ) continue;

			ret = false;
		}

		if( g_CVars.Miscellaneous.AntiAim.TurnOff )
		{
			if( ret ) return;
		}

		if( ( MoveType == Valve::MoveType_t::MOVETYPE_LADDER ) && ( pCmd->buttons & IN_DUCK ) )
		{
			if( !g_bSendPacket ) pCmd->buttons &= ~IN_DUCK;
		}

		if( g_CVars.Miscellaneous.AntiAim.AtTargets ) g_Stuff.AntiAim.AtTargets( LocalPlayer, pCmd );

		if( g_CVars.Miscellaneous.AntiAim.WallDetection && Velocity.Length( ) < 300.f )
		{
			if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 0 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, 0.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 1 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( g_bSendPacket ) ? 0.f : 180.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 2 ) WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( g_bSendPacket ) ? 180.f : 0.f );
			else if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode == 3 )
			{
				if( g_bSendPacket )
				{
					edgetwitch = !edgetwitch;
					WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( edgetwitch ) ? 0.f : 180.f );
				}
				else
				{
					edgetwitchfake = !edgetwitch;
					edgetwitchfake = !edgetwitchfake;
					WallDTC = g_Stuff.AntiAim.WallDetection( LocalPlayer, pCmd, ( edgetwitch ) ? 0.f : 180.f );
				}
			}
		}

		if( MoveType != Valve::MoveType_t::MOVETYPE_LADDER )
		{
			if( WallDTC ) pCmd->viewangles.x = 89.f;
			else
			{
				AntiAimPitch( pCmd, LocalPlayer );
				if( g_bSendPacket ) AntiAimYaw( pCmd, LocalPlayer, false, false );
				else
				{
					AntiAimYaw( pCmd, LocalPlayer, true, true );
					if( g_CVars.Miscellaneous.AntiAim.DuckInAir && LocalPlayer->GetVelocity( ).z > 0 ) pCmd->buttons |= IN_DUCK;
				}
			}
		}

		// flick-sync: a hard flick on a choke tick is sent instantly (choke 0)
		// so the enemy sees every flick crisply instead of a blurred choke.
		// Same-side comparison, so normal real/fake alternation never triggers.
		static float lastRealYaw = 0.f, lastFakeYaw = 0.f;
		static bool yawSyncInit = false;
		if( !yawSyncInit ) { lastRealYaw = lastFakeYaw = pCmd->viewangles.y; yawSyncInit = true; }
		if( g_bSendPacket ) lastRealYaw = pCmd->viewangles.y;
		else if( !g_DTInBurst )
		{
			float dv = g_Stuff.GuwopNormalize( pCmd->viewangles.y - lastFakeYaw );
			if( dv < 0.f ) dv = -dv;
			if( dv > 35.f ) { g_bSendPacket = true; queue = 0; }
			lastFakeYaw = pCmd->viewangles.y;
		}
	}

	// r43 LEGIT AA: the fake yaw goes onto choked AND flush commands - the enemy
	// only ever renders the last SENT angle, so a choke-only fake was invisible.
	// With the auto-choke above every burst ends on a fake-angled packet: enemies
	// see the fake between bursts, the untouched real view leaves only on shot
	// ticks (bullets must register true) and during DT bursts.
	if( Legit_IsActive( ) && g_CVars.Legit.DesyncAA && !g_DTInBurst && !( pCmd->buttons & IN_ATTACK ) )
	{
		int fakeYaw = g_CVars.Legit.DesyncYaw;
		if( fakeYaw < -180 ) fakeYaw = -180;
		if( fakeYaw > 180 ) fakeYaw = 180;
		pCmd->viewangles.y = ( float )fakeYaw;
	}

	if( g_bSendPacket ) g_flAARealYaw = pCmd->viewangles.y;
	else g_flAAFakeYaw = pCmd->viewangles.y;

	pass = false;
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
		int btRec = 0;
		if( g_Aimbot.TargetIndex >= 0 && g_Aimbot.TargetIndex < 65 ) btRec = g_Aimbot.BacktrackRecord[ g_Aimbot.TargetIndex ];
		if( btRec < 0 || btRec > 31 ) btRec = 0;
		tick = TIME_TO_TICKS( pPlayerHistory[ g_Aimbot.TargetIndex ][ btRec ].m_SimulationTime );
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

	g_bSendPacket = true;
	if( g_DTInBurst ) g_bSendPacket = false; // DT burst re-calls always choke (even if the AA path is skipped)
	Netchan_UpdateHook( ); // choke hook: (re)install on the live netchannel
	ClientInterpolation( ); // r25: Segregation enemy interp bypass (idempotent, installs only when enabled)
	g_DTAttack = false; // doubletap: set true below only on a locked live shot
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
	Log_Tick( ); // event log: shot without hurt in 30 ticks = miss
	if( g_CVars.Miscellaneous.ThirdPersonKey > 0 && ( GetAsyncKeyState( g_CVars.Miscellaneous.ThirdPersonKey ) & 1 ) )
		g_CVars.Miscellaneous.ThirdPerson = !g_CVars.Miscellaneous.ThirdPerson;

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

	if( g_CVars.Miscellaneous.BunnyHop ) g_Stuff.BunnyHop( pCmd, LocalPlayer );
	//if( g_CVars.Miscellaneous.EdgeJump ) g_Stuff.EdgeJump( pCmd, LocalPlayer );

	g_Prediction.Start( pCmd, LocalPlayer );
	EyePosition = LocalPlayer->EyePosition( );

	if( g_CVars.Miscellaneous.AutoKnife ) g_Stuff.Knifebot.Main( pCmd, LocalPlayer, Weapon );

	if( Weapon && Weapon->IsWeapon( ) )
	{
		Legit_Begin( ); // legit mode: swap in legit settings (restored at block end)
		if( g_CVars.Triggerbot.Active ) Legit_Trigger( pCmd, LocalPlayer, Weapon );
		if( g_CVars.Aimbot.Active )
		{
			QAngle tmp = pCmd->viewangles;
			if( Legit_IsActive( ) && g_CVars.Legit.AimType > 0 )
				Legit_SmoothAim( pCmd, LocalPlayer ); // native legit aim
			else
			{
				// r45: per-weapon-group profile - save globals, overlay the group,
				// run the rage aimbot, restore. Single-threaded per tick = safe.
				int rgIdx = Aimbot_RageGroupForWeapon( LocalPlayer );
				bool rgApplied = false;
				bool rgAutoShoot = false, rgAutoStop = false, rgAutoWall = false, rgMultiSpot = false, rgHitScan = false,
					rgBodyVsJump = false, rgBodyAWP = false, rgAntiSMAC = false, rgHitChance = false, rgStrictPrimary = false, rgBestDamage = false;
				int rgMinDamage = 0, rgHitChanceValue = 0, rgHitbox = 12, rgFallback = 0;
				if( rgIdx >= 0 )
				{
					rgAutoShoot = g_CVars.Aimbot.AutoShoot; rgAutoStop = g_CVars.Aimbot.AutoStop; rgAutoWall = g_CVars.Aimbot.AutoWall;
					rgMultiSpot = g_CVars.Aimbot.MultiSpot; rgHitScan = g_CVars.Aimbot.HitScan; rgBodyVsJump = g_CVars.Aimbot.BodyVsJump;
					rgBodyAWP = g_CVars.Aimbot.BodyAWP; rgAntiSMAC = g_CVars.Aimbot.AntiSMAC; rgHitChance = g_CVars.Aimbot.HitChance;
					rgStrictPrimary = g_CVars.Aimbot.StrictPrimary; rgBestDamage = g_CVars.Aimbot.BestDamage;
					rgMinDamage = g_CVars.Aimbot.MinDamage; rgHitChanceValue = g_CVars.Aimbot.HitChanceValue;
					rgHitbox = g_CVars.Aimbot.Hitbox; rgFallback = g_CVars.Aimbot.FallbackHitbox;
					g_CVars.Aimbot.AutoShoot = g_CVars.Aimbot.RageGroup[ rgIdx ].AutoShoot; g_CVars.Aimbot.AutoStop = g_CVars.Aimbot.RageGroup[ rgIdx ].AutoStop;
					g_CVars.Aimbot.AutoWall = g_CVars.Aimbot.RageGroup[ rgIdx ].AutoWall; g_CVars.Aimbot.MultiSpot = g_CVars.Aimbot.RageGroup[ rgIdx ].MultiSpot;
					g_CVars.Aimbot.HitScan = g_CVars.Aimbot.RageGroup[ rgIdx ].HitScan; g_CVars.Aimbot.BodyVsJump = g_CVars.Aimbot.RageGroup[ rgIdx ].BodyVsJump;
					g_CVars.Aimbot.BodyAWP = g_CVars.Aimbot.RageGroup[ rgIdx ].BodyAWP; g_CVars.Aimbot.AntiSMAC = g_CVars.Aimbot.RageGroup[ rgIdx ].AntiSMAC;
					g_CVars.Aimbot.HitChance = g_CVars.Aimbot.RageGroup[ rgIdx ].HitChance; g_CVars.Aimbot.StrictPrimary = g_CVars.Aimbot.RageGroup[ rgIdx ].StrictPrimary;
					g_CVars.Aimbot.BestDamage = g_CVars.Aimbot.RageGroup[ rgIdx ].BestDamage;
					g_CVars.Aimbot.MinDamage = g_CVars.Aimbot.RageGroup[ rgIdx ].MinDamage; g_CVars.Aimbot.HitChanceValue = g_CVars.Aimbot.RageGroup[ rgIdx ].HitChanceValue;
					g_CVars.Aimbot.Hitbox = g_CVars.Aimbot.RageGroup[ rgIdx ].Hitbox; g_CVars.Aimbot.FallbackHitbox = g_CVars.Aimbot.RageGroup[ rgIdx ].FallbackHitbox;
					rgApplied = true;
				}
				g_Aimbot.Main( pCmd, LocalPlayer );
				if( rgApplied )
				{
					g_CVars.Aimbot.AutoShoot = rgAutoShoot; g_CVars.Aimbot.AutoStop = rgAutoStop; g_CVars.Aimbot.AutoWall = rgAutoWall;
					g_CVars.Aimbot.MultiSpot = rgMultiSpot; g_CVars.Aimbot.HitScan = rgHitScan; g_CVars.Aimbot.BodyVsJump = rgBodyVsJump;
					g_CVars.Aimbot.BodyAWP = rgBodyAWP; g_CVars.Aimbot.AntiSMAC = rgAntiSMAC; g_CVars.Aimbot.HitChance = rgHitChance;
					g_CVars.Aimbot.StrictPrimary = rgStrictPrimary; g_CVars.Aimbot.BestDamage = rgBestDamage;
					g_CVars.Aimbot.MinDamage = rgMinDamage; g_CVars.Aimbot.HitChanceValue = rgHitChanceValue;
					g_CVars.Aimbot.Hitbox = rgHitbox; g_CVars.Aimbot.FallbackHitbox = rgFallback;
				}
			}

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

			if( Legit_IsActive( ) && !g_Aimbot.IsAimbotting ) Legit_StandaloneRCS( pCmd, LocalPlayer );
			if( g_CVars.Aimbot.AutoStop && g_Aimbot.IsAimbotting )
			{
				// r14: counter-strafe stop (ground only): cancel velocity with an
				// opposite wishdir impulse -> 1-2 ticks to full stop instead of 3-5
				// friction ticks. First-bullet accuracy arrives much sooner.
				if( LocalPlayer->m_fFlags( ) & FL_ONGROUND )
				{
					Vector vel = LocalPlayer->m_vecVelocity( );
					float spd = sqrtf( vel.x * vel.x + vel.y * vel.y );
					if( spd > 50.f )
					{
						Vector fwd, right, up;
						AngleVectors( pCmd->viewangles, &fwd, &right, &up );
						pCmd->forwardmove = -( vel.x * fwd.x + vel.y * fwd.y );
						pCmd->sidemove    = -( vel.x * right.x + vel.y * right.y );
						if( pCmd->forwardmove > 450.f ) pCmd->forwardmove = 450.f;
						if( pCmd->forwardmove < -450.f ) pCmd->forwardmove = -450.f;
						if( pCmd->sidemove > 450.f ) pCmd->sidemove = 450.f;
						if( pCmd->sidemove < -450.f ) pCmd->sidemove = -450.f;
					}
					else { pCmd->forwardmove = 0.f; pCmd->sidemove = 0.f; }
				}
			}

		// r24 micromoves (zero-net port): while standing, alternate tiny side
		// impulses strictly tick-by-tick so displacement cancels every pair -
		// backtrack records jitter but the POSITION stays pinned in place.
		if( g_CVars.Miscellaneous.Micromoves && !g_DTInBurst && !( pCmd->buttons & ( IN_ATTACK | IN_JUMP ) )
			&& ( LocalPlayer->m_fFlags( ) & FL_ONGROUND )
			&& LocalPlayer->m_vecVelocity( ).Length2D( ) < 10.f )
		{
			static bool mmFlip = false; mmFlip = !mmFlip;
			static int mmEpoch = 0;
			if( ( g_iGameTicks % 13 ) == 0 && mmEpoch != g_iGameTicks ) { mmEpoch = g_iGameTicks; mmFlip = !mmFlip; } // break the cadence
			pCmd->sidemove += ( mmFlip ) ? 6.f : -6.f;
		}

		// doubletap teleport conditions: air veto + delay-shot (hold fire until charged)
		bool dtSkipBurst = false;
		if( g_CVars.Miscellaneous.DoubleTap && !g_DTInBurst && ( pCmd->buttons & IN_ATTACK ) )
		{
			if( g_CVars.Miscellaneous.DoubleTapOnlyGround && !( LocalPlayer->m_fFlags( ) & FL_ONGROUND ) ) dtSkipBurst = true;
			else if( g_CVars.Miscellaneous.DoubleTapDelayShot && !Legit_IsActive( ) )
			{
				float cycD = g_NoSpread.GetWeaponInfo( Weapon ).CycleTime;
				int needD = g_CVars.Miscellaneous.DoubleTapAuto
					? ( ( cycD > 0.f && g_pGlobals->interval_per_tick > 0.f ) ? ( ( int )( cycD / g_pGlobals->interval_per_tick ) + 2 ) : 8 )
					: g_CVars.Miscellaneous.DoubleTapTicks;
				if( needD <= 16 && g_DTCharge < needD ) pCmd->buttons &= ~IN_ATTACK; // wait for full charge
			}
		}

		if( ( pCmd->buttons & IN_ATTACK ) && !g_DTInBurst ) // burst re-entries skip this, force-block below rebuilds them
		{
			// r11: spread-aware fire gate for EVERY live shot (manual/trigger/autoshoot/DT
			// burst entry - AutoShoot alone had it before). Simulates the shared-seed
			// spread; below threshold -> hold fire this tick, retry the next.
			bool hcGate = true;
			if( g_CVars.Aimbot.HitChance && g_Aimbot.TargetIndex != -1 )
			{
				BasePlayer* hcTarget = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( g_Aimbot.TargetIndex );
				if( hcTarget ) hcGate = ( g_Aimbot.GetHitChance( pCmd, LocalPlayer, hcTarget, Weapon ) >= g_CVars.Aimbot.HitChanceValue );
			}
			if( hcGate && g_Stuff.IsReadyToShoot( LocalPlayer, Weapon ) )	
			{
				if( g_Aimbot.TargetIndex != -1 ) g_iBulletsFired[ g_Aimbot.TargetIndex ]++;
				if( g_Aimbot.TargetIndex != -1 ) Resolver_OnShot( g_Aimbot.TargetIndex ); // resolver: shot tracking
				Log_OnShoot( g_Aimbot.TargetIndex, pCmd ); // event log: miss tracking (target -1 = ignored inside)
				LuaAPI::Shot( g_Aimbot.TargetIndex ); // r23: on_shot for scripts
				lastAttackTick = g_iGameTicks; // AI fakelag: fresh packets after shots
				g_DTAttack = true; // doubletap: any live shot bursts (IN_ATTACK + ready + gun already checked)
				angelfix = false;
				pass = true;
				queue = 0;

				bool trigger = ( g_CVars.Triggerbot.Active && !g_CVars.Triggerbot.IsShooting );

				if( !g_CVars.Triggerbot.Active || trigger )
				{
					if( g_CVars.Accuracy.ForceSeed ) g_Stuff.ForceSeed( pCmd, ( g_Aimbot.TargetIndex != -1 ) ? ( BasePlayer* )g_pClientEntityList->GetClientEntity( g_Aimbot.TargetIndex ) : 0 ); // r23: prefer seeds that HIT the target
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

				if( g_CVars.Aimbot.PerfectSilent ) g_bSendPacket = false;


				CorrectTickCount( pCmd );

				// doubletap: store the live shot for shifted re-send + per-weapon shift
				if( g_DTAttack )
				{
					g_DTAimAngles = pCmd->viewangles;
					g_DTTick = pCmd->tick_count;
					if( g_CVars.Miscellaneous.DoubleTapAuto )
					{
						float cyc = g_NoSpread.GetWeaponInfo( Weapon ).CycleTime;
						int rawNeed = ( cyc > 0.f && g_pGlobals->interval_per_tick > 0.f ) ? ( ( int )( cyc / g_pGlobals->interval_per_tick ) + 2 ) : 8;
						if( rawNeed > 16 ) // unshiftable cycle (AWP): normal shot, charge saved
						{
							static bool s_dtSlowOnce = false;
							if( !s_dtSlowOnce ) { s_dtSlowOnce = true; printconsole( "[Awesware] DT: weapon cycle needs %d ticks (max 16) - burst impossible for this gun\n", rawNeed ); }
							g_DTAttack = false;
						}
						if( rawNeed < 2 ) rawNeed = 2; if( rawNeed > 16 ) rawNeed = 16;
						g_DTTicks = rawNeed;
					}
					else
					{
						int manTicks = g_CVars.Miscellaneous.DoubleTapTicks;
						if( manTicks < 2 ) manTicks = 2; if( manTicks > 16 ) manTicks = 16;
						g_DTTicks = manTicks;
					}
					if( dtSkipBurst ) g_DTAttack = false; // teleport condition (air): normal shot, charge saved
					// hold this shot for the burst: single-packet release next frame.
					// Shooting it now would split the shift across packets (no double).
						if( g_DTAttack && g_DTCharge >= g_DTTicks && !g_CVars.Miscellaneous.Speedhack ) // r22: Speedhack kills the burst in CL_Move, so the hold would sit on the shot forever (no-shoot bug)
						{
							g_bSendPacket = false;
							static bool s_dtHoldOnce = false;
							if( !s_dtHoldOnce ) { s_dtHoldOnce = true; printconsole( "[Awesware] DT hold armed: need=%d charge=%d gate=%s\n", g_DTTicks, g_DTCharge, g_CanPacketHooked ? "native" : "drop" ); }
						}
				}
			}
			else
			{
				if( g_CVars.Aimbot.AntiSMAC ) pCmd->viewangles = QAngle( 0, 0, 0 );
				else AntiAim( LocalPlayer, pCmd, ( g_CVars.Miscellaneous.Fakelag.InAttack ) ? g_CVars.Miscellaneous.Fakelag.Value : 1 );

				pCmd->buttons &= ~IN_ATTACK;
			}
		}
		else
		{
			if( g_CVars.Aimbot.AntiSMAC ) pCmd->viewangles = QAngle( 0, 0, 0 );
			else AntiAim( LocalPlayer, pCmd, g_CVars.Miscellaneous.Fakelag.Value );
		}
		if( !g_DTInBurst ) LuaAPI::CreateMove( pCmd ); // r25: script cmd access BEFORE legit AA
		if( Legit_IsActive( ) ) Legit_AutoPistol( pCmd, Weapon );
		Legit_End( );
	}

	// doubletap: shifted re-sends reuse the stored live shot (attack + aim + tick),
	// so every shifted cmd is a live aimed bullet instead of an empty move
	if( g_DTInBurst && g_DTForceLeft > 0 )
	{
		g_DTForceLeft--;
		pCmd->buttons |= IN_ATTACK;
		pCmd->viewangles = g_DTAimAngles;
		pCmd->tick_count = g_DTTick + g_DTForceIdx;
		g_DTForceIdx++;
	}

	if( g_CVars.Miscellaneous.AutoStrafe ) g_Stuff.AutoStrafe( pCmd, LocalPlayer );
	if( g_CVars.Legit.StrafeActive ) Legit_Strafe( pCmd, LocalPlayer );

	// legit anti-aim: small silent yaw padding (hides exact angles from aim-readers).
	// skipped while shooting (accuracy first), during DT burst re-sends, and when rage AA owns the angles.
	if( g_CVars.Legit.LegitAA && !g_CVars.Miscellaneous.AntiAim.Active && !( pCmd->buttons & IN_ATTACK ) &&
		g_CVars.Legit.LegitAAAngle > 0 && ( g_CVars.Legit.LegitAAKey <= 0 || ComboKeyDown( g_CVars.Legit.LegitAAKey ) ) )
	{
		float pad = ( float )g_CVars.Legit.LegitAAAngle;
		if( pad > 45.f ) pad = 45.f;
		float sign = ( ( ( g_iGameTicks / 9 ) % 2 == 0 ) ? 1.f : -1.f ); // slow side alternation
		if( g_CVars.Legit.LegitAAInvertKey > 0 && ComboKeyDown( g_CVars.Legit.LegitAAInvertKey ) ) sign = -sign; // inverter bind (hold)
		pCmd->viewangles.y += sign * pad;
	}
	g_Stuff.MovementFix.FixMove( LocalPlayer, pCmd, angelfix );

	// slowwalk: clamp movement speed (key Off = always while checked)
	if( g_CVars.Miscellaneous.SlowWalk && ( LocalPlayer->m_fFlags( ) & FL_ONGROUND ) &&
		( g_CVars.Miscellaneous.SlowWalkKey <= 0 || ComboKeyDown( g_CVars.Miscellaneous.SlowWalkKey ) ) )
	{
		float maxSpd = ( float )g_CVars.Miscellaneous.SlowWalkSpeed;
		if( maxSpd < 50.f ) maxSpd = 50.f; if( maxSpd > 260.f ) maxSpd = 260.f;
		float spd = sqrtf( pCmd->forwardmove * pCmd->forwardmove + pCmd->sidemove * pCmd->sidemove );
		if( spd > maxSpd && spd > 0.01f )
		{
			float k = maxSpd / spd;
			pCmd->forwardmove *= k;
			pCmd->sidemove *= k;
		}
	}

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
	
	if( g_bSendPacket ) g_qThirdPerson = pCmd->viewangles;
}

#ifdef _MSC_VER
void __declspec( naked ) __fastcall Hooked_CreateMove( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	__asm
	{
		push ebp
		mov ebp, esp
		mov g_bSendPacket, bl			   
		movzx eax, active
		push eax
		mov eax, input_sample_frametime
		push eax
		mov eax, sequence_number
		push eax
		call CreateMove			   
		mov bl, g_bSendPacket			   
		mov esp, ebp
		pop ebp			   
		retn 0xC
	}
}
#else
extern "C" void CM_CreateMoveBridge( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	CreateMove( ecx, edx, sequence_number, input_sample_frametime, active );
}

// mingw: naked thunks are not supported on i386; replicate the MSVC thunk 1:1 (file-scope asm).
__asm__(
	".text\n"
	".globl _Hooked_CreateMove\n"
	"_Hooked_CreateMove:\n\t"
	"pushl %ebp\n\t"
	"movl %esp, %ebp\n\t"
	"pushl %ecx\n\t"
	"pushl %edx\n\t"
	"movb %bl, _g_bSendPacket\n\t"
	"movzbl 20(%ebp), %eax\n\t"
	"pushl %eax\n\t"
	"pushl 16(%ebp)\n\t"
	"pushl 12(%ebp)\n\t"
	"call _CM_CreateMoveBridge\n\t"
	"movb _g_bSendPacket, %bl\n\t"
	"movl %ebp, %esp\n\t"
	"popl %ebp\n\t"
	"ret $12\n"
);
#endif