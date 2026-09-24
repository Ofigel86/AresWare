#ifndef __RESOLVER_H__
#define __RESOLVER_H__

// Adaptive resolver + latency-aware backtrack record picker.
// Derived from SDK / source-2007 / Segregation analysis:
//   - m_angEyeAngles is a SendPropAngle 11 bit CHANGES_OFTEN value, anglemod'ed
//     to 0..360 before encoding => step = 360/2047 (~0.176 deg), error <= 0.09 deg.
//     An anti-aim pitch of +-89 therefore arrives as ~+-88.96 — never exactly 89.
//   - LEGANIM_9WAY (base_playeranimstate): while the target is moving, feet yaw
//     tracks the networked eye yaw => the networked yaw IS the body/hitbox facing.
//     While stationary the feet stay locked and the torso may lag the eye by up
//     to m_flMaxBodyYawDegrees (90): feet = eye - {0, -90, +90, 180}.
//   - After mp_facefronttime seconds (default 3) idle, the gait snaps the feet
//     back to the eye yaw => networked yaw becomes trustworthy again.
//   - The server builds a shot as EyeAngles() + 2 * punch (weapon_csbasegun.cpp),
//     so on the tick a target fires (m_iShotsFired++) the networked yaw is the
//     direction they actually shot ("real on shot").
//   - m_vecVelocity is networked for every player (20/20/16 bit, +-2048) and is
//     not spoofable through eye angles — usable as an independent gait hint.
//   - Segregation method: feet/truth = networked_eye - Bruteforce[Shots_Fired],
//     advance the table on every shot we land on the target, memorize the offset
//     that produced a hit (player_hurt) and reuse it until behavior changes.
//   - Lag compensation: the server restores bones from records up to sv_maxunlag
//     (1s) in the past; pick the history record closest to curtime - latency and
//     send its tick in usercmd->tick_count so the validated bones match the ones
//     we aimed at.

void Resolver_ResetPlayer( int idx );
void Resolver_ResetAll( );

void Resolver_OnShot( int idx );	// local player fired a bullet at idx
void Resolver_OnHit( int idx );		// local player damaged idx (player_hurt)

// Runs at FRAME_NET_UPDATE_POSTDATAUPDATE_START, overwrites the local copy of
// the target's networked eye yaw with the resolved body facing (doResolve=false
// only updates signal tracking, e.g. for whitelisted/unselected players).
void Resolver_Apply( int idx, BasePlayer* ent, bool doResolve );

// Latency-correct pPlayerHistory[] index to aim at / send in usercmd->tick_count.
int Resolver_PickRecord( int idx );

// Snap a degree value onto the 11-bit SendPropAngle grid (360/2047 per step).
float Resolver_Quant11( float deg );

#endif // __RESOLVER_H__
