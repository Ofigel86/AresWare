#include "Input.hpp"
#include "Source.hpp"
#include "Config.hpp"
#include "Aimbot.hpp"

HWND hTarget( nullptr );

// Ищем окно игры.
//
// Раньше бралось просто первое top-level окно процесса. Если движок (или
// любой сторонний модуль) создаёт свои окна раньше игрового, WndProc вешался
// не на то окно: клавиши до чита не доходили, и меню не открывалось вообще.
//
// Приоритет: окно класса Valve001 (главное окно Source-движка) → видимое окно
// с заголовком → любое окно процесса.
BOOL WINAPI EnumWnd( HWND hWnd, LPARAM lParam )
{
	DWORD dwProcessId( NULL );

	GetWindowThreadProcessId( hWnd, &dwProcessId );

	if( dwProcessId != ( DWORD )lParam )
		return TRUE;

	char szClass[ 64 ] = {};
	char szTitle[ 128 ] = {};

	GetClassNameA( hWnd, szClass, sizeof( szClass ) );
	GetWindowTextA( hWnd, szTitle, sizeof( szTitle ) );

	if( !strcmp( szClass, XorStr( "Valve001" ) ) )
	{
		hTarget = hWnd;
		return FALSE;
	}

	if( hTarget == nullptr && IsWindowVisible( hWnd ) && szTitle[ 0 ] != '\0' )
		hTarget = hWnd;

	return TRUE;
}

namespace Input
{
	Win32::Win32()
		:	m_hTarget( nullptr ),
			m_pProcedure( nullptr )
	{

	}

	Win32::~Win32()
	{
		Release();
	}

	bool Win32::Capture()
	{
		hTarget = nullptr;

		EnumWindows( EnumWnd, GetCurrentProcessId() );

		m_hTarget = hTarget;

		if( !m_hTarget )
		{
			LOG( XorStr( "[Win32::Capture] Can't get target window!" ) );
			return false;
		}

		// SetWindowLongPtr возвращает 0 и при ошибке, и когда предыдущая
		// процедура была нулевой — поэтому сбрасываем LastError до вызова.
		SetLastError( 0 );
		m_pProcedure = ( WNDPROC )SetWindowLongPtr( m_hTarget, GWLP_WNDPROC, ( LONG_PTR )&Proxy );

		if( !m_pProcedure && GetLastError() != ERROR_SUCCESS )
		{
			LOG( XorStr( "[Win32::Capture] Can't hook window procedure (error %u)." ), ( unsigned )GetLastError() );
			m_hTarget = nullptr;
			return false;
		}

		char szClass[ 64 ] = {};
		char szTitle[ 128 ] = {};

		GetClassNameA( m_hTarget, szClass, sizeof( szClass ) );
		GetWindowTextA( m_hTarget, szTitle, sizeof( szTitle ) );

		LOG( XorStr( "[Win32::Capture] Window 0x%X hooked (class '%s', title '%s')." ), ( unsigned )m_hTarget, szClass, szTitle );

		return true;
	}

	bool Win32::Release()
	{
		if( !m_pProcedure )
		{
			m_hTarget = nullptr;
			return true;
		}

		// Окно могло быть уничтожено (выход из игры) — тогда восстанавливать
		// нечего, и SetWindowLongPtr на мёртвом HWND просто падал бы.
		if( !m_hTarget || !IsWindow( m_hTarget ) )
		{
			m_hTarget = nullptr;
			m_pProcedure = nullptr;
			return true;
		}

		if( !SetWindowLongPtr( m_hTarget, GWL_WNDPROC, ( LONG_PTR )m_pProcedure ) )
		{
			LOG( XorStr( "[Win32::Release] Can't restore window procedure (error %u)." ), ( unsigned )GetLastError() );
			m_hTarget = nullptr;
			m_pProcedure = nullptr;
			return false;
		}

		m_hTarget = nullptr;
		m_pProcedure = nullptr;

		return true;
	}

