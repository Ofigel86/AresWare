#pragma once

#include "Valve.hpp"

// Ограничение значения диапазоном.
//
// Раньше это был макрос `if( val < min ) val = min; if( val > max ) val = max;`
// без do/while — он ломался в конструкциях вида `if(...) LimitValue(...); else ...`
// и требовал точки с запятой после себя (в AntiAimList::Clamp их не было).
template< typename T, typename TMin, typename TMax >
inline void LimitValue( T& value, TMin minValue, TMax maxValue )
{
	if( value < ( T )minValue )
		value = ( T )minValue;

	if( value > ( T )maxValue )
		value = ( T )maxValue;
}

namespace Config
{
	using Direct3D9::Color;

	struct AimbotList;
	struct TriggerbotList;

	struct MainList
	{
		bool	AimbotWeaponConfig		= false;
		bool	TriggerbotWeaponConfig	= false;

		int		AimbotStyle				= 0;	// 0 - Rage | 1 - Legit

		AimbotList*		Aimbot		= nullptr;
		TriggerbotList*	Triggerbot	= nullptr;
	};

	struct CurrentList
	{
		AimbotList*		Aimbot		= nullptr;
		TriggerbotList*	Triggerbot	= nullptr;
	};

	struct AimbotList
	{
		bool LastTick = false;
		bool SetAbs = false;
		bool UpdateAnim = false;
		int ResvolerBulletsDelay = 2;
		bool Resolver = false;
		bool ResolverPitch = true;
		int ResvolerBullets = 2;
		int		Mode = 0;				// 0 - Off | 1 - Auto | 2 - On Press
		int		Key = 0;				// 
		bool	AutoFire = false;		// 
		bool	AutoStop = false;		// 
		float	HeightScale = 0.0f;
		float	HeightScaleY = 0.0f;
		float	HeightScaleX = 0.0f;
		bool	AutoCrouch = false;		// 
		bool	AntiSpawnProtection = false;		// 
		bool	AutoReload = false;		// 
		bool	AutoScope = false;		// zoom scoped rifles when target found
		int		MinDamageOverride = 0;	// min damage while override key held
		int		MinDamageOverrideKey = 0;
		int		ForceBody = 0;		// 0 - Off | 1 - Lethal | 2 - Always | 3 - Prefer | 4 - Forced
		bool	NoSwitch = false;		// 
		bool	Height = false;
		int		Spot = 0;				// 
		bool	SpotRandomize = false;	// 
		int		TargetSelection = 0;	// 
		float	FieldOfView = 0.0f;		// 
		int		Smooth = 0;				// 0 - Off | 1 - Step | 2 - Linear
		float	StepX = 0.0f;			// 0 - 100
		float	StepY = 0.0f;			// 0 - 100
		float	SmoothX = 0.0f;			// 0 - 100
		float	SmoothY = 0.0f;			// 0 - 100
		int		Duration = 0;			// ms ( 0 - 5000 )
		int		Delay = 0;				// ms ( 0 - 5000 )
		int		SwitchDelay = 0;		// ms ( 0 - 5000 ) Delay Before Aiming At Next Target
		bool	RCS = false;			// Recoil Control System
		int		RCSDelay = 0;			// bullets ( 0 - 10 )
		int		RCSAmountX = 0;			// percents ( 0 - 100 )
		int		RCSAmountY = 0;			// percents ( 0 - 100 )
		float	Randomize = 0.0f;	// legit: aim-point jitter (units)
		float	Curve = 0.0f;		// legit: approach arc (0-1)
		bool	ZoneHead = false;	// legit aim zones
		bool	ZoneChest = false;
		bool	ZoneStomach = false;
		bool	ZoneArms = false;
		bool	ZoneLegs = false;
		int		HitboxPriority = 1;	// 0 Head|1 Chest|2 Stomach|3 Arms|4 Legs
		int		HitboxSelection = 0;	// 0 Priority|1 Nearest
		int		ToggleKey = 0;
		bool	FireOnKey = false;
		int		FireKey = 0;
		bool	ThroughSmoke = false;
		bool	RCSStandalone = false;	// legit: spray control without target
		bool	FlashCheck = false;	// legit: hold aim while flashed
		bool	HumanizeDelay = false;	// legit: reaction jitter on Delay
		bool	AutoWall = false;		// Aim Through Penetrable Walls
		int		MinDamage = 0;			// Minimum Damage To Deal Through Wall
		int		HitScan = 0;			// 0 - Off | 1 - Normal | 2 - Corners | 3 - Multipoint
		float	HitScanScale = 1.0f;	// Corners Scale
		int		Target = 0;				// 0 - Everyone | 1 - Enemies | 2 - Friends
		bool	Silent = 0;				// 0 - Off | 1 - Normal | 2 - Perfect
		bool	NoSpreadActive = false;			// Spread Control
		int		NoSpread = 0;		// 0 - Off | 1 - Normal | 2 - Advancedz
		int LagCompensation = 0;
		void	Clamp()
		{
			LimitValue( Key, 0, 128 );
			LimitValue( FieldOfView, 0.0f, 180.0f );
			LimitValue( StepX, 0.0f, 100.0f );
			LimitValue( StepY, 0.0f, 100.0f );
			LimitValue( SmoothX, 0.0f, 100.0f );
			LimitValue( SmoothY, 0.0f, 100.0f );
			LimitValue( Duration, 0, 5000 );
			LimitValue( Delay, 0, 5000 );
			LimitValue( SwitchDelay, 0, 5000 );
			LimitValue( RCSDelay, 0, 10 );
			LimitValue( RCSAmountX, 0, 100 );
			LimitValue( RCSAmountY, 0, 100 );
			LimitValue( MinDamage, 0, 100 );
			LimitValue( HitScanScale, 0.0f, 1.0f );
			LimitValue( Spot, 0, 19 );
			LimitValue( MinDamageOverride, 0, 100 );
			LimitValue( MinDamageOverrideKey, 0, 128 );
			LimitValue( ForceBody, 0, 4 );
		}
	};

