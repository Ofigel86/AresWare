#include "Config.hpp"
#include "Player.hpp"
#include "Weapon.hpp"
#include "Source.hpp"
#include "Menu.hpp"

#include <iostream>
#include <fstream>

namespace Config
{
	MainList*		Main = nullptr;
	CurrentList*	Current = nullptr;
	CurrentList*	Weapon[ WEAPON_MAX ];

	ESPList*		ESP = nullptr;
	RenderList*		Render = nullptr;
	AntiAimList*	AntiAim = nullptr;
	RemovalsList*	Removals = nullptr;
	MiscList*		Misc = nullptr;
	ColorsList*		Colors = nullptr;
	BindsList*		Binds = nullptr;
	LegitbotList*	Legitbot = nullptr;

	std::string		m_config;
	std::string		m_current;

	void Startup(HMODULE hMod)
	{

			Main = new MainList();
			Main->Aimbot = new AimbotList();
			Main->Triggerbot = new TriggerbotList();

			Current = new CurrentList();
			Current->Aimbot = new AimbotList();
			Current->Triggerbot = new TriggerbotList();

			for (int i = 0; i < WEAPON_MAX; i++)
			{
				Weapon[i] = new CurrentList();

				Weapon[i]->Aimbot = new AimbotList();
				Weapon[i]->Triggerbot = new TriggerbotList();
			}

			ESP = new ESPList();
			Render = new RenderList();
			AntiAim = new AntiAimList();
			Removals = new RemovalsList();
			Misc = new MiscList();
			Colors = new ColorsList();
			Binds = new BindsList();
			Legitbot = new LegitbotList();

			m_config = Shared::m_pVars->m_loader;
			m_config.append(XorStr("\\v34\\"));

			Shared::m_strConfig = "default";
			Shared::m_bLoad = true;

		}
	

	void Release()
	{
		Memory::SafeDelete( Main->Aimbot );
		Memory::SafeDelete( Main->Triggerbot );
		Memory::SafeDelete( Main );

		Memory::SafeDelete( Current->Aimbot );
		Memory::SafeDelete( Current->Triggerbot );
		Memory::SafeDelete( Current );

		for( int i = 0; i < WEAPON_MAX; i++ )
		{
			Memory::SafeDelete( Weapon[ i ]->Aimbot );
			Memory::SafeDelete( Weapon[ i ]->Triggerbot );

			Memory::SafeDelete( Weapon[ i ] );
		}

		Memory::SafeDelete( ESP );
		Memory::SafeDelete( Render );
		Memory::SafeDelete( AntiAim );
		Memory::SafeDelete( Removals );
		Memory::SafeDelete( Misc );
		Memory::SafeDelete( Colors );
		Memory::SafeDelete( Binds );
		Memory::SafeDelete( Legitbot );
	}

