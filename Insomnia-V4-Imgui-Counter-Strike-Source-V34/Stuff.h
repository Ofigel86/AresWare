// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r44 (2026-09-19): V3 (AIMWARE) menu skin - orange header, icon tab strip, 8 pages over the same cvars, separate player-list window.
// BUILD MARKER r43 (2026-09-19): legit AA fake is now actually visible - auto-fakelag (Fake Choke Ticks, def 6) + fake goes out on the flush tick too.
// BUILD MARKER r40 (2026-09-19): lag records - per-record anti-jitter resolve (phase prediction + miss brute).
// BUILD MARKER r38 (2026-09-18): slophook directional autostrafe port (wall avoidance + key offsets + 90/100 step).
// BUILD MARKER r35 (2026-09-18): audit fixes - all player-indexed arrays [64] -> [65] (OOB at entindex 64).
// BUILD MARKER r34 (2026-09-18): ClientMod Emulator user toggle (MISC) + boot-time default.ini load.
// BUILD MARKER r31 (2026-09-18): reverted r30 Humanize+ per user request - legit anti-snap back to r29 state. Awaiting user-supplied anti-detect material.
// BUILD MARKER r29 (2026-09-18): legit max - dual FOV (near/far by distance) + visible-only target lock + anti-snap on target switch + dual FOV circles.
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r24 (2026-09-18): ported Fakeduck (deep 12/3 slow cycle) + Micromoves (zero-net micro-jitter, position pinned).
// BUILD MARKER r23 (2026-09-18): exact-seed hitchance (256-seed census + ForceSeed-aware) + ForceSeed prefers hit seeds + Lua: on_shot, draw.get_screen_size, utils.latency/choke, ents.eye_angles/hitbox.
// BUILD MARKER r21 (2026-09-18): Show Fake Pose toggle (pin off on demand) + AIC punished-combo ban + AI resolver recency/soft-ban.
// BUILD MARKER r19 (2026-09-18): Freeze Model (ModelZero) feature fully removed.
// BUILD MARKER r16 (2026-09-18): Freeze Model (0 deg) - old-school local look: frozen body, free skeleton overlay.
// BUILD MARKER r15 (2026-09-18): legit AA (real view kept, fake yaw 0 on choked ticks) + skeleton drawn per-segment (no more vanish).
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#ifndef __STUFF_H__
#define __STUFF_H_

class CVars
{
public:
	void Init( );
	class Menu
	{
	public:
		int x, y, w, h;
	};

	class Radar
	{
	public:
		int x, y, w, h;
	};

	class ColorSelector
	{
	public:
		int x, y, w, h;
		int Entity, Mode, Element;

		class ESP
		{
		public:
			Color CT, TT, Wpn;
		};

		class Chams
		{
		public:
			Color CTVis, TTVis, CTInvis, TTInvis, WpnVis, WpnInvis, CTOutline, TTOutline, WpnOutline;
		};

		int CurrentColor[ 2 ][ 3 ][ 3 ][ 4 ];

		ESP ESP;
		Chams Chams;
	};

	class Aimbot
	{
	public:
		bool Active, AutoShoot, AutoWall, MultiSpot, HitScan, FriendlyFire, Silent, PerfectSilent, AntiSMAC, BodyAWP, SnapLimiter, AutoStop, BodyVsJump, HitChance, StrictPrimary, BestDamage;
		int TargetSelection, Hitbox, HitboxMode, Height, AngleLimit, MinDamage, Key, HitChanceValue, BacktrackTicks, AimFOV, LongRangeDist, FallbackHitbox, HitboxGroup[ 7 ], AutoHeightMode[ 65 ]; // r25 +FallbackHitbox, r35: 65 (entindex 1..64)
		int ForceBodyKey, ForceMinDmgKey, ForceMinDmgValue; // rage force-keys (hold)
		// r45: per-weapon-group rage profiles (0 Pistol, 1 SMG, 2 Rifle, 3 Shotgun, 4 Sniper)
		class WeaponGroup
		{
			public:
			bool AutoShoot, AutoStop, AutoWall, MultiSpot, HitScan, BodyVsJump, BodyAWP, AntiSMAC;
			bool HitChance, StrictPrimary, BestDamage;
			int MinDamage, HitChanceValue, Hitbox, FallbackHitbox;
		};
		bool RageGroups;
		WeaponGroup RageGroup[ 5 ];
		int AimType, Smoothing, ReactionMs, RCS, StrafePower, DesyncYaw, DesyncChoke, FovNear, FovFar, FovSwitchDist; // legit-owned (rage ignores)
		bool RCSStandalone, AimLock, DesyncResolver, FlashCheck, StrafeActive, DesyncAA;
		bool ScopedCheck, AutoScope; // legit-owned (rage ignores)
		bool LegitAA; int LegitAAKey, LegitAAAngle, LegitAAInvertKey; // legit anti-aim: silent yaw padding (rage ignores)
		bool Prediction, FovCircle, AutoPistol;
		int KillDelayMs;
		float AngleLimitTens, PointScale;