	struct TriggerbotList
	{
		int		Mode = 0;				// 0 - Off | 1 - Auto | 2 - On Press
		int		Key = 0;				// 
		int		Accuracy = 0;			// 0 - Normal | 1 - Perfect | 2 - Seed
		int		Delay = 0;				// 0 - 5000 ( ms )
		int		Burst = 0;				// 0 - 10 ( bullets )
		bool	Head = false;			// 
		bool	Chest = false;			// 
		bool	Stomach = false;		// 
		bool	Arms = false;			// 
		bool	Legs = false;			// 
		bool	AutoWall = false;		// 
		int		MinDamage = 0;			// 0 - 100
		int		Target = 0;				// 0 - Everyone | 1 - Enemy | 2 - Friendly
		bool	ThroughSmoke = false;	// allow firing through smoke

		void Clamp()
		{
			LimitValue( Key, 0, 128 );
			LimitValue( Delay, 0, 5000 );
			LimitValue( Burst, 0, 10 );
			LimitValue( MinDamage, 0, 100 );
		}
	};

	struct LegitbotList
	{
		int		Mode = 0;			// 0 - Off | 1 - Auto | 2 - On Press
		int		Key = 0;			//
		int		TargetSelection = 2;// 0 - Fast | 1 - Distance | 2 - FOV
		float	FieldOfView = 5.0f;	//
		int		Smooth = 2;			// 0 - Off | 1 - Step | 2 - Linear
		float	StepX = 0.0f;		// 0 - 100
		float	StepY = 0.0f;		// 0 - 100
		float	SmoothX = 10.0f;	// 0 - 100
		float	SmoothY = 10.0f;	// 0 - 100
		int		Delay = 0;			// ms ( 0 - 5000 )
		int		Duration = 0;		// ms ( 0 - 5000 )
		bool	RCS = false;		// Recoil Control System
		int		RCSDelay = 0;		// bullets ( 0 - 10 )
		int		RCSAmountX = 100;	// percents ( 0 - 100 )
		int		RCSAmountY = 100;	// percents ( 0 - 100 )
		int		Target = 1;			// 0 - Everyone | 1 - Enemies | 2 - Friends
		bool	AutoFire = false;	//
		bool	AutoStop = false;	//
		bool	RCSStandalone = false;	// spray control without target
		bool	FlashCheck = false;	// hold aim while flashed
		bool	Backtrack = false;	// legit backtrack (server-valid window)
		bool	HumanizeDelay = false;	// reaction jitter on Delay
		bool	AutoScope = false;	// zoom scoped rifles when target found
		float	Randomize = 0.0f;	// aim-point jitter, units (0-10)
		float	Curve = 0.0f;		// approach arc (0-1)
		bool	ZoneHead = true;	// aim zones
		bool	ZoneChest = true;
		bool	ZoneStomach = true;
		bool	ZoneArms = false;
		bool	ZoneLegs = false;
		int		HitboxPriority = 1;	// 0 Head|1 Chest|2 Stomach|3 Arms|4 Legs
		int		HitboxSelection = 1;	// 0 Priority|1 Nearest
		int		ToggleKey = 0;		// master toggle key
		bool	FireOnKey = false;	// fire only with FireKey held
		int		FireKey = 0;
		int		TSD = 0;			// target switch delay, ms
		bool	ThroughSmoke = false;	// allow aiming through smoke
		bool	AutoWall = false;	// legit wallbang (off = safer)
		int		MinDamage = 0;		// for legit wallbang

