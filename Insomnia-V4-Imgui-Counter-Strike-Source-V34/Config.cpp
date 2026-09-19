// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r44 (2026-09-19): V3 (AIMWARE) menu skin - orange header, icon tab strip, 8 pages over the same cvars, separate player-list window.
// BUILD MARKER r43 (2026-09-19): legit AA fake is now actually visible - auto-fakelag (Fake Choke Ticks, def 6) + fake goes out on the flush tick too.
// BUILD MARKER r41 (2026-09-19): slow walk default bind = SHIFT (combo index 6 -> VK 0x10).
// BUILD MARKER r40 (2026-09-19): lag records - per-record anti-jitter resolve (phase prediction + miss brute).
// BUILD MARKER r38 (2026-09-18): slophook directional autostrafe port (wall avoidance + key offsets + 90/100 step).
// BUILD MARKER r34 (2026-09-18): ClientMod Emulator user toggle (MISC) + boot-time default.ini load.
// BUILD MARKER r31 (2026-09-18): reverted r30 Humanize+ per user request - legit anti-snap back to r29 state. Awaiting user-supplied anti-detect material.
// BUILD MARKER r29 (2026-09-18): legit max - dual FOV (near/far by distance) + visible-only target lock + anti-snap on target switch + dual FOV circles.
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r24 (2026-09-18): ported Fakeduck (deep 12/3 slow cycle) + Micromoves (zero-net micro-jitter, position pinned).
// BUILD MARKER r21 (2026-09-18): Show Fake Pose toggle (pin off on demand) + AIC punished-combo ban + AI resolver recency/soft-ban.
// BUILD MARKER r19 (2026-09-18): Freeze Model (ModelZero) feature fully removed.
// BUILD MARKER r16 (2026-09-18): Freeze Model (0 deg) - old-school local look: frozen body, free skeleton overlay.
// BUILD MARKER r15 (2026-09-18): legit AA (real view kept, fake yaw 0 on choked ticks) + skeleton drawn per-segment (no more vanish).
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"

CConfig g_Config;

float GetPrivateProfileFloat( LPCSTR lpAppName, LPCSTR lpKeyName, FLOAT flDefault, LPCSTR lpFileName )
{
	char szData[ 32 ];
	GetPrivateProfileStringA( lpAppName, lpKeyName, std::to_string( flDefault ).c_str( ), szData, 32, lpFileName );
	return ( float )atof( szData );
}

void WritePrivateProfileFloat( LPCSTR lpAppName, LPCSTR lpKeyName, FLOAT flValue, LPCSTR lpFileName )
{
	WritePrivateProfileStringA( lpAppName, lpKeyName, std::to_string( ( float )flValue ).c_str( ), lpFileName );
}

// =======================================================================================================================

int GetPrivateProfileInteger( LPCSTR lpAppName, LPCSTR lpKeyName, INT flDefault, LPCSTR lpFileName )
{
	char szData[ 32 ];
	GetPrivateProfileStringA( lpAppName, lpKeyName, std::to_string( flDefault ).c_str( ), szData, 32, lpFileName );
	return atoi( szData );
}

void WritePrivateProfileInteger( LPCSTR lpAppName, LPCSTR lpKeyName, INT flValue, LPCSTR lpFileName )
{
	WritePrivateProfileStringA( lpAppName, lpKeyName, std::to_string( ( int )flValue ).c_str( ), lpFileName );
}

// =======================================================================================================================

void GetPrivateProfileColor( LPCSTR lpAppName, LPCSTR lpKeyName, Color &cvar, LPCSTR lpFileName )
{
	char szData[ 32 ];
	char *red, *green, *blue;
	GetPrivateProfileStringA( lpAppName, lpKeyName, "r0,g0,b0", szData, 32, lpFileName );

	int len = strlen( szData );
	for( int i = 0; i < len; i++ )
	{
		if( szData[ i ] == 'r' && szData[ i + 2 ] == ',' ) red = &szData[ i + 1 ];
		else if( szData[ i ] == 'r' && szData[ i + 3 ] == ',' ) red = &szData[ i + 1 ];
		else if( szData[ i ] == 'r' && szData[ i + 4 ] == ',' ) red = &szData[ i + 1 ];

		if( szData[ i ] == 'g' && szData[ i + 2 ] == ',' ) green = &szData[ i + 1 ];
		else if( szData[ i ] == 'g' && szData[ i + 3 ] == ',' ) green = &szData[ i + 1 ];
		else if( szData[ i ] == 'g' && szData[ i + 4 ] == ',' ) green = &szData[ i + 1 ];

		if( szData[ i ] == 'b' ) blue = &szData[ i + 1 ];
	}

	len = strlen( red );
	
	for( int i = 0; i < len; i++ )
	{
		if( red[ i ] == ',' && red[ i + 1 ] == 'g' ) red[ i ] = 0;
	}

	len = strlen( green );

	for( int i = 0; i < len; i++ )
	{
		if( green[ i ] == ',' && green[ i + 1 ] == 'b' ) green[ i ] = 0;
	}

	std::string r, g, b;
	r = red;
	g = green;
	b = blue;

	cvar = Color( atoi( r.c_str( ) ), atoi( g.c_str( ) ), atoi( b.c_str( ) ), 255 );
}

void WritePrivateProfileColor( LPCSTR lpAppName, LPCSTR lpKeyName, Color flValue, LPCSTR lpFileName )
{
	std::string colorstring;
	colorstring += "r";
	colorstring += std::to_string( flValue.r( ) );
	colorstring += ",g";
	colorstring += std::to_string( flValue.g( ) );
	colorstring += ",b";
	colorstring += std::to_string( flValue.b( ) );
	WritePrivateProfileStringA( lpAppName, lpKeyName, colorstring.c_str( ), lpFileName );
}

// =======================================================================================================================

HMODULE CConfig::m_hModule = NULL;

void CConfig::SetModule( HMODULE hModule )
{
	m_hModule = hModule;
}

