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
	LegitbotList*	LegitbotClasses[ 5 ] = { nullptr, nullptr, nullptr, nullptr, nullptr };

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
			for( int i = 0; i < 5; i++ )
				LegitbotClasses[ i ] = new LegitbotList();

			Legitbot = LegitbotClasses[ 0 ];

			m_config = Shared::m_pVars->m_loader;
			m_config.append(XorStr("\\v34\\"));

			Shared::m_strConfig = "default";
			Shared::m_bLoad = true;

		}
	

	void Release()
	{
		// Все указатели обнуляются: Release() может быть вызван повторно
		// (Eject из потока + DLL_PROCESS_DETACH) или после провалившегося
		// Startup(), а двойное delete роняло игру.
		if( Main )
		{
			Memory::SafeDelete( Main->Aimbot );
			Memory::SafeDelete( Main->Triggerbot );
			Memory::SafeDelete( Main );
		}

		if( Current )
		{
			Memory::SafeDelete( Current->Aimbot );
			Memory::SafeDelete( Current->Triggerbot );
			Memory::SafeDelete( Current );
		}

		for( int i = 0; i < WEAPON_MAX; i++ )
		{
			if( Weapon[ i ] )
			{
				Memory::SafeDelete( Weapon[ i ]->Aimbot );
				Memory::SafeDelete( Weapon[ i ]->Triggerbot );

				Memory::SafeDelete( Weapon[ i ] );
			}

			Weapon[ i ] = nullptr;
		}

		Memory::SafeDelete( ESP );
		Memory::SafeDelete( Render );
		Memory::SafeDelete( AntiAim );
		Memory::SafeDelete( Removals );
		Memory::SafeDelete( Misc );
		Memory::SafeDelete( Colors );
		Memory::SafeDelete( Binds );

		for( int i = 0; i < 5; i++ )
		{
			Memory::SafeDelete( LegitbotClasses[ i ] );
			LegitbotClasses[ i ] = nullptr;
		}

		Legitbot = nullptr;
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

	// Как LoadInt, но при отсутствии ключа возвращает значение по умолчанию.
	// Нужен там, где «0» — невалидное значение (например, клавиша меню):
	// старый конфиг без нужной секции выключал меню навсегда.
	int LoadIntDef( const std::string& strSection, const std::string& strName, int iDefault )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), "", szData, MAX_PATH, m_current.c_str() );

		if( szData[ 0 ] == '\0' )
			return iDefault;

		return atoi( szData );
	}

	float LoadFloat( const std::string& strSection, const std::string& strName )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), XorStr( "0" ), szData, MAX_PATH, m_current.c_str() );
		return ( float )atof( szData );
	}

	// Как LoadFloat, но при отсутствии ключа возвращает значение по умолчанию
	// (нужно для настроек оружия: слот по умолчанию повторяет общий конфиг).
	float LoadFloatDef( const std::string& strSection, const std::string& strName, float flDefault )
	{
		char szData[ MAX_PATH ];
		GetPrivateProfileString( strSection.c_str(), strName.c_str(), "", szData, MAX_PATH, m_current.c_str() );

		if( szData[ 0 ] == '\0' )
			return flDefault;

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
		// Раньше здесь был указатель на временный XorString: он умирал в конце
		// строки, а использовался в следующей (UB). Копируем в std::string.
		static const std::string s_strTrue  = XorStr( "true" );
		static const std::string s_strFalse = XorStr( "false" );
		const char* szData = bValue ? s_strTrue.c_str() : s_strFalse.c_str();
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

	// Config::Create() создаёт ПУСТОЙ файл, поэтому одной проверки Exists()
	// мало: файл существует, но все ключи в нём отсутствуют — LoadInt()
	// вернул бы нули (в том числе menu = 0, из-за чего меню не открывалось).
	bool IsFileEmpty( const std::string& name )
	{
		std::ifstream f( name, std::ios::binary | std::ios::ate );

		if( !f.good() )
			return true;

		return f.tellg() <= 0;
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

		if( !Exists( m_current ) || IsFileEmpty( m_current ) )
			Save( name );

		std::string main( XorStr( "main" ) );

		Main->AimbotWeaponConfig		= LoadBool( main, XorStr( "aimbot.weapon.config" ) );
		Main->Aimbot->Mode				= LoadInt( main, XorStr( "aimbot.mode" ) );
		Main->Aimbot->Key				= LoadInt( main, XorStr( "aimbot.key" ) );
		Main->Aimbot->AutoFire			= LoadBool( main, XorStr( "aimbot.auto.fire" ) );
		Main->Aimbot->AutoStop			= LoadBool( main, XorStr( "aimbot.auto.stop" ) );
		Main->Aimbot->AutoCrouch		= LoadBool( main, XorStr( "aimbot.auto.crouch" ) );
		Main->Aimbot->AutoReload = LoadBool(main, XorStr("aimbot.auto.reload"));
		Main->Aimbot->AutoScope = LoadBool(main, XorStr("aimbot.auto.scope"));
		Main->Aimbot->MinDamageOverride = LoadInt(main, XorStr("aimbot.mindamage.override"));
		Main->Aimbot->MinDamageOverrideKey = LoadInt(main, XorStr("aimbot.mindamage.override.key"));
		Main->Aimbot->AntiSpawnProtection = LoadBool(main, XorStr("aimbot.anti.spawn.protection"));
		Main->Aimbot->NoSwitch			= LoadBool( main, XorStr( "aimbot.no.switch" ) );
		Main->Aimbot->Spot				= LoadInt( main, XorStr( "aimbot.spot" ) );
		Main->Aimbot->ForceBody		= LoadInt( main, XorStr( "aimbot.body.aim" ) );
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

		// Legit class: pistol
		LegitbotClasses[ 0 ]->Mode = LoadInt( main, XorStr( "legitbot.pistol.mode" ) );
		LegitbotClasses[ 0 ]->Key = LoadInt( main, XorStr( "legitbot.pistol.key" ) );
		LegitbotClasses[ 0 ]->TargetSelection = LoadInt( main, XorStr( "legitbot.pistol.target.selection" ) );
		LegitbotClasses[ 0 ]->FieldOfView = LoadFloat( main, XorStr( "legitbot.pistol.fov" ) );
		LegitbotClasses[ 0 ]->Smooth = LoadInt( main, XorStr( "legitbot.pistol.smooth" ) );
		LegitbotClasses[ 0 ]->StepX = LoadFloat( main, XorStr( "legitbot.pistol.step.vertical" ) );
		LegitbotClasses[ 0 ]->StepY = LoadFloat( main, XorStr( "legitbot.pistol.step.horizontal" ) );
		LegitbotClasses[ 0 ]->SmoothX = LoadFloat( main, XorStr( "legitbot.pistol.smooth.vertical" ) );
		LegitbotClasses[ 0 ]->SmoothY = LoadFloat( main, XorStr( "legitbot.pistol.smooth.horizontal" ) );
		LegitbotClasses[ 0 ]->Delay = LoadInt( main, XorStr( "legitbot.pistol.delay" ) );
		LegitbotClasses[ 0 ]->Duration = LoadInt( main, XorStr( "legitbot.pistol.duration" ) );
		LegitbotClasses[ 0 ]->RCS = LoadBool( main, XorStr( "legitbot.pistol.rcs" ) );
		LegitbotClasses[ 0 ]->RCSDelay = LoadInt( main, XorStr( "legitbot.pistol.rcs.delay" ) );
		LegitbotClasses[ 0 ]->RCSAmountX = LoadInt( main, XorStr( "legitbot.pistol.rcs.amount.vertical" ) );
		LegitbotClasses[ 0 ]->RCSAmountY = LoadInt( main, XorStr( "legitbot.pistol.rcs.amount.horizontal" ) );
		LegitbotClasses[ 0 ]->Target = LoadInt( main, XorStr( "legitbot.pistol.target" ) );
		LegitbotClasses[ 0 ]->AutoFire = LoadBool( main, XorStr( "legitbot.pistol.auto.fire" ) );
		LegitbotClasses[ 0 ]->AutoStop = LoadBool( main, XorStr( "legitbot.pistol.auto.stop" ) );
		LegitbotClasses[ 0 ]->RCSStandalone = LoadBool( main, XorStr( "legitbot.pistol.rcs.standalone" ) );
		LegitbotClasses[ 0 ]->FlashCheck = LoadBool( main, XorStr( "legitbot.pistol.flash.check" ) );
		LegitbotClasses[ 0 ]->Backtrack = LoadBool( main, XorStr( "legitbot.pistol.backtrack" ) );
		LegitbotClasses[ 0 ]->HumanizeDelay = LoadBool( main, XorStr( "legitbot.pistol.delay.humanize" ) );
		LegitbotClasses[ 0 ]->AutoScope = LoadBool( main, XorStr( "legitbot.pistol.auto.scope" ) );
		LegitbotClasses[ 0 ]->Randomize = LoadFloat( main, XorStr( "legitbot.pistol.randomize" ) );
		LegitbotClasses[ 0 ]->Curve = LoadFloat( main, XorStr( "legitbot.pistol.curve" ) );
		LegitbotClasses[ 0 ]->ZoneHead = LoadBool( main, XorStr( "legitbot.pistol.zone.head" ) );
		LegitbotClasses[ 0 ]->ZoneChest = LoadBool( main, XorStr( "legitbot.pistol.zone.chest" ) );
		LegitbotClasses[ 0 ]->ZoneStomach = LoadBool( main, XorStr( "legitbot.pistol.zone.stomach" ) );
		LegitbotClasses[ 0 ]->ZoneArms = LoadBool( main, XorStr( "legitbot.pistol.zone.arms" ) );
		LegitbotClasses[ 0 ]->ZoneLegs = LoadBool( main, XorStr( "legitbot.pistol.zone.legs" ) );
		LegitbotClasses[ 0 ]->HitboxPriority = LoadInt( main, XorStr( "legitbot.pistol.hitbox.priority" ) );
		LegitbotClasses[ 0 ]->HitboxSelection = LoadInt( main, XorStr( "legitbot.pistol.hitbox.selection" ) );
		LegitbotClasses[ 0 ]->ToggleKey = LoadInt( main, XorStr( "legitbot.pistol.toggle.key" ) );
		LegitbotClasses[ 0 ]->FireOnKey = LoadBool( main, XorStr( "legitbot.pistol.fire.on.key" ) );
		LegitbotClasses[ 0 ]->FireKey = LoadInt( main, XorStr( "legitbot.pistol.fire.key" ) );
		LegitbotClasses[ 0 ]->TSD = LoadInt( main, XorStr( "legitbot.pistol.switch.delay" ) );
		LegitbotClasses[ 0 ]->ThroughSmoke = LoadBool( main, XorStr( "legitbot.pistol.through.smoke" ) );
		LegitbotClasses[ 0 ]->AutoWall = LoadBool( main, XorStr( "legitbot.pistol.auto.wall" ) );
		LegitbotClasses[ 0 ]->MinDamage = LoadInt( main, XorStr( "legitbot.pistol.min.damage" ) );

		// Legit class: smg
		LegitbotClasses[ 1 ]->Mode = LoadInt( main, XorStr( "legitbot.smg.mode" ) );
		LegitbotClasses[ 1 ]->Key = LoadInt( main, XorStr( "legitbot.smg.key" ) );
		LegitbotClasses[ 1 ]->TargetSelection = LoadInt( main, XorStr( "legitbot.smg.target.selection" ) );
		LegitbotClasses[ 1 ]->FieldOfView = LoadFloat( main, XorStr( "legitbot.smg.fov" ) );
		LegitbotClasses[ 1 ]->Smooth = LoadInt( main, XorStr( "legitbot.smg.smooth" ) );
		LegitbotClasses[ 1 ]->StepX = LoadFloat( main, XorStr( "legitbot.smg.step.vertical" ) );
		LegitbotClasses[ 1 ]->StepY = LoadFloat( main, XorStr( "legitbot.smg.step.horizontal" ) );
		LegitbotClasses[ 1 ]->SmoothX = LoadFloat( main, XorStr( "legitbot.smg.smooth.vertical" ) );
		LegitbotClasses[ 1 ]->SmoothY = LoadFloat( main, XorStr( "legitbot.smg.smooth.horizontal" ) );
		LegitbotClasses[ 1 ]->Delay = LoadInt( main, XorStr( "legitbot.smg.delay" ) );
		LegitbotClasses[ 1 ]->Duration = LoadInt( main, XorStr( "legitbot.smg.duration" ) );
		LegitbotClasses[ 1 ]->RCS = LoadBool( main, XorStr( "legitbot.smg.rcs" ) );
		LegitbotClasses[ 1 ]->RCSDelay = LoadInt( main, XorStr( "legitbot.smg.rcs.delay" ) );
		LegitbotClasses[ 1 ]->RCSAmountX = LoadInt( main, XorStr( "legitbot.smg.rcs.amount.vertical" ) );
		LegitbotClasses[ 1 ]->RCSAmountY = LoadInt( main, XorStr( "legitbot.smg.rcs.amount.horizontal" ) );
		LegitbotClasses[ 1 ]->Target = LoadInt( main, XorStr( "legitbot.smg.target" ) );
		LegitbotClasses[ 1 ]->AutoFire = LoadBool( main, XorStr( "legitbot.smg.auto.fire" ) );
		LegitbotClasses[ 1 ]->AutoStop = LoadBool( main, XorStr( "legitbot.smg.auto.stop" ) );
		LegitbotClasses[ 1 ]->RCSStandalone = LoadBool( main, XorStr( "legitbot.smg.rcs.standalone" ) );
		LegitbotClasses[ 1 ]->FlashCheck = LoadBool( main, XorStr( "legitbot.smg.flash.check" ) );
		LegitbotClasses[ 1 ]->Backtrack = LoadBool( main, XorStr( "legitbot.smg.backtrack" ) );
		LegitbotClasses[ 1 ]->HumanizeDelay = LoadBool( main, XorStr( "legitbot.smg.delay.humanize" ) );
		LegitbotClasses[ 1 ]->AutoScope = LoadBool( main, XorStr( "legitbot.smg.auto.scope" ) );
		LegitbotClasses[ 1 ]->Randomize = LoadFloat( main, XorStr( "legitbot.smg.randomize" ) );
		LegitbotClasses[ 1 ]->Curve = LoadFloat( main, XorStr( "legitbot.smg.curve" ) );
		LegitbotClasses[ 1 ]->ZoneHead = LoadBool( main, XorStr( "legitbot.smg.zone.head" ) );
		LegitbotClasses[ 1 ]->ZoneChest = LoadBool( main, XorStr( "legitbot.smg.zone.chest" ) );
		LegitbotClasses[ 1 ]->ZoneStomach = LoadBool( main, XorStr( "legitbot.smg.zone.stomach" ) );
		LegitbotClasses[ 1 ]->ZoneArms = LoadBool( main, XorStr( "legitbot.smg.zone.arms" ) );
		LegitbotClasses[ 1 ]->ZoneLegs = LoadBool( main, XorStr( "legitbot.smg.zone.legs" ) );
		LegitbotClasses[ 1 ]->HitboxPriority = LoadInt( main, XorStr( "legitbot.smg.hitbox.priority" ) );
		LegitbotClasses[ 1 ]->HitboxSelection = LoadInt( main, XorStr( "legitbot.smg.hitbox.selection" ) );
		LegitbotClasses[ 1 ]->ToggleKey = LoadInt( main, XorStr( "legitbot.smg.toggle.key" ) );
		LegitbotClasses[ 1 ]->FireOnKey = LoadBool( main, XorStr( "legitbot.smg.fire.on.key" ) );
		LegitbotClasses[ 1 ]->FireKey = LoadInt( main, XorStr( "legitbot.smg.fire.key" ) );
		LegitbotClasses[ 1 ]->TSD = LoadInt( main, XorStr( "legitbot.smg.switch.delay" ) );
		LegitbotClasses[ 1 ]->ThroughSmoke = LoadBool( main, XorStr( "legitbot.smg.through.smoke" ) );
		LegitbotClasses[ 1 ]->AutoWall = LoadBool( main, XorStr( "legitbot.smg.auto.wall" ) );
		LegitbotClasses[ 1 ]->MinDamage = LoadInt( main, XorStr( "legitbot.smg.min.damage" ) );

		// Legit class: rifle
		LegitbotClasses[ 2 ]->Mode = LoadInt( main, XorStr( "legitbot.rifle.mode" ) );
		LegitbotClasses[ 2 ]->Key = LoadInt( main, XorStr( "legitbot.rifle.key" ) );
		LegitbotClasses[ 2 ]->TargetSelection = LoadInt( main, XorStr( "legitbot.rifle.target.selection" ) );
		LegitbotClasses[ 2 ]->FieldOfView = LoadFloat( main, XorStr( "legitbot.rifle.fov" ) );
		LegitbotClasses[ 2 ]->Smooth = LoadInt( main, XorStr( "legitbot.rifle.smooth" ) );
		LegitbotClasses[ 2 ]->StepX = LoadFloat( main, XorStr( "legitbot.rifle.step.vertical" ) );
		LegitbotClasses[ 2 ]->StepY = LoadFloat( main, XorStr( "legitbot.rifle.step.horizontal" ) );
		LegitbotClasses[ 2 ]->SmoothX = LoadFloat( main, XorStr( "legitbot.rifle.smooth.vertical" ) );
		LegitbotClasses[ 2 ]->SmoothY = LoadFloat( main, XorStr( "legitbot.rifle.smooth.horizontal" ) );
		LegitbotClasses[ 2 ]->Delay = LoadInt( main, XorStr( "legitbot.rifle.delay" ) );
		LegitbotClasses[ 2 ]->Duration = LoadInt( main, XorStr( "legitbot.rifle.duration" ) );
		LegitbotClasses[ 2 ]->RCS = LoadBool( main, XorStr( "legitbot.rifle.rcs" ) );
		LegitbotClasses[ 2 ]->RCSDelay = LoadInt( main, XorStr( "legitbot.rifle.rcs.delay" ) );
		LegitbotClasses[ 2 ]->RCSAmountX = LoadInt( main, XorStr( "legitbot.rifle.rcs.amount.vertical" ) );
		LegitbotClasses[ 2 ]->RCSAmountY = LoadInt( main, XorStr( "legitbot.rifle.rcs.amount.horizontal" ) );
		LegitbotClasses[ 2 ]->Target = LoadInt( main, XorStr( "legitbot.rifle.target" ) );
		LegitbotClasses[ 2 ]->AutoFire = LoadBool( main, XorStr( "legitbot.rifle.auto.fire" ) );
		LegitbotClasses[ 2 ]->AutoStop = LoadBool( main, XorStr( "legitbot.rifle.auto.stop" ) );
		LegitbotClasses[ 2 ]->RCSStandalone = LoadBool( main, XorStr( "legitbot.rifle.rcs.standalone" ) );
		LegitbotClasses[ 2 ]->FlashCheck = LoadBool( main, XorStr( "legitbot.rifle.flash.check" ) );
		LegitbotClasses[ 2 ]->Backtrack = LoadBool( main, XorStr( "legitbot.rifle.backtrack" ) );
		LegitbotClasses[ 2 ]->HumanizeDelay = LoadBool( main, XorStr( "legitbot.rifle.delay.humanize" ) );
		LegitbotClasses[ 2 ]->AutoScope = LoadBool( main, XorStr( "legitbot.rifle.auto.scope" ) );
		LegitbotClasses[ 2 ]->Randomize = LoadFloat( main, XorStr( "legitbot.rifle.randomize" ) );
		LegitbotClasses[ 2 ]->Curve = LoadFloat( main, XorStr( "legitbot.rifle.curve" ) );
		LegitbotClasses[ 2 ]->ZoneHead = LoadBool( main, XorStr( "legitbot.rifle.zone.head" ) );
		LegitbotClasses[ 2 ]->ZoneChest = LoadBool( main, XorStr( "legitbot.rifle.zone.chest" ) );
		LegitbotClasses[ 2 ]->ZoneStomach = LoadBool( main, XorStr( "legitbot.rifle.zone.stomach" ) );
		LegitbotClasses[ 2 ]->ZoneArms = LoadBool( main, XorStr( "legitbot.rifle.zone.arms" ) );
		LegitbotClasses[ 2 ]->ZoneLegs = LoadBool( main, XorStr( "legitbot.rifle.zone.legs" ) );
		LegitbotClasses[ 2 ]->HitboxPriority = LoadInt( main, XorStr( "legitbot.rifle.hitbox.priority" ) );
		LegitbotClasses[ 2 ]->HitboxSelection = LoadInt( main, XorStr( "legitbot.rifle.hitbox.selection" ) );
		LegitbotClasses[ 2 ]->ToggleKey = LoadInt( main, XorStr( "legitbot.rifle.toggle.key" ) );
		LegitbotClasses[ 2 ]->FireOnKey = LoadBool( main, XorStr( "legitbot.rifle.fire.on.key" ) );
		LegitbotClasses[ 2 ]->FireKey = LoadInt( main, XorStr( "legitbot.rifle.fire.key" ) );
		LegitbotClasses[ 2 ]->TSD = LoadInt( main, XorStr( "legitbot.rifle.switch.delay" ) );
		LegitbotClasses[ 2 ]->ThroughSmoke = LoadBool( main, XorStr( "legitbot.rifle.through.smoke" ) );
		LegitbotClasses[ 2 ]->AutoWall = LoadBool( main, XorStr( "legitbot.rifle.auto.wall" ) );
		LegitbotClasses[ 2 ]->MinDamage = LoadInt( main, XorStr( "legitbot.rifle.min.damage" ) );

		// Legit class: shotgun
		LegitbotClasses[ 3 ]->Mode = LoadInt( main, XorStr( "legitbot.shotgun.mode" ) );
		LegitbotClasses[ 3 ]->Key = LoadInt( main, XorStr( "legitbot.shotgun.key" ) );
		LegitbotClasses[ 3 ]->TargetSelection = LoadInt( main, XorStr( "legitbot.shotgun.target.selection" ) );
		LegitbotClasses[ 3 ]->FieldOfView = LoadFloat( main, XorStr( "legitbot.shotgun.fov" ) );
		LegitbotClasses[ 3 ]->Smooth = LoadInt( main, XorStr( "legitbot.shotgun.smooth" ) );
		LegitbotClasses[ 3 ]->StepX = LoadFloat( main, XorStr( "legitbot.shotgun.step.vertical" ) );
		LegitbotClasses[ 3 ]->StepY = LoadFloat( main, XorStr( "legitbot.shotgun.step.horizontal" ) );
		LegitbotClasses[ 3 ]->SmoothX = LoadFloat( main, XorStr( "legitbot.shotgun.smooth.vertical" ) );
		LegitbotClasses[ 3 ]->SmoothY = LoadFloat( main, XorStr( "legitbot.shotgun.smooth.horizontal" ) );
		LegitbotClasses[ 3 ]->Delay = LoadInt( main, XorStr( "legitbot.shotgun.delay" ) );
		LegitbotClasses[ 3 ]->Duration = LoadInt( main, XorStr( "legitbot.shotgun.duration" ) );
		LegitbotClasses[ 3 ]->RCS = LoadBool( main, XorStr( "legitbot.shotgun.rcs" ) );
		LegitbotClasses[ 3 ]->RCSDelay = LoadInt( main, XorStr( "legitbot.shotgun.rcs.delay" ) );
		LegitbotClasses[ 3 ]->RCSAmountX = LoadInt( main, XorStr( "legitbot.shotgun.rcs.amount.vertical" ) );
		LegitbotClasses[ 3 ]->RCSAmountY = LoadInt( main, XorStr( "legitbot.shotgun.rcs.amount.horizontal" ) );
		LegitbotClasses[ 3 ]->Target = LoadInt( main, XorStr( "legitbot.shotgun.target" ) );
		LegitbotClasses[ 3 ]->AutoFire = LoadBool( main, XorStr( "legitbot.shotgun.auto.fire" ) );
		LegitbotClasses[ 3 ]->AutoStop = LoadBool( main, XorStr( "legitbot.shotgun.auto.stop" ) );
		LegitbotClasses[ 3 ]->RCSStandalone = LoadBool( main, XorStr( "legitbot.shotgun.rcs.standalone" ) );
		LegitbotClasses[ 3 ]->FlashCheck = LoadBool( main, XorStr( "legitbot.shotgun.flash.check" ) );
		LegitbotClasses[ 3 ]->Backtrack = LoadBool( main, XorStr( "legitbot.shotgun.backtrack" ) );
		LegitbotClasses[ 3 ]->HumanizeDelay = LoadBool( main, XorStr( "legitbot.shotgun.delay.humanize" ) );
		LegitbotClasses[ 3 ]->AutoScope = LoadBool( main, XorStr( "legitbot.shotgun.auto.scope" ) );
		LegitbotClasses[ 3 ]->Randomize = LoadFloat( main, XorStr( "legitbot.shotgun.randomize" ) );
		LegitbotClasses[ 3 ]->Curve = LoadFloat( main, XorStr( "legitbot.shotgun.curve" ) );
		LegitbotClasses[ 3 ]->ZoneHead = LoadBool( main, XorStr( "legitbot.shotgun.zone.head" ) );
		LegitbotClasses[ 3 ]->ZoneChest = LoadBool( main, XorStr( "legitbot.shotgun.zone.chest" ) );
		LegitbotClasses[ 3 ]->ZoneStomach = LoadBool( main, XorStr( "legitbot.shotgun.zone.stomach" ) );
		LegitbotClasses[ 3 ]->ZoneArms = LoadBool( main, XorStr( "legitbot.shotgun.zone.arms" ) );
		LegitbotClasses[ 3 ]->ZoneLegs = LoadBool( main, XorStr( "legitbot.shotgun.zone.legs" ) );
		LegitbotClasses[ 3 ]->HitboxPriority = LoadInt( main, XorStr( "legitbot.shotgun.hitbox.priority" ) );
		LegitbotClasses[ 3 ]->HitboxSelection = LoadInt( main, XorStr( "legitbot.shotgun.hitbox.selection" ) );
		LegitbotClasses[ 3 ]->ToggleKey = LoadInt( main, XorStr( "legitbot.shotgun.toggle.key" ) );
		LegitbotClasses[ 3 ]->FireOnKey = LoadBool( main, XorStr( "legitbot.shotgun.fire.on.key" ) );
		LegitbotClasses[ 3 ]->FireKey = LoadInt( main, XorStr( "legitbot.shotgun.fire.key" ) );
		LegitbotClasses[ 3 ]->TSD = LoadInt( main, XorStr( "legitbot.shotgun.switch.delay" ) );
		LegitbotClasses[ 3 ]->ThroughSmoke = LoadBool( main, XorStr( "legitbot.shotgun.through.smoke" ) );
		LegitbotClasses[ 3 ]->AutoWall = LoadBool( main, XorStr( "legitbot.shotgun.auto.wall" ) );
		LegitbotClasses[ 3 ]->MinDamage = LoadInt( main, XorStr( "legitbot.shotgun.min.damage" ) );

		// Legit class: sniper
		LegitbotClasses[ 4 ]->Mode = LoadInt( main, XorStr( "legitbot.sniper.mode" ) );
		LegitbotClasses[ 4 ]->Key = LoadInt( main, XorStr( "legitbot.sniper.key" ) );
		LegitbotClasses[ 4 ]->TargetSelection = LoadInt( main, XorStr( "legitbot.sniper.target.selection" ) );
		LegitbotClasses[ 4 ]->FieldOfView = LoadFloat( main, XorStr( "legitbot.sniper.fov" ) );
		LegitbotClasses[ 4 ]->Smooth = LoadInt( main, XorStr( "legitbot.sniper.smooth" ) );
		LegitbotClasses[ 4 ]->StepX = LoadFloat( main, XorStr( "legitbot.sniper.step.vertical" ) );
		LegitbotClasses[ 4 ]->StepY = LoadFloat( main, XorStr( "legitbot.sniper.step.horizontal" ) );
		LegitbotClasses[ 4 ]->SmoothX = LoadFloat( main, XorStr( "legitbot.sniper.smooth.vertical" ) );
		LegitbotClasses[ 4 ]->SmoothY = LoadFloat( main, XorStr( "legitbot.sniper.smooth.horizontal" ) );
		LegitbotClasses[ 4 ]->Delay = LoadInt( main, XorStr( "legitbot.sniper.delay" ) );
		LegitbotClasses[ 4 ]->Duration = LoadInt( main, XorStr( "legitbot.sniper.duration" ) );
		LegitbotClasses[ 4 ]->RCS = LoadBool( main, XorStr( "legitbot.sniper.rcs" ) );
		LegitbotClasses[ 4 ]->RCSDelay = LoadInt( main, XorStr( "legitbot.sniper.rcs.delay" ) );
		LegitbotClasses[ 4 ]->RCSAmountX = LoadInt( main, XorStr( "legitbot.sniper.rcs.amount.vertical" ) );
		LegitbotClasses[ 4 ]->RCSAmountY = LoadInt( main, XorStr( "legitbot.sniper.rcs.amount.horizontal" ) );
		LegitbotClasses[ 4 ]->Target = LoadInt( main, XorStr( "legitbot.sniper.target" ) );
		LegitbotClasses[ 4 ]->AutoFire = LoadBool( main, XorStr( "legitbot.sniper.auto.fire" ) );
		LegitbotClasses[ 4 ]->AutoStop = LoadBool( main, XorStr( "legitbot.sniper.auto.stop" ) );
		LegitbotClasses[ 4 ]->RCSStandalone = LoadBool( main, XorStr( "legitbot.sniper.rcs.standalone" ) );
		LegitbotClasses[ 4 ]->FlashCheck = LoadBool( main, XorStr( "legitbot.sniper.flash.check" ) );
		LegitbotClasses[ 4 ]->Backtrack = LoadBool( main, XorStr( "legitbot.sniper.backtrack" ) );
		LegitbotClasses[ 4 ]->HumanizeDelay = LoadBool( main, XorStr( "legitbot.sniper.delay.humanize" ) );
		LegitbotClasses[ 4 ]->AutoScope = LoadBool( main, XorStr( "legitbot.sniper.auto.scope" ) );
		LegitbotClasses[ 4 ]->Randomize = LoadFloat( main, XorStr( "legitbot.sniper.randomize" ) );
		LegitbotClasses[ 4 ]->Curve = LoadFloat( main, XorStr( "legitbot.sniper.curve" ) );
		LegitbotClasses[ 4 ]->ZoneHead = LoadBool( main, XorStr( "legitbot.sniper.zone.head" ) );
		LegitbotClasses[ 4 ]->ZoneChest = LoadBool( main, XorStr( "legitbot.sniper.zone.chest" ) );
		LegitbotClasses[ 4 ]->ZoneStomach = LoadBool( main, XorStr( "legitbot.sniper.zone.stomach" ) );
		LegitbotClasses[ 4 ]->ZoneArms = LoadBool( main, XorStr( "legitbot.sniper.zone.arms" ) );
		LegitbotClasses[ 4 ]->ZoneLegs = LoadBool( main, XorStr( "legitbot.sniper.zone.legs" ) );
		LegitbotClasses[ 4 ]->HitboxPriority = LoadInt( main, XorStr( "legitbot.sniper.hitbox.priority" ) );
		LegitbotClasses[ 4 ]->HitboxSelection = LoadInt( main, XorStr( "legitbot.sniper.hitbox.selection" ) );
		LegitbotClasses[ 4 ]->ToggleKey = LoadInt( main, XorStr( "legitbot.sniper.toggle.key" ) );
		LegitbotClasses[ 4 ]->FireOnKey = LoadBool( main, XorStr( "legitbot.sniper.fire.on.key" ) );
		LegitbotClasses[ 4 ]->FireKey = LoadInt( main, XorStr( "legitbot.sniper.fire.key" ) );
		LegitbotClasses[ 4 ]->TSD = LoadInt( main, XorStr( "legitbot.sniper.switch.delay" ) );
		LegitbotClasses[ 4 ]->ThroughSmoke = LoadBool( main, XorStr( "legitbot.sniper.through.smoke" ) );
		LegitbotClasses[ 4 ]->AutoWall = LoadBool( main, XorStr( "legitbot.sniper.auto.wall" ) );
		LegitbotClasses[ 4 ]->MinDamage = LoadInt( main, XorStr( "legitbot.sniper.min.damage" ) );

		for( int i = 0; i < 5; i++ )
			LegitbotClasses[ i ]->Clamp();

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
		Main->Triggerbot->ThroughSmoke	= LoadBool( main, XorStr( "triggerbot.through.smoke" ) );

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
		ESP->Dormant					= LoadBool( main, XorStr( "esp.dormant" ) );
		ESP->OutOfFOV					= LoadBool( main, XorStr( "esp.out.of.fov" ) );
		ESP->Snaplines					= LoadBool( main, XorStr( "esp.snaplines" ) );
		ESP->Distance					= LoadBool( main, XorStr( "esp.distance" ) );

		Render->ChamsMode = LoadInt(main, XorStr("render.chams.mode"));
		Render->ChamsOutlined = LoadBool(main, XorStr("render.chams.outlined"));
		Render->ChamsColored			= LoadBool( main, XorStr( "render.chams.visible" ) );
		Render->ChamsTarget				= LoadInt(main, XorStr("render.chams.target"));
		Render->ChamsVisOnly = LoadBool(main, XorStr("render.chams.visibleonly"));
		Render->ChamsAlpha				= LoadInt( main, XorStr( "render.chams.alpha" ) );

		AntiAim->AtTargetEnabled = LoadBool(main, XorStr("antiaim.at.target.enabled"));
		AntiAim->AtTarget				= LoadInt( main, XorStr( "antiaim.at.target" ) );
		AntiAim->PitchStand				= LoadInt( main, XorStr( "antiaim.pitch.stand" ) );
		AntiAim->YawStand					= LoadInt( main, XorStr( "antiaim.yaw.stand" ) );
		AntiAim->StandChokedPackets = LoadIntDef(main, XorStr("antiaim.choked.packets.stand"), 2);
		AntiAim->StandCustomAnglePitch = LoadFloat(main, XorStr("antiaim.custom.pitch.stand"));
		AntiAim->StandCustomAngleFakePitch = LoadFloat(main, XorStr("antiaim.custom.fakepitch.stand"));
		AntiAim->StandCustomAngleYaw = LoadFloat(main, XorStr("antiaim.custom.yaw.stand"));
		AntiAim->StandCustomAngleFakeYaw = LoadFloat(main, XorStr("antiaim.custom.fakeyaw.stand"));
		AntiAim->StandSpinSpeed = LoadInt(main, XorStr("antiaim.spinspeed.stand"));
		AntiAim->PitchMove = LoadInt(main, XorStr("antiaim.pitch.move"));
		AntiAim->YawMove = LoadInt(main, XorStr("antiaim.yaw.move"));
		AntiAim->MoveChokedPackets = LoadIntDef(main, XorStr("antiaim.choked.packets.move"), 2);
		AntiAim->MoveCustomAnglePitch = LoadFloat(main, XorStr("antiaim.custom.pitch.move"));
		AntiAim->MoveCustomAngleFakePitch = LoadFloat(main, XorStr("antiaim.custom.fakepitch.move"));
		AntiAim->MoveCustomAngleYaw = LoadFloat(main, XorStr("antiaim.custom.yaw.move"));
		AntiAim->MoveCustomAngleFakeYaw = LoadFloat(main, XorStr("antiaim.custom.fakeyaw.move"));
		AntiAim->MoveSpinSpeed = LoadInt(main, XorStr("antiaim.spinspeed.move"));
		AntiAim->NoEnemyEnabled = LoadBool(main, XorStr("antiaim.no.enemy.enabled"));
		AntiAim->NoEnemy = LoadInt(main, XorStr("antiaim.no.enemy"));
		AntiAim->OnKnife = LoadBool(main, XorStr("antiaim.on.knife"));
		AntiAim->HitReactive = LoadBool(main, XorStr("antiaim.hit.reactive"));
		AntiAim->BreakLC = LoadBool(main, XorStr("antiaim.break.lagcomp"));
		AntiAim->DefensiveTicks = LoadInt(main, XorStr("antiaim.defensive.ticks"));
		AntiAim->ManualLeftKey = LoadInt(main, XorStr("antiaim.manual.left"));
		AntiAim->ManualRightKey = LoadInt(main, XorStr("antiaim.manual.right"));
		AntiAim->ManualBackKey = LoadInt(main, XorStr("antiaim.manual.back"));
		AntiAim->FakeWalk = LoadBool(main, XorStr("antiaim.fakewalk"));
		AntiAim->FakeWalkKey = LoadInt(main, XorStr("antiaim.fakewalk.key"));
		// Новые поля читаются через *Def: в старых конфигах этих ключей нет,
		// и обычный LoadInt вернул бы 0, обнулив разумные значения по умолчанию.
		AntiAim->FakeWalkToggle = LoadBoolDef(main, XorStr("antiaim.fakewalk.toggle"), false);
		AntiAim->FakeWalkSpeed = LoadIntDef(main, XorStr("antiaim.fakewalk.speed"), 33);
		AntiAim->FakeDuck = LoadBoolDef(main, XorStr("antiaim.fakeduck"), false);
		AntiAim->FakeDuckKey = LoadIntDef(main, XorStr("antiaim.fakeduck.key"), 0);
		AntiAim->FakeDuckTicks = LoadIntDef(main, XorStr("antiaim.fakeduck.ticks"), 5);
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
		Misc->AutoPistolDelay		= LoadInt( main, XorStr( "misc.auto.pistol.delay" ) );
		Misc->AutoStrafe				= LoadInt( main, XorStr( "misc.auto.strafe" ) );
		Misc->BombWarning				= LoadBool( main, XorStr( "misc.bomb.warning" ) );
		Misc->AutoPeek					= LoadBool( main, XorStr( "misc.auto.peek" ) );
		Misc->AutoPeekKey				= LoadInt( main, XorStr( "misc.auto.peek.key" ) );
		Misc->Crosshair					= LoadInt( main, XorStr( "misc.crosshair" ) );
		Misc->Outlined					= LoadBool( main, XorStr( "misc.crosshair.outlined" ) );
		Misc->ShowRecoil				= LoadBool( main, XorStr( "misc.crosshair.show.recoil" ) );
		Misc->FakeLag					= LoadBool( main, XorStr( "misc.fakelag" ) );
		Misc->ChokedPackets				= LoadInt( main, XorStr( "misc.fakelag.choked.packets" ) );
		Misc->FakeLagMode				= LoadIntDef( main, XorStr( "misc.fakelag.mode" ), 0 );
		Misc->FakeLagMin				= LoadIntDef( main, XorStr( "misc.fakelag.min" ), 2 );
		Misc->FakeLagMax				= LoadIntDef( main, XorStr( "misc.fakelag.max" ), 8 );
		Misc->FakeLagOnGroundOnly		= LoadBoolDef( main, XorStr( "misc.fakelag.onground" ), false );
		Misc->FakeLagBreakOnShot		= LoadBoolDef( main, XorStr( "misc.fakelag.breakonshot" ), true );
		Misc->AirStuck					= LoadBool( main, XorStr( "misc.airstuck" ) );
		Misc->StuckKey					= LoadInt( main, XorStr( "misc.airstuck.key" ) );
		Misc->Speed						= LoadBool( main, XorStr( "misc.cstrafer" ) );
		Misc->SpeedKey = LoadInt(main, XorStr("misc.cstrafer.key"));
		Misc->SpeedMod = LoadFloat(main, XorStr("misc.cstrafer.modifer"));
		Misc->Restriction = LoadInt(main, XorStr("misc.restriction"));
		Misc->AntiSMAC = LoadBool(main, XorStr("misc.anti.smac"));
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

		if( Source::m_pMenu )
		{
			Source::m_pMenu->SetColors();
			Source::m_pMenu->ApplyColors();
		}

		std::string binds( XorStr( "binds" ) );

		// 45 = VK_INSERT, 122 = VK_F11, 123 = VK_F12.
		// Значения по умолчанию обязательны: старый/пустой конфиг без секции
		// [binds] выставлял клавишу меню в 0, и меню нельзя было открыть.
		Binds->Menu						= LoadIntDef( binds, XorStr( "menu" ), 45 );
		Binds->Eject					= LoadIntDef( binds, XorStr( "eject" ), 122 );
		Binds->Panic					= LoadIntDef( binds, XorStr( "panic" ), 123 );

		// Клавиша меню не может быть нулевой/битой — иначе чит неуправляем.
		if( Binds->Menu <= 0 || Binds->Menu > 255 )
		{
			LOG( XorStr( "[Config] Invalid menu key (%d) in config, forcing INSERT (45)." ), Binds->Menu );
			Binds->Menu = 45;
		}

		if( Binds->Eject <= 0 || Binds->Eject > 255 )
			Binds->Eject = 122;

		if( Binds->Panic <= 0 || Binds->Panic > 255 )
			Binds->Panic = 123;

		for( int i = 0; i < ARRAYSIZE( WeaponList ); i++ )
		{
			auto weapon = WeaponList[ i ];
			auto index = GetWeaponID( weapon );

			// Индекс приходит из таблицы имён, а указатели — из кучи: без
			// проверки битое имя оружия (или неудачная аллокация) давали
			// выход за границы массива/разыменование null прямо при загрузке
			// конфига — чит не запускался вовсе.
			if( index <= 0 || index >= WEAPON_MAX || !Weapon[ index ]
				|| !Weapon[ index ]->Aimbot || !Weapon[ index ]->Triggerbot )
			{
				LOG( XorStr( "[Config] Weapon '%s' has no config slot (id %d), skipped." ), weapon, index );
				continue;
			}

			// Слот оружия по умолчанию повторяет общий конфиг: сам по себе
			// чекбокс «Weapon Config» лишь разрешает переопределять настройки
			// для конкретного оружия. Раньше отсутствующие в файле ключи
			// читались как 0/false, поэтому включённый Weapon Config молча
			// выключал рейджбот (Mode = Off) для любого оружия, которое не
			// настраивали руками — «чит есть, а не стреляет».
			memcpy( Weapon[ index ]->Aimbot, Main->Aimbot, sizeof( AimbotList ) );
			memcpy( Weapon[ index ]->Triggerbot, Main->Triggerbot, sizeof( TriggerbotList ) );

			// Маркер «это оружие настраивали руками»: сборки до этой правки
			// сохраняли в файл нули для каждого оружия, поэтому отличить
			// «не настроено» от «намеренно выключено» было нельзя.
			const bool bConfigured = LoadIntDef( weapon, XorStr( "configured" ), 0 ) != 0;

			Weapon[ index ]->Aimbot->Mode				= LoadIntDef( weapon, XorStr( "aimbot.mode" ), Weapon[ index ]->Aimbot->Mode );
			Weapon[ index ]->Aimbot->Key				= LoadIntDef( weapon, XorStr( "aimbot.key" ), Weapon[ index ]->Aimbot->Key );
			Weapon[ index ]->Aimbot->AutoFire			= LoadBoolDef( weapon, XorStr( "aimbot.auto.fire" ), Weapon[ index ]->Aimbot->AutoFire );
			Weapon[ index ]->Aimbot->AutoStop			= LoadBoolDef( weapon, XorStr( "aimbot.auto.stop" ), Weapon[ index ]->Aimbot->AutoStop );
			Weapon[ index ]->Aimbot->AutoCrouch			= LoadBoolDef( weapon, XorStr( "aimbot.auto.crouch" ), Weapon[ index ]->Aimbot->AutoCrouch );
			Weapon[ index ]->Aimbot->AutoReload = LoadBoolDef( weapon, XorStr("aimbot.auto.reload"), Weapon[ index ]->Aimbot->AutoReload );
			Weapon[ index ]->Aimbot->AutoScope = LoadBoolDef( weapon, XorStr("aimbot.auto.scope"), Weapon[ index ]->Aimbot->AutoScope );
			Weapon[ index ]->Aimbot->MinDamageOverride = LoadIntDef( weapon, XorStr("aimbot.mindamage.override"), Weapon[ index ]->Aimbot->MinDamageOverride );
			Weapon[ index ]->Aimbot->MinDamageOverrideKey = LoadIntDef( weapon, XorStr("aimbot.mindamage.override.key"), Weapon[ index ]->Aimbot->MinDamageOverrideKey );
			Weapon[ index ]->Aimbot->AntiSpawnProtection = LoadBoolDef( weapon, XorStr("aimbot.anti.spawn.protection"), Weapon[ index ]->Aimbot->AntiSpawnProtection );
			Weapon[ index ]->Aimbot->NoSwitch			= LoadBoolDef( weapon, XorStr( "aimbot.no.switch" ), Weapon[ index ]->Aimbot->NoSwitch );
			Weapon[ index ]->Aimbot->Spot				= LoadIntDef( weapon, XorStr( "aimbot.spot" ), Weapon[ index ]->Aimbot->Spot );
			Weapon[ index ]->Aimbot->ForceBody	= LoadIntDef( weapon, XorStr( "aimbot.body.aim" ), Weapon[ index ]->Aimbot->ForceBody );
			Weapon[ index ]->Aimbot->SpotRandomize		= LoadBoolDef( weapon, XorStr( "aimbot.spot.randomize" ), Weapon[ index ]->Aimbot->SpotRandomize );
			Weapon[ index ]->Aimbot->TargetSelection	= LoadIntDef( weapon, XorStr( "aimbot.target.selection" ), Weapon[ index ]->Aimbot->TargetSelection );
			Weapon[ index ]->Aimbot->FieldOfView		= LoadFloatDef( weapon, XorStr( "aimbot.fov" ), Weapon[ index ]->Aimbot->FieldOfView );
			Weapon[ index ]->Aimbot->Smooth				= LoadIntDef( weapon, XorStr( "aimbot.smooth" ), Weapon[ index ]->Aimbot->Smooth );
			Weapon[ index ]->Aimbot->StepX				= LoadFloatDef( weapon, XorStr( "aimbot.step.vertical" ), Weapon[ index ]->Aimbot->StepX );
			Weapon[ index ]->Aimbot->StepY				= LoadFloatDef( weapon, XorStr( "aimbot.step.horizontal" ), Weapon[ index ]->Aimbot->StepY );
			Weapon[ index ]->Aimbot->SmoothX			= LoadFloatDef( weapon, XorStr( "aimbot.smooth.vertical" ), Weapon[ index ]->Aimbot->SmoothX );
			Weapon[ index ]->Aimbot->SmoothY			= LoadFloatDef( weapon, XorStr( "aimbot.smooth.horizontal" ), Weapon[ index ]->Aimbot->SmoothY );
			Weapon[ index ]->Aimbot->Duration			= LoadIntDef( weapon, XorStr( "aimbot.duration" ), Weapon[ index ]->Aimbot->Duration );
			Weapon[ index ]->Aimbot->Delay				= LoadIntDef( weapon, XorStr( "aimbot.delay" ), Weapon[ index ]->Aimbot->Delay );
			Weapon[ index ]->Aimbot->SwitchDelay		= LoadIntDef( weapon, XorStr( "aimbot.switch.delay" ), Weapon[ index ]->Aimbot->SwitchDelay );
			Weapon[ index ]->Aimbot->RCS				= LoadBoolDef( weapon, XorStr( "aimbot.rcs" ), Weapon[ index ]->Aimbot->RCS );
			Weapon[ index ]->Aimbot->RCSDelay			= LoadIntDef( weapon, XorStr( "aimbot.rcs.delay" ), Weapon[ index ]->Aimbot->RCSDelay );
			Weapon[ index ]->Aimbot->RCSAmountX			= LoadIntDef( weapon, XorStr( "aimbot.rcs.amount.vertical" ), Weapon[ index ]->Aimbot->RCSAmountX );
			Weapon[ index ]->Aimbot->RCSAmountY			= LoadIntDef( weapon, XorStr( "aimbot.rcs.amount.horizontal" ), Weapon[ index ]->Aimbot->RCSAmountY );
			Weapon[ index ]->Aimbot->AutoWall			= LoadBoolDef( weapon, XorStr( "aimbot.autowall" ), Weapon[ index ]->Aimbot->AutoWall );
			Weapon[ index ]->Aimbot->MinDamage			= LoadIntDef( weapon, XorStr( "aimbot.min.damage" ), Weapon[ index ]->Aimbot->MinDamage );
			Weapon[ index ]->Aimbot->HitScan			= LoadIntDef( weapon, XorStr( "aimbot.hitscan" ), Weapon[ index ]->Aimbot->HitScan );
			Weapon[ index ]->Aimbot->HitScanScale		= LoadFloatDef( weapon, XorStr( "aimbot.hitscan.scale" ), Weapon[ index ]->Aimbot->HitScanScale );
			Weapon[ index ]->Aimbot->Target				= LoadIntDef( weapon, XorStr( "aimbot.target" ), Weapon[ index ]->Aimbot->Target );
			Weapon[ index ]->Aimbot->Silent				= LoadBoolDef( weapon, XorStr( "aimbot.silent" ), Weapon[ index ]->Aimbot->Silent );
			Weapon[ index ]->Aimbot->NoSpreadActive		= LoadBoolDef( weapon, XorStr("aimbot.no.spread.active"), Weapon[ index ]->Aimbot->NoSpreadActive );
			Weapon[ index ]->Aimbot->NoSpread			= LoadIntDef( weapon, XorStr("aimbot.no.spread"), Weapon[ index ]->Aimbot->NoSpread );
		//	Weapon[index]->Aimbot->SeedHelp = LoadBool(weapon, XorStr("aimbot.seed.help"));
			Weapon[ index ]->Aimbot->Height = LoadBoolDef( weapon, XorStr("aimbot.height"), Weapon[ index ]->Aimbot->Height );
			Weapon[ index ]->Aimbot->HeightScale = LoadFloatDef( weapon, XorStr("aimbot.height.scale"), Weapon[ index ]->Aimbot->HeightScale );
			Weapon[ index ]->Aimbot->LagCompensation = LoadIntDef( weapon, XorStr("aimbot.adjustment"), Weapon[ index ]->Aimbot->LagCompensation );

			Weapon[ index ]->Aimbot->Clamp();

			Weapon[ index ]->Triggerbot->Mode			= LoadIntDef( weapon, XorStr( "triggerbot.mode" ), Weapon[ index ]->Triggerbot->Mode );
			Weapon[ index ]->Triggerbot->Key			= LoadIntDef( weapon, XorStr( "triggerbot.key" ), Weapon[ index ]->Triggerbot->Key );
			Weapon[ index ]->Triggerbot->Accuracy		= LoadIntDef( weapon, XorStr( "triggerbot.accuracy" ), Weapon[ index ]->Triggerbot->Accuracy );
			Weapon[ index ]->Triggerbot->Delay			= LoadIntDef( weapon, XorStr( "triggerbot.delay" ), Weapon[ index ]->Triggerbot->Delay );
			Weapon[ index ]->Triggerbot->Burst			= LoadIntDef( weapon, XorStr( "triggerbot.burst" ), Weapon[ index ]->Triggerbot->Burst );
			Weapon[ index ]->Triggerbot->Head			= LoadBoolDef( weapon, XorStr( "triggerbot.head" ), Weapon[ index ]->Triggerbot->Head );
			Weapon[ index ]->Triggerbot->Chest			= LoadBoolDef( weapon, XorStr( "triggerbot.chest" ), Weapon[ index ]->Triggerbot->Chest );
			Weapon[ index ]->Triggerbot->Stomach		= LoadBoolDef( weapon, XorStr( "triggerbot.stomach" ), Weapon[ index ]->Triggerbot->Stomach );
			Weapon[ index ]->Triggerbot->Arms			= LoadBoolDef( weapon, XorStr( "triggerbot.arms" ), Weapon[ index ]->Triggerbot->Arms );
			Weapon[ index ]->Triggerbot->Legs			= LoadBoolDef( weapon, XorStr( "triggerbot.legs" ), Weapon[ index ]->Triggerbot->Legs );
			Weapon[ index ]->Triggerbot->AutoWall		= LoadBoolDef( weapon, XorStr( "triggerbot.autowall" ), Weapon[ index ]->Triggerbot->AutoWall );
			Weapon[ index ]->Triggerbot->MinDamage		= LoadIntDef( weapon, XorStr( "triggerbot.min.damage" ), Weapon[ index ]->Triggerbot->MinDamage );
			Weapon[ index ]->Triggerbot->Target			= LoadIntDef( weapon, XorStr( "triggerbot.target" ), Weapon[ index ]->Triggerbot->Target );
			Weapon[ index ]->Triggerbot->ThroughSmoke	= LoadBoolDef( weapon, XorStr( "triggerbot.through.smoke" ), Weapon[ index ]->Triggerbot->ThroughSmoke );

			Weapon[ index ]->Triggerbot->Clamp();

			// Рейджбот «молча не стрелял» при включённом Weapon Config: в
			// старом конфиге у каждого оружия лежал Mode = Off, и именно он
			// использовался в бою, хотя в меню настраивался общий конфиг.
			// Если это оружие никогда не настраивали (нет маркера), а общий
			// режим включён — слот повторяет общий конфиг.
			if( !bConfigured )
			{
				if( Weapon[ index ]->Aimbot->Mode == 0 && Main->Aimbot->Mode != 0 )
				{
					memcpy( Weapon[ index ]->Aimbot, Main->Aimbot, sizeof( AimbotList ) );

					LOG( XorStr( "[Config] Weapon '%s' was never configured, global aimbot config is used." ), weapon );
				}

				if( Weapon[ index ]->Triggerbot->Mode == 0 && Main->Triggerbot->Mode != 0 )
					memcpy( Weapon[ index ]->Triggerbot, Main->Triggerbot, sizeof( TriggerbotList ) );

				Weapon[ index ]->Aimbot->Clamp();
				Weapon[ index ]->Triggerbot->Clamp();
			}
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
		SaveBool(main, XorStr("aimbot.auto.scope"), Main->Aimbot->AutoScope);
		SaveInt(main, XorStr("aimbot.mindamage.override"), Main->Aimbot->MinDamageOverride);
		SaveInt(main, XorStr("aimbot.mindamage.override.key"), Main->Aimbot->MinDamageOverrideKey);
		SaveBool(main, XorStr("aimbot.anti.spawn.protection"), Main->Aimbot->AntiSpawnProtection);
		SaveBool( main, XorStr( "aimbot.no.switch" ), Main->Aimbot->NoSwitch );
		SaveInt( main, XorStr( "aimbot.spot" ), Main->Aimbot->Spot );
		SaveInt( main, XorStr( "aimbot.body.aim" ), Main->Aimbot->ForceBody );
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

		for( int i = 0; i < 5; i++ )
			LegitbotClasses[ i ]->Clamp();

		// Legit class: pistol
		SaveInt( main, XorStr( "legitbot.pistol.mode" ), LegitbotClasses[ 0 ]->Mode );
		SaveInt( main, XorStr( "legitbot.pistol.key" ), LegitbotClasses[ 0 ]->Key );
		SaveInt( main, XorStr( "legitbot.pistol.target.selection" ), LegitbotClasses[ 0 ]->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.pistol.fov" ), LegitbotClasses[ 0 ]->FieldOfView );
		SaveInt( main, XorStr( "legitbot.pistol.smooth" ), LegitbotClasses[ 0 ]->Smooth );
		SaveFloat( main, XorStr( "legitbot.pistol.step.vertical" ), LegitbotClasses[ 0 ]->StepX );
		SaveFloat( main, XorStr( "legitbot.pistol.step.horizontal" ), LegitbotClasses[ 0 ]->StepY );
		SaveFloat( main, XorStr( "legitbot.pistol.smooth.vertical" ), LegitbotClasses[ 0 ]->SmoothX );
		SaveFloat( main, XorStr( "legitbot.pistol.smooth.horizontal" ), LegitbotClasses[ 0 ]->SmoothY );
		SaveInt( main, XorStr( "legitbot.pistol.delay" ), LegitbotClasses[ 0 ]->Delay );
		SaveInt( main, XorStr( "legitbot.pistol.duration" ), LegitbotClasses[ 0 ]->Duration );
		SaveBool( main, XorStr( "legitbot.pistol.rcs" ), LegitbotClasses[ 0 ]->RCS );
		SaveInt( main, XorStr( "legitbot.pistol.rcs.delay" ), LegitbotClasses[ 0 ]->RCSDelay );
		SaveInt( main, XorStr( "legitbot.pistol.rcs.amount.vertical" ), LegitbotClasses[ 0 ]->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.pistol.rcs.amount.horizontal" ), LegitbotClasses[ 0 ]->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.pistol.target" ), LegitbotClasses[ 0 ]->Target );
		SaveBool( main, XorStr( "legitbot.pistol.auto.fire" ), LegitbotClasses[ 0 ]->AutoFire );
		SaveBool( main, XorStr( "legitbot.pistol.auto.stop" ), LegitbotClasses[ 0 ]->AutoStop );
		SaveBool( main, XorStr( "legitbot.pistol.rcs.standalone" ), LegitbotClasses[ 0 ]->RCSStandalone );
		SaveBool( main, XorStr( "legitbot.pistol.flash.check" ), LegitbotClasses[ 0 ]->FlashCheck );
		SaveBool( main, XorStr( "legitbot.pistol.backtrack" ), LegitbotClasses[ 0 ]->Backtrack );
		SaveBool( main, XorStr( "legitbot.pistol.delay.humanize" ), LegitbotClasses[ 0 ]->HumanizeDelay );
		SaveBool( main, XorStr( "legitbot.pistol.auto.scope" ), LegitbotClasses[ 0 ]->AutoScope );
		SaveFloat( main, XorStr( "legitbot.pistol.randomize" ), LegitbotClasses[ 0 ]->Randomize );
		SaveFloat( main, XorStr( "legitbot.pistol.curve" ), LegitbotClasses[ 0 ]->Curve );
		SaveBool( main, XorStr( "legitbot.pistol.zone.head" ), LegitbotClasses[ 0 ]->ZoneHead );
		SaveBool( main, XorStr( "legitbot.pistol.zone.chest" ), LegitbotClasses[ 0 ]->ZoneChest );
		SaveBool( main, XorStr( "legitbot.pistol.zone.stomach" ), LegitbotClasses[ 0 ]->ZoneStomach );
		SaveBool( main, XorStr( "legitbot.pistol.zone.arms" ), LegitbotClasses[ 0 ]->ZoneArms );
		SaveBool( main, XorStr( "legitbot.pistol.zone.legs" ), LegitbotClasses[ 0 ]->ZoneLegs );
		SaveInt( main, XorStr( "legitbot.pistol.hitbox.priority" ), LegitbotClasses[ 0 ]->HitboxPriority );
		SaveInt( main, XorStr( "legitbot.pistol.hitbox.selection" ), LegitbotClasses[ 0 ]->HitboxSelection );
		SaveInt( main, XorStr( "legitbot.pistol.toggle.key" ), LegitbotClasses[ 0 ]->ToggleKey );
		SaveBool( main, XorStr( "legitbot.pistol.fire.on.key" ), LegitbotClasses[ 0 ]->FireOnKey );
		SaveInt( main, XorStr( "legitbot.pistol.fire.key" ), LegitbotClasses[ 0 ]->FireKey );
		SaveInt( main, XorStr( "legitbot.pistol.switch.delay" ), LegitbotClasses[ 0 ]->TSD );
		SaveBool( main, XorStr( "legitbot.pistol.through.smoke" ), LegitbotClasses[ 0 ]->ThroughSmoke );
		SaveBool( main, XorStr( "legitbot.pistol.auto.wall" ), LegitbotClasses[ 0 ]->AutoWall );
		SaveInt( main, XorStr( "legitbot.pistol.min.damage" ), LegitbotClasses[ 0 ]->MinDamage );

		// Legit class: smg
		SaveInt( main, XorStr( "legitbot.smg.mode" ), LegitbotClasses[ 1 ]->Mode );
		SaveInt( main, XorStr( "legitbot.smg.key" ), LegitbotClasses[ 1 ]->Key );
		SaveInt( main, XorStr( "legitbot.smg.target.selection" ), LegitbotClasses[ 1 ]->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.smg.fov" ), LegitbotClasses[ 1 ]->FieldOfView );
		SaveInt( main, XorStr( "legitbot.smg.smooth" ), LegitbotClasses[ 1 ]->Smooth );
		SaveFloat( main, XorStr( "legitbot.smg.step.vertical" ), LegitbotClasses[ 1 ]->StepX );
		SaveFloat( main, XorStr( "legitbot.smg.step.horizontal" ), LegitbotClasses[ 1 ]->StepY );
		SaveFloat( main, XorStr( "legitbot.smg.smooth.vertical" ), LegitbotClasses[ 1 ]->SmoothX );
		SaveFloat( main, XorStr( "legitbot.smg.smooth.horizontal" ), LegitbotClasses[ 1 ]->SmoothY );
		SaveInt( main, XorStr( "legitbot.smg.delay" ), LegitbotClasses[ 1 ]->Delay );
		SaveInt( main, XorStr( "legitbot.smg.duration" ), LegitbotClasses[ 1 ]->Duration );
		SaveBool( main, XorStr( "legitbot.smg.rcs" ), LegitbotClasses[ 1 ]->RCS );
		SaveInt( main, XorStr( "legitbot.smg.rcs.delay" ), LegitbotClasses[ 1 ]->RCSDelay );
		SaveInt( main, XorStr( "legitbot.smg.rcs.amount.vertical" ), LegitbotClasses[ 1 ]->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.smg.rcs.amount.horizontal" ), LegitbotClasses[ 1 ]->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.smg.target" ), LegitbotClasses[ 1 ]->Target );
		SaveBool( main, XorStr( "legitbot.smg.auto.fire" ), LegitbotClasses[ 1 ]->AutoFire );
		SaveBool( main, XorStr( "legitbot.smg.auto.stop" ), LegitbotClasses[ 1 ]->AutoStop );
		SaveBool( main, XorStr( "legitbot.smg.rcs.standalone" ), LegitbotClasses[ 1 ]->RCSStandalone );
		SaveBool( main, XorStr( "legitbot.smg.flash.check" ), LegitbotClasses[ 1 ]->FlashCheck );
		SaveBool( main, XorStr( "legitbot.smg.backtrack" ), LegitbotClasses[ 1 ]->Backtrack );
		SaveBool( main, XorStr( "legitbot.smg.delay.humanize" ), LegitbotClasses[ 1 ]->HumanizeDelay );
		SaveBool( main, XorStr( "legitbot.smg.auto.scope" ), LegitbotClasses[ 1 ]->AutoScope );
		SaveFloat( main, XorStr( "legitbot.smg.randomize" ), LegitbotClasses[ 1 ]->Randomize );
		SaveFloat( main, XorStr( "legitbot.smg.curve" ), LegitbotClasses[ 1 ]->Curve );
		SaveBool( main, XorStr( "legitbot.smg.zone.head" ), LegitbotClasses[ 1 ]->ZoneHead );
		SaveBool( main, XorStr( "legitbot.smg.zone.chest" ), LegitbotClasses[ 1 ]->ZoneChest );
		SaveBool( main, XorStr( "legitbot.smg.zone.stomach" ), LegitbotClasses[ 1 ]->ZoneStomach );
		SaveBool( main, XorStr( "legitbot.smg.zone.arms" ), LegitbotClasses[ 1 ]->ZoneArms );
		SaveBool( main, XorStr( "legitbot.smg.zone.legs" ), LegitbotClasses[ 1 ]->ZoneLegs );
		SaveInt( main, XorStr( "legitbot.smg.hitbox.priority" ), LegitbotClasses[ 1 ]->HitboxPriority );
		SaveInt( main, XorStr( "legitbot.smg.hitbox.selection" ), LegitbotClasses[ 1 ]->HitboxSelection );
		SaveInt( main, XorStr( "legitbot.smg.toggle.key" ), LegitbotClasses[ 1 ]->ToggleKey );
		SaveBool( main, XorStr( "legitbot.smg.fire.on.key" ), LegitbotClasses[ 1 ]->FireOnKey );
		SaveInt( main, XorStr( "legitbot.smg.fire.key" ), LegitbotClasses[ 1 ]->FireKey );
		SaveInt( main, XorStr( "legitbot.smg.switch.delay" ), LegitbotClasses[ 1 ]->TSD );
		SaveBool( main, XorStr( "legitbot.smg.through.smoke" ), LegitbotClasses[ 1 ]->ThroughSmoke );
		SaveBool( main, XorStr( "legitbot.smg.auto.wall" ), LegitbotClasses[ 1 ]->AutoWall );
		SaveInt( main, XorStr( "legitbot.smg.min.damage" ), LegitbotClasses[ 1 ]->MinDamage );

		// Legit class: rifle
		SaveInt( main, XorStr( "legitbot.rifle.mode" ), LegitbotClasses[ 2 ]->Mode );
		SaveInt( main, XorStr( "legitbot.rifle.key" ), LegitbotClasses[ 2 ]->Key );
		SaveInt( main, XorStr( "legitbot.rifle.target.selection" ), LegitbotClasses[ 2 ]->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.rifle.fov" ), LegitbotClasses[ 2 ]->FieldOfView );
		SaveInt( main, XorStr( "legitbot.rifle.smooth" ), LegitbotClasses[ 2 ]->Smooth );
		SaveFloat( main, XorStr( "legitbot.rifle.step.vertical" ), LegitbotClasses[ 2 ]->StepX );
		SaveFloat( main, XorStr( "legitbot.rifle.step.horizontal" ), LegitbotClasses[ 2 ]->StepY );
		SaveFloat( main, XorStr( "legitbot.rifle.smooth.vertical" ), LegitbotClasses[ 2 ]->SmoothX );
		SaveFloat( main, XorStr( "legitbot.rifle.smooth.horizontal" ), LegitbotClasses[ 2 ]->SmoothY );
		SaveInt( main, XorStr( "legitbot.rifle.delay" ), LegitbotClasses[ 2 ]->Delay );
		SaveInt( main, XorStr( "legitbot.rifle.duration" ), LegitbotClasses[ 2 ]->Duration );
		SaveBool( main, XorStr( "legitbot.rifle.rcs" ), LegitbotClasses[ 2 ]->RCS );
		SaveInt( main, XorStr( "legitbot.rifle.rcs.delay" ), LegitbotClasses[ 2 ]->RCSDelay );
		SaveInt( main, XorStr( "legitbot.rifle.rcs.amount.vertical" ), LegitbotClasses[ 2 ]->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.rifle.rcs.amount.horizontal" ), LegitbotClasses[ 2 ]->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.rifle.target" ), LegitbotClasses[ 2 ]->Target );
		SaveBool( main, XorStr( "legitbot.rifle.auto.fire" ), LegitbotClasses[ 2 ]->AutoFire );
		SaveBool( main, XorStr( "legitbot.rifle.auto.stop" ), LegitbotClasses[ 2 ]->AutoStop );
		SaveBool( main, XorStr( "legitbot.rifle.rcs.standalone" ), LegitbotClasses[ 2 ]->RCSStandalone );
		SaveBool( main, XorStr( "legitbot.rifle.flash.check" ), LegitbotClasses[ 2 ]->FlashCheck );
		SaveBool( main, XorStr( "legitbot.rifle.backtrack" ), LegitbotClasses[ 2 ]->Backtrack );
		SaveBool( main, XorStr( "legitbot.rifle.delay.humanize" ), LegitbotClasses[ 2 ]->HumanizeDelay );
		SaveBool( main, XorStr( "legitbot.rifle.auto.scope" ), LegitbotClasses[ 2 ]->AutoScope );
		SaveFloat( main, XorStr( "legitbot.rifle.randomize" ), LegitbotClasses[ 2 ]->Randomize );
		SaveFloat( main, XorStr( "legitbot.rifle.curve" ), LegitbotClasses[ 2 ]->Curve );
		SaveBool( main, XorStr( "legitbot.rifle.zone.head" ), LegitbotClasses[ 2 ]->ZoneHead );
		SaveBool( main, XorStr( "legitbot.rifle.zone.chest" ), LegitbotClasses[ 2 ]->ZoneChest );
		SaveBool( main, XorStr( "legitbot.rifle.zone.stomach" ), LegitbotClasses[ 2 ]->ZoneStomach );
		SaveBool( main, XorStr( "legitbot.rifle.zone.arms" ), LegitbotClasses[ 2 ]->ZoneArms );
		SaveBool( main, XorStr( "legitbot.rifle.zone.legs" ), LegitbotClasses[ 2 ]->ZoneLegs );
		SaveInt( main, XorStr( "legitbot.rifle.hitbox.priority" ), LegitbotClasses[ 2 ]->HitboxPriority );
		SaveInt( main, XorStr( "legitbot.rifle.hitbox.selection" ), LegitbotClasses[ 2 ]->HitboxSelection );
		SaveInt( main, XorStr( "legitbot.rifle.toggle.key" ), LegitbotClasses[ 2 ]->ToggleKey );
		SaveBool( main, XorStr( "legitbot.rifle.fire.on.key" ), LegitbotClasses[ 2 ]->FireOnKey );
		SaveInt( main, XorStr( "legitbot.rifle.fire.key" ), LegitbotClasses[ 2 ]->FireKey );
		SaveInt( main, XorStr( "legitbot.rifle.switch.delay" ), LegitbotClasses[ 2 ]->TSD );
		SaveBool( main, XorStr( "legitbot.rifle.through.smoke" ), LegitbotClasses[ 2 ]->ThroughSmoke );
		SaveBool( main, XorStr( "legitbot.rifle.auto.wall" ), LegitbotClasses[ 2 ]->AutoWall );
		SaveInt( main, XorStr( "legitbot.rifle.min.damage" ), LegitbotClasses[ 2 ]->MinDamage );

		// Legit class: shotgun
		SaveInt( main, XorStr( "legitbot.shotgun.mode" ), LegitbotClasses[ 3 ]->Mode );
		SaveInt( main, XorStr( "legitbot.shotgun.key" ), LegitbotClasses[ 3 ]->Key );
		SaveInt( main, XorStr( "legitbot.shotgun.target.selection" ), LegitbotClasses[ 3 ]->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.shotgun.fov" ), LegitbotClasses[ 3 ]->FieldOfView );
		SaveInt( main, XorStr( "legitbot.shotgun.smooth" ), LegitbotClasses[ 3 ]->Smooth );
		SaveFloat( main, XorStr( "legitbot.shotgun.step.vertical" ), LegitbotClasses[ 3 ]->StepX );
		SaveFloat( main, XorStr( "legitbot.shotgun.step.horizontal" ), LegitbotClasses[ 3 ]->StepY );
		SaveFloat( main, XorStr( "legitbot.shotgun.smooth.vertical" ), LegitbotClasses[ 3 ]->SmoothX );
		SaveFloat( main, XorStr( "legitbot.shotgun.smooth.horizontal" ), LegitbotClasses[ 3 ]->SmoothY );
		SaveInt( main, XorStr( "legitbot.shotgun.delay" ), LegitbotClasses[ 3 ]->Delay );
		SaveInt( main, XorStr( "legitbot.shotgun.duration" ), LegitbotClasses[ 3 ]->Duration );
		SaveBool( main, XorStr( "legitbot.shotgun.rcs" ), LegitbotClasses[ 3 ]->RCS );
		SaveInt( main, XorStr( "legitbot.shotgun.rcs.delay" ), LegitbotClasses[ 3 ]->RCSDelay );
		SaveInt( main, XorStr( "legitbot.shotgun.rcs.amount.vertical" ), LegitbotClasses[ 3 ]->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.shotgun.rcs.amount.horizontal" ), LegitbotClasses[ 3 ]->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.shotgun.target" ), LegitbotClasses[ 3 ]->Target );
		SaveBool( main, XorStr( "legitbot.shotgun.auto.fire" ), LegitbotClasses[ 3 ]->AutoFire );
		SaveBool( main, XorStr( "legitbot.shotgun.auto.stop" ), LegitbotClasses[ 3 ]->AutoStop );
		SaveBool( main, XorStr( "legitbot.shotgun.rcs.standalone" ), LegitbotClasses[ 3 ]->RCSStandalone );
		SaveBool( main, XorStr( "legitbot.shotgun.flash.check" ), LegitbotClasses[ 3 ]->FlashCheck );
		SaveBool( main, XorStr( "legitbot.shotgun.backtrack" ), LegitbotClasses[ 3 ]->Backtrack );
		SaveBool( main, XorStr( "legitbot.shotgun.delay.humanize" ), LegitbotClasses[ 3 ]->HumanizeDelay );
		SaveBool( main, XorStr( "legitbot.shotgun.auto.scope" ), LegitbotClasses[ 3 ]->AutoScope );
		SaveFloat( main, XorStr( "legitbot.shotgun.randomize" ), LegitbotClasses[ 3 ]->Randomize );
		SaveFloat( main, XorStr( "legitbot.shotgun.curve" ), LegitbotClasses[ 3 ]->Curve );
		SaveBool( main, XorStr( "legitbot.shotgun.zone.head" ), LegitbotClasses[ 3 ]->ZoneHead );
		SaveBool( main, XorStr( "legitbot.shotgun.zone.chest" ), LegitbotClasses[ 3 ]->ZoneChest );
		SaveBool( main, XorStr( "legitbot.shotgun.zone.stomach" ), LegitbotClasses[ 3 ]->ZoneStomach );
		SaveBool( main, XorStr( "legitbot.shotgun.zone.arms" ), LegitbotClasses[ 3 ]->ZoneArms );
		SaveBool( main, XorStr( "legitbot.shotgun.zone.legs" ), LegitbotClasses[ 3 ]->ZoneLegs );
		SaveInt( main, XorStr( "legitbot.shotgun.hitbox.priority" ), LegitbotClasses[ 3 ]->HitboxPriority );
		SaveInt( main, XorStr( "legitbot.shotgun.hitbox.selection" ), LegitbotClasses[ 3 ]->HitboxSelection );
		SaveInt( main, XorStr( "legitbot.shotgun.toggle.key" ), LegitbotClasses[ 3 ]->ToggleKey );
		SaveBool( main, XorStr( "legitbot.shotgun.fire.on.key" ), LegitbotClasses[ 3 ]->FireOnKey );
		SaveInt( main, XorStr( "legitbot.shotgun.fire.key" ), LegitbotClasses[ 3 ]->FireKey );
		SaveInt( main, XorStr( "legitbot.shotgun.switch.delay" ), LegitbotClasses[ 3 ]->TSD );
		SaveBool( main, XorStr( "legitbot.shotgun.through.smoke" ), LegitbotClasses[ 3 ]->ThroughSmoke );
		SaveBool( main, XorStr( "legitbot.shotgun.auto.wall" ), LegitbotClasses[ 3 ]->AutoWall );
		SaveInt( main, XorStr( "legitbot.shotgun.min.damage" ), LegitbotClasses[ 3 ]->MinDamage );

		// Legit class: sniper
		SaveInt( main, XorStr( "legitbot.sniper.mode" ), LegitbotClasses[ 4 ]->Mode );
		SaveInt( main, XorStr( "legitbot.sniper.key" ), LegitbotClasses[ 4 ]->Key );
		SaveInt( main, XorStr( "legitbot.sniper.target.selection" ), LegitbotClasses[ 4 ]->TargetSelection );
		SaveFloat( main, XorStr( "legitbot.sniper.fov" ), LegitbotClasses[ 4 ]->FieldOfView );
		SaveInt( main, XorStr( "legitbot.sniper.smooth" ), LegitbotClasses[ 4 ]->Smooth );
		SaveFloat( main, XorStr( "legitbot.sniper.step.vertical" ), LegitbotClasses[ 4 ]->StepX );
		SaveFloat( main, XorStr( "legitbot.sniper.step.horizontal" ), LegitbotClasses[ 4 ]->StepY );
		SaveFloat( main, XorStr( "legitbot.sniper.smooth.vertical" ), LegitbotClasses[ 4 ]->SmoothX );
		SaveFloat( main, XorStr( "legitbot.sniper.smooth.horizontal" ), LegitbotClasses[ 4 ]->SmoothY );
		SaveInt( main, XorStr( "legitbot.sniper.delay" ), LegitbotClasses[ 4 ]->Delay );
		SaveInt( main, XorStr( "legitbot.sniper.duration" ), LegitbotClasses[ 4 ]->Duration );
		SaveBool( main, XorStr( "legitbot.sniper.rcs" ), LegitbotClasses[ 4 ]->RCS );
		SaveInt( main, XorStr( "legitbot.sniper.rcs.delay" ), LegitbotClasses[ 4 ]->RCSDelay );
		SaveInt( main, XorStr( "legitbot.sniper.rcs.amount.vertical" ), LegitbotClasses[ 4 ]->RCSAmountX );
		SaveInt( main, XorStr( "legitbot.sniper.rcs.amount.horizontal" ), LegitbotClasses[ 4 ]->RCSAmountY );
		SaveInt( main, XorStr( "legitbot.sniper.target" ), LegitbotClasses[ 4 ]->Target );
		SaveBool( main, XorStr( "legitbot.sniper.auto.fire" ), LegitbotClasses[ 4 ]->AutoFire );
		SaveBool( main, XorStr( "legitbot.sniper.auto.stop" ), LegitbotClasses[ 4 ]->AutoStop );
		SaveBool( main, XorStr( "legitbot.sniper.rcs.standalone" ), LegitbotClasses[ 4 ]->RCSStandalone );
		SaveBool( main, XorStr( "legitbot.sniper.flash.check" ), LegitbotClasses[ 4 ]->FlashCheck );
		SaveBool( main, XorStr( "legitbot.sniper.backtrack" ), LegitbotClasses[ 4 ]->Backtrack );
		SaveBool( main, XorStr( "legitbot.sniper.delay.humanize" ), LegitbotClasses[ 4 ]->HumanizeDelay );
		SaveBool( main, XorStr( "legitbot.sniper.auto.scope" ), LegitbotClasses[ 4 ]->AutoScope );
		SaveFloat( main, XorStr( "legitbot.sniper.randomize" ), LegitbotClasses[ 4 ]->Randomize );
		SaveFloat( main, XorStr( "legitbot.sniper.curve" ), LegitbotClasses[ 4 ]->Curve );
		SaveBool( main, XorStr( "legitbot.sniper.zone.head" ), LegitbotClasses[ 4 ]->ZoneHead );
		SaveBool( main, XorStr( "legitbot.sniper.zone.chest" ), LegitbotClasses[ 4 ]->ZoneChest );
		SaveBool( main, XorStr( "legitbot.sniper.zone.stomach" ), LegitbotClasses[ 4 ]->ZoneStomach );
		SaveBool( main, XorStr( "legitbot.sniper.zone.arms" ), LegitbotClasses[ 4 ]->ZoneArms );
		SaveBool( main, XorStr( "legitbot.sniper.zone.legs" ), LegitbotClasses[ 4 ]->ZoneLegs );
		SaveInt( main, XorStr( "legitbot.sniper.hitbox.priority" ), LegitbotClasses[ 4 ]->HitboxPriority );
		SaveInt( main, XorStr( "legitbot.sniper.hitbox.selection" ), LegitbotClasses[ 4 ]->HitboxSelection );
		SaveInt( main, XorStr( "legitbot.sniper.toggle.key" ), LegitbotClasses[ 4 ]->ToggleKey );
		SaveBool( main, XorStr( "legitbot.sniper.fire.on.key" ), LegitbotClasses[ 4 ]->FireOnKey );
		SaveInt( main, XorStr( "legitbot.sniper.fire.key" ), LegitbotClasses[ 4 ]->FireKey );
		SaveInt( main, XorStr( "legitbot.sniper.switch.delay" ), LegitbotClasses[ 4 ]->TSD );
		SaveBool( main, XorStr( "legitbot.sniper.through.smoke" ), LegitbotClasses[ 4 ]->ThroughSmoke );
		SaveBool( main, XorStr( "legitbot.sniper.auto.wall" ), LegitbotClasses[ 4 ]->AutoWall );
		SaveInt( main, XorStr( "legitbot.sniper.min.damage" ), LegitbotClasses[ 4 ]->MinDamage );

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
		SaveBool( main, XorStr( "triggerbot.through.smoke" ), Main->Triggerbot->ThroughSmoke );

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
		SaveBool( main, XorStr( "esp.dormant" ), ESP->Dormant );
		SaveBool( main, XorStr( "esp.out.of.fov" ), ESP->OutOfFOV );
		SaveBool( main, XorStr( "esp.snaplines" ), ESP->Snaplines );
		SaveBool( main, XorStr( "esp.distance" ), ESP->Distance );

		SaveInt(main, XorStr("render.chams.mode"), Render->ChamsMode);
		SaveBool(main, XorStr("render.chams.visible"), Render->ChamsColored);
		SaveBool(main, XorStr("render.chams.outlined"), Render->ChamsOutlined);
		SaveInt(main, XorStr("render.chams.target"), Render->ChamsTarget);
		SaveBool(main, XorStr("render.chams.visibleonly"), Render->ChamsVisOnly);
		SaveInt( main, XorStr( "render.chams.alpha" ), Render->ChamsAlpha );

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
		SaveInt(main, XorStr("antiaim.spinspeed.stand"), AntiAim->StandSpinSpeed);
		SaveInt(main, XorStr("antiaim.pitch.move"), AntiAim->PitchMove);
		SaveInt(main, XorStr("antiaim.yaw.move"), AntiAim->YawMove);
		SaveInt(main, XorStr("antiaim.choked.packets.move"), AntiAim->MoveChokedPackets);
		SaveFloat(main, XorStr("antiaim.custom.pitch.move"), AntiAim->MoveCustomAnglePitch);
		SaveFloat(main, XorStr("antiaim.custom.fakepitch.move"), AntiAim->MoveCustomAngleFakePitch);
		SaveFloat(main, XorStr("antiaim.custom.yaw.move"), AntiAim->MoveCustomAngleYaw);
		SaveFloat(main, XorStr("antiaim.custom.fakeyaw.move"), AntiAim->MoveCustomAngleFakeYaw);
		SaveInt(main, XorStr("antiaim.spinspeed.move"), AntiAim->MoveSpinSpeed);
		SaveBool(main, XorStr("antiaim.no.enemy.enabled"), AntiAim->NoEnemyEnabled);
		SaveInt(main, XorStr("antiaim.no.enemy"), AntiAim->NoEnemy);
		SaveBool(main, XorStr("antiaim.on.knife"), AntiAim->OnKnife);
		SaveBool(main, XorStr("antiaim.fakewalk"), AntiAim->FakeWalk);
		SaveInt(main, XorStr("antiaim.fakewalk.key"), AntiAim->FakeWalkKey);
		SaveBool(main, XorStr("antiaim.fakewalk.toggle"), AntiAim->FakeWalkToggle);
		SaveInt(main, XorStr("antiaim.fakewalk.speed"), AntiAim->FakeWalkSpeed);
		// FakeDuck раньше вообще не сохранялся и слетал при перезапуске.
		SaveBool(main, XorStr("antiaim.fakeduck"), AntiAim->FakeDuck);
		SaveInt(main, XorStr("antiaim.fakeduck.key"), AntiAim->FakeDuckKey);
		SaveInt(main, XorStr("antiaim.fakeduck.ticks"), AntiAim->FakeDuckTicks);
		SaveBool(main, XorStr("antiaim.hit.reactive"), AntiAim->HitReactive);
		SaveBool(main, XorStr("antiaim.break.lagcomp"), AntiAim->BreakLC);
		SaveInt(main, XorStr("antiaim.defensive.ticks"), AntiAim->DefensiveTicks);
		SaveInt(main, XorStr("antiaim.manual.left"), AntiAim->ManualLeftKey);
		SaveInt(main, XorStr("antiaim.manual.right"), AntiAim->ManualRightKey);
		SaveInt(main, XorStr("antiaim.manual.back"), AntiAim->ManualBackKey);

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
		SaveInt( main, XorStr( "misc.auto.pistol.delay" ), Misc->AutoPistolDelay );
		SaveInt( main, XorStr( "misc.auto.strafe" ), Misc->AutoStrafe );
		SaveBool( main, XorStr( "misc.bomb.warning" ), Misc->BombWarning );
		SaveBool( main, XorStr( "misc.auto.peek" ), Misc->AutoPeek );
		SaveInt( main, XorStr( "misc.auto.peek.key" ), Misc->AutoPeekKey );
		SaveInt( main, XorStr( "misc.crosshair" ), Misc->Crosshair );
		SaveBool( main, XorStr( "misc.crosshair.outlined" ), Misc->Outlined );
		SaveBool( main, XorStr( "misc.crosshair.show.recoil" ), Misc->ShowRecoil );
		SaveBool( main, XorStr( "misc.fakelag" ), Misc->FakeLag );
		SaveInt( main, XorStr( "misc.fakelag.choked.packets" ), Misc->ChokedPackets );
		SaveInt( main, XorStr( "misc.fakelag.mode" ), Misc->FakeLagMode );
		SaveInt( main, XorStr( "misc.fakelag.min" ), Misc->FakeLagMin );
		SaveInt( main, XorStr( "misc.fakelag.max" ), Misc->FakeLagMax );
		SaveBool( main, XorStr( "misc.fakelag.onground" ), Misc->FakeLagOnGroundOnly );
		SaveBool( main, XorStr( "misc.fakelag.breakonshot" ), Misc->FakeLagBreakOnShot );
		SaveBool( main, XorStr( "misc.airstuck" ), Misc->AirStuck );
		SaveInt( main, XorStr( "misc.airstuck.key" ), Misc->StuckKey );
		SaveBool( main, XorStr( "misc.cstrafer" ), Misc->Speed );
		SaveInt(main, XorStr("misc.cstrafer.key"), Misc->SpeedKey);
		SaveFloat(main, XorStr("misc.cstrafer.modifer"), Misc->SpeedMod);
		SaveInt(main, XorStr("misc.restriction"), Misc->Restriction);
		SaveBool(main, XorStr("misc.anti.smac"), Misc->AntiSMAC);
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

			// Та же проверка, что и при загрузке: сохранение не должно
			// падать на битом имени оружия или null-слоте.
			if( index <= 0 || index >= WEAPON_MAX || !Weapon[ index ]
				|| !Weapon[ index ]->Aimbot || !Weapon[ index ]->Triggerbot )
				continue;

			Main->Aimbot->Clamp();

			// Маркер для следующей загрузки: настройки этого оружия уже
			// записаны пользователем (или унаследованы от общего конфига).
			SaveInt( weapon, XorStr( "configured" ), 1 );
			SaveInt( weapon, XorStr( "aimbot.mode" ), Weapon[ index ]->Aimbot->Mode );
			SaveInt( weapon, XorStr( "aimbot.key" ), Weapon[ index ]->Aimbot->Key );
			SaveBool( weapon, XorStr( "aimbot.auto.fire" ), Weapon[ index ]->Aimbot->AutoFire );
			SaveBool( weapon, XorStr( "aimbot.auto.stop" ), Weapon[ index ]->Aimbot->AutoStop );
			SaveBool( weapon, XorStr( "aimbot.auto.crouch" ), Weapon[ index ]->Aimbot->AutoCrouch );
			SaveBool(weapon, XorStr("aimbot.auto.reload"), Weapon[index]->Aimbot->AutoReload);
			SaveBool(weapon, XorStr("aimbot.auto.scope"), Weapon[index]->Aimbot->AutoScope);
			SaveInt(weapon, XorStr("aimbot.mindamage.override"), Weapon[index]->Aimbot->MinDamageOverride);
			SaveInt(weapon, XorStr("aimbot.mindamage.override.key"), Weapon[index]->Aimbot->MinDamageOverrideKey);
			SaveBool(weapon, XorStr("aimbot.anti.spawn.protection"), Weapon[index]->Aimbot->AntiSpawnProtection);
			SaveBool( weapon, XorStr( "aimbot.no.switch" ), Weapon[ index ]->Aimbot->NoSwitch );
			SaveInt( weapon, XorStr( "aimbot.spot" ), Weapon[ index ]->Aimbot->Spot );
			SaveInt( weapon, XorStr( "aimbot.body.aim" ), Weapon[ index ]->Aimbot->ForceBody );
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
			SaveBool( weapon, XorStr( "triggerbot.through.smoke" ), Weapon[ index ]->Triggerbot->ThroughSmoke );
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

	// Легит: класс конфига по оружию (0 Pistol|1 SMG|2 Rifle|3 Shotgun|4 Sniper).
	static int LegitClassForWeapon( int iWeaponID )
	{
		switch( iWeaponID )
		{
		case WEAPON_P228:
		case WEAPON_GLOCK:
		case WEAPON_ELITE:
		case WEAPON_FIVESEVEN:
		case WEAPON_USP:
		case WEAPON_DEAGLE:
			return 0;
		case WEAPON_MAC10:
		case WEAPON_UMP45:
		case WEAPON_MP5NAVY:
		case WEAPON_TMP:
		case WEAPON_P90:
			return 1;
		case WEAPON_XM1014:
		case WEAPON_M3:
			return 3;
		case WEAPON_SCOUT:
		case WEAPON_AWP:
		case WEAPON_G3SG1:
			return 4;
		default: // rifles, M249, misc
			return 2;
		}
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

		// GetWeaponID() приходит из игры, поэтому индекс обязан проверяться:
		// раньше стояло Weapon[ i ] без проверок — id больше WEAPON_MAX
		// (кастомный сервер, другой билд клиента) или WEAPON_NONE давали выход
		// за границы массива указателей и memcpy из мусора. Если id не наш,
		// тихо используем общий конфиг (как при выключенном Weapon Config).
		const bool bPerWeapon = i > 0 && i < WEAPON_MAX
			&& Weapon[ i ] && Weapon[ i ]->Aimbot && Weapon[ i ]->Triggerbot;

		static int s_iLastBadWeapon = -1;

		if( Main->AimbotWeaponConfig && !bPerWeapon && s_iLastBadWeapon != i )
		{
			s_iLastBadWeapon = i;
			LOG( XorStr( "[Config] Weapon id %d is outside the weapon table, global config is used." ), i );
		}

		if( Main->AimbotWeaponConfig && bPerWeapon )
			memcpy( Current->Aimbot, Weapon[ i ]->Aimbot, sizeof( AimbotList ) );
		else
			memcpy( Current->Aimbot, Main->Aimbot, sizeof( AimbotList ) );
		
		if( Main->TriggerbotWeaponConfig && bPerWeapon )
			memcpy( Current->Triggerbot, Weapon[ i ]->Triggerbot, sizeof( TriggerbotList ) );
		else
			memcpy( Current->Triggerbot, Main->Triggerbot, sizeof( TriggerbotList ) );

		// Legit-стиль: поверх текущего конфига кладём легитные настройки,
		// а рейдж-фичи принудительно гасим. Вся логика (Aimbot/Hooked)
		// читает только Current->Aimbot, поэтому переключение атомарно.
		auto legit = LegitbotClasses[ LegitClassForWeapon( i ) ];

		if( Main->AimbotStyle == 1 && legit )
		{
			Current->Aimbot->Mode				= legit->Mode;
			Current->Aimbot->Key				= legit->Key;
			// Легит: Spot заменён зонами — сюда кость приоритета (нужна бектреку).
			static const int iPrioBone[ 5 ] = { 12, 10, 0, 13, 1 };
			Current->Aimbot->Spot = ( legit->HitboxPriority >= 0 && legit->HitboxPriority < 5 ) ? iPrioBone[ legit->HitboxPriority ] : 12;
			Current->Aimbot->TargetSelection	= legit->TargetSelection;
			Current->Aimbot->FieldOfView		= legit->FieldOfView;
			Current->Aimbot->Smooth			= legit->Smooth;
			Current->Aimbot->StepX				= legit->StepX;
			Current->Aimbot->StepY				= legit->StepY;
			Current->Aimbot->SmoothX			= legit->SmoothX;
			Current->Aimbot->SmoothY			= legit->SmoothY;
			Current->Aimbot->Delay				= legit->Delay;
			Current->Aimbot->Duration			= legit->Duration;
			Current->Aimbot->RCS				= legit->RCS;
			Current->Aimbot->RCSDelay			= legit->RCSDelay;
			Current->Aimbot->RCSAmountX		= legit->RCSAmountX;
			Current->Aimbot->RCSAmountY		= legit->RCSAmountY;
			Current->Aimbot->Target				= legit->Target;
			Current->Aimbot->AutoFire			= legit->AutoFire;
			Current->Aimbot->AutoStop			= legit->AutoStop;
			Current->Aimbot->RCSStandalone		= legit->RCSStandalone;
			Current->Aimbot->FlashCheck			= legit->FlashCheck;
			Current->Aimbot->HumanizeDelay		= legit->HumanizeDelay;
			Current->Aimbot->AutoScope			= legit->AutoScope;
			Current->Aimbot->Randomize = legit->Randomize;
			Current->Aimbot->Curve = legit->Curve;
			Current->Aimbot->ZoneHead = legit->ZoneHead;
			Current->Aimbot->ZoneChest = legit->ZoneChest;
			Current->Aimbot->ZoneStomach = legit->ZoneStomach;
			Current->Aimbot->ZoneArms = legit->ZoneArms;
			Current->Aimbot->ZoneLegs = legit->ZoneLegs;
			Current->Aimbot->HitboxPriority = legit->HitboxPriority;
			Current->Aimbot->HitboxSelection = legit->HitboxSelection;
			Current->Aimbot->ToggleKey = legit->ToggleKey;
			Current->Aimbot->FireOnKey = legit->FireOnKey;
			Current->Aimbot->FireKey = legit->FireKey;
			Current->Aimbot->ThroughSmoke = legit->ThroughSmoke;

			// Рейдж-фичи в легите недоступны.
			Current->Aimbot->AutoCrouch			= false;
			Current->Aimbot->AutoReload			= false;
			Current->Aimbot->AntiSpawnProtection	= false;
			Current->Aimbot->NoSwitch = ( legit->TSD <= 0 ); // TSD>0: ждём перед сменой вместо лока
			Current->Aimbot->SpotRandomize			= false;
			Current->Aimbot->Height				= false;
			Current->Aimbot->HeightScale			= 0.0f;
			Current->Aimbot->HeightScaleX			= 0.0f;
			Current->Aimbot->HeightScaleY			= 0.0f;
			Current->Aimbot->SwitchDelay = legit->TSD;
			Current->Aimbot->AutoWall = legit->AutoWall;
			Current->Aimbot->MinDamage = legit->MinDamage;
			Current->Aimbot->HitScan				= 0;
			Current->Aimbot->HitScanScale			= 0.0f;
			Current->Aimbot->Silent				= false;
			Current->Aimbot->ForceBody		= 0; // rage-only: legit has own zones
			Current->Aimbot->NoSpreadActive		= false;
			Current->Aimbot->NoSpread			= 0;
			Current->Aimbot->LagCompensation		= legit->Backtrack ? 1 : 0; // legit backtrack
			Current->Aimbot->LastTick			= false;
			Current->Aimbot->SetAbs				= false;
			Current->Aimbot->UpdateAnim			= legit->Backtrack;
			Current->Aimbot->Resolver			= false;
			Current->Aimbot->ResvolerBullets		= 0;
			Current->Aimbot->ResvolerBulletsDelay	= 0;
			Current->Aimbot->MinDamageOverride = 0; // у легита нет оверрайда
			Current->Aimbot->MinDamageOverrideKey = 0;
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
		else if (!std::strcmp(name, XorStr("Five-SeveN")) || !std::strcmp(name, XorStr("Five-Seven")))
			return WEAPON_FIVESEVEN;
		else if (!std::strcmp(name, XorStr("M3")))
			return WEAPON_M3;
		else if (!std::strcmp(name, XorStr("XM1014")) || !std::strcmp(name, XorStr("XM1024")))
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