		void	Clamp()
		{
			LimitValue( Key, 0, 128 );
			LimitValue( TargetSelection, 0, 4 );
			LimitValue( FieldOfView, 0.0f, 180.0f );
			LimitValue( Smooth, 0, 2 );
			LimitValue( StepX, 0.0f, 100.0f );
			LimitValue( StepY, 0.0f, 100.0f );
			LimitValue( SmoothX, 0.0f, 100.0f );
			LimitValue( SmoothY, 0.0f, 100.0f );
			LimitValue( Delay, 0, 5000 );
			LimitValue( Duration, 0, 5000 );
			LimitValue( RCSDelay, 0, 10 );
			LimitValue( RCSAmountX, 0, 100 );
			LimitValue( RCSAmountY, 0, 100 );
			LimitValue( Target, 0, 2 );
			LimitValue( Randomize, 0.0f, 10.0f );
			LimitValue( Curve, 0.0f, 1.0f );
			LimitValue( HitboxPriority, 0, 4 );
			LimitValue( HitboxSelection, 0, 1 );
			LimitValue( ToggleKey, 0, 128 );
			LimitValue( FireKey, 0, 128 );
			LimitValue( TSD, 0, 5000 );
			LimitValue( MinDamage, 0, 100 );
		}
	};

	struct ESPList
	{
		bool Hitbox = false;
		bool Filled = false;
		bool Spread = false;
	int		Box = 0;		// 0 - Off | 1 - Normal | 2 - Corners
		bool	Outlined = false;	// Draw Black Outline
		int viewfov = 0;
		bool	Name = false;		// Draw Player Name
		bool	Weapon = false;		// Draw Player Weapon
		bool	AimSpot = false;	// Draw Aim Spot	
		bool    Fov = false;

		int		Health = 0;			// 0 - Off | 1 - Text | 2 - Bar
		int		Armor = 0;			// 0 - Off | 1 - Text | 2 - Bar

		int	Skeleton = 0;	// Draw Player Skeleton
		bool	Colored = false;	// 
		bool	Defusing = false;	// Is Defusing Bomb
		bool	Bomb = false;		// Show Bomb

		int		Target = 0;			// 0 - Everyone | 1 - Enemy | 2 - Friendly
		bool	Dormant = false;	// show last known pos of dormant players
		bool	OutOfFOV = false;	// off-screen direction arrows
		bool	Snaplines = false;	// line from screen bottom to player
		bool	Distance = false;	// distance text under the box
	};

	struct RenderList
	{
		bool ChamsOutlined = false;
		int		ChamsMode = 0;			// 0 - Off | 1 - Flat | 2 - Shadow
		bool	ChamsColored = false;	// 
		int		ChamsTarget = 0;		// 0 - Everyone | 1 - Enemy | 2 - FriendlyF
		bool	ChamsVisOnly = false;
		int		ChamsAlpha = 255;		// 0 - 255 global chams opacity
		bool    out = false;
	};