bool CConfig::ReadProfile( const std::string& path )
{
		g_CVars.Aimbot.Active = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AutoShoot = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AutoShoot*/XorStr<0x11,10,0xA8DBBE49>("\x50\x67\x67\x7B\x46\x7E\x78\x77\x6D"+0xA8DBBE49).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AutoWall = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AutoWall*/XorStr<0xEF,9,0x87A95EA9>("\xAE\x85\x85\x9D\xA4\x95\x99\x9A"+0x87A95EA9).s, 0, path.c_str( ) );
	g_CVars.Aimbot.MultiSpot = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*MultiSpot*/XorStr<0xD1,10,0x86EC0E0A>("\x9C\xA7\xBF\xA0\xBC\x85\xA7\xB7\xAD"+0x86EC0E0A).s, 0, path.c_str( ) );
	g_CVars.Aimbot.HitScan = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*HitScan*/XorStr<0xCB,8,0xFC340577>("\x83\xA5\xB9\x9D\xAC\xB1\xBF"+0xFC340577).s, 0, path.c_str( ) );
	g_CVars.Aimbot.FriendlyFire = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*FriendlyFire*/XorStr<0xFC,13,0x954C6400>("\xBA\x8F\x97\x9A\x6E\x65\x6E\x7A\x42\x6C\x74\x62"+0x954C6400).s, 0, path.c_str( ) );
	g_CVars.Aimbot.TargetSelection = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*TargetSelection*/XorStr<0xC3,16,0x9E78006D>("\x97\xA5\xB7\xA1\xA2\xBC\x9A\xAF\xA7\xA9\xAE\xBA\xA6\xBF\xBF"+0x9E78006D).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Hitbox = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Hitbox*/XorStr<0x61,7,0xE7859F64>("\x29\x0B\x17\x06\x0A\x1E"+0xE7859F64).s, 0, path.c_str( ) );
	if( g_CVars.Aimbot.Hitbox < 9 || g_CVars.Aimbot.Hitbox > 12 ) g_CVars.Aimbot.Hitbox = 12; // FIX r8: stale 0 aimed body while menu showed Head
	g_CVars.Aimbot.HitboxMode = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*HitboxMode*/XorStr<0xDB,11,0xBA9B04D1>("\x93\xB5\xA9\xBC\xB0\x98\xAC\x8D\x87\x81"+0xBA9B04D1).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Silent = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Silent*/XorStr<0xE1,7,0xA335B50B>("\xB2\x8B\x8F\x81\x8B\x92"+0xA335B50B).s, 0, path.c_str( ) );
	g_CVars.Aimbot.PerfectSilent = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*PerfectSilent*/XorStr<0xEC,14,0xBB750D0C>("\xBC\x88\x9C\x89\x95\x92\x86\xA0\x9D\x99\x93\x99\x8C"+0xBB750D0C).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AntiSMAC = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AntiSMAC*/XorStr<0x07,9,0xDB906D0E>("\x46\x66\x7D\x63\x58\x41\x4C\x4D"+0xDB906D0E).s, 0, path.c_str( ) );
	g_CVars.Aimbot.BodyAWP = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*BodyAWP*/XorStr<0xCA,8,0x15077618>("\x88\xA4\xA8\xB4\x8F\x98\x80"+0x15077618).s, 0, path.c_str( ) );
	g_CVars.Aimbot.PointScale = GetPrivateProfileFloat( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*PointScale*/XorStr<0xBE,11,0x1EFB65EC>("\xEE\xD0\xA9\xAF\xB6\x90\xA7\xA4\xAA\xA2"+0x1EFB65EC).s, 0.75, path.c_str( ) );
	g_CVars.Aimbot.SnapLimiter = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*SnapLimiter*/XorStr<0xCD,12,0xE92FD079>("\x9E\xA0\xAE\xA0\x9D\xBB\xBE\xBD\xA1\xB3\xA5"+0xE92FD079).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AngleLimit = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AngleLimit*/XorStr<0x55,11,0x2BCB8202>("\x14\x38\x30\x34\x3C\x16\x32\x31\x34\x2A"+0x2BCB8202).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AngleLimitTens = GetPrivateProfileFloat( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AngleLimitTens*/XorStr<0x4F,15,0x42DB868E>("\x0E\x3E\x36\x3E\x36\x18\x3C\x3B\x3E\x2C\x0D\x3F\x35\x2F"+0x42DB868E).s, 0, path.c_str( ) );
	g_CVars.Aimbot.MinDamage = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*MinDamage*/XorStr<0x43,10,0xD1CF37C2>("\x0E\x2D\x2B\x02\x26\x25\x28\x2D\x2E"+0xD1CF37C2).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Key = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Key*/XorStr<0x19,4,0xBAF51996>("\x52\x7F\x62"+0xBAF51996).s, 0, path.c_str( ) );
	g_CVars.Aimbot.AutoStop = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "AutoStop", 0, path.c_str( ) );
	g_CVars.Aimbot.BodyVsJump = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BodyVsJump", 0, path.c_str( ) );
	g_CVars.Aimbot.ForceBodyKey = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceBodyKey", 0, path.c_str( ) );
	g_CVars.Aimbot.ForceMinDmgKey = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceMinDmgKey", 0, path.c_str( ) );
	g_CVars.Aimbot.ForceMinDmgValue = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceMinDmgValue", 10, path.c_str( ) );
	g_CVars.Aimbot.StrictPrimary = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "StrictPrimary", 0, path.c_str( ) );
	g_CVars.Aimbot.BestDamage = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BestDamage", 0, path.c_str( ) );
	g_CVars.Aimbot.HitChance = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitChance", 0, path.c_str( ) );
	g_CVars.Aimbot.HitChanceValue = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitChanceValue", 60, path.c_str( ) );
	g_CVars.Aimbot.BacktrackTicks = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BacktrackTicks", 6, path.c_str( ) );
	g_CVars.Aimbot.AimFOV = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "AimFOV", 90, path.c_str( ) );
	g_CVars.Aimbot.LongRangeDist = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "LongRangeDist", 1000, path.c_str( ) );
	{
		int packedGroups = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitboxGroups", 5461, path.c_str( ) );
		for( int hg = 0; hg < 7; hg++ ) { int v = ( packedGroups >> ( hg * 2 ) ) & 3; g_CVars.Aimbot.HitboxGroup[ hg ] = ( v > 2 ) ? 2 : v; }
	}

	g_CVars.Legit.Active = GetPrivateProfileInteger( "Legit", "Active", 0, path.c_str( ) );
	g_CVars.Legit.AutoShoot = GetPrivateProfileInteger( "Legit", "AutoShoot", 0, path.c_str( ) );
	g_CVars.Legit.Silent = GetPrivateProfileInteger( "Legit", "Silent", 0, path.c_str( ) );
	g_CVars.Legit.SnapLimiter = GetPrivateProfileInteger( "Legit", "SnapLimiter", 1, path.c_str( ) );
	g_CVars.Legit.AngleLimit = GetPrivateProfileInteger( "Legit", "AngleLimit", 8, path.c_str( ) );
	g_CVars.Legit.AngleLimitTens = GetPrivateProfileFloat( "Legit", "AngleLimitTens", 0, path.c_str( ) );
	g_CVars.Legit.Key = GetPrivateProfileInteger( "Legit", "Key", 0, path.c_str( ) );
	g_CVars.Legit.AimFOV = GetPrivateProfileInteger( "Legit", "AimFOV", 4, path.c_str( ) );
	g_CVars.Legit.FovNear = GetPrivateProfileInteger( "Legit", "FovNear", 14, path.c_str( ) ); // r29
	g_CVars.Legit.FovFar = GetPrivateProfileInteger( "Legit", "FovFar", 5, path.c_str( ) ); // r29
	g_CVars.Legit.FovSwitchDist = GetPrivateProfileInteger( "Legit", "FovSwitchDist", 450, path.c_str( ) ); // r29
	g_CVars.Legit.Hitbox = GetPrivateProfileInteger( "Legit", "Hitbox", 0, path.c_str( ) );
	g_CVars.Legit.AutoStop = GetPrivateProfileInteger( "Legit", "AutoStop", 0, path.c_str( ) );
	g_CVars.Legit.ScopedCheck = GetPrivateProfileInteger( "Legit", "ScopedCheck", 0, path.c_str( ) );
	g_CVars.Legit.AutoScope = GetPrivateProfileInteger( "Legit", "AutoScope", 0, path.c_str( ) );
	g_CVars.Legit.LegitAA = GetPrivateProfileInteger( "Legit", "LegitAA", 0, path.c_str( ) );
	g_CVars.Legit.LegitAAKey = GetPrivateProfileInteger( "Legit", "LegitAAKey", 0, path.c_str( ) );
	g_CVars.Legit.LegitAAAngle = GetPrivateProfileInteger( "Legit", "LegitAAAngle", 14, path.c_str( ) );
	g_CVars.Legit.LegitAAInvertKey = GetPrivateProfileInteger( "Legit", "LegitAAInvertKey", 0, path.c_str( ) );
	g_CVars.Legit.BacktrackTicks = GetPrivateProfileInteger( "Legit", "BacktrackTicks", 6, path.c_str( ) );
	g_CVars.Legit.AimType = GetPrivateProfileInteger( "Legit", "AimType", 2, path.c_str( ) );
	g_CVars.Legit.Smoothing = GetPrivateProfileInteger( "Legit", "Smoothing", 8, path.c_str( ) );
	g_CVars.Legit.ReactionMs = GetPrivateProfileInteger( "Legit", "ReactionMs", 120, path.c_str( ) );
	g_CVars.Legit.RCS = GetPrivateProfileInteger( "Legit", "RCS", 70, path.c_str( ) );
	g_CVars.Legit.RCSStandalone = GetPrivateProfileInteger( "Legit", "RCSStandalone", 1, path.c_str( ) );
	g_CVars.Legit.AimLock = GetPrivateProfileInteger( "Legit", "AimLock", 1, path.c_str( ) );
	g_CVars.Legit.DesyncResolver = GetPrivateProfileInteger( "Legit", "DesyncResolver", 1, path.c_str( ) );
	g_CVars.Legit.DesyncAA = GetPrivateProfileInteger( "Legit", "DesyncAA", 1, path.c_str( ) ); // r15
	g_CVars.Legit.DesyncYaw = GetPrivateProfileInteger( "Legit", "DesyncYaw", 0, path.c_str( ) ); // r15
	g_CVars.Legit.DesyncChoke = GetPrivateProfileInteger( "Legit", "DesyncChoke", 6, path.c_str( ) ); // r43: auto-fakelag depth (1-14)
	g_CVars.Legit.FlashCheck = GetPrivateProfileInteger( "Legit", "FlashCheck", 1, path.c_str( ) );
	g_CVars.Legit.TargetSelection = GetPrivateProfileInteger( "Legit", "TargetSelection", 4, path.c_str( ) );
	g_CVars.Legit.StrafeActive = GetPrivateProfileInteger( "Legit", "StrafeActive", 0, path.c_str( ) );
	g_CVars.Legit.StrafePower = GetPrivateProfileInteger( "Legit", "StrafePower", 4, path.c_str( ) );
	g_CVars.Legit.Prediction = GetPrivateProfileInteger( "Legit", "Prediction", 1, path.c_str( ) );
	g_CVars.Legit.FovCircle = GetPrivateProfileInteger( "Legit", "FovCircle", 1, path.c_str( ) );
	g_CVars.Legit.AutoPistol = GetPrivateProfileInteger( "Legit", "AutoPistol", 1, path.c_str( ) );
	g_CVars.Legit.KillDelayMs = GetPrivateProfileInteger( "Legit", "KillDelayMs", 250, path.c_str( ) );
	for( int lg = 0; lg < 7; lg++ ) { char k[ 8 ]; sprintf( k, "GH%d", lg ); g_CVars.Legit.HitboxGroup[ lg ] = GetPrivateProfileInteger( "Legit", k, 1, path.c_str( ) ); }

	g_CVars.Aimbot.Interpolation.LagPrediction = GetPrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*LagPrediction*/XorStr<0x0C,14,0x8FC405E0>("\x40\x6C\x69\x5F\x62\x74\x76\x7A\x77\x61\x7F\x78\x76"+0x8FC405E0).s, 0, path.c_str( ) );

	g_CVars.Aimbot.Resolver.Active = GetPrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Resolver.Smart = GetPrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Smart*/XorStr<0x1B,6,0x3199AD52>("\x48\x71\x7C\x6C\x6B"+0x3199AD52).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Resolver.Mode = GetPrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Mode*/XorStr<0xA1,5,0x038D62E9>("\xEC\xCD\xC7\xC1"+0x038D62E9).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Resolver.Type = GetPrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Type*/XorStr<0x33,5,0x7A74F612>("\x67\x4D\x45\x53"+0x7A74F612).s, 0, path.c_str( ) );
	g_CVars.Aimbot.Resolver.LagRecords = GetPrivateProfileInteger( "Resolver", "LagRecords", 1, path.c_str( ) ); // r40
	g_CVars.Aimbot.RageGroups = GetPrivateProfileInteger( "RageGroup", "Enabled", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].AutoShoot = GetPrivateProfileInteger( "RageGroup", "0_AutoShoot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].AutoStop = GetPrivateProfileInteger( "RageGroup", "0_AutoStop", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].AutoWall = GetPrivateProfileInteger( "RageGroup", "0_AutoWall", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].MultiSpot = GetPrivateProfileInteger( "RageGroup", "0_MultiSpot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].HitScan = GetPrivateProfileInteger( "RageGroup", "0_HitScan", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].BodyVsJump = GetPrivateProfileInteger( "RageGroup", "0_BodyVsJump", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].BodyAWP = GetPrivateProfileInteger( "RageGroup", "0_BodyAWP", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].AntiSMAC = GetPrivateProfileInteger( "RageGroup", "0_AntiSMAC", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].HitChance = GetPrivateProfileInteger( "RageGroup", "0_HitChance", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].StrictPrimary = GetPrivateProfileInteger( "RageGroup", "0_StrictPrimary", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].BestDamage = GetPrivateProfileInteger( "RageGroup", "0_BestDamage", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].MinDamage = GetPrivateProfileInteger( "RageGroup", "0_MinDamage", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].HitChanceValue = GetPrivateProfileInteger( "RageGroup", "0_HitChanceValue", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].Hitbox = GetPrivateProfileInteger( "RageGroup", "0_Hitbox", 12, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 0 ].FallbackHitbox = GetPrivateProfileInteger( "RageGroup", "0_FallbackHitbox", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].AutoShoot = GetPrivateProfileInteger( "RageGroup", "1_AutoShoot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].AutoStop = GetPrivateProfileInteger( "RageGroup", "1_AutoStop", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].AutoWall = GetPrivateProfileInteger( "RageGroup", "1_AutoWall", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].MultiSpot = GetPrivateProfileInteger( "RageGroup", "1_MultiSpot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].HitScan = GetPrivateProfileInteger( "RageGroup", "1_HitScan", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].BodyVsJump = GetPrivateProfileInteger( "RageGroup", "1_BodyVsJump", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].BodyAWP = GetPrivateProfileInteger( "RageGroup", "1_BodyAWP", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].AntiSMAC = GetPrivateProfileInteger( "RageGroup", "1_AntiSMAC", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].HitChance = GetPrivateProfileInteger( "RageGroup", "1_HitChance", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].StrictPrimary = GetPrivateProfileInteger( "RageGroup", "1_StrictPrimary", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].BestDamage = GetPrivateProfileInteger( "RageGroup", "1_BestDamage", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].MinDamage = GetPrivateProfileInteger( "RageGroup", "1_MinDamage", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].HitChanceValue = GetPrivateProfileInteger( "RageGroup", "1_HitChanceValue", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].Hitbox = GetPrivateProfileInteger( "RageGroup", "1_Hitbox", 12, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 1 ].FallbackHitbox = GetPrivateProfileInteger( "RageGroup", "1_FallbackHitbox", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].AutoShoot = GetPrivateProfileInteger( "RageGroup", "2_AutoShoot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].AutoStop = GetPrivateProfileInteger( "RageGroup", "2_AutoStop", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].AutoWall = GetPrivateProfileInteger( "RageGroup", "2_AutoWall", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].MultiSpot = GetPrivateProfileInteger( "RageGroup", "2_MultiSpot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].HitScan = GetPrivateProfileInteger( "RageGroup", "2_HitScan", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].BodyVsJump = GetPrivateProfileInteger( "RageGroup", "2_BodyVsJump", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].BodyAWP = GetPrivateProfileInteger( "RageGroup", "2_BodyAWP", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].AntiSMAC = GetPrivateProfileInteger( "RageGroup", "2_AntiSMAC", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].HitChance = GetPrivateProfileInteger( "RageGroup", "2_HitChance", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].StrictPrimary = GetPrivateProfileInteger( "RageGroup", "2_StrictPrimary", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].BestDamage = GetPrivateProfileInteger( "RageGroup", "2_BestDamage", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].MinDamage = GetPrivateProfileInteger( "RageGroup", "2_MinDamage", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].HitChanceValue = GetPrivateProfileInteger( "RageGroup", "2_HitChanceValue", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].Hitbox = GetPrivateProfileInteger( "RageGroup", "2_Hitbox", 12, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 2 ].FallbackHitbox = GetPrivateProfileInteger( "RageGroup", "2_FallbackHitbox", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].AutoShoot = GetPrivateProfileInteger( "RageGroup", "3_AutoShoot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].AutoStop = GetPrivateProfileInteger( "RageGroup", "3_AutoStop", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].AutoWall = GetPrivateProfileInteger( "RageGroup", "3_AutoWall", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].MultiSpot = GetPrivateProfileInteger( "RageGroup", "3_MultiSpot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].HitScan = GetPrivateProfileInteger( "RageGroup", "3_HitScan", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].BodyVsJump = GetPrivateProfileInteger( "RageGroup", "3_BodyVsJump", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].BodyAWP = GetPrivateProfileInteger( "RageGroup", "3_BodyAWP", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].AntiSMAC = GetPrivateProfileInteger( "RageGroup", "3_AntiSMAC", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].HitChance = GetPrivateProfileInteger( "RageGroup", "3_HitChance", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].StrictPrimary = GetPrivateProfileInteger( "RageGroup", "3_StrictPrimary", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].BestDamage = GetPrivateProfileInteger( "RageGroup", "3_BestDamage", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].MinDamage = GetPrivateProfileInteger( "RageGroup", "3_MinDamage", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].HitChanceValue = GetPrivateProfileInteger( "RageGroup", "3_HitChanceValue", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].Hitbox = GetPrivateProfileInteger( "RageGroup", "3_Hitbox", 12, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 3 ].FallbackHitbox = GetPrivateProfileInteger( "RageGroup", "3_FallbackHitbox", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].AutoShoot = GetPrivateProfileInteger( "RageGroup", "4_AutoShoot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].AutoStop = GetPrivateProfileInteger( "RageGroup", "4_AutoStop", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].AutoWall = GetPrivateProfileInteger( "RageGroup", "4_AutoWall", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].MultiSpot = GetPrivateProfileInteger( "RageGroup", "4_MultiSpot", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].HitScan = GetPrivateProfileInteger( "RageGroup", "4_HitScan", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].BodyVsJump = GetPrivateProfileInteger( "RageGroup", "4_BodyVsJump", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].BodyAWP = GetPrivateProfileInteger( "RageGroup", "4_BodyAWP", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].AntiSMAC = GetPrivateProfileInteger( "RageGroup", "4_AntiSMAC", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].HitChance = GetPrivateProfileInteger( "RageGroup", "4_HitChance", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].StrictPrimary = GetPrivateProfileInteger( "RageGroup", "4_StrictPrimary", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].BestDamage = GetPrivateProfileInteger( "RageGroup", "4_BestDamage", 1, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].MinDamage = GetPrivateProfileInteger( "RageGroup", "4_MinDamage", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].HitChanceValue = GetPrivateProfileInteger( "RageGroup", "4_HitChanceValue", 0, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].Hitbox = GetPrivateProfileInteger( "RageGroup", "4_Hitbox", 12, path.c_str( ) ); // r45
	g_CVars.Aimbot.RageGroup[ 4 ].FallbackHitbox = GetPrivateProfileInteger( "RageGroup", "4_FallbackHitbox", 0, path.c_str( ) ); // r45

	g_CVars.Triggerbot.Active = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Seed = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Seed*/XorStr<0x50,5,0x5388F9B1>("\x03\x34\x37\x37"+0x5388F9B1).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Strength = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Strength*/XorStr<0x4A,9,0x26680ABC>("\x19\x3F\x3E\x28\x20\x28\x24\x39"+0x26680ABC).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Key = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Key*/XorStr<0x19,4,0xBAF51996>("\x52\x7F\x62"+0xBAF51996).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Delay = GetPrivateProfileInteger( "Triggerbot", "Delay", 90, path.c_str( ) );
	g_CVars.Triggerbot.Hitbox = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Hitbox*/XorStr<0x61,7,0xE7859F64>("\x29\x0B\x17\x06\x0A\x1E"+0xE7859F64).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Spread = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Spread*/XorStr<0x0E,7,0x101D52DD>("\x5D\x7F\x62\x74\x73\x77"+0x101D52DD).s, 0, path.c_str( ) );
	g_CVars.Triggerbot.Recoil = GetPrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Recoil*/XorStr<0xB0,7,0x998D4C43>("\xE2\xD4\xD1\xDC\xDD\xD9"+0x998D4C43).s, 0, path.c_str( ) );

	g_CVars.Accuracy.ForceSeed = GetPrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*ForceSeed*/XorStr<0x17,10,0xB103C520>("\x51\x77\x6B\x79\x7E\x4F\x78\x7B\x7B"+0xB103C520).s, 0, path.c_str( ) );
	g_CVars.Accuracy.PerfectAccuracy = GetPrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*PerfectAccuracy*/XorStr<0x62,16,0xCAB6BBCD>("\x32\x06\x16\x03\x03\x04\x1C\x28\x09\x08\x19\x1F\x0F\x0C\x09"+0xCAB6BBCD).s, 0, path.c_str( ) );
	g_CVars.Accuracy.NoSpreadMode = GetPrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*NoSpreadMode*/XorStr<0x89,13,0xABE30353>("\xC7\xE5\xD8\xFC\xFF\xEB\xEE\xF4\xDC\xFD\xF7\xF1"+0xABE30353).s, 1, path.c_str( ) );

	g_CVars.Visuals.ESP.Box = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Box", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Name = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Name", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Health = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Health", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Weapon = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Weapon", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Bone = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Bone", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.AimSpot = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "AimSpot", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Hit = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Hit", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.Ground = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Ground", 0, path.c_str( ) );
	g_CVars.Visuals.ESP.EnemyOnly = GetPrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "EnemyOnly", 0, path.c_str( ) );

	g_CVars.Visuals.ESP.BoxStyle = GetPrivateProfileInteger( "ESP", "BoxStyle", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.HealthStyle = GetPrivateProfileInteger( "ESP", "HealthStyle", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.Armor = GetPrivateProfileInteger( "ESP", "Armor", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.Ammo = GetPrivateProfileInteger( "ESP", "Ammo", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.Fake = GetPrivateProfileInteger( "ESP", "Fake", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.ShowFake = GetPrivateProfileInteger( "ESP", "ShowFake", 0, path.c_str( ) ); // r21

	g_CVars.Visuals.ESP.Dormant = GetPrivateProfileInteger( "ESP", "Dormant", 1, path.c_str( ) );
	g_CVars.Visuals.ESP.OOF = GetPrivateProfileInteger( "ESP", "OOF", 1, path.c_str( ) );

	g_CVars.Visuals.Chams.Active = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Visuals.Chams.Weapons = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Weapons", 0, path.c_str( ) );
	g_CVars.Visuals.Chams.Shadows = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Shadows", 0, path.c_str( ) );
	g_CVars.Visuals.Chams.Outline = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Outline", 0, path.c_str( ) );
	g_CVars.Visuals.Chams.HandsOutline = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "HandsOutline", 0, path.c_str( ) );
	g_CVars.Visuals.Chams.EnemyOnly = GetPrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "EnemyOnly", 0, path.c_str( ) );
	g_CVars.Visuals.Chams.Style = GetPrivateProfileInteger( "Chams", "ChamStyle", 0, path.c_str( ) );

	g_CVars.Visuals.Crosshair.Type = GetPrivateProfileInteger( /*Crosshair*/XorStr<0x43,10,0x4B4B568D>("\x00\x36\x2A\x35\x34\x20\x28\x23\x39"+0x4B4B568D).s, /*Type*/XorStr<0x33,5,0x7A74F612>("\x67\x4D\x45\x53"+0x7A74F612).s, 0, path.c_str( ) );
	g_CVars.Visuals.Crosshair.Dynamic = GetPrivateProfileInteger( /*Crosshair*/XorStr<0x43,10,0x4B4B568D>("\x00\x36\x2A\x35\x34\x20\x28\x23\x39"+0x4B4B568D).s, "Dynamic", 0, path.c_str( ) );

	g_CVars.Visuals.ASUS = GetPrivateProfileFloat( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "ASUS", 0, path.c_str( ) );
	g_CVars.Visuals.Radar = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "Radar", 0, path.c_str( ) );
	g_CVars.Visuals.EventLog = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "EventLog", 0, path.c_str( ) );
	g_CVars.Aimbot.FallbackHitbox = GetPrivateProfileInteger( "Aimbot", "FallbackHitbox", 0, path.c_str( ) ); // r25
	g_CVars.Aimbot.Interpolation.DisableInterp = GetPrivateProfileInteger( "Aimbot", "DisableInterp", 0, path.c_str( ) ); // r25
	g_CVars.Visuals.SpectatorList = GetPrivateProfileInteger( "ESP", "SpectatorList", 0, path.c_str( ) ); // r25
	g_CVars.Visuals.PlayerList = GetPrivateProfileInteger( "ESP", "PlayerList", 0, path.c_str( ) ); // r44
	g_CVars.Visuals.ShotLog = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "ShotLog", 0, path.c_str( ) );
	g_CVars.Visuals.Indicators = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "Indicators", 1, path.c_str( ) );
	g_CVars.Visuals.NoSky = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoSky", 0, path.c_str( ) );
	g_CVars.Visuals.NoHands = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoHands", 0, path.c_str( ) );
	g_CVars.Visuals.NoSmoke = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoSmoke", 0, path.c_str( ) );
	g_CVars.Visuals.NoFlash = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoFlash", 0, path.c_str( ) );
	g_CVars.Visuals.NoVisualRecoil = GetPrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoVisualRecoil", 0, path.c_str( ) );

	g_CVars.Miscellaneous.AntiAim.Active = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.Pitch = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Pitch", 0, path.c_str( ) );

	g_CVars.Miscellaneous.AntiAim.Yaw = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Yaw", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.Variation = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Variation", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.WallDetectionMode = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "WallDetectionMode", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickEnable = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickEnable", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickTicks = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickTicks", 8, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickRandom = GetPrivateProfileInteger( XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickRandom", 1, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickOnShot = GetPrivateProfileInteger( XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickOnShot", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickSide = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickSide", 8, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.Static = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Static", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.WallDetection = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "WallDetection", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.AtTargets = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "AtTargets", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.DuckInAir = GetPrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "DuckInAir", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.RealValue = GetPrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "RealValue", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FakeValue = GetPrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FakeValue", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.FlickAngle = GetPrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickAngle", 90, path.c_str( ) );
	g_CVars.Miscellaneous.AntiAim.TurnOff = GetPrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "EnemyCheck", 0, path.c_str( ) );

	g_CVars.Miscellaneous.Fakelag.Active = GetPrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, 0, path.c_str( ) );
	g_CVars.Miscellaneous.Fakelag.Mode = GetPrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, /*Mode*/XorStr<0xA1,5,0x038D62E9>("\xEC\xCD\xC7\xC1"+0x038D62E9).s, 0, path.c_str( ) );
	g_CVars.Miscellaneous.Fakelag.Value = GetPrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "Value", 0, path.c_str( ) );
	g_CVars.Miscellaneous.Fakelag.InAttack = GetPrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "InAttack", 0, path.c_str( ) );
	g_CVars.Miscellaneous.Fakelag.AirOnly = GetPrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "AirOnly", 0, path.c_str( ) );

	g_CVars.Miscellaneous.BunnyHop = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "BunnyHop", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AutoStrafe = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoStrafe", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AutoStrafeMode = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoStrafeMode", 0, path.c_str( ) ); // r38
	g_CVars.Miscellaneous.StrafeAvoidDist = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "StrafeAvoidDist", 64, path.c_str( ) ); // r38
	g_CVars.Miscellaneous.CircleStrafe = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "CircleStrafe", 0, path.c_str( ) );
	g_CVars.Miscellaneous.EdgeJump = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "EdgeJump", 0, path.c_str( ) );
	g_CVars.Miscellaneous.Speedhack = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Speedhack", 0, path.c_str( ) );
	g_CVars.Miscellaneous.SpeedhackValue = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SpeedhackValue", 0, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTap = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTap", 0, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTapAuto = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapAuto", 1, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTapTicks = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapTicks", 2, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTapMode = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapMode", 0, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTapOnlyGround = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapOnlyGround", 1, path.c_str( ) );
	g_CVars.Miscellaneous.DoubleTapDelayShot = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapDelayShot", 1, path.c_str( ) );
	g_CVars.Miscellaneous.ThirdPersonKey = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "ThirdPersonKey", 0, path.c_str( ) );
	g_CVars.Miscellaneous.ThirdPersonDist = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "ThirdPersonDist", 120, path.c_str( ) );
	g_CVars.Miscellaneous.SlowWalk = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalk", 1, path.c_str( ) ); // r41: default ON
	g_CVars.Miscellaneous.SlowWalkKey = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalkKey", 6, path.c_str( ) ); // r41: default SHIFT (index 6)
	g_CVars.Miscellaneous.SlowWalkSpeed = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalkSpeed", 120, path.c_str( ) );
	g_CVars.Miscellaneous.SpeedhackKey = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SpeedhackKey", 0, path.c_str( ) );
	g_CVars.Miscellaneous.MenuKey = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuKey", 45, path.c_str( ) );
	g_CVars.Miscellaneous.MenuTheme = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuTheme", 0, path.c_str( ) );
	g_CVars.Miscellaneous.MenuMode = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuMode", 0, path.c_str( ) );
	g_CVars.Miscellaneous.Fakeduck = GetPrivateProfileInteger( "Miscellaneous", "Fakeduck", 0, path.c_str( ) ); // r24
	g_CVars.Miscellaneous.ClientModEmulator = GetPrivateProfileInteger( "Miscellaneous", "ClientModEmulator", 1, path.c_str( ) ); // r34
	g_CVars.Miscellaneous.FakeduckKey = GetPrivateProfileInteger( "Miscellaneous", "FakeduckKey", 0, path.c_str( ) ); // r24
	g_CVars.Miscellaneous.Micromoves = GetPrivateProfileInteger( "Miscellaneous", "Micromoves", 0, path.c_str( ) ); // r24
	g_CVars.Miscellaneous.AutoKnife = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoKnife", 0, path.c_str( ) );
	g_CVars.Miscellaneous.RoundSay = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "RoundSay", 0, path.c_str( ) );
	g_CVars.Miscellaneous.CheatsBypass = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "CheatsBypass", 0, path.c_str( ) );
	g_CVars.Miscellaneous.AirStuck = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AirStuck", 0, path.c_str( ) );

	g_CVars.Menu.x = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Menu.x", 0, path.c_str( ) );
	g_CVars.Menu.y = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Menu.y", 0, path.c_str( ) );

	g_CVars.Radar.x = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Radar.x", 0, path.c_str( ) );
	g_CVars.Radar.y = GetPrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Radar.y", 0, path.c_str( ) );

	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.TT", g_CVars.ColorSelector.ESP.TT, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.CT", g_CVars.ColorSelector.ESP.CT, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.Wpn", g_CVars.ColorSelector.ESP.Wpn, path.c_str( ) );

	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTInvis", g_CVars.ColorSelector.Chams.CTInvis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTVis", g_CVars.ColorSelector.Chams.CTVis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTOutline", g_CVars.ColorSelector.Chams.CTOutline, path.c_str( ) );

	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTInvis", g_CVars.ColorSelector.Chams.TTInvis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTVis", g_CVars.ColorSelector.Chams.TTVis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTOutline", g_CVars.ColorSelector.Chams.TTOutline, path.c_str( ) );

	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnInvis", g_CVars.ColorSelector.Chams.WpnInvis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnVis", g_CVars.ColorSelector.Chams.WpnVis, path.c_str( ) );
	GetPrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnOutline", g_CVars.ColorSelector.Chams.WpnOutline, path.c_str( ) );
	return true;
}