		class Resolver
		{
		public:
			bool Active, Smart, State[ 65 ], LastState[ 65 ]; // r35: 65 (entindex 1..64)
			int Mode, Type;
			// --- Hit memory (runtime, not saved to config) ---
			float HitYaw[ 65 ];		// last resolved yaw that hit
			float ShotYaw[ 65 ];	// resolved yaw at shot time
			int HitTick[ 65 ];		// tick of last hit
			int Shots[ 65 ];		// shots fired at player
			int Hits[ 65 ];			// confirmed hits
			int Misses[ 65 ];		// consecutive misses
			int Step[ 65 ];			// bruteforce step per player
			int StepShots[ 65 ];	// shots fired on current step
			int LastShotTick[ 65 ];	// tick of last shot
			bool ShotPending[ 65 ];	// shot fired, hit not confirmed yet
			// --- Jitter detection (runtime) ---
			float YawHist[ 65 ][ 8 ];
			int YawHistPos[ 65 ];
			int YawHistCount[ 65 ];
			bool Jitter[ 65 ];
			float JitterA[ 65 ], JitterB[ 65 ];
			// --- Anim Test resolver / Type 4 (runtime, not saved to config) ---
			float AnimFeetAvg[ 65 ];// smoothed server feet yaw (m_angRotation)
			float AnimEyePrev[ 65 ];// previous networked eye yaw
			float AnimFireYaw[ 65 ];// eye yaw captured at enemy fire moment
			int AnimFireTick[ 65 ];// tick of fire capture
			bool AnimInit[ 65 ];// feet average initialized
			bool AnimSpin[ 65 ];// spin detected this frame
			// --- AI Learn resolver / Type 5 (runtime, persistent across deaths) ---
			int AIChosen[ 65 ];// candidate offset used by last applied resolve
			int AIShots[ 65 ][ 12 ];// shots fired per candidate
			int AIHits[ 65 ][ 12 ];// confirmed hits per candidate
			bool AIMem[ 65 ];// last shot used hit-memory (don't train AI on it)
			int EnemyChoke[ 65 ];// enemy choked ticks (simtime frozen), for fakelag detection
			int AIHitTick[ 65 ][ 12 ];// r21: tick of last confirmed hit per candidate
			int AIMissTick[ 65 ][ 12 ];// r21: tick of last registered miss per candidate
			// --- Honest Shot resolver / Type 6 (runtime, not saved to config) ---
			float HonestYaw[ 65 ];// real server shot yaw measured from bullet_impact
			int HonestTick[ 65 ];  // tick of the measurement
			// --- r40: Lag records / anti-jitter (LagRecords saved to config, rest runtime) ---
			bool LagRecords;          // per-record anti-jitter resolve (backtrack + choke-ahead prediction)
			bool LagSide[ 65 ];       // cluster of the latest SENT yaw (false = JitterA, true = JitterB)
			bool LagPhaseBrute[ 65 ]; // 1-bit phase bruteforce (flips on registered miss)
		};
		Resolver Resolver;

		class Interpolation
		{
		public:
			int LagPrediction;
			bool DisableInterp; // r25: Segregation enemy interp bypass
		};
		Interpolation Interpolation;
	};

