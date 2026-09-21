// ============================================================================
// Точка входа DLL.
//
// Режимы сборки:
//   - по умолчанию: приватная HWID-проверка ВЫКЛЮЧЕНА, чит работает на любом ПК;
//   - приватная сборка: включите PRIVATE_BUILD в License.hpp (или /DPRIVATE_BUILD)
//     и добавьте свой HWID-ключ в License.cpp -> AllowedKeys.
//
// Диагностика: с момента создания рабочей папки все сообщения (включая ошибки
// старта в Release-сборке) пишутся в <рабочая папка>\v34\aresware.log.
// Если чит не заработал — сначала смотрите этот файл.
// ============================================================================

#include "Source.hpp"
#include "Config.hpp"
#include "License.hpp"
#include "Valve.hpp"
#include "ImGui.hpp"
#include "Debug.hpp"

#include <cstdlib>
#include <string>
#include <atomic>
#include <windows.h>

// Рабочая папка чита. Config::Startup допишет сюда "\\v34\\" для конфигов.
static Shared::Vars g_Vars =
{
	"C:\\rraggerr\\"
};

// Выставляется после Config::Startup, чтобы Eject() не трогал
// неинициализированные структуры (раньше это роняло игру).
static bool g_bConfigReady = false;

// Защита от повторного Eject(): поток чита и DLL_PROCESS_DETACH приходят сюда
// оба, и двойное снятие хуков роняло игру.
static std::atomic< bool > g_bEjecting( false );

static void CreateWorkDirectories()
{
	// ERROR_ALREADY_EXISTS — нормальная ситуация (папка уже есть),
	// остальные ошибки логируем: без папки конфиг/лог не создадутся.
	if( !CreateDirectoryA( g_Vars.m_loader, nullptr ) && GetLastError() != ERROR_ALREADY_EXISTS )
		LOG( XorStr( "[Startup] Can't create directory '%s' (error %u)." ), g_Vars.m_loader, ( unsigned )GetLastError() );

	const std::string cfgDir = std::string( g_Vars.m_loader ) + XorStr( "v34" );

	if( !CreateDirectoryA( cfgDir.c_str(), nullptr ) && GetLastError() != ERROR_ALREADY_EXISTS )
		LOG( XorStr( "[Startup] Can't create directory '%s' (error %u)." ), cfgDir.c_str(), ( unsigned )GetLastError() );

	// Лог открываем как можно раньше: любая ошибка ниже будет видна в файле.
	const std::string logPath = cfgDir + XorStr( "\\aresware.log" );

	Debug::LogOpen( logPath.c_str() );
}

// bFromProcessDetach == true: вызов из DLL_PROCESS_DETACH (мы под loader lock,
// спать и делать тяжёлую очистку нельзя).
void Eject( bool bFromProcessDetach )
{
	bool bExpected = false;

	if( !g_bEjecting.compare_exchange_strong( bExpected, true ) )
		return;

	if( !Source::Release() )
		LOG( XorStr( "[Eject] Can't release 'Source' hooks!" ) );

	// Даём хукам время сняться перед выгрузкой — но только когда мы в своём
	// потоке. Под loader lock (DLL_PROCESS_DETACH) спать нельзя: ОС в это
	// время не может загружать/выгружать модули, и игра висит секунду.
	if( !bFromProcessDetach )
		Sleep( 1000 );

	Source::Free();

	if( g_bConfigReady )
	{
		Config::Release();
		g_bConfigReady = false;
	}

	Shared::m_pVars = nullptr;

	if( !bFromProcessDetach )
		Debug::LogClose();
}

// Основной поток чита: инициализация + обработка запросов на Load/Save конфига.
static DWORD WINAPI CheatThread( LPVOID lpParam )
{
	HMODULE hMod = ( HMODULE )lpParam;

	Config::Startup( hMod );
	g_bConfigReady = true;

	// Инжект часто происходит раньше, чем игра создаст D3D-устройство и
	// загрузит все модули (особенно при инжекте во время загрузки карты).
	// Раньше единственная неудачная попытка означала молчаливую выгрузку:
	// «чит вроде загрузился, а меню нет и ничего не работает». Теперь
	// повторяем инициализацию ~60 секунд.
	const int kMaxStartupAttempts = 120;
	const DWORD kStartupRetryDelay = 500;

	bool bInitialized = false;

	for( int i = 0; i < kMaxStartupAttempts && !Shared::m_bEject; i++ )
	{
		if( Source::Startup() )
		{
			bInitialized = true;
			break;
		}

		LOG( XorStr( "[Startup] Attempt %d/%d failed, retrying in %u ms." ), i + 1, kMaxStartupAttempts, ( unsigned )kStartupRetryDelay );

		// Снимаем то, что успело поставиться: иначе следующая попытка
		// наложила бы хуки поверх уже установленных.
		Source::Release();

		Sleep( kStartupRetryDelay );
	}

	if( !bInitialized )
	{
		LOG( XorStr( "[Startup] Can't initialize 'Source' hooks! Ejecting!" ) );

		Eject( false );
		FreeLibraryAndExitThread( hMod, EXIT_SUCCESS );
		return 1;
	}

	LOG( XorStr( "[Startup] Initialized. Menu key: %d (45 = INSERT)." ), Config::Binds ? Config::Binds->Menu : -1 );

	while( !Shared::m_bEject )
	{
		if( Shared::m_bLoad )
		{
			Config::Load( Shared::m_strConfig );
			ImGui::LoadSettings( std::string( Config::GetPath() + XorStr( "gui" ) ).c_str() );
			Shared::m_bLoad = false;

			LOG( XorStr( "[Config] Loaded '%s'. Menu key: %d." ), Shared::m_strConfig.c_str(), Config::Binds ? Config::Binds->Menu : -1 );
		}

		if( Shared::m_bSave )
		{
			Config::Save( Shared::m_strConfig );
			ImGui::SaveSettings( std::string( Config::GetPath() + XorStr( "gui" ) ).c_str() );
			Shared::m_bSave = false;
		}

		Sleep( 100 );
	}

	Eject( false );
	FreeLibraryAndExitThread( hMod, EXIT_SUCCESS );
	return 0;
}

static BOOL Startup( HMODULE hMod )
{
	CreateWorkDirectories();

	LOG( XorStr( "[Startup] AresWare attaching, module base 0x%X." ), ( unsigned )hMod );

#ifdef PRIVATE_BUILD
	if( !License::Check() )
	{
		LOG( XorStr( "[Startup] License check failed, unloading." ) );
		return FALSE;
	}
#endif

	DisableThreadLibraryCalls( hMod );

	if( !CreateThread( nullptr, 0, CheatThread, hMod, 0, nullptr ) )
	{
		LOG( XorStr( "[Startup] Can't create cheat thread (error %u), unloading." ), ( unsigned )GetLastError() );
		return FALSE;
	}

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

void OnProcessDetach( LPVOID lpReserved )
{
	// lpReserved != nullptr означает, что процесс завершается: движок уже
	// сносит свои структуры, и трогать его память (снимать хуки) опасно.
	// Именно этот случай раньше давал краш при выходе из игры.
	if( lpReserved != nullptr )
		return;

	Eject( true );
}

BOOL WINAPI DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{
	switch( dwReason )
	{
	case DLL_PROCESS_ATTACH:
		return OnProcessAttach( hMod, lpReserved );

	case DLL_PROCESS_DETACH:
		OnProcessDetach( lpReserved );
		break;
	}

	return TRUE;
}
