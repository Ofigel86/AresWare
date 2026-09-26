#pragma once
#ifndef _D3D9HOOK_H_
#define _D3D9HOOK_H_

#include <d3d9.h>

typedef HRESULT( __stdcall* EndScene_t )( IDirect3DDevice9* );
typedef HRESULT( __stdcall* Reset_t )( IDirect3DDevice9*, D3DPRESENT_PARAMETERS* );

extern EndScene_t oEndScene;
extern Reset_t oReset;
extern WNDPROC oWndProc;
extern HWND g_hGameWindow;
extern bool g_bImGuiInitialized;

bool InitializeD3D9Hook( void );
void ShutdownD3D9Hook( void );

HRESULT __stdcall Hooked_EndScene( IDirect3DDevice9* pDevice );
HRESULT __stdcall Hooked_Reset( IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );
LRESULT CALLBACK Hooked_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

#endif // _D3D9HOOK_H_