	bool LoadBool( const std::string& strSection, const std::string& strName )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), XorStr( "0" ), szData, MAX_PATH, m_current.c_str() );

		if( strcmp( szData, XorStr( "on" ) ) == 0 || strcmp( szData, XorStr( "true" ) ) == 0 || atoi( szData ) == 1 )
			return true;

		return false;
	}

	bool LoadBoolDef( const std::string& strSection, const std::string& strName, bool bDefault )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), "", szData, MAX_PATH, m_current.c_str() );

		if( szData[ 0 ] == '\0' )
			return bDefault;

		if( strcmp( szData, XorStr( "on" ) ) == 0 || strcmp( szData, XorStr( "true" ) ) == 0 || atoi( szData ) == 1 )
			return true;

		return false;
	}

	int LoadInt( const std::string& strSection, const std::string& strName )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), XorStr( "0" ), szData, MAX_PATH, m_current.c_str() );
		return atoi( szData );
	}

	float LoadFloat( const std::string& strSection, const std::string& strName )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), XorStr( "0" ), szData, MAX_PATH, m_current.c_str() );
		return ( float )atof( szData );
	}

	Color LoadColor( const std::string& strSection, const std::string& strName )
	{
		Color result;
		result.R = LoadInt( strSection, std::string( strName ).append( ".r" ) );
		result.G = LoadInt( strSection, std::string( strName ).append( ".g" ) );
		result.B = LoadInt( strSection, std::string( strName ).append( ".b" ) );
		result.A = LoadInt( strSection, std::string( strName ).append( ".a" ) );
		return result;
	}

	void SaveBool( const std::string& strSection, const std::string& strName, bool bValue )
	{
		const char* szData = bValue ? XorStr( "true" ) : XorStr( "false" );
		WritePrivateProfileString( strSection.c_str(), strName.c_str(), szData, m_current.c_str() );
	}

	void SaveInt( const std::string& strSection, const std::string& strName, int iValue )
	{
		WritePrivateProfileString( strSection.c_str(), strName.c_str(), std::to_string( iValue ).c_str(), m_current.c_str() );
	}

	void SaveFloat( const std::string& strSection, const std::string& strName, float flValue )
	{
		WritePrivateProfileString( strSection.c_str(), strName.c_str(), std::to_string( flValue ).c_str(), m_current.c_str() );
	}

	void SaveColor( const std::string& strSection, const std::string& strName, const Color& color )
	{
		SaveInt( strSection, std::string( strName ).append( ".r" ), color.R );
		SaveInt( strSection, std::string( strName ).append( ".g" ), color.G );
		SaveInt( strSection, std::string( strName ).append( ".b" ), color.B );
		SaveInt( strSection, std::string( strName ).append( ".a" ), color.A );
	}

	bool Exists( const std::string& name )
	{
		std::ifstream f( name );
		return f.good();
	}

	bool Create( const std::string& name )
	{
		auto dir = name.substr( 0, name.find_last_of( "\\" ) );

		if( CreateDirectory( dir.c_str(), nullptr ) || GetLastError() == ERROR_ALREADY_EXISTS )
		{
			std::ofstream f( name );

			if( f.good() )
			{
				f.close();

				return true;
			}
		}

		return false;
	}

	void Load( const std::string& name )
	{
		auto current = name;

		if( current.find( XorStr( ".cfg" ) ) == std::string::npos )
			current.append( XorStr( ".cfg" ) );

		m_current = m_config + current;

		if( !Exists( m_current ) )
			Save( name );

		std::string main( XorStr( "main" ) );

		Main->AimbotWeaponConfig		= LoadBool( main, XorStr( "aimbot.weapon.config" ) );
		Main->Aimbot->Mode				= LoadInt( main, XorStr( "aimbot.mode" ) );
		Main->Aimbot->Key				= LoadInt( main, XorStr( "aimbot.key" ) );
		Main->Aimbot->AutoFire			= LoadBool( main, XorStr( "aimbot.auto.fire" ) );
		Main->Aimbot->AutoStop			= LoadBool( main, XorStr( "aimbot.auto.stop" ) );
		Main->Aimbot->AutoCrouch		= LoadBool( main, XorStr( "aimbot.auto.crouch" ) );
		Main->Aimbot->AutoReload = LoadBool(main, XorStr("aimbot.auto.reload"));
		Main->Aimbot->AntiSpawnProtection = LoadBool(main, XorStr("aimbot.anti.spawn.protection"));
		Main->Aimbot->NoSwitch			= LoadBool( main, XorStr( "aimbot.no.switch" ) );
		Main->Aimbot->Spot				= LoadInt( main, XorStr( "aimbot.spot" ) );
		Main->Aimbot->SpotRandomize		= LoadBool( main, XorStr( "aimbot.spot.randomize" ) );
		Main->Aimbot->TargetSelection	= LoadInt( main, XorStr( "aimbot.target.selection" ) );
		Main->Aimbot->FieldOfView = LoadFloat(main, XorStr("aimbot.fov"));
		Main->Aimbot->Smooth = LoadInt(main, XorStr("aimbot.smooth"));
		Main->Aimbot->Resolver = LoadBool(main, XorStr("aimbot.resolver"));
		Main->Aimbot->ResolverPitch = LoadBoolDef(main, XorStr("aimbot.resolver.pitch"), true);
		Main->Aimbot->ResvolerBullets = LoadInt(main, XorStr("aimbot.resolver.bullets"));
		Main->Aimbot->ResvolerBulletsDelay = LoadInt(main, XorStr("aimbot.resolver.bullets.delay"));
		Main->Aimbot->StepX				= LoadFloat( main, XorStr( "aimbot.step.vertical" ) );
		Main->Aimbot->StepY				= LoadFloat( main, XorStr( "aimbot.step.horizontal" ) );
		Main->Aimbot->SmoothX			= LoadFloat( main, XorStr( "aimbot.smooth.vertical" ) );
		Main->Aimbot->SmoothY			= LoadFloat( main, XorStr( "aimbot.smooth.horizontal" ) );
		Main->Aimbot->Duration			= LoadInt( main, XorStr( "aimbot.duration" ) );
		Main->Aimbot->Delay				= LoadInt( main, XorStr( "aimbot.delay" ) );
		Main->Aimbot->SwitchDelay		= LoadInt( main, XorStr( "aimbot.switch.delay" ) );
		Main->Aimbot->RCS				= LoadBool( main, XorStr( "aimbot.rcs" ) );
		Main->Aimbot->RCSDelay			= LoadInt( main, XorStr( "aimbot.rcs.delay" ) );
		Main->Aimbot->RCSAmountX		= LoadInt( main, XorStr( "aimbot.rcs.amount.vertical" ) );
		Main->Aimbot->RCSAmountY		= LoadInt( main, XorStr( "aimbot.rcs.amount.horizontal" ) );
		Main->Aimbot->AutoWall			= LoadBool( main, XorStr( "aimbot.autowall" ) );
		Main->Aimbot->MinDamage			= LoadInt( main, XorStr( "aimbot.min.damage" ) );
		Main->Aimbot->HitScan			= LoadInt( main, XorStr( "aimbot.hitscan" ) );
		Main->Aimbot->HitScanScale		= LoadFloat( main, XorStr( "aimbot.hitscan.scale" ) );
		Main->Aimbot->Target			= LoadInt( main, XorStr( "aimbot.target" ) );
		Main->Aimbot->Silent			= LoadBool( main, XorStr( "aimbot.silent" ) );
		Main->Aimbot->NoSpreadActive	= LoadBool(main, XorStr("aimbot.no.spread.active"));
		Main->Aimbot->NoSpread			= LoadInt(main, XorStr("aimbot.no.spread"));
	//	Main->Aimbot->SeedHelp = LoadBool(main, XorStr("aimbot.seed.help"));
		Main->Aimbot->Height = LoadBool(main, XorStr("aimbot.height"));
		Main->Aimbot->HeightScale = LoadFloat(main, XorStr("aimbot.height.scale"));
		Main->Aimbot->LagCompensation = LoadInt(main, XorStr("aimbot.adjustment"));
		Main->Aimbot->Clamp();

		Main->AimbotStyle = LoadInt( main, XorStr( "aimbot.style" ) );

		if( Main->AimbotStyle < 0 || Main->AimbotStyle > 1 )
			Main->AimbotStyle = 0;

		Legitbot->Mode				= LoadInt( main, XorStr( "legitbot.mode" ) );
		Legitbot->Key				= LoadInt( main, XorStr( "legitbot.key" ) );
		Legitbot->Spot				= LoadInt( main, XorStr( "legitbot.spot" ) );
		Legitbot->TargetSelection	= LoadInt( main, XorStr( "legitbot.target.selection" ) );
		Legitbot->FieldOfView		= LoadFloat( main, XorStr( "legitbot.fov" ) );
		Legitbot->Smooth			= LoadInt( main, XorStr( "legitbot.smooth" ) );
		Legitbot->StepX			= LoadFloat( main, XorStr( "legitbot.step.vertical" ) );
		Legitbot->StepY			= LoadFloat( main, XorStr( "legitbot.step.horizontal" ) );
		Legitbot->SmoothX			= LoadFloat( main, XorStr( "legitbot.smooth.vertical" ) );
		Legitbot->SmoothY			= LoadFloat( main, XorStr( "legitbot.smooth.horizontal" ) );
		Legitbot->Delay			= LoadInt( main, XorStr( "legitbot.delay" ) );
		Legitbot->Duration			= LoadInt( main, XorStr( "legitbot.duration" ) );
		Legitbot->RCS				= LoadBool( main, XorStr( "legitbot.rcs" ) );
		Legitbot->RCSDelay			= LoadInt( main, XorStr( "legitbot.rcs.delay" ) );
		Legitbot->RCSAmountX		= LoadInt( main, XorStr( "legitbot.rcs.amount.vertical" ) );
		Legitbot->RCSAmountY		= LoadInt( main, XorStr( "legitbot.rcs.amount.horizontal" ) );
		Legitbot->Target			= LoadInt( main, XorStr( "legitbot.target" ) );
		Legitbot->AutoFire			= LoadBool( main, XorStr( "legitbot.auto.fire" ) );
		Legitbot->AutoStop			= LoadBool( main, XorStr( "legitbot.auto.stop" ) );

		Legitbot->Clamp();

		Main->TriggerbotWeaponConfig	= LoadBool( main, XorStr( "triggerbot.weapon.config" ) );
		Main->Triggerbot->Mode			= LoadInt( main, XorStr( "triggerbot.mode" ) );
		Main->Triggerbot->Key			= LoadInt( main, XorStr( "triggerbot.key" ) );
		Main->Triggerbot->Accuracy		= LoadInt( main, XorStr( "triggerbot.accuracy" ) );
		Main->Triggerbot->Delay			= LoadInt( main, XorStr( "triggerbot.delay" ) );
		Main->Triggerbot->Burst			= LoadInt( main, XorStr( "triggerbot.burst" ) );
		Main->Triggerbot->Head			= LoadBool( main, XorStr( "triggerbot.head" ) );
		Main->Triggerbot->Chest			= LoadBool( main, XorStr( "triggerbot.chest" ) );
		Main->Triggerbot->Stomach		= LoadBool( main, XorStr( "triggerbot.stomach" ) );
		Main->Triggerbot->Arms			= LoadBool( main, XorStr( "triggerbot.arms" ) );
		Main->Triggerbot->Legs			= LoadBool( main, XorStr( "triggerbot.legs" ) );
		Main->Triggerbot->AutoWall		= LoadBool( main, XorStr( "triggerbot.autowall" ) );
		Main->Triggerbot->MinDamage		= LoadInt( main, XorStr( "triggerbot.min.damage" ) );
		Main->Triggerbot->Target		= LoadInt( main, XorStr( "triggerbot.target" ) );

		Main->Triggerbot->Clamp();

		ESP->Box = LoadInt(main, XorStr("esp.box"));
		ESP->Outlined = LoadBool(main, XorStr("esp.outlined"));
		ESP->Filled = LoadBool(main, XorStr("esp.filled"));
		ESP->Name						= LoadBool( main, XorStr( "esp.name" ) );
		ESP->Weapon						= LoadBool( main, XorStr( "esp.weapon" ) );
		ESP->AimSpot					= LoadBool( main, XorStr( "esp.aimspot" ) );
		ESP->Health						= LoadInt( main, XorStr( "esp.health" ) );
		ESP->Armor						= LoadInt( main, XorStr( "esp.armor" ) );
		ESP->Skeleton					= LoadInt( main, XorStr( "esp.skeleton" ) );
		ESP->Colored					= LoadBool( main, XorStr( "esp.visible" ) );
		ESP->Defusing					= LoadBool( main, XorStr( "esp.defusing" ) );
		ESP->Bomb						= LoadBool( main, XorStr( "esp.bomb" ) );
		ESP->Target						= LoadInt( main, XorStr( "esp.target" ) );
		ESP->Fov						= LoadBool(main, XorStr("esp.draw.fov"));

		Render->ChamsMode = LoadInt(main, XorStr("render.chams.mode"));
		Render->ChamsOutlined = LoadBool(main, XorStr("render.chams.outlined"));
		Render->ChamsColored			= LoadBool( main, XorStr( "render.chams.visible" ) );
		Render->ChamsTarget				= LoadInt(main, XorStr("render.chams.target"));
		Render->ChamsVisOnly = LoadBool(main, XorStr("render.chams.visibleonly"));

		AntiAim->AtTargetEnabled = LoadBool(main, XorStr("antiaim.at.target.enabled"));
		AntiAim->AtTarget				= LoadInt( main, XorStr( "antiaim.at.target" ) );
		AntiAim->PitchStand				= LoadInt( main, XorStr( "antiaim.pitch.stand" ) );
		AntiAim->YawStand					= LoadInt( main, XorStr( "antiaim.yaw.stand" ) );
		AntiAim->StandChokedPackets = LoadInt(main, XorStr("antiaim.choked.packets.stand"));
		AntiAim->StandCustomAnglePitch = LoadFloat(main, XorStr("antiaim.custom.pitch.stand"));
		AntiAim->StandCustomAngleFakePitch = LoadFloat(main, XorStr("antiaim.custom.fakepitch.stand"));
		AntiAim->StandCustomAngleYaw = LoadFloat(main, XorStr("antiaim.custom.yaw.stand"));
		AntiAim->StandCustomAngleFakeYaw = LoadFloat(main, XorStr("antiaim.custom.fakeyaw.stand"));
		AntiAim->StandStaticModifer = LoadFloat(main, XorStr("antiaim.static.modifer.stand"));
		AntiAim->StandSwitchPitchDelay = LoadInt(main, XorStr("antiaim.stand.switch.delay"));
		AntiAim->MoveSwitchPitchDelay = LoadInt(main, XorStr("antiaim.move.switch.delay"));
		AntiAim->StandSpinSpeed = LoadInt(main, XorStr("antiaim.spinspeed.stand"));
		AntiAim->PitchMove = LoadInt(main, XorStr("antiaim.pitch.move"));
		AntiAim->YawMove = LoadInt(main, XorStr("antiaim.yaw.move"));
		AntiAim->MoveChokedPackets = LoadInt(main, XorStr("antiaim.choked.packets.move"));
		AntiAim->MoveCustomAnglePitch = LoadFloat(main, XorStr("antiaim.custom.pitch.move"));
		AntiAim->MoveCustomAngleFakePitch = LoadFloat(main, XorStr("antiaim.custom.fakepitch.move"));
		AntiAim->MoveCustomAngleYaw = LoadFloat(main, XorStr("antiaim.custom.yaw.move"));
		AntiAim->MoveCustomAngleFakeYaw = LoadFloat(main, XorStr("antiaim.custom.fakeyaw.move"));
		AntiAim->MoveStaticModifer = LoadFloat(main, XorStr("antiaim.static.modifer.move"));
		AntiAim->MoveSpinSpeed = LoadInt(main, XorStr("antiaim.spinspeed.move"));
		AntiAim->NoEnemyEnabled = LoadBool(main, XorStr("antiaim.no.enemy.enabled"));
		AntiAim->NoEnemy = LoadInt(main, XorStr("antiaim.no.enemy"));
		AntiAim->OnKnife = LoadBool(main, XorStr("antiaim.on.knife"));
		AntiAim->StandCustomAngleYaw1 = LoadFloat(main, XorStr("antiaim.stand.breaker.real.first"));
		AntiAim->StandCustomAngleYaw2 = LoadFloat(main, XorStr("antiaim.stand.breaker.real.second"));
		AntiAim->StandCustomAngleFakeYaw1 = LoadFloat(main, XorStr("antiaim.stand.breaker.fake.first"));
		AntiAim->StandCustomAngleFakeYaw2 = LoadFloat(main, XorStr("antiaim.stand.breaker.fake.second"));
		AntiAim->MoveCustomAngleYaw1 = LoadFloat(main, XorStr("antiaim.move.breaker.real.first"));
		AntiAim->MoveCustomAngleYaw2 = LoadFloat(main, XorStr("antiaim.move.breaker.real.second"));
		AntiAim->MoveCustomAngleFakeYaw1 = LoadFloat(main, XorStr("antiaim.move.breaker.fake.first"));
		AntiAim->MoveCustomAngleFakeYaw2 = LoadFloat(main, XorStr("antiaim.move.breaker.fake.second"));
		AntiAim->HitReactive = LoadBool(main, XorStr("antiaim.hit.reactive"));
		AntiAim->BreakLC = LoadBool(main, XorStr("antiaim.break.lagcomp"));
		AntiAim->FakeWalk = LoadBool(main, XorStr("antiaim.fakewalk"));
		AntiAim->FakeWalkKey = LoadInt(main, XorStr("antiaim.fakewalk.key"));
		AntiAim->Clamp();
		Removals->NoRecoil				= LoadBool( main, XorStr( "removals.no.recoil" ) );
		Removals->NoVisualRecoil		= LoadBool( main, XorStr( "removals.no.visual.recoil" ) );
		Removals->NoSmoke				= LoadBool( main, XorStr( "removals.no.smoke" ) );
		Removals->FlashAmount			= LoadInt( main, XorStr( "removals.flash.amount" ) );

		Removals->Clamp();

		Misc->AutoJump = LoadBool(main, XorStr("misc.auto.jump"));
		Misc->Lag = LoadBool(main, XorStr("misc.thirdperson"));
		Misc->LagKey = LoadInt(main, XorStr("misc.thirdperson.key"));
		Misc->Hitmarker = LoadInt(main, XorStr("misc.hitsound"));
		Misc->HitmarkerEnabled = LoadBool(main, XorStr("misc.hitmarker"));
		Misc->HitmarkerHP = LoadBool(main, XorStr("misc.hitmarker.hp"));
		Misc->AutoPistol				= LoadBool( main, XorStr( "misc.auto.pistol" ) );
		Misc->AutoStrafe				= LoadInt( main, XorStr( "misc.auto.strafe" ) );
		Misc->BombWarning				= LoadBool( main, XorStr( "misc.bomb.warning" ) );
		Misc->Crosshair					= LoadInt( main, XorStr( "misc.crosshair" ) );
		Misc->Outlined					= LoadBool( main, XorStr( "misc.crosshair.outlined" ) );
		Misc->ShowRecoil				= LoadBool( main, XorStr( "misc.crosshair.show.recoil" ) );
		Misc->FakeLag					= LoadBool( main, XorStr( "misc.fakelag" ) );
		Misc->ChokedPackets				= LoadInt( main, XorStr( "misc.fakelag.choked.packets" ) );
		Misc->AirStuck					= LoadBool( main, XorStr( "misc.airstuck" ) );
		Misc->StuckKey					= LoadInt( main, XorStr( "misc.airstuck.key" ) );
		Misc->Speed						= LoadBool( main, XorStr( "misc.cstrafer" ) );
		Misc->SpeedKey = LoadInt(main, XorStr("misc.cstrafer.key"));
		Misc->SpeedMod = LoadFloat(main, XorStr("misc.cstrafer.modifer"));
		Misc->Restriction = LoadInt(main, XorStr("misc.restriction"));
		Misc->ResolverAng = LoadFloat(main, XorStr("misc.resolver.angle"));
		Misc->ResolverLog = LoadBoolDef(main, XorStr("misc.resolver.log"), true);

		Misc->Clamp();

		std::string colors( XorStr( "colors" ) );

		Colors->T_ESP_Normal			= LoadColor( colors, XorStr( "t.esp.normal" ) );
		Colors->T_ESP_Colored			= LoadColor( colors, XorStr( "t.esp.visible" ) );
		Colors->T_Chams_Normal			= LoadColor( colors, XorStr( "t.chams.normal" ) );
		Colors->T_Chams_Colored			= LoadColor( colors, XorStr( "t.chams.visible" ) );
		Colors->CT_ESP_Normal			= LoadColor( colors, XorStr( "ct.esp.normal" ) );
		Colors->CT_ESP_Colored			= LoadColor( colors, XorStr( "ct.esp.visible" ) );
		Colors->CT_Chams_Normal			= LoadColor( colors, XorStr( "ct.chams.normal" ) );
		Colors->CT_Chams_Colored = LoadColor(colors, XorStr("ct.chams.visible"));
		Colors->Crosshair = LoadColor(colors, XorStr("crosshair"));

		Source::m_pMenu->SetColors();
		Source::m_pMenu->ApplyColors();

		std::string binds( XorStr( "binds" ) );

		Binds->Menu						= LoadInt( binds, XorStr( "menu" ) );
		Binds->Eject					= LoadInt( binds, XorStr( "eject" ) );
		Binds->Panic					= LoadInt( binds, XorStr( "panic" ) );

		for( int i = 0; i < ARRAYSIZE( WeaponList ); i++ )
		{
			auto weapon = WeaponList[ i ];
			auto index = GetWeaponID( weapon );

			Weapon[ index ]->Aimbot->Mode				= LoadInt( weapon, XorStr( "aimbot.mode" ) );
			Weapon[ index ]->Aimbot->Key				= LoadInt( weapon, XorStr( "aimbot.key" ) );
			Weapon[ index ]->Aimbot->AutoFire			= LoadBool( weapon, XorStr( "aimbot.auto.fire" ) );
			Weapon[ index ]->Aimbot->AutoStop			= LoadBool( weapon, XorStr( "aimbot.auto.stop" ) );
			Weapon[ index ]->Aimbot->AutoCrouch			= LoadBool( weapon, XorStr( "aimbot.auto.crouch" ) );
			Weapon[index]->Aimbot->AutoReload = LoadBool(weapon, XorStr("aimbot.auto.reload"));
			Weapon[index]->Aimbot->AntiSpawnProtection = LoadBool(weapon, XorStr("aimbot.anti.spawn.protection"));
			Weapon[ index ]->Aimbot->NoSwitch			= LoadBool( weapon, XorStr( "aimbot.no.switch" ) );
			Weapon[ index ]->Aimbot->Spot				= LoadInt( weapon, XorStr( "aimbot.spot" ) );
			Weapon[ index ]->Aimbot->SpotRandomize		= LoadBool( weapon, XorStr( "aimbot.spot.randomize" ) );
			Weapon[ index ]->Aimbot->TargetSelection	= LoadInt( weapon, XorStr( "aimbot.target.selection" ) );
			Weapon[ index ]->Aimbot->FieldOfView		= LoadFloat( weapon, XorStr( "aimbot.fov" ) );
			Weapon[ index ]->Aimbot->Smooth				= LoadInt( weapon, XorStr( "aimbot.smooth" ) );
			Weapon[ index ]->Aimbot->StepX				= LoadFloat( weapon, XorStr( "aimbot.step.vertical" ) );
			Weapon[ index ]->Aimbot->StepY				= LoadFloat( weapon, XorStr( "aimbot.step.horizontal" ) );
			Weapon[ index ]->Aimbot->SmoothX			= LoadFloat( weapon, XorStr( "aimbot.smooth.vertical" ) );
			Weapon[ index ]->Aimbot->SmoothY			= LoadFloat( weapon, XorStr( "aimbot.smooth.horizontal" ) );
			Weapon[ index ]->Aimbot->Duration			= LoadInt( weapon, XorStr( "aimbot.duration" ) );
			Weapon[ index ]->Aimbot->Delay				= LoadInt( weapon, XorStr( "aimbot.delay" ) );
			Weapon[ index ]->Aimbot->SwitchDelay		= LoadInt( weapon, XorStr( "aimbot.switch.delay" ) );
			Weapon[ index ]->Aimbot->RCS				= LoadBool( weapon, XorStr( "aimbot.rcs" ) );
			Weapon[ index ]->Aimbot->RCSDelay			= LoadInt( weapon, XorStr( "aimbot.rcs.delay" ) );
			Weapon[ index ]->Aimbot->RCSAmountX			= LoadInt( weapon, XorStr( "aimbot.rcs.amount.vertical" ) );
			Weapon[ index ]->Aimbot->RCSAmountY			= LoadInt( weapon, XorStr( "aimbot.rcs.amount.horizontal" ) );
			Weapon[ index ]->Aimbot->AutoWall			= LoadBool( weapon, XorStr( "aimbot.autowall" ) );
			Weapon[ index ]->Aimbot->MinDamage			= LoadInt( weapon, XorStr( "aimbot.min.damage" ) );
			Weapon[ index ]->Aimbot->HitScan			= LoadInt( weapon, XorStr( "aimbot.hitscan" ) );
			Weapon[ index ]->Aimbot->HitScanScale		= LoadFloat( weapon, XorStr( "aimbot.hitscan.scale" ) );
			Weapon[ index ]->Aimbot->Target				= LoadInt( weapon, XorStr( "aimbot.target" ) );
			Weapon[ index ]->Aimbot->Silent				= LoadBool( weapon, XorStr( "aimbot.silent" ) );
			Weapon[ index ]->Aimbot->NoSpreadActive		= LoadBool(weapon, XorStr("aimbot.no.spread.active"));
			Weapon[ index ]->Aimbot->NoSpread			= LoadInt(weapon, XorStr("aimbot.no.spread"));
		//	Weapon[index]->Aimbot->SeedHelp = LoadBool(weapon, XorStr("aimbot.seed.help"));
			Weapon[index]->Aimbot->Height = LoadBool(weapon, XorStr("aimbot.height"));
			Weapon[index]->Aimbot->HeightScale = LoadFloat(weapon, XorStr("aimbot.height.scale"));
			Weapon[index]->Aimbot->LagCompensation = LoadInt(weapon, XorStr("aimbot.adjustment"));

			Weapon[ index ]->Aimbot->Clamp();

			Weapon[ index ]->Triggerbot->Mode			= LoadInt( weapon, XorStr( "triggerbot.mode" ) );
			Weapon[ index ]->Triggerbot->Key			= LoadInt( weapon, XorStr( "triggerbot.key" ) );
			Weapon[ index ]->Triggerbot->Accuracy		= LoadInt( weapon, XorStr( "triggerbot.accuracy" ) );
			Weapon[ index ]->Triggerbot->Delay			= LoadInt( weapon, XorStr( "triggerbot.delay" ) );
			Weapon[ index ]->Triggerbot->Burst			= LoadInt( weapon, XorStr( "triggerbot.burst" ) );
			Weapon[ index ]->Triggerbot->Head			= LoadBool( weapon, XorStr( "triggerbot.head" ) );
			Weapon[ index ]->Triggerbot->Chest			= LoadBool( weapon, XorStr( "triggerbot.chest" ) );
			Weapon[ index ]->Triggerbot->Stomach		= LoadBool( weapon, XorStr( "triggerbot.stomach" ) );
			Weapon[ index ]->Triggerbot->Arms			= LoadBool( weapon, XorStr( "triggerbot.arms" ) );
			Weapon[ index ]->Triggerbot->Legs			= LoadBool( weapon, XorStr( "triggerbot.legs" ) );
			Weapon[ index ]->Triggerbot->AutoWall		= LoadBool( weapon, XorStr( "triggerbot.autowall" ) );
			Weapon[ index ]->Triggerbot->MinDamage		= LoadInt( weapon, XorStr( "triggerbot.min.damage" ) );
			Weapon[ index ]->Triggerbot->Target			= LoadInt( weapon, XorStr( "triggerbot.target" ) );

			Weapon[ index ]->Triggerbot->Clamp();
		}
	}

	void Save( const std::string& name )
	{
		auto current = name;

		if( current.find( XorStr( ".cfg" ) ) == std::string::npos )
			current.append( XorStr( ".cfg" ) );

		m_current = m_config + current;

		if( !Exists( m_current ) )
		{
			if( !Create( m_current ) )
				DPRINT( XorStr( "[Config::Save] Can't create config directory or file!" ) );
		}

		std::string main( XorStr( "main" ) );

		Main->Aimbot->Clamp();

		SaveBool( main, XorStr( "aimbot.weapon.config" ), Main->AimbotWeaponConfig );
		SaveInt( main, XorStr( "aimbot.mode" ), Main->Aimbot->Mode );
		SaveInt( main, XorStr( "aimbot.key" ), Main->Aimbot->Key );
		SaveBool( main, XorStr( "aimbot.auto.fire" ), Main->Aimbot->AutoFire );
		SaveBool( main, XorStr( "aimbot.auto.stop" ), Main->Aimbot->AutoStop );
		SaveBool( main, XorStr( "aimbot.auto.crouch" ), Main->Aimbot->AutoCrouch );
		SaveBool(main, XorStr("aimbot.auto.reload"), Main->Aimbot->AutoReload);
		SaveBool(main, XorStr("aimbot.anti.spawn.protection"), Main->Aimbot->AntiSpawnProtection);
		SaveBool( main, XorStr( "aimbot.no.switch" ), Main->Aimbot->NoSwitch );
		SaveInt( main, XorStr( "aimbot.spot" ), Main->Aimbot->Spot );
		SaveBool( main, XorStr( "aimbot.spot.randomize" ), Main->Aimbot->SpotRandomize );
		SaveInt( main, XorStr( "aimbot.target.selection" ), Main->Aimbot->TargetSelection );
		SaveFloat(main, XorStr("aimbot.fov"), Main->Aimbot->FieldOfView);
		SaveInt(main, XorStr("aimbot.smooth"), Main->Aimbot->Smooth);
		SaveBool(main, XorStr("aimbot.resolver"), Main->Aimbot->Resolver);
		SaveBool(main, XorStr("aimbot.resolver.pitch"), Main->Aimbot->ResolverPitch);
		SaveInt(main, XorStr("aimbot.resolver.bullets"), Main->Aimbot->ResvolerBullets);
		SaveInt(main, XorStr("aimbot.resolver.bullets.delay"), Main->Aimbot->ResvolerBulletsDelay);
		SaveFloat( main, XorStr( "aimbot.step.vertical" ), Main->Aimbot->StepX );
		SaveFloat( main, XorStr( "aimbot.step.horizontal" ), Main->Aimbot->StepY );
		SaveFloat( main, XorStr( "aimbot.smooth.vertical" ), Main->Aimbot->SmoothX );
		SaveFloat( main, XorStr( "aimbot.smooth.horizontal" ), Main->Aimbot->SmoothY );
		SaveInt( main, XorStr( "aimbot.duration" ), Main->Aimbot->Duration );
		SaveInt( main, XorStr( "aimbot.delay" ), Main->Aimbot->Delay );
		SaveInt( main, XorStr( "aimbot.switch.delay" ), Main->Aimbot->SwitchDelay );
		SaveBool( main, XorStr( "aimbot.rcs" ), Main->Aimbot->RCS );
		SaveInt( main, XorStr( "aimbot.rcs.delay" ), Main->Aimbot->RCSDelay );
		SaveInt( main, XorStr( "aimbot.rcs.amount.vertical" ), Main->Aimbot->RCSAmountX );
		SaveInt( main, XorStr( "aimbot.rcs.amount.horizontal" ), Main->Aimbot->RCSAmountY );
		SaveBool( main, XorStr( "aimbot.autowall" ), Main->Aimbot->AutoWall );
		SaveInt( main, XorStr( "aimbot.min.damage" ), Main->Aimbot->MinDamage );
		SaveInt( main, XorStr( "aimbot.hitscan" ), Main->Aimbot->HitScan );
		SaveFloat( main, XorStr( "aimbot.hitscan.scale" ), Main->Aimbot->HitScanScale );
		SaveInt( main, XorStr( "aimbot.target" ), Main->Aimbot->Target );
		SaveBool( main, XorStr( "aimbot.silent" ), Main->Aimbot->Silent );
		SaveBool(main, XorStr("aimbot.no.spread.active"), Main->Aimbot->NoSpreadActive);
		SaveInt(main, XorStr("aimbot.no.spread"), Main->Aimbot->NoSpread);
	//	SaveBool(main, XorStr("aimbot.seed.help"), Main->Aimbot->SeedHelp);
		SaveBool(main, XorStr("aimbot.height"), Main->Aimbot->Height);
		SaveFloat(main, XorStr("aimbot.height.scale"), Main->Aimbot->HeightScale);
		SaveInt(main, XorStr("aimbot.adjustment"), Main->Aimbot->LagCompensation);

		SaveInt( main, XorStr( "aimbot.style" ), Main->AimbotStyle );

		Legitbot->Clamp();

		SaveInt( main, XorStr( "legitbot.mode" ), Legitbot->Mode );
		SaveInt( main, XorStr( "legitbot.key" ), Legitbot->Key );
		SaveInt( main, XorStr( "legitbot.spot" ), Legitbot->Spot );
		SaveInt( main, XorStr( "legitbot.target.selection" ), Legitbot->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.fov" ), Legitbot->FieldOfView );
		SaveInt( main, XorStr( "legitbot.smooth" ), Legitbot->Smooth );
		SaveFloat( main, XorStr( "legitbot.step.vertical" ), Legitbot->StepX );
		SaveFloat( main, XorStr( "legitbot.step.horizontal" ), Legitbot->StepY );
		SaveFloat( main, XorStr( "legitbot.smooth.vertical" ), Legitbot->SmoothX );
		SaveFloat( main, XorStr( "legitbot.smooth.horizontal" ), Legitbot->SmoothY );
		SaveInt( main, XorStr( "legitbot.delay" ), Legitbot->Delay );
		SaveInt( main, XorStr( "legitbot.duration" ), Legitbot->Duration );
		SaveBool( main, XorStr( "legitbot.rcs" ), Legitbot->RCS );
		SaveInt( main, XorStr( "legitbot.rcs.delay" ), Legitbot->RCSDelay );
		SaveInt( main, XorStr( "legitbot.rcs.amount.vertical" ), Legitbot->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.rcs.amount.horizontal" ), Legitbot->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.target" ), Legitbot->Target );
		SaveBool( main, XorStr( "legitbot.auto.fire" ), Legitbot->AutoFire );
		SaveBool( main, XorStr( "legitbot.auto.stop" ), Legitbot->AutoStop );

		Main->Triggerbot->Clamp();

		SaveBool( main, XorStr( "triggerbot.weapon.config" ), Main->TriggerbotWeaponConfig );
		SaveInt( main, XorStr( "triggerbot.mode" ), Main->Triggerbot->Mode );
		SaveInt( main, XorStr( "triggerbot.key" ), Main->Triggerbot->Key );
		SaveInt( main, XorStr( "triggerbot.accuracy" ), Main->Triggerbot->Accuracy );
		SaveInt( main, XorStr( "triggerbot.delay" ), Main->Triggerbot->Delay );
		SaveInt( main, XorStr( "triggerbot.burst" ), Main->Triggerbot->Burst );
		SaveBool( main, XorStr( "triggerbot.head" ), Main->Triggerbot->Head );
		SaveBool( main, XorStr( "triggerbot.chest" ), Main->Triggerbot->Chest );
		SaveBool( main, XorStr( "triggerbot.stomach" ), Main->Triggerbot->Stomach );
		SaveBool( main, XorStr( "triggerbot.arms" ), Main->Triggerbot->Arms );
		SaveBool( main, XorStr( "triggerbot.legs" ), Main->Triggerbot->Legs );
		SaveBool( main, XorStr( "triggerbot.autowall" ), Main->Triggerbot->AutoWall );
		SaveInt( main, XorStr( "triggerbot.min.damage" ), Main->Triggerbot->MinDamage );
		SaveInt( main, XorStr( "triggerbot.target" ), Main->Triggerbot->Target );

		SaveInt(main, XorStr("esp.box"), ESP->Box);
		SaveBool(main, XorStr("esp.outlined"), ESP->Outlined);
		SaveBool(main, XorStr("esp.filled"), ESP->Filled);
		SaveBool( main, XorStr( "esp.name" ), ESP->Name );
		SaveBool( main, XorStr( "esp.weapon" ), ESP->Weapon );
		SaveBool( main, XorStr( "esp.aimspot" ), ESP->AimSpot );
		SaveInt( main, XorStr( "esp.health" ), ESP->Health );
		SaveInt( main, XorStr( "esp.armor" ), ESP->Armor );
		SaveInt( main, XorStr( "esp.skeleton" ), ESP->Skeleton );
		SaveBool( main, XorStr( "esp.visible" ), ESP->Colored );
		SaveBool( main, XorStr( "esp.defusing" ), ESP->Defusing );
		SaveBool( main, XorStr( "esp.bomb" ), ESP->Bomb );
		SaveInt( main, XorStr( "esp.target" ), ESP->Target );
		SaveBool(main, XorStr("esp.draw.fov"), ESP->Fov);

		SaveInt(main, XorStr("render.chams.mode"), Render->ChamsMode);
		SaveBool(main, XorStr("render.chams.visible"), Render->ChamsColored);
		SaveBool(main, XorStr("render.chams.outlined"), Render->ChamsOutlined);
		SaveInt(main, XorStr("render.chams.target"), Render->ChamsTarget);
		SaveBool(main, XorStr("render.chams.visibleonly"), Render->ChamsVisOnly);

		AntiAim->Clamp();

		SaveBool(main, XorStr("antiaim.at.target.enabled"), AntiAim->AtTargetEnabled);
		SaveInt(main, XorStr("antiaim.at.target"), AntiAim->AtTarget);
		SaveInt( main, XorStr( "antiaim.pitch.stand" ), AntiAim->PitchStand );
		SaveInt(main, XorStr("antiaim.yaw.stand"), AntiAim->YawStand);
		SaveInt(main, XorStr("antiaim.choked.packets.stand"), AntiAim->StandChokedPackets);
		SaveFloat(main, XorStr("antiaim.custom.pitch.stand"), AntiAim->StandCustomAnglePitch);
		SaveFloat(main, XorStr("antiaim.custom.fakepitch.stand"), AntiAim->StandCustomAngleFakePitch);
		SaveFloat(main, XorStr("antiaim.custom.yaw.stand"), AntiAim->StandCustomAngleYaw);
		SaveFloat(main, XorStr("antiaim.custom.fakeyaw.stand"), AntiAim->StandCustomAngleFakeYaw);
		SaveFloat(main, XorStr("antiaim.static.modifer.stand"), AntiAim->StandStaticModifer);
		SaveInt(main, XorStr("antiaim.stand.switch.delay"), AntiAim->StandSwitchPitchDelay);
		SaveInt(main, XorStr("antiaim.move.switch.delay"), AntiAim->MoveSwitchPitchDelay);
		SaveInt(main, XorStr("antiaim.spinspeed.stand"), AntiAim->StandSpinSpeed);
		SaveInt(main, XorStr("antiaim.pitch.move"), AntiAim->PitchMove);
		SaveInt(main, XorStr("antiaim.yaw.move"), AntiAim->YawMove);
		SaveInt(main, XorStr("antiaim.choked.packets.move"), AntiAim->MoveChokedPackets);
		SaveFloat(main, XorStr("antiaim.custom.pitch.move"), AntiAim->MoveCustomAnglePitch);
		SaveFloat(main, XorStr("antiaim.custom.fakepitch.move"), AntiAim->MoveCustomAngleFakePitch);
		SaveFloat(main, XorStr("antiaim.custom.yaw.move"), AntiAim->MoveCustomAngleYaw);
		SaveFloat(main, XorStr("antiaim.custom.fakeyaw.move"), AntiAim->MoveCustomAngleFakeYaw);
		SaveFloat(main, XorStr("antiaim.static.modifer.move"), AntiAim->MoveStaticModifer);
		SaveInt(main, XorStr("antiaim.spinspeed.move"), AntiAim->MoveSpinSpeed);
		SaveBool(main, XorStr("antiaim.no.enemy.enabled"), AntiAim->NoEnemyEnabled);
		SaveInt(main, XorStr("antiaim.no.enemy"), AntiAim->NoEnemy);
		SaveBool(main, XorStr("antiaim.on.knife"), AntiAim->OnKnife);
		SaveBool(main, XorStr("antiaim.fakewalk"), AntiAim->FakeWalk);
		SaveInt(main, XorStr("antiaim.fakewalk.key"), AntiAim->FakeWalkKey);
		SaveFloat(main, XorStr("antiaim.stand.breaker.real.first"), AntiAim->StandCustomAngleYaw1);
		SaveFloat(main, XorStr("antiaim.stand.breaker.real.second"), AntiAim->StandCustomAngleYaw2);
		SaveFloat(main, XorStr("antiaim.stand.breaker.fake.first"), AntiAim->StandCustomAngleFakeYaw1);
		SaveFloat(main, XorStr("antiaim.stand.breaker.fake.second"), AntiAim->StandCustomAngleFakeYaw2);
		SaveFloat(main, XorStr("antiaim.move.breaker.real.first"), AntiAim->MoveCustomAngleYaw1);
		SaveFloat(main, XorStr("antiaim.move.breaker.real.second"), AntiAim->MoveCustomAngleYaw2);
		SaveFloat(main, XorStr("antiaim.move.breaker.fake.first"), AntiAim->MoveCustomAngleFakeYaw1);
		SaveFloat(main, XorStr("antiaim.move.breaker.fake.second"), AntiAim->MoveCustomAngleFakeYaw2);
		SaveBool(main, XorStr("antiaim.hit.reactive"), AntiAim->HitReactive);
		SaveBool(main, XorStr("antiaim.break.lagcomp"), AntiAim->BreakLC);

		Removals->Clamp();
		SaveBool( main, XorStr( "removals.no.recoil" ), Removals->NoRecoil );
		SaveBool( main, XorStr( "removals.no.visual.recoil" ), Removals->NoVisualRecoil );
		SaveBool( main, XorStr( "removals.no.smoke" ), Removals->NoSmoke );
		SaveInt( main, XorStr( "removals.flash.amount" ), Removals->FlashAmount );

		Misc->Clamp();
	
		SaveBool(main, XorStr("misc.auto.jump"), Misc->AutoJump);
		SaveBool(main, XorStr("misc.thirdperson"), Misc->Lag);
		SaveInt(main, XorStr("misc.thirdperson.key"), Misc->LagKey);
		SaveInt(main, XorStr("misc.hitsound"), Misc->Hitmarker);
		SaveBool(main, XorStr("misc.hitmarker"), Misc->HitmarkerEnabled);
		SaveBool(main, XorStr("misc.hitmarker.hp"), Misc->HitmarkerHP);
		SaveBool( main, XorStr( "misc.auto.pistol" ), Misc->AutoPistol );
		SaveInt( main, XorStr( "misc.auto.strafe" ), Misc->AutoStrafe );
		SaveBool( main, XorStr( "misc.bomb.warning" ), Misc->BombWarning );
		SaveInt( main, XorStr( "misc.crosshair" ), Misc->Crosshair );
		SaveBool( main, XorStr( "misc.crosshair.outlined" ), Misc->Outlined );
		SaveBool( main, XorStr( "misc.crosshair.show.recoil" ), Misc->ShowRecoil );
		SaveBool( main, XorStr( "misc.fakelag" ), Misc->FakeLag );
		SaveInt( main, XorStr( "misc.fakelag.choked.packets" ), Misc->ChokedPackets );
		SaveBool( main, XorStr( "misc.airstuck" ), Misc->AirStuck );
		SaveInt( main, XorStr( "misc.airstuck.key" ), Misc->StuckKey );
		SaveBool( main, XorStr( "misc.cstrafer" ), Misc->Speed );
		SaveInt(main, XorStr("misc.cstrafer.key"), Misc->SpeedKey);
		SaveFloat(main, XorStr("misc.cstrafer.modifer"), Misc->SpeedMod);
		SaveInt(main, XorStr("misc.restriction"), Misc->Restriction);
		SaveFloat(main, XorStr("misc.resolver.angle"), Misc->ResolverAng);
		SaveBool(main, XorStr("misc.resolver.log"), Misc->ResolverLog);

		
		std::string colors( XorStr( "colors" ) );

		SaveColor( colors, XorStr( "t.esp.normal" ), Colors->T_ESP_Normal );
		SaveColor( colors, XorStr( "t.esp.visible" ), Colors->T_ESP_Colored );
		SaveColor( colors, XorStr( "t.chams.normal" ), Colors->T_Chams_Normal );
		SaveColor( colors, XorStr( "t.chams.visible" ), Colors->T_Chams_Colored );
		SaveColor( colors, XorStr( "ct.esp.normal" ), Colors->CT_ESP_Normal );
		SaveColor( colors, XorStr( "ct.esp.visible" ), Colors->CT_ESP_Colored );
		SaveColor( colors, XorStr( "ct.chams.normal" ), Colors->CT_Chams_Normal );
		SaveColor(colors, XorStr("ct.chams.visible"), Colors->CT_Chams_Colored);
		SaveColor(colors, XorStr("crosshair"), Colors->Crosshair);


		std::string binds( XorStr( "binds" ) );

		SaveInt( binds, XorStr( "menu" ), Binds->Menu );
		SaveInt( binds, XorStr( "eject" ), Binds->Eject );
		SaveInt( binds, XorStr( "panic" ), Binds->Panic );

		for( int i = 0; i < ARRAYSIZE( WeaponList ); i++ )
		{
			auto weapon = WeaponList[ i ];
			auto index = GetWeaponID( weapon );

			Main->Aimbot->Clamp();

			SaveInt( weapon, XorStr( "aimbot.mode" ), Weapon[ index ]->Aimbot->Mode );
			SaveInt( weapon, XorStr( "aimbot.key" ), Weapon[ index ]->Aimbot->Key );
			SaveBool( weapon, XorStr( "aimbot.auto.fire" ), Weapon[ index ]->Aimbot->AutoFire );
			SaveBool( weapon, XorStr( "aimbot.auto.stop" ), Weapon[ index ]->Aimbot->AutoStop );
			SaveBool( weapon, XorStr( "aimbot.auto.crouch" ), Weapon[ index ]->Aimbot->AutoCrouch );
			SaveBool(weapon, XorStr("aimbot.auto.reload"), Weapon[index]->Aimbot->AutoReload);
			SaveBool(weapon, XorStr("aimbot.anti.spawn.protection"), Weapon[index]->Aimbot->AntiSpawnProtection);
			SaveBool( weapon, XorStr( "aimbot.no.switch" ), Weapon[ index ]->Aimbot->NoSwitch );
			SaveInt( weapon, XorStr( "aimbot.spot" ), Weapon[ index ]->Aimbot->Spot );
			SaveBool( weapon, XorStr( "aimbot.spot.randomize" ), Weapon[ index ]->Aimbot->SpotRandomize );
			SaveInt( weapon, XorStr( "aimbot.target.selection" ), Weapon[ index ]->Aimbot->TargetSelection );
			SaveFloat( weapon, XorStr( "aimbot.fov" ), Weapon[ index ]->Aimbot->FieldOfView );
			SaveInt( weapon, XorStr( "aimbot.smooth" ), Weapon[ index ]->Aimbot->Smooth );
			SaveFloat( weapon, XorStr( "aimbot.step.vertical" ), Weapon[ index ]->Aimbot->StepX );
			SaveFloat( weapon, XorStr( "aimbot.step.horizontal" ), Weapon[ index ]->Aimbot->StepY );
			SaveFloat( weapon, XorStr( "aimbot.smooth.vertical" ), Weapon[ index ]->Aimbot->SmoothX );
			SaveFloat( weapon, XorStr( "aimbot.smooth.horizontal" ), Weapon[ index ]->Aimbot->SmoothY );
			SaveInt( weapon, XorStr( "aimbot.duration" ), Weapon[ index ]->Aimbot->Duration );
			SaveInt( weapon, XorStr( "aimbot.delay" ), Weapon[ index ]->Aimbot->Delay );
			SaveInt( weapon, XorStr( "aimbot.switch.delay" ), Weapon[ index ]->Aimbot->SwitchDelay );
			SaveBool( weapon, XorStr( "aimbot.rcs" ), Weapon[ index ]->Aimbot->RCS );
			SaveInt( weapon, XorStr( "aimbot.rcs.delay" ), Weapon[ index ]->Aimbot->RCSDelay );
			SaveInt( weapon, XorStr( "aimbot.rcs.amount.vertical" ), Weapon[ index ]->Aimbot->RCSAmountX );
			SaveInt( weapon, XorStr( "aimbot.rcs.amount.horizontal" ), Weapon[ index ]->Aimbot->RCSAmountY );
			SaveBool( weapon, XorStr( "aimbot.autowall" ), Weapon[ index ]->Aimbot->AutoWall );
			SaveInt( weapon, XorStr( "aimbot.min.damage" ), Weapon[ index ]->Aimbot->MinDamage );
			SaveInt( weapon, XorStr( "aimbot.hitscan" ), Weapon[ index ]->Aimbot->HitScan );
			SaveFloat( weapon, XorStr( "aimbot.hitscan.scale" ), Weapon[ index ]->Aimbot->HitScanScale );
			SaveInt( weapon, XorStr( "aimbot.target" ), Weapon[ index ]->Aimbot->Target );
			SaveBool( weapon, XorStr( "aimbot.silent" ), Weapon[ index ]->Aimbot->Silent );
			SaveBool(weapon, XorStr("aimbot.no.spread.active"), Weapon[index]->Aimbot->NoSpreadActive);
			SaveInt(weapon, XorStr("aimbot.no.spread"), Weapon[ index ]->Aimbot->NoSpread);
		//	SaveBool(weapon, XorStr("aimbot.seed.help"), Weapon[ index ]->Aimbot->SeedHelp);

			Weapon[ index ]->Triggerbot->Clamp();

			SaveInt( weapon, XorStr( "triggerbot.mode" ), Weapon[ index ]->Triggerbot->Mode );
			SaveInt( weapon, XorStr( "triggerbot.key" ), Weapon[ index ]->Triggerbot->Key );
			SaveInt( weapon, XorStr( "triggerbot.accuracy" ), Weapon[ index ]->Triggerbot->Accuracy );
			SaveInt( weapon, XorStr( "triggerbot.delay" ), Weapon[ index ]->Triggerbot->Delay );
			SaveInt( weapon, XorStr( "triggerbot.burst" ), Weapon[ index ]->Triggerbot->Burst );
			SaveBool( weapon, XorStr( "triggerbot.head" ), Weapon[ index ]->Triggerbot->Head );
			SaveBool( weapon, XorStr( "triggerbot.chest" ), Weapon[ index ]->Triggerbot->Chest );
			SaveBool( weapon, XorStr( "triggerbot.stomach" ), Weapon[ index ]->Triggerbot->Stomach );
			SaveBool( weapon, XorStr( "triggerbot.arms" ), Weapon[ index ]->Triggerbot->Arms );
			SaveBool( weapon, XorStr( "triggerbot.legs" ), Weapon[ index ]->Triggerbot->Legs );
			SaveBool( weapon, XorStr( "triggerbot.autowall" ), Weapon[ index ]->Triggerbot->AutoWall );
			SaveInt( weapon, XorStr( "triggerbot.min.damage" ), Weapon[ index ]->Triggerbot->MinDamage );
			SaveInt( weapon, XorStr( "triggerbot.target" ), Weapon[ index ]->Triggerbot->Target );
		}
	}

	void Delete( const std::string& name )
	{
		auto current = name;

		if( current.find( XorStr( ".cfg" ) ) == std::string::npos )
			current.append( XorStr( ".cfg" ) );

		m_current = m_config + current;

		DeleteFileA( m_current.c_str() );
	}

	void OnCreateMove()
	{
		auto player = C_CSPlayer::GetLocalPlayer();

		if( !player )
			return;

		auto weapon = player->GetActiveWeapon();

		if( !weapon )
			return;

		if( !Main || !Main->Aimbot || !Main->Triggerbot )
			return;

		auto i = weapon->GetWeaponID();

		if( Main->AimbotWeaponConfig )
			memcpy( Current->Aimbot, Weapon[ i ]->Aimbot, sizeof( AimbotList ) );
		else
			memcpy( Current->Aimbot, Main->Aimbot, sizeof( AimbotList ) );
		
		if( Main->TriggerbotWeaponConfig )
			memcpy( Current->Triggerbot, Weapon[ i ]->Triggerbot, sizeof( TriggerbotList ) );
		else
			memcpy( Current->Triggerbot, Main->Triggerbot, sizeof( TriggerbotList ) );

		// Legit-стиль: поверх текущего конфига кладём легитные настройки,
		// а рейдж-фичи принудительно гасим. Вся логика (Aimbot/Hooked)
		// читает только Current->Aimbot, поэтому переключение атомарно.
		if( Main->AimbotStyle == 1 && Legitbot )
		{
			Current->Aimbot->Mode				= Legitbot->Mode;
			Current->Aimbot->Key				= Legitbot->Key;
			Current->Aimbot->Spot				= Legitbot->Spot;
			Current->Aimbot->TargetSelection	= Legitbot->TargetSelection;
			Current->Aimbot->FieldOfView		= Legitbot->FieldOfView;
			Current->Aimbot->Smooth			= Legitbot->Smooth;
			Current->Aimbot->StepX				= Legitbot->StepX;
			Current->Aimbot->StepY				= Legitbot->StepY;
			Current->Aimbot->SmoothX			= Legitbot->SmoothX;
			Current->Aimbot->SmoothY			= Legitbot->SmoothY;
			Current->Aimbot->Delay				= Legitbot->Delay;
			Current->Aimbot->Duration			= Legitbot->Duration;
			Current->Aimbot->RCS				= Legitbot->RCS;
			Current->Aimbot->RCSDelay			= Legitbot->RCSDelay;
			Current->Aimbot->RCSAmountX		= Legitbot->RCSAmountX;
			Current->Aimbot->RCSAmountY		= Legitbot->RCSAmountY;
			Current->Aimbot->Target				= Legitbot->Target;
			Current->Aimbot->AutoFire			= Legitbot->AutoFire;
			Current->Aimbot->AutoStop			= Legitbot->AutoStop;

			// Рейдж-фичи в легите недоступны.
			Current->Aimbot->AutoCrouch			= false;
			Current->Aimbot->AutoReload			= false;
			Current->Aimbot->AntiSpawnProtection	= false;
			Current->Aimbot->NoSwitch			= true;
			Current->Aimbot->SpotRandomize			= false;
			Current->Aimbot->Height				= false;
			Current->Aimbot->HeightScale			= 0.0f;
			Current->Aimbot->HeightScaleX			= 0.0f;
			Current->Aimbot->HeightScaleY			= 0.0f;
			Current->Aimbot->SwitchDelay			= 0;
			Current->Aimbot->AutoWall			= false;
			Current->Aimbot->MinDamage			= 0;
			Current->Aimbot->HitScan				= 0;
			Current->Aimbot->HitScanScale			= 0.0f;
			Current->Aimbot->Silent				= false;
			Current->Aimbot->NoSpreadActive		= false;
			Current->Aimbot->NoSpread			= 0;
			Current->Aimbot->LagCompensation		= 0;
			Current->Aimbot->LastTick			= false;
			Current->Aimbot->SetAbs				= false;
			Current->Aimbot->UpdateAnim			= false;
			Current->Aimbot->Resolver			= false;
			Current->Aimbot->ResvolerBullets		= 0;
			Current->Aimbot->ResvolerBulletsDelay	= 0;
		}
	}

	auto GetPath() -> std::string
	{
		return m_config;
	}

	const char* WeaponList[ 24 ]
	{
		"Glock-18",
		"USP",
		"P228",
		"Desert Eagle",
		"Dual Berettas",
		"Five-Seven",
		"M3",
		"XM1024",
		"MAC-10",
		"TMP",
		"MP5",
		"UMP-45",
		"P90",
		"Famas",
		"Galil",
		"Scout",
		"M4A1",
		"AK-47",
		"AUG",
		"SG 552",
		"G3SG1",
		"SG 550",
		"AWP",
		"M249"
	};

	CSWeaponID GetWeaponID( const char* name )
	{
		if (!std::strcmp(name, XorStr("Glock-18")))
			return WEAPON_GLOCK;
		else if (!std::strcmp(name, XorStr("USP")))
			return WEAPON_USP;
		else if( !std::strcmp( name, XorStr( "P228" ) ) )
			return WEAPON_P228;
		else if (!std::strcmp(name, XorStr("Desert Eagle")))
			return WEAPON_DEAGLE;
		else if (!std::strcmp(name, XorStr("Dual Berettas")))
			return WEAPON_ELITE;
		else if (!std::strcmp(name, XorStr("Five-SeveN")))
			return WEAPON_FIVESEVEN;
		else if (!std::strcmp(name, XorStr("M3")))
			return WEAPON_M3;
		else if (!std::strcmp(name, XorStr("XM1014")))
			return WEAPON_XM1014;
		else if (!std::strcmp(name, XorStr("MAC-10")))
			return WEAPON_MAC10;
		else if (!std::strcmp(name, XorStr("TMP")))
			return WEAPON_TMP;
		else if (!std::strcmp(name, XorStr("MP5")))
			return WEAPON_MP5NAVY;
		else if (!std::strcmp(name, XorStr("UMP-45")))
			return WEAPON_UMP45;
		else if (!std::strcmp(name, XorStr("P90")))
			return WEAPON_P90;
		else if (!std::strcmp(name, XorStr("Famas")))
			return WEAPON_FAMAS;
		else if (!std::strcmp(name, XorStr("Galil")))
			return WEAPON_GALIL;
		else if( !std::strcmp( name, XorStr( "Scout" ) ) )
			return WEAPON_SCOUT;
		else if (!std::strcmp(name, XorStr("M4A1")))
			return WEAPON_M4A1;
		else if (!std::strcmp(name, XorStr("AK-47")))
			return WEAPON_AK47;
		else if( !std::strcmp( name, XorStr( "AUG" ) ) )
			return WEAPON_AUG;
		else if( !std::strcmp( name, XorStr( "SG 552" ) ) )
			return WEAPON_SG552;
		else if (!std::strcmp(name, XorStr("G3SG1")))
			return WEAPON_G3SG1;
		else if (!std::strcmp(name, XorStr("SG 550")))
			return WEAPON_SG550;
		else if( !std::strcmp( name, XorStr( "AWP" ) ) )
			return WEAPON_AWP;
		else if (!std::strcmp(name, XorStr("M249")))
			return WEAPON_M249;
		else
			return WEAPON_NONE;
	}
}