bool CConfig::WriteProfile( const std::string& path )
{
	// config identity header: every named config keeps its own name inside
	{
		std::string fname = path;
		size_t slash = fname.find_last_of( '\\' );
		if( slash != std::string::npos ) fname = fname.substr( slash + 1 );
		if( fname.size( ) > 4 ) fname = fname.substr( 0, fname.size( ) - 4 );
		WritePrivateProfileStringA( "Awesware", "Name", fname.c_str( ), path.c_str( ) );
		WritePrivateProfileStringA( "Awesware", "Game", "Counter-Strike: Source v34", path.c_str( ) );
		char stamp[ 64 ]; sprintf( stamp, "%s %s", __DATE__, __TIME__ );
		WritePrivateProfileStringA( "Awesware", "Saved", stamp, path.c_str( ) );
	}
		WritePrivateProfileInteger( "Legit", "Active", g_CVars.Legit.Active, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AutoShoot", g_CVars.Legit.AutoShoot, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "Silent", g_CVars.Legit.Silent, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "SnapLimiter", g_CVars.Legit.SnapLimiter, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AngleLimit", g_CVars.Legit.AngleLimit, path.c_str( ) );
	WritePrivateProfileFloat( "Legit", "AngleLimitTens", g_CVars.Legit.AngleLimitTens, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "Key", g_CVars.Legit.Key, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AimFOV", g_CVars.Legit.AimFOV, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "FovNear", g_CVars.Legit.FovNear, path.c_str( ) ); // r29
	WritePrivateProfileInteger( "Legit", "FovFar", g_CVars.Legit.FovFar, path.c_str( ) ); // r29
	WritePrivateProfileInteger( "Legit", "FovSwitchDist", g_CVars.Legit.FovSwitchDist, path.c_str( ) ); // r29
	WritePrivateProfileInteger( "Legit", "Hitbox", g_CVars.Legit.Hitbox, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AutoStop", g_CVars.Legit.AutoStop, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "ScopedCheck", g_CVars.Legit.ScopedCheck, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AutoScope", g_CVars.Legit.AutoScope, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "LegitAA", g_CVars.Legit.LegitAA, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "LegitAAKey", g_CVars.Legit.LegitAAKey, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "LegitAAAngle", g_CVars.Legit.LegitAAAngle, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "LegitAAInvertKey", g_CVars.Legit.LegitAAInvertKey, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "BacktrackTicks", g_CVars.Legit.BacktrackTicks, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AimType", g_CVars.Legit.AimType, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "Smoothing", g_CVars.Legit.Smoothing, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "ReactionMs", g_CVars.Legit.ReactionMs, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "RCS", g_CVars.Legit.RCS, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "RCSStandalone", g_CVars.Legit.RCSStandalone, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AimLock", g_CVars.Legit.AimLock, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "DesyncResolver", g_CVars.Legit.DesyncResolver, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "DesyncAA", g_CVars.Legit.DesyncAA, path.c_str( ) ); // r15
	WritePrivateProfileInteger( "Legit", "DesyncYaw", g_CVars.Legit.DesyncYaw, path.c_str( ) ); // r15
	WritePrivateProfileInteger( "Legit", "DesyncChoke", g_CVars.Legit.DesyncChoke, path.c_str( ) ); // r43
	WritePrivateProfileInteger( "Legit", "FlashCheck", g_CVars.Legit.FlashCheck, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "TargetSelection", g_CVars.Legit.TargetSelection, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "StrafeActive", g_CVars.Legit.StrafeActive, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "StrafePower", g_CVars.Legit.StrafePower, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "Prediction", g_CVars.Legit.Prediction, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "FovCircle", g_CVars.Legit.FovCircle, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "AutoPistol", g_CVars.Legit.AutoPistol, path.c_str( ) );
	WritePrivateProfileInteger( "Legit", "KillDelayMs", g_CVars.Legit.KillDelayMs, path.c_str( ) );
	for( int lg = 0; lg < 7; lg++ ) { char k[ 8 ]; sprintf( k, "GH%d", lg ); WritePrivateProfileInteger( "Legit", k, g_CVars.Legit.HitboxGroup[ lg ], path.c_str( ) ); }

	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Aimbot.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AutoShoot*/XorStr<0x11,10,0xA8DBBE49>("\x50\x67\x67\x7B\x46\x7E\x78\x77\x6D"+0xA8DBBE49).s, g_CVars.Aimbot.AutoShoot, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AutoWall*/XorStr<0xEF,9,0x87A95EA9>("\xAE\x85\x85\x9D\xA4\x95\x99\x9A"+0x87A95EA9).s, g_CVars.Aimbot.AutoWall, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*MultiSpot*/XorStr<0xD1,10,0x86EC0E0A>("\x9C\xA7\xBF\xA0\xBC\x85\xA7\xB7\xAD"+0x86EC0E0A).s, g_CVars.Aimbot.MultiSpot, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*HitScan*/XorStr<0xCB,8,0xFC340577>("\x83\xA5\xB9\x9D\xAC\xB1\xBF"+0xFC340577).s, g_CVars.Aimbot.HitScan, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*FriendlyFire*/XorStr<0xFC,13,0x954C6400>("\xBA\x8F\x97\x9A\x6E\x65\x6E\x7A\x42\x6C\x74\x62"+0x954C6400).s, g_CVars.Aimbot.FriendlyFire, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*TargetSelection*/XorStr<0xC3,16,0x9E78006D>("\x97\xA5\xB7\xA1\xA2\xBC\x9A\xAF\xA7\xA9\xAE\xBA\xA6\xBF\xBF"+0x9E78006D).s, g_CVars.Aimbot.TargetSelection, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Hitbox*/XorStr<0x61,7,0xE7859F64>("\x29\x0B\x17\x06\x0A\x1E"+0xE7859F64).s, g_CVars.Aimbot.Hitbox, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*HitboxMode*/XorStr<0xDB,11,0xBA9B04D1>("\x93\xB5\xA9\xBC\xB0\x98\xAC\x8D\x87\x81"+0xBA9B04D1).s, g_CVars.Aimbot.HitboxMode, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Silent*/XorStr<0xE1,7,0xA335B50B>("\xB2\x8B\x8F\x81\x8B\x92"+0xA335B50B).s, g_CVars.Aimbot.Silent, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*PerfectSilent*/XorStr<0xEC,14,0xBB750D0C>("\xBC\x88\x9C\x89\x95\x92\x86\xA0\x9D\x99\x93\x99\x8C"+0xBB750D0C).s, g_CVars.Aimbot.PerfectSilent, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AntiSMAC*/XorStr<0x07,9,0xDB906D0E>("\x46\x66\x7D\x63\x58\x41\x4C\x4D"+0xDB906D0E).s, g_CVars.Aimbot.AntiSMAC, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*BodyAWP*/XorStr<0xCA,8,0x15077618>("\x88\xA4\xA8\xB4\x8F\x98\x80"+0x15077618).s, g_CVars.Aimbot.BodyAWP, path.c_str( ) );
	WritePrivateProfileFloat( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*PointScale*/XorStr<0xBE,11,0x1EFB65EC>("\xEE\xD0\xA9\xAF\xB6\x90\xA7\xA4\xAA\xA2"+0x1EFB65EC).s, g_CVars.Aimbot.PointScale, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*SnapLimiter*/XorStr<0xCD,12,0xE92FD079>("\x9E\xA0\xAE\xA0\x9D\xBB\xBE\xBD\xA1\xB3\xA5"+0xE92FD079).s, g_CVars.Aimbot.SnapLimiter, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AngleLimit*/XorStr<0x55,11,0x2BCB8202>("\x14\x38\x30\x34\x3C\x16\x32\x31\x34\x2A"+0x2BCB8202).s, g_CVars.Aimbot.AngleLimit, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*AngleLimitTens*/XorStr<0x4F,15,0x42DB868E>("\x0E\x3E\x36\x3E\x36\x18\x3C\x3B\x3E\x2C\x0D\x3F\x35\x2F"+0x42DB868E).s, g_CVars.Aimbot.AngleLimitTens, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*MinDamage*/XorStr<0x43,10,0xD1CF37C2>("\x0E\x2D\x2B\x02\x26\x25\x28\x2D\x2E"+0xD1CF37C2).s, g_CVars.Aimbot.MinDamage, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*Key*/XorStr<0x19,4,0xBAF51996>("\x52\x7F\x62"+0xBAF51996).s, g_CVars.Aimbot.Key, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "AutoStop", g_CVars.Aimbot.AutoStop, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BodyVsJump", g_CVars.Aimbot.BodyVsJump, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceBodyKey", g_CVars.Aimbot.ForceBodyKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceMinDmgKey", g_CVars.Aimbot.ForceMinDmgKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "ForceMinDmgValue", g_CVars.Aimbot.ForceMinDmgValue, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "StrictPrimary", g_CVars.Aimbot.StrictPrimary, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BestDamage", g_CVars.Aimbot.BestDamage, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitChance", g_CVars.Aimbot.HitChance, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitChanceValue", g_CVars.Aimbot.HitChanceValue, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "BacktrackTicks", g_CVars.Aimbot.BacktrackTicks, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "AimFOV", g_CVars.Aimbot.AimFOV, path.c_str( ) );
	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "LongRangeDist", g_CVars.Aimbot.LongRangeDist, path.c_str( ) );
	{
		int packedGroups = 0;
		for( int hg = 0; hg < 7; hg++ ) packedGroups |= ( ( g_CVars.Aimbot.HitboxGroup[ hg ] & 3 ) << ( hg * 2 ) );
		WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, "HitboxGroups", packedGroups, path.c_str( ) );
	}

	WritePrivateProfileInteger( /*Aimbot*/XorStr<0x06,7,0x1D2CE0D1>("\x47\x6E\x65\x6B\x65\x7F"+0x1D2CE0D1).s, /*LagPrediction*/XorStr<0x0C,14,0x8FC405E0>("\x40\x6C\x69\x5F\x62\x74\x76\x7A\x77\x61\x7F\x78\x76"+0x8FC405E0).s, g_CVars.Aimbot.Interpolation.LagPrediction, path.c_str( ) );

	WritePrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Aimbot.Resolver.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Smart*/XorStr<0x1B,6,0x3199AD52>("\x48\x71\x7C\x6C\x6B"+0x3199AD52).s, g_CVars.Aimbot.Resolver.Smart, path.c_str( ) );
	WritePrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Mode*/XorStr<0xA1,5,0x038D62E9>("\xEC\xCD\xC7\xC1"+0x038D62E9).s, g_CVars.Aimbot.Resolver.Mode, path.c_str( ) );
	WritePrivateProfileInteger( /*Resolver*/XorStr<0xC1,9,0xE5DCA663>("\x93\xA7\xB0\xAB\xA9\xB0\xA2\xBA"+0xE5DCA663).s, /*Type*/XorStr<0x33,5,0x7A74F612>("\x67\x4D\x45\x53"+0x7A74F612).s, g_CVars.Aimbot.Resolver.Type, path.c_str( ) );
	WritePrivateProfileInteger( "Resolver", "LagRecords", g_CVars.Aimbot.Resolver.LagRecords ? 1 : 0, path.c_str( ) ); // r40
	WritePrivateProfileInteger( "RageGroup", "Enabled", g_CVars.Aimbot.RageGroups ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_AutoShoot", g_CVars.Aimbot.RageGroup[ 0 ].AutoShoot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_AutoStop", g_CVars.Aimbot.RageGroup[ 0 ].AutoStop ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_AutoWall", g_CVars.Aimbot.RageGroup[ 0 ].AutoWall ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_MultiSpot", g_CVars.Aimbot.RageGroup[ 0 ].MultiSpot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_HitScan", g_CVars.Aimbot.RageGroup[ 0 ].HitScan ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_BodyVsJump", g_CVars.Aimbot.RageGroup[ 0 ].BodyVsJump ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_BodyAWP", g_CVars.Aimbot.RageGroup[ 0 ].BodyAWP ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_AntiSMAC", g_CVars.Aimbot.RageGroup[ 0 ].AntiSMAC ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_HitChance", g_CVars.Aimbot.RageGroup[ 0 ].HitChance ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_StrictPrimary", g_CVars.Aimbot.RageGroup[ 0 ].StrictPrimary ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_BestDamage", g_CVars.Aimbot.RageGroup[ 0 ].BestDamage ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_MinDamage", g_CVars.Aimbot.RageGroup[ 0 ].MinDamage, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_HitChanceValue", g_CVars.Aimbot.RageGroup[ 0 ].HitChanceValue, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_Hitbox", g_CVars.Aimbot.RageGroup[ 0 ].Hitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "0_FallbackHitbox", g_CVars.Aimbot.RageGroup[ 0 ].FallbackHitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_AutoShoot", g_CVars.Aimbot.RageGroup[ 1 ].AutoShoot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_AutoStop", g_CVars.Aimbot.RageGroup[ 1 ].AutoStop ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_AutoWall", g_CVars.Aimbot.RageGroup[ 1 ].AutoWall ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_MultiSpot", g_CVars.Aimbot.RageGroup[ 1 ].MultiSpot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_HitScan", g_CVars.Aimbot.RageGroup[ 1 ].HitScan ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_BodyVsJump", g_CVars.Aimbot.RageGroup[ 1 ].BodyVsJump ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_BodyAWP", g_CVars.Aimbot.RageGroup[ 1 ].BodyAWP ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_AntiSMAC", g_CVars.Aimbot.RageGroup[ 1 ].AntiSMAC ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_HitChance", g_CVars.Aimbot.RageGroup[ 1 ].HitChance ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_StrictPrimary", g_CVars.Aimbot.RageGroup[ 1 ].StrictPrimary ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_BestDamage", g_CVars.Aimbot.RageGroup[ 1 ].BestDamage ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_MinDamage", g_CVars.Aimbot.RageGroup[ 1 ].MinDamage, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_HitChanceValue", g_CVars.Aimbot.RageGroup[ 1 ].HitChanceValue, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_Hitbox", g_CVars.Aimbot.RageGroup[ 1 ].Hitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "1_FallbackHitbox", g_CVars.Aimbot.RageGroup[ 1 ].FallbackHitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_AutoShoot", g_CVars.Aimbot.RageGroup[ 2 ].AutoShoot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_AutoStop", g_CVars.Aimbot.RageGroup[ 2 ].AutoStop ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_AutoWall", g_CVars.Aimbot.RageGroup[ 2 ].AutoWall ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_MultiSpot", g_CVars.Aimbot.RageGroup[ 2 ].MultiSpot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_HitScan", g_CVars.Aimbot.RageGroup[ 2 ].HitScan ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_BodyVsJump", g_CVars.Aimbot.RageGroup[ 2 ].BodyVsJump ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_BodyAWP", g_CVars.Aimbot.RageGroup[ 2 ].BodyAWP ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_AntiSMAC", g_CVars.Aimbot.RageGroup[ 2 ].AntiSMAC ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_HitChance", g_CVars.Aimbot.RageGroup[ 2 ].HitChance ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_StrictPrimary", g_CVars.Aimbot.RageGroup[ 2 ].StrictPrimary ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_BestDamage", g_CVars.Aimbot.RageGroup[ 2 ].BestDamage ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_MinDamage", g_CVars.Aimbot.RageGroup[ 2 ].MinDamage, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_HitChanceValue", g_CVars.Aimbot.RageGroup[ 2 ].HitChanceValue, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_Hitbox", g_CVars.Aimbot.RageGroup[ 2 ].Hitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "2_FallbackHitbox", g_CVars.Aimbot.RageGroup[ 2 ].FallbackHitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_AutoShoot", g_CVars.Aimbot.RageGroup[ 3 ].AutoShoot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_AutoStop", g_CVars.Aimbot.RageGroup[ 3 ].AutoStop ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_AutoWall", g_CVars.Aimbot.RageGroup[ 3 ].AutoWall ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_MultiSpot", g_CVars.Aimbot.RageGroup[ 3 ].MultiSpot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_HitScan", g_CVars.Aimbot.RageGroup[ 3 ].HitScan ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_BodyVsJump", g_CVars.Aimbot.RageGroup[ 3 ].BodyVsJump ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_BodyAWP", g_CVars.Aimbot.RageGroup[ 3 ].BodyAWP ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_AntiSMAC", g_CVars.Aimbot.RageGroup[ 3 ].AntiSMAC ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_HitChance", g_CVars.Aimbot.RageGroup[ 3 ].HitChance ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_StrictPrimary", g_CVars.Aimbot.RageGroup[ 3 ].StrictPrimary ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_BestDamage", g_CVars.Aimbot.RageGroup[ 3 ].BestDamage ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_MinDamage", g_CVars.Aimbot.RageGroup[ 3 ].MinDamage, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_HitChanceValue", g_CVars.Aimbot.RageGroup[ 3 ].HitChanceValue, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_Hitbox", g_CVars.Aimbot.RageGroup[ 3 ].Hitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "3_FallbackHitbox", g_CVars.Aimbot.RageGroup[ 3 ].FallbackHitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_AutoShoot", g_CVars.Aimbot.RageGroup[ 4 ].AutoShoot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_AutoStop", g_CVars.Aimbot.RageGroup[ 4 ].AutoStop ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_AutoWall", g_CVars.Aimbot.RageGroup[ 4 ].AutoWall ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_MultiSpot", g_CVars.Aimbot.RageGroup[ 4 ].MultiSpot ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_HitScan", g_CVars.Aimbot.RageGroup[ 4 ].HitScan ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_BodyVsJump", g_CVars.Aimbot.RageGroup[ 4 ].BodyVsJump ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_BodyAWP", g_CVars.Aimbot.RageGroup[ 4 ].BodyAWP ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_AntiSMAC", g_CVars.Aimbot.RageGroup[ 4 ].AntiSMAC ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_HitChance", g_CVars.Aimbot.RageGroup[ 4 ].HitChance ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_StrictPrimary", g_CVars.Aimbot.RageGroup[ 4 ].StrictPrimary ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_BestDamage", g_CVars.Aimbot.RageGroup[ 4 ].BestDamage ? 1 : 0, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_MinDamage", g_CVars.Aimbot.RageGroup[ 4 ].MinDamage, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_HitChanceValue", g_CVars.Aimbot.RageGroup[ 4 ].HitChanceValue, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_Hitbox", g_CVars.Aimbot.RageGroup[ 4 ].Hitbox, path.c_str( ) ); // r45
	WritePrivateProfileInteger( "RageGroup", "4_FallbackHitbox", g_CVars.Aimbot.RageGroup[ 4 ].FallbackHitbox, path.c_str( ) ); // r45

	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Triggerbot.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Seed*/XorStr<0x50,5,0x5388F9B1>("\x03\x34\x37\x37"+0x5388F9B1).s, g_CVars.Triggerbot.Seed, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Strength*/XorStr<0x4A,9,0x26680ABC>("\x19\x3F\x3E\x28\x20\x28\x24\x39"+0x26680ABC).s, g_CVars.Triggerbot.Strength, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Key*/XorStr<0x19,4,0xBAF51996>("\x52\x7F\x62"+0xBAF51996).s, g_CVars.Triggerbot.Key, path.c_str( ) );
	WritePrivateProfileInteger( "Triggerbot", "Delay", g_CVars.Triggerbot.Delay, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Hitbox*/XorStr<0x61,7,0xE7859F64>("\x29\x0B\x17\x06\x0A\x1E"+0xE7859F64).s, g_CVars.Triggerbot.Hitbox, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Recoil*/XorStr<0xB0,7,0x998D4C43>("\xE2\xD4\xD1\xDC\xDD\xD9"+0x998D4C43).s, g_CVars.Triggerbot.Recoil, path.c_str( ) );
	WritePrivateProfileInteger( /*Triggerbot*/XorStr<0x57,11,0x2DB2B1FA>("\x03\x2A\x30\x3D\x3C\x39\x2F\x3C\x30\x14"+0x2DB2B1FA).s, /*Spread*/XorStr<0x0E,7,0x101D52DD>("\x5D\x7F\x62\x74\x73\x77"+0x101D52DD).s, g_CVars.Triggerbot.Spread, path.c_str( ) );

	WritePrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*ForceSeed*/XorStr<0x17,10,0xB103C520>("\x51\x77\x6B\x79\x7E\x4F\x78\x7B\x7B"+0xB103C520).s, g_CVars.Accuracy.ForceSeed, path.c_str( ) );
	WritePrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*PerfectAccuracy*/XorStr<0x62,16,0xCAB6BBCD>("\x32\x06\x16\x03\x03\x04\x1C\x28\x09\x08\x19\x1F\x0F\x0C\x09"+0xCAB6BBCD).s, g_CVars.Accuracy.PerfectAccuracy, path.c_str( ) );
	WritePrivateProfileInteger( /*Accuracy*/XorStr<0xAF,9,0x175251A2>("\xEE\xD3\xD2\xC7\xC1\xD5\xD6\xCF"+0x175251A2).s, /*NoSpreadMode*/XorStr<0x89,13,0xABE30353>("\xC7\xE5\xD8\xFC\xFF\xEB\xEE\xF4\xDC\xFD\xF7\xF1"+0xABE30353).s, g_CVars.Accuracy.NoSpreadMode, path.c_str( ) );

	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Box", g_CVars.Visuals.ESP.Box, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Name", g_CVars.Visuals.ESP.Name, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Health", g_CVars.Visuals.ESP.Health, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Weapon", g_CVars.Visuals.ESP.Weapon, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Bone", g_CVars.Visuals.ESP.Bone, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "AimSpot", g_CVars.Visuals.ESP.AimSpot, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Hit", g_CVars.Visuals.ESP.Hit, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "Ground", g_CVars.Visuals.ESP.Ground, path.c_str( ) );
	WritePrivateProfileInteger( /*ESP*/XorStr<0x4F,4,0x16AF489D>("\x0A\x03\x01"+0x16AF489D).s, "EnemyOnly", g_CVars.Visuals.ESP.EnemyOnly, path.c_str( ) );

	WritePrivateProfileInteger( "ESP", "BoxStyle", g_CVars.Visuals.ESP.BoxStyle, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "HealthStyle", g_CVars.Visuals.ESP.HealthStyle, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "Armor", g_CVars.Visuals.ESP.Armor, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "Ammo", g_CVars.Visuals.ESP.Ammo, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "Fake", g_CVars.Visuals.ESP.Fake, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "ShowFake", g_CVars.Visuals.ESP.ShowFake, path.c_str( ) ); // r21

	WritePrivateProfileInteger( "ESP", "Dormant", g_CVars.Visuals.ESP.Dormant, path.c_str( ) );
	WritePrivateProfileInteger( "ESP", "OOF", g_CVars.Visuals.ESP.OOF, path.c_str( ) );

	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Visuals.Chams.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Weapons", g_CVars.Visuals.Chams.Weapons, path.c_str( ) );
	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Shadows", g_CVars.Visuals.Chams.Shadows, path.c_str( ) );
	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "Outline", g_CVars.Visuals.Chams.Outline, path.c_str( ) );
	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "HandsOutline", g_CVars.Visuals.Chams.HandsOutline, path.c_str( ) );
	WritePrivateProfileInteger( /*Chams*/XorStr<0xEC,6,0xBE77CCE8>("\xAF\x85\x8F\x82\x83"+0xBE77CCE8).s, "EnemyOnly", g_CVars.Visuals.Chams.EnemyOnly, path.c_str( ) );
	WritePrivateProfileInteger( "Chams", "ChamStyle", g_CVars.Visuals.Chams.Style, path.c_str( ) );

	WritePrivateProfileInteger( /*Crosshair*/XorStr<0x43,10,0x4B4B568D>("\x00\x36\x2A\x35\x34\x20\x28\x23\x39"+0x4B4B568D).s, /*Type*/XorStr<0x33,5,0x7A74F612>("\x67\x4D\x45\x53"+0x7A74F612).s, g_CVars.Visuals.Crosshair.Type, path.c_str( ) );
	WritePrivateProfileInteger( /*Crosshair*/XorStr<0x43,10,0x4B4B568D>("\x00\x36\x2A\x35\x34\x20\x28\x23\x39"+0x4B4B568D).s, "Dynamic", g_CVars.Visuals.Crosshair.Dynamic, path.c_str( ) );

	WritePrivateProfileFloat( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "ASUS", g_CVars.Visuals.ASUS, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "Radar", g_CVars.Visuals.Radar, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "EventLog", g_CVars.Visuals.EventLog, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "ShotLog", g_CVars.Visuals.ShotLog, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "Indicators", g_CVars.Visuals.Indicators, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoSky", g_CVars.Visuals.NoSky, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoHands", g_CVars.Visuals.NoHands, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoSmoke", g_CVars.Visuals.NoSmoke, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoFlash", g_CVars.Visuals.NoFlash, path.c_str( ) );
	WritePrivateProfileInteger( /*Visuals*/XorStr<0xE2,8,0xAB24030E>("\xB4\x8A\x97\x90\x87\x8B\x9B"+0xAB24030E).s, "NoVisualRecoil", g_CVars.Visuals.NoVisualRecoil, path.c_str( ) );

	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Miscellaneous.AntiAim.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Pitch", g_CVars.Miscellaneous.AntiAim.Pitch, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Yaw", g_CVars.Miscellaneous.AntiAim.Yaw, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Variation", g_CVars.Miscellaneous.AntiAim.Variation, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "Static", g_CVars.Miscellaneous.AntiAim.Static, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "WallDetection", g_CVars.Miscellaneous.AntiAim.WallDetection, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "WallDetectionMode", g_CVars.Miscellaneous.AntiAim.WallDetectionMode, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickEnable", g_CVars.Miscellaneous.AntiAim.FlickEnable, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickTicks", g_CVars.Miscellaneous.AntiAim.FlickTicks, path.c_str( ) );
	WritePrivateProfileInteger( XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickRandom", g_CVars.Miscellaneous.AntiAim.FlickRandom, path.c_str( ) );
	WritePrivateProfileInteger( XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickOnShot", g_CVars.Miscellaneous.AntiAim.FlickOnShot, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickSide", g_CVars.Miscellaneous.AntiAim.FlickSide, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "AtTargets", g_CVars.Miscellaneous.AntiAim.AtTargets, path.c_str( ) );
	WritePrivateProfileInteger( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "DuckInAir", g_CVars.Miscellaneous.AntiAim.DuckInAir, path.c_str( ) );
	WritePrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "RealValue", g_CVars.Miscellaneous.AntiAim.RealValue, path.c_str( ) );
	WritePrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FakeValue", g_CVars.Miscellaneous.AntiAim.FakeValue, path.c_str( ) );
	WritePrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "FlickAngle", g_CVars.Miscellaneous.AntiAim.FlickAngle, path.c_str( ) );
	WritePrivateProfileFloat( /*AntiAim*/XorStr<0x67,8,0x84564416>("\x26\x06\x1D\x03\x2A\x05\x00"+0x84564416).s, "EnemyCheck", g_CVars.Miscellaneous.AntiAim.TurnOff, path.c_str( ) );

	WritePrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, /*Active*/XorStr<0x05,7,0x27ACD844>("\x44\x65\x73\x61\x7F\x6F"+0x27ACD844).s, g_CVars.Miscellaneous.Fakelag.Active, path.c_str( ) );
	WritePrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, /*Mode*/XorStr<0xA1,5,0x038D62E9>("\xEC\xCD\xC7\xC1"+0x038D62E9).s, g_CVars.Miscellaneous.Fakelag.Mode, path.c_str( ) );
	WritePrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "Value", g_CVars.Miscellaneous.Fakelag.Value, path.c_str( ) );
	WritePrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "InAttack", g_CVars.Miscellaneous.Fakelag.InAttack, path.c_str( ) );
	WritePrivateProfileInteger( /*Fakelag*/XorStr<0x0D,8,0xB5531447>("\x4B\x6F\x64\x75\x7D\x73\x74"+0xB5531447).s, "AirOnly", g_CVars.Miscellaneous.Fakelag.AirOnly, path.c_str( ) );

	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "BunnyHop", g_CVars.Miscellaneous.BunnyHop, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoStrafe", g_CVars.Miscellaneous.AutoStrafe, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoStrafeMode", g_CVars.Miscellaneous.AutoStrafeMode, path.c_str( ) ); // r38
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "StrafeAvoidDist", g_CVars.Miscellaneous.StrafeAvoidDist, path.c_str( ) ); // r38
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "CircleStrafe", g_CVars.Miscellaneous.CircleStrafe, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "EdgeJump", g_CVars.Miscellaneous.EdgeJump, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Speedhack", g_CVars.Miscellaneous.Speedhack, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SpeedhackValue", g_CVars.Miscellaneous.SpeedhackValue, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTap", g_CVars.Miscellaneous.DoubleTap, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapAuto", g_CVars.Miscellaneous.DoubleTapAuto, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapTicks", g_CVars.Miscellaneous.DoubleTapTicks, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapMode", g_CVars.Miscellaneous.DoubleTapMode, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapOnlyGround", g_CVars.Miscellaneous.DoubleTapOnlyGround, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "DoubleTapDelayShot", g_CVars.Miscellaneous.DoubleTapDelayShot, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "ThirdPersonKey", g_CVars.Miscellaneous.ThirdPersonKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "ThirdPersonDist", g_CVars.Miscellaneous.ThirdPersonDist, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalk", g_CVars.Miscellaneous.SlowWalk, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalkKey", g_CVars.Miscellaneous.SlowWalkKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SlowWalkSpeed", g_CVars.Miscellaneous.SlowWalkSpeed, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "SpeedhackKey", g_CVars.Miscellaneous.SpeedhackKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuKey", g_CVars.Miscellaneous.MenuKey, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuTheme", g_CVars.Miscellaneous.MenuTheme, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "MenuMode", g_CVars.Miscellaneous.MenuMode, path.c_str( ) );
	WritePrivateProfileInteger( "Miscellaneous", "Fakeduck", g_CVars.Miscellaneous.Fakeduck, path.c_str( ) ); // r24
	WritePrivateProfileInteger( "Miscellaneous", "ClientModEmulator", g_CVars.Miscellaneous.ClientModEmulator, path.c_str( ) ); // r34
	WritePrivateProfileInteger( "Miscellaneous", "FakeduckKey", g_CVars.Miscellaneous.FakeduckKey, path.c_str( ) ); // r24
	WritePrivateProfileInteger( "Miscellaneous", "Micromoves", g_CVars.Miscellaneous.Micromoves, path.c_str( ) ); // r24
	WritePrivateProfileInteger( "Aimbot", "FallbackHitbox", g_CVars.Aimbot.FallbackHitbox, path.c_str( ) ); // r25
	WritePrivateProfileInteger( "Aimbot", "DisableInterp", g_CVars.Aimbot.Interpolation.DisableInterp, path.c_str( ) ); // r25
	WritePrivateProfileInteger( "ESP", "SpectatorList", g_CVars.Visuals.SpectatorList, path.c_str( ) ); // r25
	WritePrivateProfileInteger( "ESP", "PlayerList", g_CVars.Visuals.PlayerList, path.c_str( ) ); // r44
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AutoKnife", g_CVars.Miscellaneous.AutoKnife, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "RoundSay", g_CVars.Miscellaneous.RoundSay, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "CheatsBypass", g_CVars.Miscellaneous.CheatsBypass, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "AirStuck", g_CVars.Miscellaneous.AirStuck, path.c_str( ) );

	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Menu.x", g_CVars.Menu.x, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Menu.y", g_CVars.Menu.y, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Radar.x", g_CVars.Radar.x, path.c_str( ) );
	WritePrivateProfileInteger( /*Miscellaneous*/XorStr<0x45,14,0x11CF7276>("\x08\x2F\x34\x2B\x2C\x26\x27\x2D\x23\x2B\x20\x25\x22"+0x11CF7276).s, "Radar.y", g_CVars.Radar.y, path.c_str( ) );

	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.TT", g_CVars.ColorSelector.ESP.TT, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.CT", g_CVars.ColorSelector.ESP.CT, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "ESP.Wpn", g_CVars.ColorSelector.ESP.Wpn, path.c_str( ) );

	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTInvis", g_CVars.ColorSelector.Chams.CTInvis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTVis", g_CVars.ColorSelector.Chams.CTVis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.CTOutline", g_CVars.ColorSelector.Chams.CTOutline, path.c_str( ) );

	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTInvis", g_CVars.ColorSelector.Chams.TTInvis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTVis", g_CVars.ColorSelector.Chams.TTVis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.TTOutline", g_CVars.ColorSelector.Chams.TTOutline, path.c_str( ) );

	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnInvis", g_CVars.ColorSelector.Chams.WpnInvis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnVis", g_CVars.ColorSelector.Chams.WpnVis, path.c_str( ) );
	WritePrivateProfileColor( /*Colors*/XorStr<0x02,7,0x76D4C00A>("\x41\x6C\x68\x6A\x74\x74"+0x76D4C00A).s, "Chams.WpnOutline", g_CVars.ColorSelector.Chams.WpnOutline, path.c_str( ) );
	return true;
}