	typedef Aimbot AimbotSettings; // alias: member Aimbot below hides the class name

	class Triggerbot
	{
	public:
		bool Active, Seed, IsShooting, Spread, Recoil;
		int Strength, Key, Hitbox, Delay; // Delay = trigger reaction ms
	};

	class Accuracy
	{
	public:
		bool ForceSeed, PerfectAccuracy;
		int NoSpreadMode;
	};

	class Visuals
	{
	public:
		class ESP
		{
		public:
			bool Box, Name, Health, Weapon, Bone, AimSpot, Hit, Ground, EnemyOnly;
			int BoxStyle, HealthStyle; // 0 full/bottom, 1 corner/left, 2 3d/top
			bool Armor, Ammo, Fake, ShowFake; // r21: disable the render pin -> live real/fake pose in ThirdPerson
			bool Dormant, OOF; // dormant ESP + offscreen arrows
		};

		class Chams
		{
		public:
			bool Active, Weapons, Shadows, Outline, HandsOutline, EnemyOnly;
			int Style; // 0 flat, 1 lit, 2 wireframe, 3 glow
		};

		class Crosshair
		{
		public:
			int Type;
			bool Dynamic;
		};

		float ASUS;
		bool Radar, NoSky, NoHands, NoSmoke, NoFlash, NoVisualRecoil, EventLog, ShotLog, Indicators, SpectatorList, PlayerList; // r25, r44

		ESP ESP;
		Chams Chams;
		Crosshair Crosshair;
		
	};

	class Miscellaneous
	{
	public:
		class AntiAim
		{
		public:
			bool Active, Static, WallDetection, DuckInAir, TurnOff, AtTargets, FlickEnable, FlickRandom, FlickOnShot;
			int Pitch, Yaw, Variation, DuckPitch, DuckYaw, DuckVariation, WallDetectionMode, FlickTicks, FlickSide;
			float RealValue, FakeValue, FlickAngle;
		};

		class Fakelag
		{
		public:
			bool Active, InAttack, AirOnly;
			int Mode, Value;
		};

		bool BunnyHop, AutoStrafe, CircleStrafe, EdgeJump, Speedhack, OriginCorrection, AutoKnife, RoundSay, CheatsBypass, AirStuck, AirStuckPress, ThirdPerson, DoubleTap, DoubleTapAuto, DoubleTapOnlyGround, DoubleTapDelayShot, Save, Load, SlowWalk, Fakeduck, Micromoves, ClientModEmulator; // r24 + r34
		int SpeedhackValue, DoubleTapTicks, DoubleTapMode, ThirdPersonKey, ThirdPersonDist, MenuKey, MenuTheme, MenuMode, SlowWalkKey, SlowWalkSpeed, SpeedhackKey, FakeduckKey, AutoStrafeMode, StrafeAvoidDist; // r24 + r38

		AntiAim AntiAim;
		Fakelag Fakelag;
	};

	class PlayerList
	{
	public:
		int Index, Pitch[ 65 ], Yaw[ 65 ]; // r35: 65 (entindex 1..64)
		bool Friend[ 65 ], Jitter[ 65 ]; // r35: 65 (entindex 1..64)
		QAngle ViewAngles[ 65 ]; // r35: 65 (entindex 1..64)
	};

	class Colors
	{
	public:
		int Scheme;

		Color gui_fill;
		Color gui_fill2;
		Color gui_outline;
		Color gui_outline2;
		Color gui_sections;
		Color inside_fill;
		Color inside_outline;

		Color tabs_fill;
		Color tabs_outline;

		Color radar_fill;
		Color radar_fill2;
		Color radar_outline;
		Color radar_outline2;
		Color radar_inside_fill;
		Color radar_inside_outline;
		Color radar_separator;

		Color maincolor;
		Color maincolorfade;
		Color mouseoutline;
	};

	class MovementRecorder
	{
	public:
		bool Active, angle_y_in_m_equal, surf_style, setangleyinm;
		float angle_y_in_m;
		int rerecord;
	};