	// Проверяем, что в окне всё ещё стоит наша процедура. Переключение
	// полноэкранный/оконный режим и смена разрешения могут пересоздать окно —
	// тогда Windows-хук теряется и меню перестаёт открываться.
	// Вызывается на Reset устройства (см. Hooked_Reset).
	bool Win32::EnsureCaptured()
	{
		if( m_hTarget && IsWindow( m_hTarget )
			&& GetWindowLongPtr( m_hTarget, GWLP_WNDPROC ) == ( LONG_PTR )&Proxy )
			return true;

		LOG( XorStr( "[Win32::EnsureCaptured] Window procedure lost, re-capturing." ) );

		Release();

		return Capture();
	}

	const HWND Win32::GetTarget() const
	{
		return m_hTarget;
	}

	// ------------------------------------------------------------------------
	// Единый обработчик «горячая клавиша → действие».
	//
	// Раньше этот блок (мышь 1/2/3/4/5 + WM_KEYDOWN/WM_SYSKEYDOWN) был
	// скопирован шесть раз — в Speed, AirStuck, Menu, Panic и Eject. В копиях
	// легко было забыть XBUTTON2 или DBLCLK, поэтому логика теперь одна.
	//
	// bind: 1/2/4 — ЛКМ/ПКМ/СКМ, 5/6 — XBUTTON1/2, иначе — VK-код.
	// ------------------------------------------------------------------------
	template< typename F >
	static bool HandleToggleKey( int bind, UINT message, WPARAM wParam, F&& toggle )
	{
		switch( bind )
		{
		case 1: // Mouse 1
			if( message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK )
			{
				toggle();
				return true;
			}
			break;

		case 2: // Mouse 2
			if( message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK )
			{
				toggle();
				return true;
			}
			break;

		case 4: // Mouse 3
			if( message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK )
			{
				toggle();
				return true;
			}
			break;

		case 5: // Mouse 4
			if( HIWORD( wParam ) == XBUTTON1 && ( message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK ) )
			{
				toggle();
				return true;
			}
			break;

		case 6: // Mouse 5
			if( HIWORD( wParam ) == XBUTTON2 && ( message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK ) )
			{
				toggle();
				return true;
			}
			break;

		default:
			if( bind > 0 && ( int )wParam == bind && ( message == WM_KEYDOWN || message == WM_SYSKEYDOWN ) )
			{
				toggle();
				return true;
			}
			break;
		}

		return false;
	}

	void Speed( UINT message, WPARAM wParam, LPARAM lParam )
	{
		( void )lParam;

		HandleToggleKey( Config::Misc->SpeedKey, message, wParam, [] { Shared::m_bSpeed = !Shared::m_bSpeed; } );
	}

	void AirStuck( UINT message, WPARAM wParam, LPARAM lParam )
	{
		( void )lParam;

		HandleToggleKey( Config::Misc->StuckKey, message, wParam, [] { Shared::m_bStuck = !Shared::m_bStuck; } );
	}

	void Extra( UINT message, WPARAM wParam, LPARAM lParam )
	{
		( void )lParam;

		HandleToggleKey( Config::Binds->Menu, message, wParam, [] { Shared::m_bMenu = !Shared::m_bMenu; } );
		HandleToggleKey( Config::Binds->Panic, message, wParam, [] { Shared::m_bPanic = !Shared::m_bPanic; } );
		HandleToggleKey( Config::Binds->Eject, message, wParam, [] { Shared::m_bEject = true; } );
	}

	LRESULT WINAPI Win32::Proxy( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		Extra( message, wParam, lParam );

		if( Source::m_pMenu )
		{
			if (Source::m_pMenu->OnKeyEvent(hWnd, message, wParam, lParam))
				return FALSE;
		}

		if( Source::m_pAimbot )
			Source::m_pAimbot->OnKeyEvent( message, wParam, lParam );

		if( Source::m_pTriggerbot )
			Source::m_pTriggerbot->OnKeyEvent( message, wParam, lParam );

		Speed( message, wParam, lParam );
		AirStuck( message, wParam, lParam );

		return CallWindowProc( Source::m_pTargetInput->m_pProcedure, hWnd, message, wParam, lParam );
	}
}