// ---- named config manager (C:\Awesware) ----

std::string CConfig::RootDir( void ) { return "C:\\Awesware\\"; }
std::string CConfig::ConfigDir( void ) { return RootDir( ) + "configs\\"; }
std::string CConfig::ScriptDir( void ) { return RootDir( ) + "scripts\\"; }

void CConfig::EnsureDirs( void )
{
	CreateDirectoryA( RootDir( ).c_str( ), NULL );
	CreateDirectoryA( ConfigDir( ).c_str( ), NULL );
	CreateDirectoryA( ScriptDir( ).c_str( ), NULL );

	// one-time migration: old config.cfg next to the DLL -> configs\default.ini
	if( m_hModule )
	{
		char szPath[ MAX_PATH ];
		GetModuleFileNameA( m_hModule, szPath, MAX_PATH );
		std::string oldPath( szPath );
		oldPath = oldPath.substr( 0, oldPath.find_last_of( '\\' ) + 1 );
		oldPath += "config.cfg";
		std::string newPath = ConfigDir( ) + "default.ini";
		if( GetFileAttributesA( oldPath.c_str( ) ) != INVALID_FILE_ATTRIBUTES &&
			GetFileAttributesA( newPath.c_str( ) ) == INVALID_FILE_ATTRIBUTES )
			CopyFileA( oldPath.c_str( ), newPath.c_str( ), TRUE );
	}
}