	struct AntiAimList
	{
		float StandCustomAngleFakeYaw2 = 0.0f;
		float StandCustomAngleFakeYaw1 = 0.0f;
		float StandCustomAngleYaw1 = 0.0f;
		float StandCustomAngleYaw2 = 0.0f;
		float MoveCustomAngleFakeYaw2 = 0.0f;
		float MoveCustomAngleFakeYaw1 = 0.0f;
		float MoveCustomAngleYaw1 = 0.0f;
		float MoveCustomAngleYaw2 = 0.0f;
		int StandSwitchPitchDelay = 20;
		int MoveSwitchPitchDelay = 20;
		// Удалены неиспользуемые поля: YawMoveJitterSpeed, YawRealMove,
		// YawFakeMove, YawRealStand, YawFakeStand, test1..test3 — их не читали
		// ни логика, ни меню, и они не сохранялись в конфиг.
		bool FakeDuck = false;
		int FakeDuckKey = 0;			// 0 = работать при обычном приседании
		bool FakeWalkToggle = false;	// клавиша-переключатель вместо удержания
		int FakeWalkSpeed = 33;			// % от обычной скорости
		int	AtTarget = 0;		// 
		int NoEnemy = 0;
		bool NoEnemyEnabled = false;
		bool AtTargetEnabled = false;
		int		PitchStand = 0;				// 0 - Off | 1 - Up | 2 - Down | 3 - Emotion Up | 4 - Emotion Down | 5 - Fake Up
		int		YawStand = 0;				// 0 - Off | 1 - Backward | 2 - Fake Forward | 3 - Sideways Left | 4 - Sideways Right | 5 - Fake Sideways Left | 6 - Fake Sideways Right | 7 - Slow Spin | 8 - Fast Spin | 9 - Jitter | 10 - Jitter2 | 11 - Jitter3
		int		PitchMove = 0;
		int		YawMove = 0;
		int		StandChokedPackets = 0;		// 1 - 15
		int		MoveChokedPackets = 0;		// 1 - 15
		float		MoveCustomAnglePitch = 0;		// 0 - 180
		float		MoveCustomAngleYaw = 0;		// 0 - 360
		float		MoveCustomAngleFakePitch = 0;		// 0 - 180
		float		MoveCustomAngleFakeYaw = 0;		// 0 - 360
		float		MoveStaticModifer = 0;		// 0 - 360
		int			MoveSpinSpeed = 0;
		float		StandCustomAnglePitch = 0;		// 0 - 180
		float		StandCustomAngleYaw = 0;		// 0 - 360
		float		StandCustomAngleFakePitch = 0;		// 0 - 180
		float		StandCustomAngleFakeYaw = 0;		// 0 - 360
		float		StandStaticModifer = 0;		// 0 - 360
		int			StandSpinSpeed = 0;
		
