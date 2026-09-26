#include "Main.h"
#include "D3D9Hook.h"
#include "detours.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "GUI.h"

#pragma comment( lib, "d3d9.lib" )

typedef HRESULT( __stdcall* Present_t )( IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA* );

EndScene_t oEndScene = nullptr;
Reset_t oReset = nullptr;
Present_t oPresent = nullptr;
WNDPROC oWndProc = nullptr;
HWND g_hGameWindow = nullptr;
bool g_bImGuiInitialized = false;
bool g_bD3D9Hooked = false; // true when EndScene/Present detour lives - vgui ESP must yield Box/Name/Health to the ImGui fallback

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

static HWND GetGameWindow( void )
{
	HWND hWnd = FindWindowA( "Valve001", NULL );
	if( hWnd )
		return hWnd;

	DWORD dwCurPid = GetCurrentProcessId( );
	struct EnumData
	{
		DWORD pid;
		HWND hWnd;
	} data = { dwCurPid, NULL };

	EnumWindows( []( HWND h, LPARAM lp ) -> BOOL
	{
		EnumData* d = ( EnumData* )lp;
		DWORD pid = 0;
		GetWindowThreadProcessId( h, &pid );
		if( pid == d->pid && IsWindowVisible( h ) )
		{
			char title[ 256 ];
			GetWindowTextA( h, title, sizeof( title ) );
			if( strlen( title ) > 0 )
			{
				d->hWnd = h;
				return FALSE;
			}
		}
		return TRUE;
	}, ( LPARAM )&data );

	return data.hWnd;
}

LRESULT CALLBACK Hooked_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_KEYDOWN && wParam == VK_INSERT )
	{
		bMouse = !bMouse;
		if( g_bImGuiInitialized )
		{
			ImGui::GetIO( ).MouseDrawCursor = bMouse;
		}
	}

	if( g_bImGuiInitialized && bMouse )
	{
		if( ImGui_ImplWin32_WndProcHandler( hWnd, uMsg, wParam, lParam ) )
			return 1;

		ImGuiIO& io = ImGui::GetIO( );
		if( io.WantCaptureMouse )
		{
			if( uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST )
				return 1;
		}
		if( io.WantCaptureKeyboard )
		{
			if( uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST && wParam != VK_INSERT )
				return 1;
		}
	}

	return CallWindowProcA( oWndProc, hWnd, uMsg, wParam, lParam );
}

// Runs one full ImGui frame against the game device; shared by the EndScene
// hook and the Present fallback hook (whenever one of them is installed).
static void ImGuiFrame( IDirect3DDevice9* pDevice )
{
	if( !g_bImGuiInitialized )
	{
		D3DDEVICE_CREATION_PARAMETERS params;
		if( SUCCEEDED( pDevice->GetCreationParameters( &params ) ) && params.hFocusWindow )
		{
			g_hGameWindow = params.hFocusWindow;
		}
		else
		{
			g_hGameWindow = GetGameWindow( );
		}

		if( g_hGameWindow )
		{
			oWndProc = ( WNDPROC )SetWindowLongPtrA( g_hGameWindow, GWLP_WNDPROC, ( LONG_PTR )Hooked_WndProc );

			ImGui::CreateContext( );
			ImGuiIO& io = ImGui::GetIO( );
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.IniFilename = NULL;
			io.MouseDrawCursor = bMouse;

			g_GUI.SetupStyle( );

			ImGui_ImplWin32_Init( g_hGameWindow );
			ImGui_ImplDX9_Init( pDevice );

			g_bImGuiInitialized = true;
		}
	}

	if( g_bImGuiInitialized )
	{
		ImGui_ImplDX9_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );

		g_GUI.DrawImGuiESP( );
		g_GUI.DrawImGui( );

		ImGui::EndFrame( );
		ImGui::Render( );
		ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );
	}
}

HRESULT __stdcall Hooked_EndScene( IDirect3DDevice9* pDevice )
{
	if( !pDevice )
		return oEndScene ? oEndScene( pDevice ) : D3D_OK;

	ImGuiFrame( pDevice );

	return oEndScene( pDevice );
}