std::string CConfig::SanitizeName( const char* name )
{
	std::string out;
	if( !name ) return out;
	const char* bad = "\\/:*?\"<>|";
	for( const char* p = name; *p && out.size( ) < 64; p++ )
	{
		if( ( unsigned char )*p >= 32 && !strchr( bad, *p ) ) out += *p;
	}
	if( out.empty( ) ) out = "unnamed";
	return out;
}

bool CConfig::SaveAs( const char* name )
{
	if( !name || !name[ 0 ] ) return false;
	EnsureDirs( );
	return WriteProfile( ConfigDir( ) + SanitizeName( name ) + ".ini" );
}

bool CConfig::LoadFrom( const char* name )
{
	if( !name || !name[ 0 ] ) return false;
	EnsureDirs( );
	std::string path = ConfigDir( ) + SanitizeName( name ) + ".ini";
	if( GetFileAttributesA( path.c_str( ) ) == INVALID_FILE_ATTRIBUTES ) return false;
	return ReadProfile( path );
}

bool CConfig::Delete( const char* name )
{
	if( !name || !name[ 0 ] ) return false;
	EnsureDirs( );
	return DeleteFileA( ( ConfigDir( ) + SanitizeName( name ) + ".ini" ).c_str( ) ) != 0;
}

void CConfig::List( std::vector<std::string>& out )
{
	out.clear( );
	EnsureDirs( );
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA( ( ConfigDir( ) + "*.ini" ).c_str( ), &fd );
	if( h == INVALID_HANDLE_VALUE ) return;
	do
	{
		std::string file( fd.cFileName );
		if( file.size( ) > 4 ) out.push_back( file.substr( 0, file.size( ) - 4 ) );
	} while( FindNextFileA( h, &fd ) );
	FindClose( h );
}

// legacy wrappers: default.ini in the config dir
void CConfig::Load( void )
{
	EnsureDirs( );
	ReadProfile( ConfigDir( ) + "default.ini" );
}

void CConfig::Save( void )
{
	EnsureDirs( );
	WriteProfile( ConfigDir( ) + "default.ini" );
}