		bool OnKnife = false;
		bool FakeWalk = false;
		int FakeWalkKey = 0;
		float StandFakeSpinAngle = 0.0f;
		int StandFakeSpinSpeed = 0;
		float MoveFakeSpinAngle = 0.0f;
		int MoveFakeSpinSpeed = 0;
		bool HitReactive = false;
		bool BreakLC = false;
		int DefensiveTicks = 0;		// post-shot choke (CS:S defensive)
		int ManualLeftKey = 0;
		int ManualRightKey = 0;
		int ManualBackKey = 0;
		void Clamp()
		{
			LimitValue(StandSwitchPitchDelay, 20, 1020);
			LimitValue(MoveSwitchPitchDelay, 20, 1020);
			LimitValue(StandChokedPackets, 0, 15);
			LimitValue(StandCustomAnglePitch, -9999999.f, 9999999.f);
			LimitValue(StandCustomAngleYaw, -9999999.f, 9999999.f);
			LimitValue(StandCustomAngleFakePitch, -9999999.f, 9999999.f);
			LimitValue(StandCustomAngleFakeYaw, -9999999.f, 9999999.f);
			LimitValue(StandStaticModifer, -9999999.f, 9999999.f);
			LimitValue(StandSpinSpeed, -100, 100);
			LimitValue(MoveChokedPackets, 0, 15);
			LimitValue(MoveCustomAnglePitch, -9999999.f, 9999999.f);
			LimitValue(MoveCustomAngleYaw, -9999999.f, 9999999.f);
			LimitValue(MoveCustomAngleFakePitch, -9999999.f, 9999999.f);
			LimitValue(MoveCustomAngleFakeYaw, -9999999.f, 9999999.f);
			LimitValue(MoveStaticModifer, -9999999.f, 9999999.f);
			LimitValue(MoveSpinSpeed, -100, 100);
			LimitValue(StandFakeSpinAngle, -999999.f, 9999999.f);
			LimitValue(StandFakeSpinSpeed, -9999, 9999);
			LimitValue(MoveFakeSpinAngle, -999999.f, 9999999.f);
			LimitValue(MoveFakeSpinSpeed, -9999, 9999);
				LimitValue(DefensiveTicks, 0, 10);
				LimitValue(FakeDuckKey, 0, 128);
				LimitValue(FakeWalkSpeed, 10, 90);
				LimitValue(ManualLeftKey, 0, 128);
				LimitValue(ManualRightKey, 0, 128);
				LimitValue(ManualBackKey, 0, 128);
		}
	};

	struct RemovalsList
	{
		int		NoSpread = 0;			// 0 - Off | 1 - Normal | 2 - Advanced
		bool	SeedHelp = false;		// 
		bool	NoRecoil = false;		// 
		bool	NoVisualRecoil = false;	// 
		
		bool	NoSmoke = false;		// 
		int		FlashAmount = 100;		// 0 - 100 ( percents )

		void Clamp()
		{
			LimitValue( FlashAmount, 0, 100 );
		}
	};

	struct MiscList
	{
		int fr = 0;
		int LagKey = 0;
		float ResolverAng = 0.0f;
		bool ResolverLog = true;
		int Angles = 0;
		int target = 0;
		int FakePing = 0;
		bool HitmarkerHP = false;
		bool aaa = false;
		bool HitmarkerEnabled = false;
		int Hitmarker = 3;
		float hitmarkerAlpha = 0.0f;
		bool icons = true;
		int CrashRestricion = 3880;
		int test0 = 5;
		bool LagExploitSpeed = false;
		int test = 5;
		bool LagExploitSwitch = false;
		int LagExploitSpeedKey = 0;
		int LagExploit = 0;
		int LagExploitKey = 0;
		int Val0 = 0;
		int Val1 = 0;
		int Val2 = 0;
		bool RecorderSilent = false;
		int RecorderPlayKey = 0;
		int RecorderRecKey = 0;
		bool Recorder = false;
		int CrashKey = 0;
		int Crash = 0;
		bool Lag = false;
		bool	AutoJump = false;		// 
		bool	AutoPistol = false;
	int	AutoPistolDelay = 0;	// ms between refires, 0 = engine rate		// 
		int		AutoStrafe = 0;			// 0 - Off | 1 - Optimal | 2 - Optimal + WASD
		bool	BombWarning = false;	// 
		float viewfov = 0;
		float viewfov2 = 0;
		float viewfov3 = 0;
		int		Crosshair = 0;			// 0 - Off | 1 - Dot | 2 - Cross | 3 - Swastika
		int		CrosshairSniper = 0;
		bool	Outlined = false;		// 
		bool	OutlinedSniper = false;
		bool	ShowRecoil = false;		// 
		float SpeedMod = 0.0f;
		bool	FakeLag = false;		// 
		int		FakeLagMode = 0;		// 0 Static|1 Adaptive|2 Random|3 On Peek|4 On Shot
		int		ChokedPackets = 1;		// 1 - 15 (Static: постоянный чок)
		int		FakeLagMin = 2;			// Random: нижняя граница
		int		FakeLagMax = 8;			// Random: верхняя граница
		bool	FakeLagOnGroundOnly = false;	// не чокать в воздухе
		bool	FakeLagBreakOnShot = true;	// отпускать чок в момент выстрела

