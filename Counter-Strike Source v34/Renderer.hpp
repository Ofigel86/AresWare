#pragma once

#include "Font.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define INVALID_FONT_HANDLE -1

enum
{
	FONT_ALIGN_LEFT = 0,
	FONT_ALIGN_RIGHT = 1,
	FONT_ALIGN_CENTER_V = 2,
	FONT_ALIGN_CENTER_H = 4,
	FONT_ALIGN_CENTER = FONT_ALIGN_CENTER_V | FONT_ALIGN_CENTER_H,
};

namespace Direct3D9
{
	typedef long HFont;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		bool									Create( IDirect3DDevice9* pDevice );

		void									OnLostDevice();
		void									OnResetDevice();

		void									Begin();
		void									End();

		HFont									CreateFont( const std::string& name, std::uint32_t size, std::uint8_t flags = FONT_CREATE_NORMAL );

		void									DrawText( HFont hFont, float x, float y, int flags, const Color& color, const char* fmt, ... );

		void									DrawCircle(float x, float y, float r, float s, Color color);
		void									DrawRect( int x, int y, int w, int h, const Color& color );
		void									DrawRectOut( int x, int y, int w, int h, const Color& color, const Color& out = Color::Black );
		void									DrawBorderBox( int x, int y, int w, int h, int t, const Color& color );
		void DrawCircle1(float x, float y, float r, float s, Color color);
		void									DrawBorderBoxOut( int x, int y, int w, int h, int t, const Color& color, const Color& out = Color::Black );


		void									DrawLine( int x0, int y0, int x1, int y1, const Color& color );
		void									DrawTriangle( int x0, int y0, int x1, int y1, int x2, int y2, const Color& color );
		void									DrawTriangleOut( int x0, int y0, int x1, int y1, int x2, int y2, const Color& color, const Color& out = Color::Black );

		void								DrawRectRainbow(int x, int y, int width, int height, float flSpeed, float & flRainbow);

	private:
		IDirect3DDevice9*						m_pDevice;
		IDirect3DStateBlock9*					m_pStateBlock;
		std::vector< std::unique_ptr< Font > >	m_pFontList;
	};
}