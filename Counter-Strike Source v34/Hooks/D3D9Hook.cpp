#include "Main.h"
#include "D3D9Hook.h"
#include "detours.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "GUI.h"

#pragma comment( lib, "d3d9.lib" )

EndScene_t oEndScene = nullptr;
Reset_t oReset = nullptr;
WNDPROC oWndProc = nullptr;
HWND g_hGameWindow = nullptr;
bool g_bImGuiInitialized = false;

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

HRESULT __stdcall Hooked_EndScene( IDirect3DDevice9* pDevice )
{
	if( !pDevice )
		return oEndScene ? oEndScene( pDevice ) : D3D_OK;

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

		g_GUI.DrawImGui( );

		ImGui::EndFrame( );
		ImGui::Render( );
		ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );
	}

	return oEndScene( pDevice );
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

	LPDIRECT3D9 pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if( !pD3D )
	{
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
		pD3D->Release( );
		DestroyWindow( hWnd );
		UnregisterClassA( "DX9_Hook_Window", wc.hInstance );
		return false;
	}

	void** pVTable = *( void*** )pDummyDevice;
	if( pVTable )
	{
		oEndScene = ( EndScene_t )DetourFunction( ( PBYTE )pVTable[ 42 ], ( PBYTE )Hooked_EndScene );
		oReset = ( Reset_t )DetourFunction( ( PBYTE )pVTable[ 16 ], ( PBYTE )Hooked_Reset );
	}

	pDummyDevice->Release( );
	pD3D->Release( );
	DestroyWindow( hWnd );
	UnregisterClassA( "DX9_Hook_Window", wc.hInstance );

	return ( oEndScene != nullptr && oReset != nullptr );
}

void ShutdownD3D9Hook( void )
{
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

	if( g_bImGuiInitialized )
	{
		ImGui_ImplDX9_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );
		g_bImGuiInitialized = false;
	}
}
