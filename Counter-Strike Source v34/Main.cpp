// ============================================================================
// Точка входа DLL.
//
// Режимы сборки:
//   - по умолчанию: приватная HWID-проверка ВЫКЛЮЧЕНА, чит работает на любом ПК;
//   - приватная сборка: включите PRIVATE_BUILD в License.hpp (или /DPRIVATE_BUILD)
//     и добавьте свой HWID-ключ в License.cpp -> AllowedKeys.
// ============================================================================

#include "Source.hpp"
#include "Config.hpp"
#include "License.hpp"
#include "Valve.hpp"
#include "ImGui.hpp"

#include <cstdlib>
#include <string>
#include <windows.h>

// Рабочая папка чита. Config::Startup допишет сюда "\\v34\\" для конфигов.
static Shared::Vars g_Vars =
{
	"C:\\rraggerr\\"
};

// Выставляется после Config::Startup, чтобы Eject() не трогал
// неинициализированные структуры (раньше это роняло игру).
static bool g_bConfigReady = false;

static void CreateWorkDirectories()
{
	CreateDirectoryA( g_Vars.m_loader, nullptr );

	const std::string cfgDir = std::string( g_Vars.m_loader ) + XorStr( "v34" );
	CreateDirectoryA( cfgDir.c_str(), nullptr );
}

void Eject()
{
	if( !Source::Release() )
		DPRINT( XorStr( "[Eject] Can't release 'Source' hooks!" ) );

	// Даём хукам время сняться перед выгрузкой.
	Sleep( 1000 );

	Source::Free();

	if( g_bConfigReady )
	{
		Config::Release();
		g_bConfigReady = false;
	}

	Shared::m_pVars = nullptr;
}

// Основной поток чита: инициализация + обработка запросов на Load/Save конфига.
static DWORD WINAPI CheatThread( LPVOID lpParam )
{
	HMODULE hMod = ( HMODULE )lpParam;

	Config::Startup( hMod );
	g_bConfigReady = true;

	if( !Source::Startup() )
	{
		DPRINT( XorStr( "[Startup] Can't initialize 'Source' hooks! Ejecting!" ) );

		Eject();
		FreeLibraryAndExitThread( hMod, EXIT_SUCCESS );
		return 1;
	}

	while( !Shared::m_bEject )
	{
		if( Shared::m_bLoad )
		{
			Config::Load( Shared::m_strConfig );
			ImGui::LoadSettings( std::string( Config::GetPath() + XorStr( "gui" ) ).c_str() );
			Shared::m_bLoad = false;
		}

		if( Shared::m_bSave )
		{
			Config::Save( Shared::m_strConfig );
			ImGui::SaveSettings( std::string( Config::GetPath() + XorStr( "gui" ) ).c_str() );
			Shared::m_bSave = false;
		}

		Sleep( 100 );
	}

	Eject();
	FreeLibraryAndExitThread( hMod, EXIT_SUCCESS );
	return 0;
}

static BOOL Startup( HMODULE hMod )
{
#ifdef PRIVATE_BUILD
	if( !License::Check() )
	{
		DPRINT( XorStr( "[Startup] License check failed, unloading." ) );
		return FALSE;
	}
#endif

	CreateWorkDirectories();

	DisableThreadLibraryCalls( hMod );
	CreateThread( nullptr, 0, CheatThread, hMod, 0, nullptr );

	return TRUE;
}

BOOL OnProcessAttach( HMODULE hMod, LPVOID lpReserved )
{
	( void )lpReserved;

	Shared::m_pVars = &g_Vars;

	// FALSE из DllMain при DLL_PROCESS_ATTACH: загрузчик сам выгрузит DLL.
	// (FreeLibraryAndExitThread здесь вызывать нельзя — loader lock.)
	return Startup( hMod );
}

void OnProcessDetach()
{
	Eject();
}

BOOL WINAPI DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{
	switch( dwReason )
	{
	case DLL_PROCESS_ATTACH:
		return OnProcessAttach( hMod, lpReserved );

	case DLL_PROCESS_DETACH:
		OnProcessDetach();
		break;
	}

	return TRUE;
}