// Fallback render hook: used when the EndScene detour could not be installed
// (an old Detours CDetourException was being thrown there on the test rig).
HRESULT __stdcall Hooked_Present( IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion )
{
	if( pDevice && oPresent )
		ImGuiFrame( pDevice );

	if( !oPresent )
		return D3D_OK;

	return oPresent( pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}

HRESULT __stdcall Hooked_Reset( IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters )
{
	if( g_bImGuiInitialized )
	{
		ImGui_ImplDX9_InvalidateDeviceObjects( );
	}

	HRESULT hr = oReset ? oReset( pDevice, pPresentationParameters ) : D3D_OK;

	if( SUCCEEDED( hr ) )
	{
		if( g_bImGuiInitialized )
		{
			ImGui_ImplDX9_CreateDeviceObjects( );
		}
	}

	return hr;
}

bool InitializeD3D9Hook( void )
{
	WNDCLASSEXA wc = { sizeof( WNDCLASSEXA ), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandleA( NULL ), NULL, NULL, NULL, NULL, "DX9_Hook_Window", NULL };
	RegisterClassExA( &wc );
	HWND hWnd = CreateWindowExA( 0, "DX9_Hook_Window", "DX9_Hook_Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL );
	if( !hWnd )
	{
		Logger::Write( "D3D9 hook: dummy window FAILED (error %d)", GetLastError( ) );
		UnregisterClassA( "DX9_Hook_Window", wc.hInstance );
		return false;
	}

	LPDIRECT3D9 pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if( !pD3D )
	{
		Logger::Write( "D3D9 hook: Direct3DCreate9 returned NULL" );
		DestroyWindow( hWnd );
		UnregisterClassA( "DX9_Hook_Window", wc.hInstance );
		return false;
	}

	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;

	LPDIRECT3DDEVICE9 pDummyDevice = nullptr;
	HRESULT hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice );
	if( FAILED( hr ) || !pDummyDevice )
	{
		Logger::Write( "D3D9 hook: dummy CreateDevice failed, hr=0x%08X device=%p", ( unsigned )hr, pDummyDevice );
		if( pDummyDevice ) pDummyDevice->Release( );
		pD3D->Release( );
		DestroyWindow( hWnd );
		UnregisterClassA( "DX9_Hook_Window", wc.hInstance );
		return false;
	}

	void** pVTable = *( void*** )pDummyDevice;
	Logger::Write( "D3D9 hook: dummy device ok, vtable=%p reset=%p present=%p endscene=%p",
		pVTable, pVTable ? pVTable[ 16 ] : nullptr, pVTable ? pVTable[ 17 ] : nullptr, pVTable ? pVTable[ 42 ] : nullptr );

	if( pVTable )
	{
		// old Detours can raise a C++ CDetourException; never let it escape
		try { oEndScene = ( EndScene_t )DetourFunction( ( PBYTE )pVTable[ 42 ], ( PBYTE )Hooked_EndScene ); }
		catch(...){ Logger::Write( "D3D9 hook: EndScene detour threw a C++ exception (will try Present)" ); oEndScene = nullptr; }
		Logger::Write( "D3D9 hook: EndScene trampoline = %p", oEndScene );

		if( !oEndScene )
		{
			try { oPresent = ( Present_t )DetourFunction( ( PBYTE )pVTable[ 17 ], ( PBYTE )Hooked_Present ); }
			catch(...){ Logger::Write( "D3D9 hook: Present detour threw a C++ exception" ); oPresent = nullptr; }
			Logger::Write( "D3D9 hook: Present trampoline = %p", oPresent );
		}

		try { oReset = ( Reset_t )DetourFunction( ( PBYTE )pVTable[ 16 ], ( PBYTE )Hooked_Reset ); }
		catch(...){ Logger::Write( "D3D9 hook: Reset detour threw a C++ exception" ); oReset = nullptr; }
		Logger::Write( "D3D9 hook: Reset trampoline = %p", oReset );
	}

	pDummyDevice->Release( );
	pD3D->Release( );
	DestroyWindow( hWnd );
	UnregisterClassA( "DX9_Hook_Window", wc.hInstance );

	const bool ok = ( oEndScene != nullptr || oPresent != nullptr );
	g_bD3D9Hooked = ok;
	Logger::Write( "D3D9 hook: %s (endscene=%p present=%p reset=%p)", ok ? "INSTALLED" : "FAILED", oEndScene, oPresent, oReset );
	return ok;
}

void ShutdownD3D9Hook( void )
{
	g_bD3D9Hooked = false;
	if( g_hGameWindow && oWndProc )
	{
		SetWindowLongPtrA( g_hGameWindow, GWLP_WNDPROC, ( LONG_PTR )oWndProc );
		oWndProc = nullptr;
	}

	if( oEndScene )
	{
		DetourRemove( ( PBYTE )oEndScene, ( PBYTE )Hooked_EndScene );
		oEndScene = nullptr;
	}

	if( oReset )
	{
		DetourRemove( ( PBYTE )oReset, ( PBYTE )Hooked_Reset );
		oReset = nullptr;
	}

	if( oPresent )
	{
		DetourRemove( ( PBYTE )oPresent, ( PBYTE )Hooked_Present );
		oPresent = nullptr;
	}

	if( g_bImGuiInitialized )
	{
		ImGui_ImplDX9_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );
		g_bImGuiInitialized = false;
	}
}