		bool	AirStuck = false;		// 
		int		StuckKey = 0;			// 

		bool	Speed = false;			// 
		int		SpeedKey = 0;			// 
		int		SpeedFactor = 1;		// 1 - 15

		int		Restriction = 0;		// 0 - Off | 1 - SMAC | 2 - Ultr@
		bool	AntiSMAC = false;		// sanitize usercmd angles (SMAC eye-test)
		bool	AutoPeek = false;	// return to peek start on key release
		int		AutoPeekKey = 0;

		void Clamp()
		{
			LimitValue( ChokedPackets, 1, 32 );
			LimitValue( FakeLagMode, 0, 4 );
			LimitValue( FakeLagMin, 1, 32 );
			LimitValue( FakeLagMax, 1, 32 );

			// Перепутанные границы дали бы пустой диапазон случайного чока.
			if( FakeLagMin > FakeLagMax )
			{
				const int iTmp = FakeLagMin;
				FakeLagMin = FakeLagMax;
				FakeLagMax = iTmp;
			}
			LimitValue( SpeedKey, 0, 128 );
			LimitValue( SpeedFactor, 1, 15 );
			LimitValue( AutoPeekKey, 0, 128 );
			LimitValue( AutoPistolDelay, 0, 500 );
			// FakePing пишется прямо в net_fakelag: из руки отредактированный
			// конфиг с огромным значением отправлял клиента в бесконечный
			// choke (сервер отключает за таймаут).
			LimitValue( FakePing, 0, 260 );
		}
	};

	struct ColorsList
	{
		int curr = 0;
		Color ChamsOutlinedC = Color(255, 255, 255, 255);
		Color T_ESP_Normal = Color( 255, 0, 0, 255 );		// 
		Color T_ESP_Colored = Color( 255, 255, 0, 255 );	// 

		Color T_Chams_Normal = Color( 255, 0, 0, 255 );		// 
		Color T_Chams_Colored = Color( 255, 255, 0, 255 );	// 
		
		Color CT_ESP_Normal = Color( 0, 128, 255, 255 );	// 
		Color CT_ESP_Colored = Color( 0, 255, 0, 255 );		// 

		Color CT_Chams_Normal = Color( 0, 128, 255, 255 );	// 
		Color CT_Chams_Colored = Color( 0, 255, 0, 255 );	// 

		Color Crosshair = Color( 0, 0, 0, 255 );		// 

		Color Main = Color(0, 0, 0, 255);				// main menu color
		Color Main2 = Color(0, 0, 0, 255);				// main2 menu color	
		Color Main3 = Color(0, 0, 0, 255);				// main3 menu color	
		Color Main4 = Color(0, 0, 0, 255);				// bg3 menu color
	};

	struct BindsList
	{
		int	Menu		= 45;		// VK_INSERT
		int Eject		= 122;		// VK_F11
		int Panic		= 123;		// VK_F12
		bool MenuSpeed = true;
	};

	extern MainList*		Main;
	extern CurrentList*		Current;
	extern CurrentList*		Weapon[ WEAPON_MAX ];

	extern LegitbotList*	Legitbot;
	extern LegitbotList*	LegitbotClasses[ 5 ]; // per-class: 0 Pistol|1 SMG|2 Rifle|3 Shotgun|4 Sniper

	extern ESPList*			ESP;
	extern RenderList*		Render;
	extern AntiAimList*		AntiAim;
	extern RemovalsList*	Removals;
	extern MiscList*		Misc;
	extern ColorsList*		Colors;
	extern BindsList*		Binds;

	extern void				Startup( HMODULE hMod );
	extern void				Release();

	extern void				Load( const std::string& name );
	extern void				Save( const std::string& name );
	extern void				Delete( const std::string& name );

	extern void				OnCreateMove();

	extern auto				GetPath() -> std::string;

	extern const char*		WeaponList[ 24 ];
	extern CSWeaponID		GetWeaponID(const char* name);
}