	Menu Menu;
	Radar Radar;
	ColorSelector ColorSelector;
	Aimbot Aimbot;
	AimbotSettings Legit; // legitbot owns these fields, the rest inherits rage base at runtime (see Legit_Begin)
	Accuracy Accuracy;
	Triggerbot Triggerbot;
	Visuals Visuals;
	Miscellaneous Miscellaneous;
	PlayerList PlayerList;
	Colors Colors;
	MovementRecorder MovementRecorder;
};
extern CVars g_CVars;

enum InvalidatePhysicsBits_t
{
	POSITION_CHANGED	= 0x1,
	ANGLES_CHANGED		= 0x2,
	VELOCITY_CHANGED	= 0x4,
	ANIMATION_CHANGED	= 0x8,
};

class CTickRecord
{
public:
	CTickRecord( ){
		Reset( );
	}

	~CTickRecord( ) {
		Reset( );
	}

	CTickRecord& operator = ( CTickRecord &v )
	{
		m_Origin = v.m_Origin;
		m_Velocity = v.m_Velocity;
		m_Mins = v.m_Mins;
		m_Maxs = v.m_Maxs;

		m_EyeAngles = v.m_EyeAngles;

		m_SimulationTime = v.m_SimulationTime;
		m_Cycle = v.m_Cycle;

		for( int i = 0; i < 24; i++ ) m_PoseParameter[ i ] = v.m_PoseParameter[ i ];
		for( int i = 0; i < 4; i++ ) m_EncodedController[ i ] = v.m_EncodedController[ i ];
		for( int i = 0; i < 64; i++ ) m_flexWeight[ i ] = v.m_flexWeight[ i ];
		for( int i = 0; i < 15; i++ ) m_AnimationLayer[ i ] = v.m_AnimationLayer[ i ];

		m_Sequence = v.m_Sequence;
		m_Flags = v.m_Flags;
		return *this;
	}

	void Reset( )
	{
		m_Origin = Vector( 0, 0, 0 );
		m_Velocity = Vector( 0, 0, 0 );
		m_Mins = Vector( 0, 0, 0 );
		m_Maxs = Vector( 0, 0, 0 );

		m_EyeAngles = QAngle( 0, 0, 0 );

		m_SimulationTime = 0.f;
		m_Cycle = 0.f;

		for( int i = 0; i < 24; i++ ) m_PoseParameter[ i ] = 0.f;
		for( int i = 0; i < 4; i++ ) m_EncodedController[ i ] = 0.f;
		for( int i = 0; i < 64; i++ ) m_flexWeight[ i ] = 0.f;
		for( int i = 0; i < 15; i++ ) m_AnimationLayer[ i ] = C_AnimationLayer( );

		m_Sequence = 0;
		m_Flags = 0;
	}

	Vector m_Origin, m_Velocity, m_Mins, m_Maxs;
	float m_SimulationTime, m_Cycle;
	int m_Sequence, m_Flags;
	QAngle m_EyeAngles;
	float m_PoseParameter[ 24 ], m_EncodedController[ 4 ], m_flexWeight[ 64 ];
	C_AnimationLayer m_AnimationLayer[ 15 ];
};

extern CTickRecord pPlayerHistory[ 65 ][ 32 ]; // r35: 65 (entindex 1..64)
extern CTickRecord pSimulationData[ 65 ]; // r35
extern CTickRecord pBackupData[ 65 ]; // r35

class Stuff
{
public:
	int Ticks[ 65 ]; // r35: 65 (entindex 1..64)
	bool IsMoving[ 65 ]; // r35

	float sidemove_old;
	float forwardmove_old;
	QAngle viewangles_old;
	QAngle radarangles;
	QAngle cmdangles;

	class Mouse
	{
	public:
		void Draw( Color, Color );
		bool IsInBox( int, int, int, int );
		void Wrapper( );
		bool Click( int, int, int, int );
		bool RightClick( int, int, int, int );
		bool Hold( int, int, int, int );
		bool RightHold( int, int, int, int );
		void DragMenu( int&, int&, int, int );
		void DragRadar( int&, int&, int, int );
	};
	Mouse Mouse;

	class MovementFix
	{
	public:
		Vector vMove;
		float flSpeed, flYaw;
		QAngle qMove;
		void FixMove( BasePlayer*, CUserCmd*, bool );
	};
	MovementFix MovementFix;

	class AntiAim
	{
	public:
		QAngle tmp;
		void AtTargets( BasePlayer*, CUserCmd* );
		bool WallDetection( BasePlayer*, CUserCmd*, float );
		void WallDetection2( BasePlayer*, CUserCmd*, bool, float, float );
	};
	AntiAim AntiAim;

	class KnifeBot
	{
	public:
		bool CanKnife( bool, CUserCmd*, BasePlayer*, BasePlayer* );
		void Main( CUserCmd*, BasePlayer* , CSWeapon* );
		int TargetIndex;
	};
	KnifeBot Knifebot;

	float GuwopNormalize( float );
	int Flags;
	void PredictLaggedMovement( BasePlayer* );
	void ForceCVars( );
	bool NextShotPredict( BasePlayer*, CSWeapon* );
	int GetFontIndexByDistance( int );
	void GetWorldSpaceCenter( BasePlayer*, Vector& );
	bool WorldToScreen( const Vector&, Vector& );
	void ForceMaterial( float, float, float, float, IMaterial*, bool, bool );
	IMaterial* CreateMaterial( bool, bool, bool );
	QAngle Clamp( QAngle& );
	float Clamp( float& );
	void ForceSeed( CUserCmd*, BasePlayer* Target = 0 ); // r23: optional target -> prefer seeds that HIT
	bool CanHit( Vector, QAngle, Vector, Vector, Vector, float, int, CSWeapon*, BasePlayer*, int );
	void NoRecoil( CUserCmd*, BasePlayer*, bool );
	void AutoStrafe( CUserCmd*, BasePlayer* );
	void AutoStrafeDirectional( CUserCmd*, BasePlayer* ); // r38: slophook directional port


	void FixMove( CUserCmd*, BasePlayer* );
	void BunnyHop( CUserCmd*, BasePlayer* );
	void EdgeJump( CUserCmd*, BasePlayer* );
	bool CheckGround( BasePlayer* );
	float __fastcall fastSqrt( float );
	void VectorAngles_( const Vector&, const Vector&, QAngle& );
	void inline SinCos( float, float*, float* );
	void AngleVectors_( const float*, float*, float*, float* );
	void CalculateAngles( Vector, Vector, QAngle& );
	int lerp( int, int, int );
	bool IsReadyToShoot( BasePlayer*, CSWeapon* );
	void TraceFilterSkip2Entities( void*, const BaseEntity*, const BaseEntity*, int );
	void UTIL_TraceLine( const Vector&, const Vector&, unsigned int, const IHandleEntity*, int, trace_t* );
	void UTIL_TraceHull( const Vector&, const Vector&, const Vector&, const Vector&, unsigned int, const IHandleEntity*, int, trace_t* );
	void UTIL_TraceHull2( const Vector&, const Vector&, const Vector&, const Vector&, unsigned int, const IHandleEntity*, int, trace_t* );
	void FindHullIntersection( const Vector&, trace_t&, const Vector&, const Vector&, BaseEntity* );
	bool IsBSPModel( void* );
	void ClipTraceToPlayers( const Vector&, const Vector&, unsigned int, void*, trace_t* );
	void MoveToLastReceivedPosition( BasePlayer*, bool );
	void RemoveFromInterpolationList( BasePlayer* );
	void AddToInterpolationList( BasePlayer* );
	void SetAbsOrigin( BasePlayer*, const Vector& );
	void SetAbsAngles( BasePlayer*, const QAngle& );
	void EstimateAbsVelocity( BasePlayer*, Vector& );
	uintptr_t PatternScan( const std::string&, const std::string& );
	void FixCollision( BasePlayer*, BasePlayer* );
	void ApplyTickRecord( BasePlayer*, CTickRecord* );
	void StoreTickRecord( BasePlayer*, CTickRecord* );
};
extern Stuff g_Stuff;
#endif //__STUFF_H__