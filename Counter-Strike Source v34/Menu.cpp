#include "Menu.hpp"
#include "Source.hpp"
#include "Config.hpp"
#include "Player.hpp"
#include "ImGui.hpp"
#include "ImGuiDX9.hpp"
#include "KeyTranslate.hpp"
#include <ctime>

int iTab;
float mainmenu1 = 810.0f;
float mainmenu2 = 670.0f;
IDirect3DTexture9* tImage = nullptr;
IDirect3DTexture9* tIcons[ 12 ];

ImFont* bad = nullptr;
ImFont* def = nullptr;
ImFont* def1 = nullptr;
ImFont* und = nullptr;
ImFont* fntBody = nullptr;
ImFont* fntTitle = nullptr;

#include "picture.hpp"
#include "MenuIcons.hpp"

extern LRESULT ImGui_ImplDX9_WndProcHandler( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

namespace AW
{
	static ImU32 Col( int r, int g, int b, int a = 255 )
	{
		return IM_COL32( r, g, b, a );
	}

	static float g_flContentX = 0.0f;
	static float g_flContentW = 300.0f;
	static float g_flCtrlW = 140.0f;
	static ImVec2 g_PanelPos = ImVec2( 0, 0 );
	static ImVec2 g_PanelSize = ImVec2( 0, 0 );
	static int* s_pKeyCap = nullptr;
	static bool s_bKeyWaitRel = false;

	static const int kTabIconW[ 7 ] = { 24, 18, 16, 18, 22, 20, 22 };
	static const int kTabIconH[ 7 ] = { 22, 18, 18, 17, 20, 19, 23 };
	static const int kWpnIconW[ 5 ] = { 27, 32, 29, 32, 28 };
	static const int kWpnIconH[ 5 ] = { 15, 11, 10, 11, 7 };

	static const int kClsFirst[ 5 ] = { 0, 8, 13, 6, 15 };
	static const int kClsCount[ 5 ] = { 6, 5, 9, 2, 2 };
	static const int kClsRifle[ 9 ] = { 13, 14, 16, 17, 18, 19, 20, 21, 23 };
	static const int kClsSniper[ 2 ] = { 15, 22 };

	static int ClassWeapon( int cls, int j )
	{
		if( cls == 2 )
			return kClsRifle[ j ];

		if( cls == 4 )
			return kClsSniper[ j ];

		return kClsFirst[ cls ] + j;
	}

	static int ClassIndex( int cls, int w )
	{
		for( int j = 0; j < kClsCount[ cls ]; j++ )
		{
			if( ClassWeapon( cls, j ) == w )
				return j;
		}

		return 0;
	}

	static void Text( const ImVec2& p, ImU32 col, const char* text )
	{
		ImGui::GetWindowDrawList()->AddText( p, col, text );
	}

	static float TextW( const char* text )
	{
		return ImGui::CalcTextSize( text ).x;
	}

	static void TextCentered( float x0, float x1, float y, ImU32 col, const char* text )
	{
		float w = TextW( text );
		Text( ImVec2( x0 + ( x1 - x0 - w ) * 0.5f, y ), col, text );
	}

	static ImVec2 RowReserve( float h )
	{
		float y = ImGui::GetCursorScreenPos().y;
		ImVec2 p( g_flContentX, y );
		ImGui::SetCursorScreenPos( p );
		ImGui::Dummy( ImVec2( g_flContentW, h ) );
		ImGui::SetCursorScreenPos( ImVec2( g_flContentX, y + h ) );
		return p;
	}

	static void BeginPanel( const char* title, const ImVec2& p, const ImVec2& size, float ctrlW = -1.0f )
	{
		g_PanelPos = p;
		g_PanelSize = size;
		ImGui::SetCursorScreenPos( p );
		ImGui::PushID( title );
		ImDrawList* d = ImGui::GetWindowDrawList();
		d->AddRectFilledMultiColor( p, ImVec2( p.x + size.x, p.y + size.y ), Col( 212, 212, 212 ), Col( 212, 212, 212 ), Col( 236, 236, 236 ), Col( 236, 236, 236 ) );
		Text( ImVec2( p.x + 8.0f, p.y + 5.0f ), Col( 28, 28, 28 ), title );
		d->AddLine( ImVec2( p.x + 8.0f, p.y + 23.0f ), ImVec2( p.x + size.x - 8.0f, p.y + 23.0f ), Col( 175, 175, 175 ) );
		ImGui::PushStyleColor( ImGuiCol_ChildWindowBg, ImVec4( 0, 0, 0, 0 ) );
		ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.93f, 0.93f, 0.93f, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_ScrollbarGrab, ImVec4( 0.72f, 0.72f, 0.72f, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabHovered, ImVec4( 0.62f, 0.62f, 0.62f, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabActive, ImVec4( 0.58f, 0.58f, 0.58f, 1 ) );
		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 26.0f ) );
		ImGui::BeginChild( "##c", ImVec2( size.x, size.y - 26.0f ), false );
		g_flContentX = p.x + 8.0f;
		g_flContentW = size.x - 28.0f;
		float cw = ( ctrlW > 0.0f ) ? ctrlW : g_flContentW * 0.45f;

		if( cw < 110.0f )
			cw = 110.0f;

		if( cw > 200.0f )
			cw = 200.0f;

		g_flCtrlW = cw;
		ImGui::SetCursorScreenPos( ImVec2( g_flContentX, p.y + 28.0f ) );
	}

	static void EndPanel()
	{
		ImGui::EndChild();
		ImGui::PopStyleColor( 5 );
		ImGui::GetWindowDrawList()->AddRect( g_PanelPos, ImVec2( g_PanelPos.x + g_PanelSize.x, g_PanelPos.y + g_PanelSize.y ), Col( 179, 179, 179 ) );
		ImGui::PopID();
	}

	static void Notice( const char* text )
	{
		ImVec2 p = RowReserve( 22.0f );
		Text( ImVec2( p.x, p.y + 5.0f ), Col( 170, 50, 40 ), text );
	}

	static bool Checkbox( const char* label, bool* v )
	{
		float w = g_flContentW;
		ImVec2 p = RowReserve( 26.0f );
		ImDrawList* d = ImGui::GetWindowDrawList();
		float bx = p.x + w - g_flCtrlW;
		ImGui::PushClipRect( ImVec2( p.x, p.y ), ImVec2( bx - 6.0f, p.y + 26.0f ), true );
		Text( ImVec2( p.x, p.y + 7.0f ), Col( 28, 28, 28 ), label );
		ImGui::PopClipRect();
		ImVec2 b0( bx, p.y + 5.0f ), b1( bx + 16.0f, p.y + 21.0f );
		d->AddRectFilled( b0, b1, Col( 255, 255, 255 ) );
		d->AddRect( b0, b1, Col( 160, 160, 160 ) );
		d->AddLine( ImVec2( b0.x + 1.0f, b0.y + 1.0f ), ImVec2( b1.x - 1.0f, b0.y + 1.0f ), Col( 225, 225, 225 ) );

		if( *v )
		{
			d->AddLine( ImVec2( bx + 3.5f, p.y + 13.0f ), ImVec2( bx + 7.0f, p.y + 16.5f ), Col( 193, 25, 8 ), 2.0f );
			d->AddLine( ImVec2( bx + 7.0f, p.y + 16.5f ), ImVec2( bx + 13.0f, p.y + 9.0f ), Col( 193, 25, 8 ), 2.0f );
		}

		ImGui::SetCursorScreenPos( p );
		ImGui::InvisibleButton( label, ImVec2( w, 26.0f ) );
		bool clicked = ImGui::IsItemClicked();
		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 26.0f ) );

		if( clicked )
			*v = !*v;

		return clicked;
	}

	static bool ComboRaw( const char* id, int* v, const char* const* items, int n, const ImVec2& b0, const ImVec2& b1 )
	{
		if( *v < 0 || *v >= n )
			*v = 0;

		ImDrawList* d = ImGui::GetWindowDrawList();
		d->AddRectFilled( b0, b1, Col( 255, 255, 255 ) );
		d->AddRect( b0, b1, Col( 165, 165, 165 ) );
		d->AddLine( ImVec2( b0.x + 1.0f, b0.y + 1.0f ), ImVec2( b1.x - 1.0f, b0.y + 1.0f ), Col( 228, 228, 228 ) );
		ImGui::PushClipRect( ImVec2( b0.x + 5.0f, b0.y ), ImVec2( b1.x - 4.0f, b1.y ), true );
		Text( ImVec2( b0.x + 5.0f, b0.y + 4.0f ), Col( 30, 30, 30 ), items[ *v ] );
		ImGui::PopClipRect();
		bool changed = false;
		ImGui::PushID( id );
		ImGui::SetCursorScreenPos( b0 );
		ImGui::InvisibleButton( "##box", ImVec2( b1.x - b0.x, b1.y - b0.y ) );

		if( ImGui::IsItemClicked() )
			ImGui::OpenPopup( "##pop" );

		ImGui::SetNextWindowPos( ImVec2( b0.x, b1.y + 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 1, 1, 1, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.6f, 0.6f, 0.6f, 1 ) );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 1, 1 ) );

		if( ImGui::BeginPopup( "##pop" ) )
		{
			ImDrawList* pd = ImGui::GetWindowDrawList();
			float pw = b1.x - b0.x;
			float rh = 20.0f;
			float ph = ( n > 10 ) ? 200.0f : ( float )n * rh;
			ImGui::PushStyleColor( ImGuiCol_ChildWindowBg, ImVec4( 1, 1, 1, 1 ) );
			ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.93f, 0.93f, 0.93f, 1 ) );
			ImGui::PushStyleColor( ImGuiCol_ScrollbarGrab, ImVec4( 0.72f, 0.72f, 0.72f, 1 ) );
			ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabHovered, ImVec4( 0.62f, 0.62f, 0.62f, 1 ) );
			ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabActive, ImVec4( 0.58f, 0.58f, 0.58f, 1 ) );
			ImGui::BeginChild( "##rows", ImVec2( pw, ph ), false );
			ImVec2 rp = ImGui::GetCursorScreenPos();

			for( int i = 0; i < n; i++ )
			{
				ImVec2 ip( rp.x, rp.y + ( float )i * rh );
				ImGui::SetCursorScreenPos( ip );
				ImGui::PushID( i );
				ImGui::InvisibleButton( "##it", ImVec2( pw, rh ) );
				bool hov = ImGui::IsItemHovered();
				bool clk = ImGui::IsItemClicked();
				ImGui::PopID();

				if( hov )
					pd->AddRectFilled( ip, ImVec2( ip.x + pw, ip.y + rh ), Col( 232, 232, 232 ) );

				ImU32 tc = ( i == *v ) ? Col( 193, 25, 8 ) : Col( 30, 30, 30 );
				ImGui::PushClipRect( ImVec2( ip.x + 6.0f, ip.y ), ImVec2( ip.x + pw - 4.0f, ip.y + rh ), true );
				Text( ImVec2( ip.x + 6.0f, ip.y + 4.0f ), tc, items[ i ] );
				ImGui::PopClipRect();

				if( clk )
				{
					*v = i;
					changed = true;
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::SetCursorScreenPos( ImVec2( rp.x, rp.y + ( float )n * rh ) );
			ImGui::Dummy( ImVec2( pw, 1 ) );
			ImGui::EndChild();
			ImGui::PopStyleColor( 5 );
			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor( 2 );
		ImGui::PopID();
		return changed;
	}

	static bool Combo( const char* label, int* v, const char* const* items, int n )
	{
		float w = g_flContentW;
		ImVec2 p = RowReserve( 26.0f );
		float bx = p.x + w - g_flCtrlW;
		ImGui::PushClipRect( ImVec2( p.x, p.y ), ImVec2( bx - 6.0f, p.y + 26.0f ), true );
		Text( ImVec2( p.x, p.y + 7.0f ), Col( 28, 28, 28 ), label );
		ImGui::PopClipRect();
		ImVec2 b0( bx, p.y + 3.0f ), b1( p.x + w, p.y + 23.0f );
		bool ch = ComboRaw( label, v, items, n, b0, b1 );
		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 26.0f ) );
		return ch;
	}

	static bool KeyRaw( const char* id, int* key, const ImVec2& b0, const ImVec2& b1 )
	{
		ImDrawList* d = ImGui::GetWindowDrawList();
		d->AddRectFilled( b0, b1, Col( 255, 255, 255 ) );
		d->AddRect( b0, b1, Col( 165, 165, 165 ) );
		d->AddLine( ImVec2( b0.x + 1.0f, b0.y + 1.0f ), ImVec2( b1.x - 1.0f, b0.y + 1.0f ), Col( 228, 228, 228 ) );
		const char* nm = XorStr( "-" );

		if( s_pKeyCap != key && *key >= 0 && *key < 124 )
			nm = ImGui::GetNameFromCode( ( uint32_t )*key ).c_str();

		if( s_pKeyCap == key )
			nm = XorStr( "..." );

		ImGui::PushClipRect( ImVec2( b0.x + 5.0f, b0.y ), ImVec2( b1.x - 4.0f, b1.y ), true );
		Text( ImVec2( b0.x + 5.0f, b0.y + 4.0f ), Col( 30, 30, 30 ), nm );
		ImGui::PopClipRect();
		bool changed = false;
		ImGui::SetCursorScreenPos( b0 );
		ImGui::InvisibleButton( id, ImVec2( b1.x - b0.x, b1.y - b0.y ) );
		bool clk = ImGui::IsItemClicked();

		if( clk && s_pKeyCap != key )
		{
			s_pKeyCap = key;
			s_bKeyWaitRel = true;
		}
		else if( s_pKeyCap == key )
		{
			ImGuiIO& io = ImGui::GetIO();

			if( s_bKeyWaitRel )
			{
				if( !io.MouseDown[ 0 ] )
					s_bKeyWaitRel = false;
			}
			else
			{
				for( int i = 0; i < 124; i++ )
				{
					if( io.KeysDown[ i ] )
					{
						*key = i;
						changed = true;
						s_pKeyCap = nullptr;
						break;
					}
				}

				if( s_pKeyCap )
				{
					if( ImGui::IsMouseClicked( 0 ) )
						s_pKeyCap = nullptr;
					else if( ImGui::IsMouseClicked( 1 ) )
					{
						*key = 2;
						changed = true;
						s_pKeyCap = nullptr;
					}
					else if( ImGui::IsMouseClicked( 2 ) )
					{
						*key = 4;
						changed = true;
						s_pKeyCap = nullptr;
					}
					else if( ImGui::IsMouseClicked( 3 ) )
					{
						*key = 5;
						changed = true;
						s_pKeyCap = nullptr;
					}
					else if( ImGui::IsMouseClicked( 4 ) )
					{
						*key = 6;
						changed = true;
						s_pKeyCap = nullptr;
					}
				}
			}
		}

		return changed;
	}

	static bool KeyBox( const char* label, int* key )
	{
		float w = g_flContentW;
		ImVec2 p = RowReserve( 26.0f );
		float bx = p.x + w - g_flCtrlW;
		ImGui::PushClipRect( ImVec2( p.x, p.y ), ImVec2( bx - 6.0f, p.y + 26.0f ), true );
		Text( ImVec2( p.x, p.y + 7.0f ), Col( 28, 28, 28 ), label );
		ImGui::PopClipRect();
		ImVec2 b0( bx, p.y + 3.0f ), b1( p.x + w, p.y + 23.0f );
		bool ch = KeyRaw( label, key, b0, b1 );
		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 26.0f ) );
		return ch;
	}

	static bool SliderFloat( const char* label, float* v, float vmin, float vmax, const char* fmt )
	{
		float w = g_flContentW;
		ImVec2 p = RowReserve( 40.0f );
		ImDrawList* d = ImGui::GetWindowDrawList();
		float cx = p.x + w - g_flCtrlW;
		ImGui::PushClipRect( ImVec2( p.x, p.y ), ImVec2( cx - 6.0f, p.y + 24.0f ), true );
		Text( ImVec2( p.x, p.y + 4.0f ), Col( 28, 28, 28 ), label );
		ImGui::PopClipRect();
		ImVec2 t0( cx, p.y + 4.0f ), t1( cx + g_flCtrlW, p.y + 16.0f );

		if( *v < vmin )
			*v = vmin;

		if( *v > vmax )
			*v = vmax;

		float f = ( vmax > vmin ) ? ( *v - vmin ) / ( vmax - vmin ) : 0.0f;
		d->AddRectFilled( t0, t1, Col( 255, 255, 255 ) );
		float fx = t0.x + f * ( t1.x - t0.x );

		if( fx > t0.x + 1.0f )
			d->AddRectFilled( ImVec2( t0.x + 1.0f, t0.y + 1.0f ), ImVec2( fx, t1.y - 1.0f ), Col( 193, 25, 8 ) );

		d->AddRect( t0, t1, Col( 165, 165, 165 ) );
		float hx = fx;

		if( hx < t0.x + 5.0f )
			hx = t0.x + 5.0f;

		if( hx > t1.x - 5.0f )
			hx = t1.x - 5.0f;

		d->AddRectFilled( ImVec2( hx - 5.0f, t0.y - 2.0f ), ImVec2( hx + 5.0f, t1.y + 2.0f ), Col( 255, 255, 255 ) );
		d->AddRect( ImVec2( hx - 5.0f, t0.y - 2.0f ), ImVec2( hx + 5.0f, t1.y + 2.0f ), Col( 150, 150, 150 ) );
		char buf[ 32 ];
		sprintf_s( buf, sizeof( buf ), fmt, *v );
		TextCentered( cx, cx + g_flCtrlW, p.y + 24.0f, Col( 66, 66, 66 ), buf );
		bool changed = false;
		ImGui::SetCursorScreenPos( ImVec2( t0.x, t0.y - 2.0f ) );
		ImGui::InvisibleButton( label, ImVec2( t1.x - t0.x, 16.0f ) );

		if( ImGui::IsItemActive() && ImGui::GetIO().MouseDown[ 0 ] )
		{
			float t = ( ImGui::GetIO().MousePos.x - t0.x ) / ( t1.x - t0.x );

			if( t < 0.0f )
				t = 0.0f;

			if( t > 1.0f )
				t = 1.0f;

			float nv = vmin + t * ( vmax - vmin );

			if( nv != *v )
			{
				*v = nv;
				changed = true;
			}
		}

		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 40.0f ) );
		return changed;
	}

	static bool SliderInt( const char* label, int* v, int vmin, int vmax, const char* fmt )
	{
		const char* ff = ( fmt[ 0 ] == '%' && fmt[ 1 ] == 'd' && fmt[ 2 ] == '\0' ) ? XorStr( "%.0f" ) : fmt;
		float f = ( float )*v;
		bool ch = SliderFloat( label, &f, ( float )vmin, ( float )vmax, ff );
		int ni = ( int )( f + ( ( f >= 0.0f ) ? 0.5f : -0.5f ) );

		if( ni < vmin )
			ni = vmin;

		if( ni > vmax )
			ni = vmax;

		if( ni != *v )
		{
			*v = ni;
			ch = true;
		}

		return ch;
	}

	static bool Button( const char* label, float w, float h )
	{
		ImVec2 p = RowReserve( h + 6.0f );
		ImVec2 b0( p.x, p.y ), b1( p.x + w, p.y + h );
		ImDrawList* d = ImGui::GetWindowDrawList();
		d->AddRectFilledMultiColor( b0, b1, Col( 255, 255, 255 ), Col( 255, 255, 255 ), Col( 233, 233, 233 ), Col( 233, 233, 233 ) );
		d->AddRect( b0, b1, Col( 170, 170, 170 ) );
		TextCentered( b0.x, b1.x, b0.y + ( h - 11.0f ) * 0.5f, Col( 45, 45, 45 ), label );
		ImGui::SetCursorScreenPos( b0 );
		ImGui::InvisibleButton( label, ImVec2( w, h ) );
		bool hov = ImGui::IsItemHovered();
		bool act = ImGui::IsItemActive() && ImGui::GetIO().MouseDown[ 0 ];
		bool clk = ImGui::IsItemClicked();

		if( act )
			d->AddRectFilled( b0, b1, Col( 200, 200, 200, 110 ) );
		else if( hov )
			d->AddRectFilled( b0, b1, Col( 255, 255, 255, 70 ) );

		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + h + 6.0f ) );
		return clk;
	}

	static void SubTabs( const char* const* labels, int n, int* v, const ImVec2& p, const ImVec2& size )
	{
		ImDrawList* d = ImGui::GetWindowDrawList();
		float sw = size.x / ( float )n;

		for( int i = 0; i < n; i++ )
		{
			float x0 = p.x + ( float )i * sw;
			float x1 = ( i == n - 1 ) ? p.x + size.x : x0 + sw;

			if( i == *v )
				d->AddRectFilledMultiColor( ImVec2( x0, p.y ), ImVec2( x1, p.y + size.y ), Col( 36, 36, 36 ), Col( 36, 36, 36 ), Col( 58, 58, 58 ), Col( 58, 58, 58 ) );
			else
				d->AddRectFilled( ImVec2( x0, p.y ), ImVec2( x1, p.y + size.y ), Col( 26, 26, 26 ) );

			if( i > 0 )
				d->AddLine( ImVec2( x0, p.y ), ImVec2( x0, p.y + size.y ), Col( 10, 10, 10 ) );

			TextCentered( x0, x1, p.y + ( size.y - 11.0f ) * 0.5f, Col( 255, 255, 255 ), labels[ i ] );
			ImGui::SetCursorScreenPos( ImVec2( x0, p.y ) );
			ImGui::PushID( i );
			ImGui::InvisibleButton( "##seg", ImVec2( x1 - x0, size.y ) );

			if( ImGui::IsItemClicked() )
				*v = i;

			ImGui::PopID();
		}
	}

	static bool ListRow( int idx, const char* text, bool selected )
	{
		float w = g_flContentW;
		ImVec2 p = RowReserve( 22.0f );
		ImDrawList* d = ImGui::GetWindowDrawList();

		if( selected )
			d->AddRectFilled( p, ImVec2( p.x + w, p.y + 22.0f ), Col( 255, 255, 255 ) );

		d->AddLine( ImVec2( p.x, p.y + 21.0f ), ImVec2( p.x + w, p.y + 21.0f ), Col( 208, 208, 208 ) );
		ImGui::PushClipRect( ImVec2( p.x + 6.0f, p.y ), ImVec2( p.x + w - 4.0f, p.y + 22.0f ), true );
		Text( ImVec2( p.x + 6.0f, p.y + 5.0f ), Col( 30, 30, 30 ), text );
		ImGui::PopClipRect();
		ImGui::SetCursorScreenPos( p );
		ImGui::PushID( idx );
		ImGui::InvisibleButton( "##lr", ImVec2( w, 22.0f ) );
		bool clk = ImGui::IsItemClicked();
		ImGui::PopID();
		ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 22.0f ) );
		return clk;
	}
}

const char* AimbotStyleList[ ] =
{
	"Rage",
	"Legit"
};

const char* ModeList[ ] =
{
	"Off",
	"Auto",
	"On Press"
};

const char* SpotList[ ] =
{
	"Pelvis",
	"Left Thigh",
	"Left Calf",
	"Left Foot",
	"Left Toe",
	"Right Thigh",
	"Right Calf",
	"Right Foot",
	"Right Toe",
	"Spine 1",
	"Spine 2",
	"Neck",
	"Head",
	"Left Upper Arm",
	"Left Forearm",
	"Left Hand",
	"Right Upper Arm",
	"Right Forearm",
	"Right Hand"
};

const char* TargetSelectionList[ ] =
{
	"Fast",
	"Distance",
	"FOV"
};

const char* SmoothList[ ] =
{
	"Off",
	"Step",
	"Linear"
};

const char* NoSpreadList[ ] =
{
	"Off",
	"Pitch/Yaw",
	"Perfect"
};

const char* HitScanList[ ] =
{
	"Off",
	"Normal",
	"Extra",
	"Multipoint"
};

const char* AimTargetList[ ] =
{
	"Everyone",
	"Enemy",
	"Friendly"
};

const char* EspTargetList[ ] =
{
	"Everyone",
	"Enemy",
	"Friendly"
};

const char* BoxList[ ] =
{
	"Off",
	"Normal",
	"Corners"
};

const char* InfoTypeList[ ] =
{
	"Off",
	"Text",
	"Bar"
};

const char* SkeletonList[ ] =
{
	"Off",
	"Normal",
	"BackTrack"
};

const char* ChamsModeList[ ] =
{
	"Off",
	"Flat",
	"Shadow",
	"Shadow Flat",
	"Chipolino"
};

const char* ChamsTargetList[ ] =
{
	"Everyone",
	"Enemy",
	"Friendly"
};

const char* AtTargetList[ ] =
{
	"Everyone",
	"Enemy",
	"Friendly"
};

const char* NoEnemyList[ ] =
{
	"Everyone",
	"Enemy",
	"Friendly"
};

const char* PitchStandList[ ] =
{
	"Off",
	"Emotion",
	"FakeUp",
	"Custom",
	"Flip",
	"Switch",
	"Jitter"
};

const char* PitchMoveList[ ] =
{
	"Off",
	"Emotion",
	"FakeUp",
	"Custom",
	"Flip",
	"Switch",
	"Jitter"
};

const char* YawStandList[ ] =
{
	"Off",
	"Backward",
	"Legit",
	"Fake Sideway Left",
	"Fake Sideway Right",
	"Spin",
	"Custom Double Fake",
	"Custom Static Jitter",
	"Custom Jitter",
	"Custom Static",
	"Custom Fake",
	"Custom Static Fake",
	"Fake Spin",
	"Fake Spin 2",
	"Unbalanced"
};

const char* YawMoveList[ ] =
{
	"Off",
	"Backward",
	"Legit",
	"Fake Sideway Left",
	"Fake Sideway Right",
	"Spin",
	"Custom Double Fake",
	"Custom Static Jitter",
	"Custom Jitter",
	"Custom Static",
	"Custom Fake",
	"Custom Static Fake",
	"Fake Spin",
	"Fake Spin 2",
	"Unbalanced"
};

const char* AccuracyList[ ] =
{
	"Normal",
	"Perfect",
	"Seed"
};

const char* CrosshairList[ ] =
{
	"Off",
	"Dot",
	"Cross",
	"Swastika",
	"Small Cross",
	"Default",
	"Aimware"
};

const char* AutoStrafeList[ ] =
{
	"Off",
	"Optimal",
	"Optimal + WASD"
};

const char* RestrictionList[ ] =
{
	"Off",
	"SMAC ULTR@",
	"KAC(disabled)"
};

const char* Crashlist[ ] =
{
	"Off",
	"Mass Disconnect",
	"Crash Server",
	"Custom Packets"
};

const char* Laglist[ ] =
{
	"Off",
	"Classic",
	"Custom Classic",
	"Custom Reversed",
	"Custom Reversed + Classic"
};

const char* ggg[ ] =
{
	"Off",
	"call of duty",
	"skeet",
	"pew"
};

const char* backtracklist[ ] =
{
	"Off",
	"BackTrack",
	"LagFix",
	"Both(LOW FPS)"
};

const char* colorlist[ ] =
{
	"ESP T Not-Visible",
	"ESP T Visible",
	"ESP CT Not-Visible",
	"ESP CT Visible",
	"Chams T Visible",
	"Chams T Not-Visible",
	"Chams CT Visible",
	"Chams CT Not-Visible",
	"Crosshair",
	"Chams Outline"
};

const char* TabList[ ] =
{
	"Legitbot",
	"Ragebot",
	"Visuals",
	"Misc",
	"Colors",
	"GUI",
	"Settings"
};

const char* WpnList[ ] =
{
	"Pistol",
	"SMG",
	"Rifle",
	"Shotgun",
	"Sniper"
};

const char* RageSubList[ ] =
{
	"Weapons",
	"Anti-Aim"
};

const char* VisPartList[ ] =
{
	"Part 1",
	"Part 2"
};

const char* LegitSubList[ ] =
{
	"Aimbot",
	"Extra"
};

const char* HitboxPriorityList[ ] =
{
	"Head",
	"Chest",
	"Stomach",
	"Arms",
	"Legs"
};

const char* HitboxSelectionList[ ] =
{
	"Priority",
	"Nearest"
};

const char* TeamList[ ] =
{
	"None",
	"Spectator",
	"T",
	"CT"
};

const char* PitchModList[ ] =
{
	"Normal",
	"Zero",
	"Up",
	"Down",
	"Auto"
};

const char* YawModList[ ] =
{
	"Normal",
	"Zero",
	"Reversed",
	"Forward",
	"Backward",
	"Sideway Left",
	"Sideway Right",
	"Auto",
	"Resolver"
};

std::string GetExtension( const std::string& target )
{
	auto found = target.find_last_of( "." );
	std::string extension( "" );

	if( found != std::string::npos )
		extension = target.substr( found + 1, target.length() );

	return extension;
}

namespace Feature
{
	Menu::Menu()
		:	m_bMouse( false ),
			m_iWeaponAimbot( 0 ),
			m_iWeaponTriggerbot( 0 ),
			m_iRageClass( 0 ),
			m_iLegitClass( 0 ),
			m_iRageSub( 0 ),
			m_iLegitSub( 0 ),
			m_iVisPart( 0 ),
			m_iConfig( -1 ),
			m_bAimbot( false ),
			m_bTriggerbot( false ),
			m_bESP( false ),
			m_bAntiAim( false ),
			m_bRemovals( false ),
			m_bMisc( false ),
			m_bPlayers( false ),
			m_bColors( false ),
			m_bBinds( false ),
			m_bConfig( false )
	{
		ZeroMemory( m_szConfigName, sizeof( m_szConfigName ) );
	}

	Menu::~Menu()
	{
		Source::m_pCvar->FindVar( XorStr( "cl_mouseenable" ) )->m_nValue = 1;
		ImGui_ImplDX9_Shutdown();
	}

	bool Menu::Create( HWND hWnd, IDirect3DDevice9* pDevice )
	{
		if( !ImGui_ImplDX9_Init( hWnd, pDevice ) )
			return false;

		ImGuiIO& io = ImGui::GetIO();
		bad = io.Fonts->AddFontFromFileTTF( XorStr( "C:\\\\Windows\\\\Fonts\\\\badcache.ttf" ), 22.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );
		und = io.Fonts->AddFontFromFileTTF( XorStr( "C:\\\\Windows\\\\Fonts\\\\toma.ttf" ), 12.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );
		def = io.Fonts->AddFontFromFileTTF( XorStr( u8"C:\\\\Windows\\\\Fonts\\\\tahoma.ttf" ), 14.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );
		def1 = io.Fonts->AddFontFromFileTTF( XorStr( u8"C:\\\\Windows\\\\Fonts\\\\tahoma.ttf" ), 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );
		fntBody = io.Fonts->AddFontFromFileTTF( XorStr( "C:\\\\Windows\\\\Fonts\\\\verdana.ttf" ), 11.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );
		fntTitle = io.Fonts->AddFontFromFileTTF( XorStr( "C:\\\\Windows\\\\Fonts\\\\verdanab.ttf" ), 11.0f, NULL, io.Fonts->GetGlyphRangesCyrillic() );

		if( tImage == nullptr )
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &pic, sizeof( pic ), 96, 96, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tImage );

		if( tIcons[ 0 ] == nullptr )
		{
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_legit, sizeof( aw_tab_legit ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 0 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_rage, sizeof( aw_tab_rage ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 1 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_visuals, sizeof( aw_tab_visuals ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 2 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_misc, sizeof( aw_tab_misc ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 3 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_colors, sizeof( aw_tab_colors ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 4 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_gui, sizeof( aw_tab_gui ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 5 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_tab_settings, sizeof( aw_tab_settings ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 6 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_wpn_pistol, sizeof( aw_wpn_pistol ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 7 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_wpn_smg, sizeof( aw_wpn_smg ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 8 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_wpn_rifle, sizeof( aw_wpn_rifle ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 9 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_wpn_shotgun, sizeof( aw_wpn_shotgun ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 10 ] );
			D3DXCreateTextureFromFileInMemoryEx( pDevice, &aw_wpn_sniper, sizeof( aw_wpn_sniper ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tIcons[ 11 ] );
		}

		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ ImGuiCol_WindowBg ] = ImColor( 150, 40, 0, 255 );
		style.Colors[ ImGuiCol_Border ] = ImColor( 150, 40, 0, 255 );
		style.Colors[ ImGuiCol_Text ] = ImColor( 30, 30, 30, 255 );
		style.WindowRounding = 0.f;
		style.FramePadding = ImVec2( 4, 0 );
		style.WindowPadding = ImVec2( 0, 0 );
		style.ItemSpacing = ImVec2( 0, 0 );
		style.ScrollbarSize = 10.f;
		style.ScrollbarRounding = 0.f;
		style.GrabMinSize = 5.f;
		SetColors();
		return true;
	}

	void Menu::OnPresentDevice()
	{
		if( m_bMouse != Shared::m_bMenu )
		{
			m_bMouse = Shared::m_bMenu;
			Source::m_pCvar->FindVar( XorStr( "cl_mouseenable" ) )->m_nValue = ( int )!m_bMouse;
		}

		if( !Shared::m_bMenu )
			return;

		ImGui_ImplDX9_NewFrame();
		ImGui::GetIO().MouseDrawCursor = true;
		ImGuiWindowFlags Flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos( ImVec2( ( io.DisplaySize.x - mainmenu1 ) * 0.5f, ( io.DisplaySize.y - mainmenu2 ) * 0.5f ) );
		ImGui::SetNextWindowSize( ImVec2( mainmenu1, mainmenu2 ) );
		ImGui::Begin( XorStr( "mainmenu" ), NULL, ImVec2( mainmenu1, mainmenu2 ), 1.f, Flags );
		{
			ImGui::PushFont( fntBody );
			ImVec2 pos = ImGui::GetWindowPos();
			ImDrawList* d = ImGui::GetWindowDrawList();
			d->AddRectFilledMultiColor( pos, ImVec2( pos.x + 810.0f, pos.y + 670.0f ), AW::Col( 218, 72, 3 ), AW::Col( 201, 71, 0 ), AW::Col( 150, 42, 0 ), AW::Col( 165, 48, 0 ) );
			d->AddRectFilled( ImVec2( pos.x + 10.0f, pos.y + 67.0f ), ImVec2( pos.x + 800.0f, pos.y + 658.0f ), AW::Col( 238, 238, 238 ) );
			d->AddLine( ImVec2( pos.x + 10.0f, pos.y + 67.0f ), ImVec2( pos.x + 10.0f, pos.y + 658.0f ), AW::Col( 255, 190, 140 ) );
			d->AddLine( ImVec2( pos.x + 799.0f, pos.y + 67.0f ), ImVec2( pos.x + 799.0f, pos.y + 658.0f ), AW::Col( 255, 190, 140 ) );
			d->AddLine( ImVec2( pos.x + 10.0f, pos.y + 657.0f ), ImVec2( pos.x + 800.0f, pos.y + 657.0f ), AW::Col( 255, 190, 140 ) );
			ImGui::PushFont( fntTitle );
			d->AddText( ImVec2( pos.x + 15.0f, pos.y + 6.0f ), AW::Col( 130, 25, 5 ), XorStr( "ARESWARE for Counter-Strike: Source" ) );
			d->AddText( ImVec2( pos.x + 14.0f, pos.y + 5.0f ), AW::Col( 255, 255, 255 ), XorStr( "ARESWARE for Counter-Strike: Source" ) );
			ImGui::PopFont();
			float tabW = 790.0f / 7.0f;

			for( int i = 0; i < 7; i++ )
			{
				float x0 = pos.x + 10.0f + ( float )i * tabW;
				float x1 = ( i == 6 ) ? pos.x + 800.0f : x0 + tabW;
				ImVec2 t0( x0, pos.y + 22.0f ), t1( x1, pos.y + 67.0f );

				if( i == iTab )
					d->AddRectFilledMultiColor( t0, t1, AW::Col( 30, 30, 30 ), AW::Col( 30, 30, 30 ), AW::Col( 62, 62, 62 ), AW::Col( 62, 62, 62 ) );
				else
					d->AddRectFilledMultiColor( t0, t1, AW::Col( 49, 49, 49 ), AW::Col( 49, 49, 49 ), AW::Col( 22, 22, 22 ), AW::Col( 22, 22, 22 ) );

				float cx = ( x0 + x1 ) * 0.5f;

				if( Config::Misc->icons && tIcons[ i ] )
				{
					float iw = ( float )AW::kTabIconW[ i ], ih = ( float )AW::kTabIconH[ i ];
					d->AddImage( ( ImTextureID )tIcons[ i ], ImVec2( cx - iw * 0.5f, t0.y + 4.0f ), ImVec2( cx + iw * 0.5f, t0.y + 4.0f + ih ) );
					AW::TextCentered( x0, x1, t0.y + 26.0f, AW::Col( 255, 255, 255 ), TabList[ i ] );
				}
				else
				{
					AW::TextCentered( x0, x1, t0.y + 16.0f, AW::Col( 255, 255, 255 ), TabList[ i ] );
				}

				ImGui::SetCursorScreenPos( t0 );
				ImGui::PushID( 100 + i );
				ImGui::InvisibleButton( "##tab", ImVec2( x1 - x0, 45.0f ) );

				if( ImGui::IsItemClicked() )
					iTab = i;

				ImGui::PopID();
			}

			ImGui::PushID( iTab );

			if( iTab == 0 )
				DrawLegitTab();
			else if( iTab == 1 )
				DrawRageTab();
			else if( iTab == 2 )
				DrawVisualsTab();
			else if( iTab == 3 )
				DrawMiscTab();
			else if( iTab == 4 )
				DrawColorsTab();
			else if( iTab == 5 )
				DrawGUITab();
			else if( iTab == 6 )
				DrawSettingsTab();

			ImGui::PopID();
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::Render();
	}

	bool Menu::OnKeyEvent( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		if( !Shared::m_bMenu )
			return false;

		if( ImGui_ImplDX9_WndProcHandler( hWnd, message, wParam, lParam ) )
			return true;

		return false;
	}

	void Menu::OnLostDevice()
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}

	void Menu::OnResetDevice()
	{
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	void Menu::DrawAimbotBlock( Config::AimbotList* aim, bool bGlobal )
	{
		AW::Combo( XorStr( "Mode" ), &aim->Mode, ModeList, ARRAYSIZE( ModeList ) );

		if( aim->Mode == 2 )
			AW::KeyBox( XorStr( "Key" ), &aim->Key );

		AW::Checkbox( XorStr( "Auto Fire" ), &aim->AutoFire );
		AW::Checkbox( XorStr( "Auto Stop" ), &aim->AutoStop );
		AW::Checkbox( XorStr( "Auto Crouch" ), &aim->AutoCrouch );
		AW::Checkbox( XorStr( "Auto Reload" ), &aim->AutoReload );
		AW::Checkbox( XorStr( "Auto Scope" ), &aim->AutoScope );
		AW::Checkbox( XorStr( "Anti Spawn-Protection" ), &aim->AntiSpawnProtection );
		AW::Checkbox( XorStr( "No Switch" ), &aim->NoSwitch );
		AW::Checkbox( XorStr( "Resolver" ), &aim->Resolver );

		if( aim->Resolver )
		{
			AW::SliderInt( XorStr( "Resolver Bullet" ), &aim->ResvolerBullets, 1, 7, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "Resolver Grace Shots" ), &aim->ResvolerBulletsDelay, 2, 8, XorStr( "%d" ) );
			AW::SliderFloat( XorStr( "Resolver Add Y" ), &Config::Misc->ResolverAng, 0.0f, 180.0f, XorStr( "%.1f" ) );
			AW::Checkbox( XorStr( "Resolver Pitch" ), &aim->ResolverPitch );
			AW::Checkbox( XorStr( "Resolver Log" ), &Config::Misc->ResolverLog );
		}

		AW::Combo( XorStr( "Spot" ), &aim->Spot, SpotList, ARRAYSIZE( SpotList ) );
		AW::Checkbox( XorStr( "Randomize" ), &aim->SpotRandomize );
		AW::Checkbox( XorStr( "Height" ), &aim->Height );

		if( aim->Height )
		{
			AW::SliderFloat( XorStr( "Height Scale" ), &aim->HeightScale, -5.0f, 5.0f, XorStr( "%.2f" ) );

			if( bGlobal )
			{
				AW::SliderFloat( XorStr( "X Scale" ), &aim->HeightScaleX, -15.0f, 15.0f, XorStr( "%.1f" ) );
				AW::SliderFloat( XorStr( "Y Scale" ), &aim->HeightScaleY, -15.0f, 15.0f, XorStr( "%.1f" ) );
			}
		}

		AW::Combo( XorStr( "Target Selection" ), &aim->TargetSelection, TargetSelectionList, ARRAYSIZE( TargetSelectionList ) );

		if( aim->TargetSelection == 2 )
			AW::SliderFloat( XorStr( "Field Of View" ), &aim->FieldOfView, 0.0f, 180.0f, XorStr( "%.1f" ) );

		AW::Combo( XorStr( "Smooth" ), &aim->Smooth, SmoothList, ARRAYSIZE( SmoothList ) );

		if( aim->Smooth == 1 )
		{
			AW::SliderFloat( XorStr( "Vertical" ), &aim->StepX, 0.0f, 100.0f, XorStr( "%.1f" ) );
			AW::SliderFloat( XorStr( "Horizontal" ), &aim->StepY, 0.0f, 100.0f, XorStr( "%.1f" ) );
		}
		else if( aim->Smooth == 2 )
		{
			AW::SliderFloat( XorStr( "Vertical" ), &aim->SmoothX, 0.0f, 100.0f, XorStr( "%.1f" ) );
			AW::SliderFloat( XorStr( "Horizontal" ), &aim->SmoothY, 0.0f, 100.0f, XorStr( "%.1f" ) );
		}

		AW::SliderInt( XorStr( "Duration" ), &aim->Duration, 0, 5000, XorStr( "%d" ) );
		AW::SliderInt( XorStr( "Delay" ), &aim->Delay, 0, 5000, XorStr( "%d" ) );
		AW::SliderInt( XorStr( "Switch Delay" ), &aim->SwitchDelay, 0, 5000, XorStr( "%d" ) );
		AW::Checkbox( XorStr( "RCS Active" ), &aim->RCS );
		AW::Checkbox( XorStr( "NoSpread Active" ), &aim->NoSpreadActive );

		if( aim->RCS )
		{
			AW::SliderInt( XorStr( "RCS Delay" ), &aim->RCSDelay, 0, 10, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "RCS Amount X" ), &aim->RCSAmountX, 0, 100, XorStr( "%.0f%%" ) );
			AW::SliderInt( XorStr( "RCS Amount Y" ), &aim->RCSAmountY, 0, 100, XorStr( "%.0f%%" ) );
		}

		if( aim->NoSpreadActive )
			AW::Combo( XorStr( "No Spread" ), &aim->NoSpread, NoSpreadList, ARRAYSIZE( NoSpreadList ) );

		AW::Checkbox( XorStr( "Auto Wall" ), &aim->AutoWall );

		if( aim->AutoWall )
		{
			AW::SliderInt( XorStr( "Min Damage" ), &aim->MinDamage, 0, 100, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "Min Damage Override" ), &aim->MinDamageOverride, 0, 100, XorStr( "%d" ) );
			AW::KeyBox( XorStr( "Override Key" ), &aim->MinDamageOverrideKey );
		}

		AW::Combo( XorStr( "Hit Scan" ), &aim->HitScan, HitScanList, ARRAYSIZE( HitScanList ) );

			if( aim->HitScan == 2 || aim->HitScan == 3 )
				AW::SliderFloat( XorStr( "Scale" ), &aim->HitScanScale, 0.0f, 1.0f, XorStr( "%.2f" ) );

		AW::Combo( XorStr( "Target" ), &aim->Target, AimTargetList, ARRAYSIZE( AimTargetList ) );

		if( Config::Misc->Restriction != 1 )
		{
			AW::Checkbox( XorStr( "Perfect Silent" ), &aim->Silent );

			if( bGlobal )
			{
				AW::Combo( XorStr( "Adjustment" ), &aim->LagCompensation, backtracklist, ARRAYSIZE( backtracklist ) );
				AW::Checkbox( XorStr( "Adjustment Only Last Tick" ), &aim->LastTick );
				AW::Checkbox( XorStr( "Update Anim" ), &aim->UpdateAnim );
				AW::Checkbox( XorStr( "Update Abs" ), &aim->SetAbs );
			}
		}

		aim->Clamp();
	}

	void Menu::DrawTriggerBlock( Config::TriggerbotList* trigger )
	{
		AW::Combo( XorStr( "Mode" ), &trigger->Mode, ModeList, ARRAYSIZE( ModeList ) );

		if( trigger->Mode == 2 )
			AW::KeyBox( XorStr( "Key" ), &trigger->Key );

		AW::Combo( XorStr( "Accuracy" ), &trigger->Accuracy, AccuracyList, ARRAYSIZE( AccuracyList ) );
		AW::SliderInt( XorStr( "Delay" ), &trigger->Delay, 0, 5000, XorStr( "%d" ) );
		AW::SliderInt( XorStr( "Burst" ), &trigger->Burst, 0, 10, XorStr( "%d" ) );
		AW::Checkbox( XorStr( "Auto Wall" ), &trigger->AutoWall );

		if( trigger->AutoWall )
			AW::SliderInt( XorStr( "Min Damage" ), &trigger->MinDamage, 0, 100, XorStr( "%d" ) );

		AW::Checkbox( XorStr( "Through Smoke" ), &trigger->ThroughSmoke );
		AW::Combo( XorStr( "Target" ), &trigger->Target, AimTargetList, ARRAYSIZE( AimTargetList ) );
		trigger->Clamp();
	}

	void Menu::DrawRageTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::BeginPanel( XorStr( "Aimbot" ), ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 380.0f, 582.0f ) );
		AW::Combo( XorStr( "Style" ), &Config::Main->AimbotStyle, AimbotStyleList, ARRAYSIZE( AimbotStyleList ) );

		if( Config::Main->AimbotStyle == 1 )
			AW::Notice( XorStr( "Rage inactive: Legit style selected" ) );

		DrawAimbotBlock( Config::Main->Aimbot, true );
		AW::EndPanel();

		if( m_iRageSub == 0 )
		{
			AW::SubTabs( RageSubList, 2, &m_iRageSub, ImVec2( pos.x + 402.0f, pos.y + 70.0f ), ImVec2( 394.0f, 26.0f ) );
			int prevCls = m_iRageClass;
			ImVec2 bp( pos.x + 402.0f, pos.y + 100.0f );
			ImDrawList* d = ImGui::GetWindowDrawList();
			float sw = 394.0f / 5.0f;

			for( int i = 0; i < 5; i++ )
			{
				float x0 = bp.x + ( float )i * sw;
				float x1 = ( i == 4 ) ? bp.x + 394.0f : x0 + sw;

				if( i == m_iRageClass )
					d->AddRectFilledMultiColor( ImVec2( x0, bp.y ), ImVec2( x1, bp.y + 48.0f ), AW::Col( 36, 36, 36 ), AW::Col( 36, 36, 36 ), AW::Col( 58, 58, 58 ), AW::Col( 58, 58, 58 ) );
				else
					d->AddRectFilled( ImVec2( x0, bp.y ), ImVec2( x1, bp.y + 48.0f ), AW::Col( 26, 26, 26 ) );

				if( i > 0 )
					d->AddLine( ImVec2( x0, bp.y ), ImVec2( x0, bp.y + 48.0f ), AW::Col( 10, 10, 10 ) );

				float cx = ( x0 + x1 ) * 0.5f;

				if( tIcons[ 7 + i ] )
				{
					float iw = ( float )AW::kWpnIconW[ i ], ih = ( float )AW::kWpnIconH[ i ];
					d->AddImage( ( ImTextureID )tIcons[ 7 + i ], ImVec2( cx - iw * 0.5f, bp.y + 6.0f ), ImVec2( cx + iw * 0.5f, bp.y + 6.0f + ih ) );
				}

				AW::TextCentered( x0, x1, bp.y + 29.0f, AW::Col( 255, 255, 255 ), WpnList[ i ] );
				ImGui::SetCursorScreenPos( ImVec2( x0, bp.y ) );
				ImGui::PushID( 200 + i );
				ImGui::InvisibleButton( "##wpn", ImVec2( x1 - x0, 48.0f ) );

				if( ImGui::IsItemClicked() )
					m_iRageClass = i;

				ImGui::PopID();
			}

			if( m_iRageClass != prevCls )
				m_iWeaponAimbot = AW::ClassWeapon( m_iRageClass, 0 );

			if( m_iWeaponAimbot < 0 || m_iWeaponAimbot > 23 )
				m_iWeaponAimbot = 0;

			int clsCount = AW::kClsCount[ m_iRageClass ];
			char title[ 64 ];
			sprintf_s( title, sizeof( title ), XorStr( "%s Settings" ), Config::WeaponList[ m_iWeaponAimbot ] );
			AW::BeginPanel( title, ImVec2( pos.x + 402.0f, pos.y + 152.0f ), ImVec2( 394.0f, 500.0f ) );
			AW::Checkbox( XorStr( "Weapon Config" ), &Config::Main->AimbotWeaponConfig );
			const char* names[ 9 ];

			for( int j = 0; j < clsCount; j++ )
				names[ j ] = Config::WeaponList[ AW::ClassWeapon( m_iRageClass, j ) ];

			int j = AW::ClassIndex( m_iRageClass, m_iWeaponAimbot );
			AW::Combo( XorStr( "Weapon" ), &j, names, clsCount );
			m_iWeaponAimbot = AW::ClassWeapon( m_iRageClass, j );
			CSWeaponID wid = Config::GetWeaponID( Config::WeaponList[ m_iWeaponAimbot ] );
			DrawAimbotBlock( Config::Weapon[ wid ]->Aimbot, false );
			AW::EndPanel();
		}
		else
		{
			AW::SubTabs( RageSubList, 2, &m_iRageSub, ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 782.0f, 26.0f ) );

			if( Config::Misc->Restriction != 1 )
			{
				Config::AntiAimList* aa = Config::AntiAim;
				AW::BeginPanel( XorStr( "Stand" ), ImVec2( pos.x + 14.0f, pos.y + 100.0f ), ImVec2( 380.0f, 268.0f ) );
				AW::Combo( XorStr( "Stand Pitch" ), &aa->PitchStand, PitchStandList, ARRAYSIZE( PitchStandList ) );
				AW::Combo( XorStr( "Stand Yaw" ), &aa->YawStand, YawStandList, ARRAYSIZE( YawStandList ) );
				int ys = aa->YawStand;
				int ps = aa->PitchStand;

				if( ys == 2 || ys == 3 || ys == 4 || ys == 6 || ys == 10 || ys == 11 || ys == 12 || ys == 13 || ys == 14 )
					AW::SliderInt( XorStr( "Stand Choked Packets" ), &aa->StandChokedPackets, 0, 15, XorStr( "%d" ) );

				if( ys == 9 )
					AW::SliderFloat( XorStr( "Stand Static Modifier" ), &aa->StandStaticModifer, -180.0f, 180.0f, XorStr( "%.1f" ) );

				if( ys == 5 )
					AW::SliderInt( XorStr( "Stand Spin Speed" ), &aa->StandSpinSpeed, -100, 100, XorStr( "%d" ) );

				if( ys == 12 )
				{
					AW::SliderInt( XorStr( "Stand Custom Fake Spin Speed" ), &aa->StandFakeSpinSpeed, -100, 100, XorStr( "%d" ) );
					AW::SliderFloat( XorStr( "Stand Custom Fake Spin" ), &aa->StandFakeSpinAngle, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( ps == 3 || ps == 5 || ps == 6 )
				{
					AW::SliderFloat( XorStr( "Stand Custom Angle Pitch" ), &aa->StandCustomAnglePitch, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Stand Custom Angle FakePitch" ), &aa->StandCustomAngleFakePitch, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( ps == 5 )
					AW::SliderInt( XorStr( "Stand Switch Delay" ), &aa->StandSwitchPitchDelay, 20, 620, XorStr( "%d" ) );

				if( ys == 7 || ys == 8 || ys == 10 || ys == 11 || ys == 14 )
				{
					AW::SliderFloat( XorStr( "Stand Custom Angle Yaw" ), &aa->StandCustomAngleYaw, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Stand Custom Angle FakeYaw" ), &aa->StandCustomAngleFakeYaw, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( ys == 6 )
				{
					AW::SliderFloat( XorStr( "Stand First Fake" ), &aa->StandCustomAngleFakeYaw1, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Stand Second Fake" ), &aa->StandCustomAngleFakeYaw2, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Stand First Real" ), &aa->StandCustomAngleYaw1, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Stand Second Real" ), &aa->StandCustomAngleYaw2, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				AW::EndPanel();
				AW::BeginPanel( XorStr( "Move" ), ImVec2( pos.x + 14.0f, pos.y + 376.0f ), ImVec2( 380.0f, 276.0f ) );
				AW::Combo( XorStr( "Move Pitch" ), &aa->PitchMove, PitchMoveList, ARRAYSIZE( PitchMoveList ) );
				AW::Combo( XorStr( "Move Yaw" ), &aa->YawMove, YawMoveList, ARRAYSIZE( YawMoveList ) );
				int ym = aa->YawMove;
				int pm = aa->PitchMove;

				if( ym == 2 || ym == 3 || ym == 4 || ym == 6 || ym == 10 || ym == 11 || ym == 12 || ym == 13 || ym == 14 )
					AW::SliderInt( XorStr( "Move Choked Packets" ), &aa->MoveChokedPackets, 0, 15, XorStr( "%d" ) );

				if( ym == 9 )
					AW::SliderFloat( XorStr( "Move Static Modifier" ), &aa->MoveStaticModifer, -180.0f, 180.0f, XorStr( "%.1f" ) );

				if( ym == 5 )
					AW::SliderInt( XorStr( "Move Spin Speed" ), &aa->MoveSpinSpeed, -100, 100, XorStr( "%d" ) );

				if( ym == 12 )
				{
					AW::SliderInt( XorStr( "Move Custom Fake Spin Speed" ), &aa->MoveFakeSpinSpeed, -100, 100, XorStr( "%d" ) );
					AW::SliderFloat( XorStr( "Move Custom Fake Spin" ), &aa->MoveFakeSpinAngle, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( pm == 3 || pm == 5 || pm == 6 )
				{
					AW::SliderFloat( XorStr( "Move Custom Angle Pitch" ), &aa->MoveCustomAnglePitch, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Move Custom Angle FakePitch" ), &aa->MoveCustomAngleFakePitch, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( pm == 5 )
					AW::SliderInt( XorStr( "Move Switch Delay" ), &aa->MoveSwitchPitchDelay, 20, 620, XorStr( "%d" ) );

				if( ym == 7 || ym == 8 || ym == 10 || ym == 11 || ym == 14 )
				{
					AW::SliderFloat( XorStr( "Move Custom Angle Yaw" ), &aa->MoveCustomAngleYaw, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Move Custom Angle FakeYaw" ), &aa->MoveCustomAngleFakeYaw, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				if( ym == 6 )
				{
					AW::SliderFloat( XorStr( "Move First Fake" ), &aa->MoveCustomAngleFakeYaw1, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Move Second Fake" ), &aa->MoveCustomAngleFakeYaw2, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Move First Real" ), &aa->MoveCustomAngleYaw1, -180.0f, 180.0f, XorStr( "%.1f" ) );
					AW::SliderFloat( XorStr( "Move Second Real" ), &aa->MoveCustomAngleYaw2, -180.0f, 180.0f, XorStr( "%.1f" ) );
				}

				AW::EndPanel();
				AW::BeginPanel( XorStr( "General" ), ImVec2( pos.x + 402.0f, pos.y + 100.0f ), ImVec2( 394.0f, 240.0f ) );
				AW::Checkbox( XorStr( "At Target Enabled" ), &aa->AtTargetEnabled );

				if( aa->AtTargetEnabled )
					AW::Combo( XorStr( "At Target" ), &aa->AtTarget, AtTargetList, ARRAYSIZE( AtTargetList ) );

				AW::Checkbox( XorStr( "No Enemy Enabled" ), &aa->NoEnemyEnabled );

				if( aa->NoEnemyEnabled )
					AW::Combo( XorStr( "No Enemy" ), &aa->NoEnemy, NoEnemyList, ARRAYSIZE( NoEnemyList ) );

				AW::Checkbox( XorStr( "On Knife" ), &aa->OnKnife );
				AW::Checkbox( XorStr( "FakeDuck" ), &aa->FakeDuck );
				AW::Checkbox( XorStr( "FakeWalk" ), &aa->FakeWalk );

				if( aa->FakeWalk )
					AW::KeyBox( XorStr( "FakeWalk Key" ), &aa->FakeWalkKey );

				AW::Checkbox( XorStr( "Hit Reactive" ), &aa->HitReactive );
				AW::Checkbox( XorStr( "Break LagComp" ), &aa->BreakLC );
				AW::EndPanel();
				AW::BeginPanel( XorStr( "Players" ), ImVec2( pos.x + 402.0f, pos.y + 348.0f ), ImVec2( 394.0f, 304.0f ) );
				DrawPlayersBlock();
				AW::EndPanel();
				Config::AntiAim->Clamp();
			}
			else
			{
				AW::Text( ImVec2( pos.x + 22.0f, pos.y + 108.0f ), AW::Col( 170, 50, 40 ), XorStr( "Anti-Aim disabled by Restriction" ) );
			}
		}
	}

	void Menu::DrawLegitTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::Text( ImVec2( pos.x + 22.0f, pos.y + 77.0f ), AW::Col( 28, 28, 28 ), XorStr( "Active" ) );
		AW::ComboRaw( "##legitmode", &Config::Legitbot->Mode, ModeList, ARRAYSIZE( ModeList ), ImVec2( pos.x + 90.0f, pos.y + 73.0f ), ImVec2( pos.x + 230.0f, pos.y + 93.0f ) );

		if( Config::Legitbot->Mode == 2 )
			AW::KeyRaw( "##legitkey", &Config::Legitbot->Key, ImVec2( pos.x + 240.0f, pos.y + 73.0f ), ImVec2( pos.x + 360.0f, pos.y + 93.0f ) );

		int prevCls = m_iLegitClass;
		ImVec2 bp( pos.x + 14.0f, pos.y + 100.0f );
		ImDrawList* d = ImGui::GetWindowDrawList();
		float sw = 380.0f / 5.0f;

		for( int i = 0; i < 5; i++ )
		{
			float x0 = bp.x + ( float )i * sw;
			float x1 = ( i == 4 ) ? bp.x + 380.0f : x0 + sw;

			if( i == m_iLegitClass )
				d->AddRectFilledMultiColor( ImVec2( x0, bp.y ), ImVec2( x1, bp.y + 48.0f ), AW::Col( 36, 36, 36 ), AW::Col( 36, 36, 36 ), AW::Col( 58, 58, 58 ), AW::Col( 58, 58, 58 ) );
			else
				d->AddRectFilled( ImVec2( x0, bp.y ), ImVec2( x1, bp.y + 48.0f ), AW::Col( 26, 26, 26 ) );

			if( i > 0 )
				d->AddLine( ImVec2( x0, bp.y ), ImVec2( x0, bp.y + 48.0f ), AW::Col( 10, 10, 10 ) );

			float cx = ( x0 + x1 ) * 0.5f;

			if( tIcons[ 7 + i ] )
			{
				float iw = ( float )AW::kWpnIconW[ i ], ih = ( float )AW::kWpnIconH[ i ];
				d->AddImage( ( ImTextureID )tIcons[ 7 + i ], ImVec2( cx - iw * 0.5f, bp.y + 6.0f ), ImVec2( cx + iw * 0.5f, bp.y + 6.0f + ih ) );
			}

			AW::TextCentered( x0, x1, bp.y + 29.0f, AW::Col( 255, 255, 255 ), WpnList[ i ] );
			ImGui::SetCursorScreenPos( ImVec2( x0, bp.y ) );
			ImGui::PushID( 300 + i );
			ImGui::InvisibleButton( "##wpn", ImVec2( x1 - x0, 48.0f ) );

			if( ImGui::IsItemClicked() )
			{
				m_iLegitClass = i;
				Config::Legitbot = Config::LegitbotClasses[ i ];
			}

			ImGui::PopID();
		}

		if( m_iLegitClass != prevCls )
			m_iWeaponTriggerbot = AW::ClassWeapon( m_iLegitClass, 0 );

		if( m_iWeaponTriggerbot < 0 || m_iWeaponTriggerbot > 23 )
			m_iWeaponTriggerbot = 0;

		Config::LegitbotList* legit = Config::Legitbot;
		AW::BeginPanel( XorStr( "Accuracy" ), ImVec2( pos.x + 14.0f, pos.y + 152.0f ), ImVec2( 380.0f, 270.0f ) );
		AW::Combo( XorStr( "Target Selection" ), &legit->TargetSelection, TargetSelectionList, ARRAYSIZE( TargetSelectionList ) );
		AW::SliderFloat( XorStr( "Field Of View" ), &legit->FieldOfView, 0.0f, 30.0f, XorStr( "%.1f" ) );
		AW::Combo( XorStr( "Smooth" ), &legit->Smooth, SmoothList, ARRAYSIZE( SmoothList ) );

		if( legit->Smooth == 1 )
		{
			AW::SliderFloat( XorStr( "Vertical" ), &legit->StepX, 0.0f, 100.0f, XorStr( "%.1f" ) );
			AW::SliderFloat( XorStr( "Horizontal" ), &legit->StepY, 0.0f, 100.0f, XorStr( "%.1f" ) );
		}
		else if( legit->Smooth == 2 )
		{
			AW::SliderFloat( XorStr( "Vertical" ), &legit->SmoothX, 0.0f, 100.0f, XorStr( "%.1f" ) );
			AW::SliderFloat( XorStr( "Horizontal" ), &legit->SmoothY, 0.0f, 100.0f, XorStr( "%.1f" ) );
		}

		AW::SliderInt( XorStr( "Duration" ), &legit->Duration, 0, 5000, XorStr( "%d" ) );
		AW::SliderInt( XorStr( "Delay" ), &legit->Delay, 0, 5000, XorStr( "%d" ) );
		AW::Checkbox( XorStr( "Backtrack" ), &legit->Backtrack );
		AW::Checkbox( XorStr( "Humanize Delay" ), &legit->HumanizeDelay );
		AW::EndPanel();
		AW::BeginPanel( XorStr( "Target" ), ImVec2( pos.x + 14.0f, pos.y + 426.0f ), ImVec2( 380.0f, 226.0f ) );
		AW::Checkbox( XorStr( "RCS Active" ), &legit->RCS );

		if( legit->RCS )
		{
			AW::SliderInt( XorStr( "RCS Delay" ), &legit->RCSDelay, 0, 10, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "RCS Amount X" ), &legit->RCSAmountX, 0, 100, XorStr( "%.0f%%" ) );
			AW::SliderInt( XorStr( "RCS Amount Y" ), &legit->RCSAmountY, 0, 100, XorStr( "%.0f%%" ) );
			AW::Checkbox( XorStr( "Standalone RCS" ), &legit->RCSStandalone );
		}

		AW::Combo( XorStr( "Target" ), &legit->Target, AimTargetList, ARRAYSIZE( AimTargetList ) );
		AW::Checkbox( XorStr( "Flash Check" ), &legit->FlashCheck );
		AW::Checkbox( XorStr( "Auto Scope" ), &legit->AutoScope );
		AW::EndPanel();
		Config::Legitbot->Clamp();
		AW::SubTabs( LegitSubList, 2, &m_iLegitSub, ImVec2( pos.x + 402.0f, pos.y + 70.0f ), ImVec2( 394.0f, 26.0f ) );

		if( m_iLegitSub == 0 )
		{
			AW::BeginPanel( XorStr( "Aimbot" ), ImVec2( pos.x + 402.0f, pos.y + 100.0f ), ImVec2( 394.0f, 140.0f ) );
			AW::Combo( XorStr( "Style" ), &Config::Main->AimbotStyle, AimbotStyleList, ARRAYSIZE( AimbotStyleList ) );
	
			if( Config::Main->AimbotStyle == 0 )
				AW::Notice( XorStr( "Legit inactive: Rage style selected" ) );
	
			AW::Checkbox( XorStr( "Auto Fire" ), &legit->AutoFire );
			AW::Checkbox( XorStr( "Auto Stop" ), &legit->AutoStop );
			AW::EndPanel();
			int clsCount = AW::kClsCount[ m_iLegitClass ];
			CSWeaponID twid = Config::GetWeaponID( Config::WeaponList[ m_iWeaponTriggerbot ] );
			Config::TriggerbotList* shown = Config::Main->TriggerbotWeaponConfig ? Config::Weapon[ twid ]->Triggerbot : Config::Main->Triggerbot;
			AW::BeginPanel( XorStr( "Triggerbot" ), ImVec2( pos.x + 402.0f, pos.y + 248.0f ), ImVec2( 394.0f, 258.0f ) );
			AW::Checkbox( XorStr( "Weapon Config" ), &Config::Main->TriggerbotWeaponConfig );
			const char* names[ 9 ];
	
			for( int j = 0; j < clsCount; j++ )
				names[ j ] = Config::WeaponList[ AW::ClassWeapon( m_iLegitClass, j ) ];
	
			int j = AW::ClassIndex( m_iLegitClass, m_iWeaponTriggerbot );
			AW::Combo( XorStr( "Weapon" ), &j, names, clsCount );
			m_iWeaponTriggerbot = AW::ClassWeapon( m_iLegitClass, j );
			twid = Config::GetWeaponID( Config::WeaponList[ m_iWeaponTriggerbot ] );
			shown = Config::Main->TriggerbotWeaponConfig ? Config::Weapon[ twid ]->Triggerbot : Config::Main->Triggerbot;
			DrawTriggerBlock( shown );
			AW::EndPanel();
			AW::BeginPanel( XorStr( "Filter" ), ImVec2( pos.x + 402.0f, pos.y + 510.0f ), ImVec2( 394.0f, 142.0f ) );
			AW::Checkbox( XorStr( "Head" ), &shown->Head );
			AW::Checkbox( XorStr( "Chest" ), &shown->Chest );
			AW::Checkbox( XorStr( "Stomach" ), &shown->Stomach );
			AW::Checkbox( XorStr( "Arms" ), &shown->Arms );
			AW::Checkbox( XorStr( "Legs" ), &shown->Legs );
			AW::EndPanel();
		}
		else
		{
			AW::BeginPanel( XorStr( "Hitboxes" ), ImVec2( pos.x + 402.0f, pos.y + 100.0f ), ImVec2( 394.0f, 232.0f ) );
			AW::Checkbox( XorStr( "Head" ), &legit->ZoneHead );
			AW::Checkbox( XorStr( "Chest" ), &legit->ZoneChest );
			AW::Checkbox( XorStr( "Stomach" ), &legit->ZoneStomach );
			AW::Checkbox( XorStr( "Arms" ), &legit->ZoneArms );
			AW::Checkbox( XorStr( "Legs" ), &legit->ZoneLegs );
			AW::Combo( XorStr( "Hitbox Priority" ), &legit->HitboxPriority, HitboxPriorityList, ARRAYSIZE( HitboxPriorityList ) );
			AW::Combo( XorStr( "Hitbox Selection" ), &legit->HitboxSelection, HitboxSelectionList, ARRAYSIZE( HitboxSelectionList ) );
			AW::EndPanel();
			AW::BeginPanel( XorStr( "Humanize" ), ImVec2( pos.x + 402.0f, pos.y + 340.0f ), ImVec2( 394.0f, 312.0f ) );
			AW::SliderFloat( XorStr( "Randomize" ), &legit->Randomize, 0.0f, 10.0f, XorStr( "%.1f" ) );
			AW::SliderFloat( XorStr( "Curve" ), &legit->Curve, 0.0f, 1.0f, XorStr( "%.2f" ) );
			AW::SliderInt( XorStr( "Switch Delay" ), &legit->TSD, 0, 5000, XorStr( "%d" ) );
			AW::Checkbox( XorStr( "Through Smoke" ), &legit->ThroughSmoke );
			AW::Checkbox( XorStr( "Auto Wall" ), &legit->AutoWall );

			if( legit->AutoWall )
				AW::SliderInt( XorStr( "Min Damage" ), &legit->MinDamage, 0, 100, XorStr( "%d" ) );

			AW::KeyBox( XorStr( "Toggle Key" ), &legit->ToggleKey );
			AW::Checkbox( XorStr( "Fire on Key" ), &legit->FireOnKey );

			if( legit->FireOnKey )
				AW::KeyBox( XorStr( "Fire Key" ), &legit->FireKey );

			AW::EndPanel();
		}
	}

	void Menu::DrawVisualsTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::g_flContentX = pos.x + 22.0f;
		AW::g_flContentW = 200.0f;
		AW::g_flCtrlW = 120.0f;
		ImGui::SetCursorScreenPos( ImVec2( pos.x + 22.0f, pos.y + 70.0f ) );
		bool act = Config::ESP->Box != 0;

		if( AW::Checkbox( XorStr( "Active" ), &act ) )
			Config::ESP->Box = act ? 1 : 0;

		AW::SubTabs( VisPartList, 2, &m_iVisPart, ImVec2( pos.x + 14.0f, pos.y + 100.0f ), ImVec2( 782.0f, 26.0f ) );

		if( m_iVisPart == 0 )
		{
			AW::BeginPanel( XorStr( "Options" ), ImVec2( pos.x + 14.0f, pos.y + 130.0f ), ImVec2( 255.0f, 522.0f ), 110.0f );
			AW::Combo( XorStr( "Box" ), &Config::ESP->Box, BoxList, ARRAYSIZE( BoxList ) );
			AW::Checkbox( XorStr( "Outlined" ), &Config::ESP->Outlined );
			AW::Checkbox( XorStr( "Filled" ), &Config::ESP->Filled );
			AW::Checkbox( XorStr( "Name" ), &Config::ESP->Name );
			AW::Checkbox( XorStr( "Weapon" ), &Config::ESP->Weapon );
			AW::Checkbox( XorStr( "Aim Spot" ), &Config::ESP->AimSpot );
			AW::EndPanel();
			AW::BeginPanel( XorStr( "Information" ), ImVec2( pos.x + 273.0f, pos.y + 130.0f ), ImVec2( 255.0f, 522.0f ), 110.0f );
			AW::Combo( XorStr( "Health" ), &Config::ESP->Health, InfoTypeList, ARRAYSIZE( InfoTypeList ) );
			AW::Combo( XorStr( "Armor" ), &Config::ESP->Armor, InfoTypeList, ARRAYSIZE( InfoTypeList ) );
			AW::Combo( XorStr( "Skeleton" ), &Config::ESP->Skeleton, SkeletonList, ARRAYSIZE( SkeletonList ) );
			AW::Checkbox( XorStr( "Defusing" ), &Config::ESP->Defusing );
			AW::Checkbox( XorStr( "Bomb" ), &Config::ESP->Bomb );
			AW::EndPanel();
			AW::BeginPanel( XorStr( "Other" ), ImVec2( pos.x + 532.0f, pos.y + 130.0f ), ImVec2( 264.0f, 522.0f ), 110.0f );
			AW::Checkbox( XorStr( "Draw Fov" ), &Config::ESP->Fov );
			AW::Checkbox( XorStr( "Draw Spread" ), &Config::ESP->Spread );
			AW::Combo( XorStr( "Esp Target" ), &Config::ESP->Target, EspTargetList, ARRAYSIZE( EspTargetList ) );
			AW::Checkbox( XorStr( "Esp V. color" ), &Config::ESP->Colored );
			AW::EndPanel();
		}
		else
		{
			AW::BeginPanel( XorStr( "Chams" ), ImVec2( pos.x + 14.0f, pos.y + 130.0f ), ImVec2( 255.0f, 522.0f ), 110.0f );
			AW::Combo( XorStr( "Mode" ), &Config::Render->ChamsMode, ChamsModeList, ARRAYSIZE( ChamsModeList ) );

			if( Config::Render->ChamsMode != 0 )
			{
				AW::Checkbox( XorStr( "Chams V. color" ), &Config::Render->ChamsColored );
				AW::Checkbox( XorStr( "Chams outlined" ), &Config::Render->ChamsOutlined );
				AW::Checkbox( XorStr( "Chams V. only" ), &Config::Render->ChamsVisOnly );
				AW::Combo( XorStr( "Chams Target" ), &Config::Render->ChamsTarget, ChamsTargetList, ARRAYSIZE( ChamsTargetList ) );
			}

			AW::EndPanel();
			AW::BeginPanel( XorStr( "Crosshair" ), ImVec2( pos.x + 273.0f, pos.y + 130.0f ), ImVec2( 255.0f, 522.0f ), 110.0f );
			AW::Combo( XorStr( "Crosshair" ), &Config::Misc->Crosshair, CrosshairList, ARRAYSIZE( CrosshairList ) );

			if( Config::Misc->Crosshair )
			{
				if( Config::Misc->Crosshair != 6 )
					AW::Checkbox( XorStr( "Outlined" ), &Config::Misc->Outlined );

				AW::Checkbox( XorStr( "Show Recoil" ), &Config::Misc->ShowRecoil );
			}

			AW::EndPanel();
			AW::BeginPanel( XorStr( "Hitmarker" ), ImVec2( pos.x + 532.0f, pos.y + 130.0f ), ImVec2( 264.0f, 522.0f ), 110.0f );

			if( Config::Misc->Restriction != 1 )
			{
				AW::Checkbox( XorStr( "Hitmarker" ), &Config::Misc->HitmarkerEnabled );
				AW::Checkbox( XorStr( "Hitmarker Damage" ), &Config::Misc->HitmarkerHP );
				AW::Combo( XorStr( "Hitsound" ), &Config::Misc->Hitmarker, ggg, ARRAYSIZE( ggg ) );
			}
			else
			{
				AW::Notice( XorStr( "Disabled by Restriction" ) );
			}

			AW::EndPanel();
		}
	}

	void Menu::DrawMiscTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::BeginPanel( XorStr( "General" ), ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 380.0f, 270.0f ) );

		if( Config::Misc->Restriction != 1 )
		{
			AW::Checkbox( XorStr( "ThirdPerson" ), &Config::Misc->Lag );

			if( Config::Misc->Lag )
				AW::KeyBox( XorStr( "Thirdperson Key" ), &Config::Misc->LagKey );

			AW::Checkbox( XorStr( "BunnyHop" ), &Config::Misc->AutoJump );
			AW::Checkbox( XorStr( "Auto Pistol" ), &Config::Misc->AutoPistol );

			if( Config::Misc->AutoPistol )
				AW::SliderInt( XorStr( "Refire Delay" ), &Config::Misc->AutoPistolDelay, 0, 500, XorStr( "%d ms" ) );
			AW::Combo( XorStr( "Auto Strafe" ), &Config::Misc->AutoStrafe, AutoStrafeList, ARRAYSIZE( AutoStrafeList ) );
		}

		AW::Checkbox( XorStr( "Bomb Warning" ), &Config::Misc->BombWarning );
		AW::SliderInt( XorStr( "Fake Ping" ), &Config::Misc->FakePing, 0, 260, XorStr( "%d" ) );
		AW::EndPanel();
		AW::BeginPanel( XorStr( "Recorder" ), ImVec2( pos.x + 14.0f, pos.y + 344.0f ), ImVec2( 380.0f, 160.0f ) );
		AW::Checkbox( XorStr( "Movement Recorder" ), &Config::Misc->Recorder );

		if( Config::Misc->Recorder )
		{
			AW::Checkbox( XorStr( "Movement Recorder Silent" ), &Config::Misc->RecorderSilent );
			AW::KeyBox( XorStr( "Movement Recorder Record Key" ), &Config::Misc->RecorderRecKey );
			AW::KeyBox( XorStr( "Movement Recorder Play Key" ), &Config::Misc->RecorderPlayKey );
		}

		AW::EndPanel();
		AW::BeginPanel( XorStr( "Restriction" ), ImVec2( pos.x + 14.0f, pos.y + 508.0f ), ImVec2( 380.0f, 180.0f ) );
		AW::Combo( XorStr( "Restriction" ), &Config::Misc->Restriction, RestrictionList, ARRAYSIZE( RestrictionList ) );
		AW::Checkbox( XorStr( "Anti SMAC" ), &Config::Misc->AntiSMAC );
		AW::EndPanel();
		AW::BeginPanel( XorStr( "Effects" ), ImVec2( pos.x + 402.0f, pos.y + 70.0f ), ImVec2( 394.0f, 300.0f ) );
		AW::Checkbox( XorStr( "Fake Lag" ), &Config::Misc->FakeLag );

		if( Config::Misc->FakeLag )
			AW::SliderInt( XorStr( "Amount" ), &Config::Misc->ChokedPackets, 1, 32, XorStr( "%d" ) );

		if( Config::Misc->Restriction != 1 )
		{
			AW::Checkbox( XorStr( "Air Stuck" ), &Config::Misc->AirStuck );

			if( Config::Misc->AirStuck )
				AW::KeyBox( XorStr( "Stuck Key" ), &Config::Misc->StuckKey );
		}

		AW::Checkbox( XorStr( "Circle Strafer" ), &Config::Misc->Speed );

		if( Config::Misc->Speed )
		{
			AW::KeyBox( XorStr( "Circle Key" ), &Config::Misc->SpeedKey );
			AW::SliderFloat( XorStr( "Circle Modifier" ), &Config::Misc->SpeedMod, 1.0f, 7.0f, XorStr( "%.2f" ) );
		}

		AW::Checkbox( XorStr( "No Recoil" ), &Config::Removals->NoRecoil );

		if( Config::Misc->Restriction != 1 )
			AW::Checkbox( XorStr( "No Visual Recoil" ), &Config::Removals->NoVisualRecoil );

		AW::Checkbox( XorStr( "No Smoke" ), &Config::Removals->NoSmoke );
		AW::SliderInt( XorStr( "Flash Amount" ), &Config::Removals->FlashAmount, 0, 100, XorStr( "%.0f%%" ) );
		AW::EndPanel();
		AW::BeginPanel( XorStr( "Exploits" ), ImVec2( pos.x + 402.0f, pos.y + 374.0f ), ImVec2( 394.0f, 278.0f ) );
		AW::Combo( XorStr( "Exploit" ), &Config::Misc->Crash, Crashlist, ARRAYSIZE( Crashlist ) );

		if( Config::Misc->Crash != 0 )
		{
			AW::KeyBox( XorStr( "Exploits Key" ), &Config::Misc->CrashKey );
			AW::SliderInt( XorStr( "Exploits Restriction" ), &Config::Misc->CrashRestricion, 0, 10000, XorStr( "%d" ) );
		}

		if( Config::Misc->Crash == 3 )
		{
			AW::SliderInt( XorStr( "Bytes" ), &Config::Misc->Val0, -1, 1, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "Packet0" ), &Config::Misc->Val1, 0, 100, XorStr( "%d" ) );
			AW::SliderInt( XorStr( "Packet1" ), &Config::Misc->Val2, 0, 100, XorStr( "%d" ) );
		}

		AW::Combo( XorStr( "Lag Exploit" ), &Config::Misc->LagExploit, Laglist, ARRAYSIZE( Laglist ) );

		if( Config::Misc->LagExploit != 0 )
		{
			AW::KeyBox( XorStr( "Lag Exploit Key" ), &Config::Misc->LagExploitKey );
			AW::Checkbox( XorStr( "Lag Exploit Speedhack" ), &Config::Misc->LagExploitSpeed );

			if( Config::Misc->LagExploitSpeed )
				AW::KeyBox( XorStr( "Lag Exploit Speedhack Key" ), &Config::Misc->LagExploitSpeedKey );

			AW::Checkbox( XorStr( "Instant Switch" ), &Config::Misc->LagExploitSwitch );

			if( Config::Misc->LagExploit == 1 || Config::Misc->LagExploit == 2 || Config::Misc->LagExploit == 3 )
			{
				AW::SliderInt( XorStr( "Custom value" ), &Config::Misc->test, 0, 4000, XorStr( "%d" ) );
				AW::SliderInt( XorStr( "Custom value2" ), &Config::Misc->test0, 0, 4000, XorStr( "%d" ) );
			}
		}

		AW::EndPanel();
		Config::Misc->Clamp();
	}

	void Menu::DrawColorsTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::BeginPanel( XorStr( "Colors" ), ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 300.0f, 582.0f ) );

		for( int i = 0; i < 10; i++ )
		{
			if( AW::ListRow( i, colorlist[ i ], Config::Colors->curr == i ) )
				Config::Colors->curr = i;
		}

		AW::EndPanel();
		AW::BeginPanel( XorStr( "Color" ), ImVec2( pos.x + 322.0f, pos.y + 70.0f ), ImVec2( 474.0f, 582.0f ) );
		int curr = Config::Colors->curr;

		if( curr < 0 || curr > 9 )
		{
			curr = 0;
			Config::Colors->curr = 0;
		}

		float cx = AW::g_flContentX;
		float cy = ImGui::GetCursorScreenPos().y;
		ImGui::SetCursorScreenPos( ImVec2( cx, cy ) );

		if( ImGui::ColorPicker( m_flColors[ curr ] ) )
			ApplyColors();

		float* col = m_flColors[ curr ];
		ImDrawList* d = ImGui::GetWindowDrawList();
		ImVec2 p0( cx + 252.0f, cy ), p1( cx + 372.0f, cy + 60.0f );
		d->AddRectFilled( p0, p1, AW::Col( ( int )( col[ 0 ] * 255.0f ), ( int )( col[ 1 ] * 255.0f ), ( int )( col[ 2 ] * 255.0f ) ) );
		d->AddRect( p0, p1, AW::Col( 150, 150, 150 ) );
		AW::Text( ImVec2( cx + 252.0f, cy + 66.0f ), AW::Col( 28, 28, 28 ), XorStr( "Preview" ) );
		char rgba[ 64 ];
		sprintf_s( rgba, sizeof( rgba ), XorStr( "R %d G %d B %d" ), ( int )( col[ 0 ] * 255.0f ), ( int )( col[ 1 ] * 255.0f ), ( int )( col[ 2 ] * 255.0f ) );
		AW::Text( ImVec2( cx + 252.0f, cy + 82.0f ), AW::Col( 66, 66, 66 ), rgba );
		ImGui::SetCursorScreenPos( ImVec2( cx, cy + 216.0f ) );

		if( AW::Button( XorStr( "Reset" ), 130.0f, 30.0f ) )
			ResetColors();

		AW::EndPanel();
	}

	void Menu::DrawGUITab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::BeginPanel( XorStr( "Menu" ), ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 380.0f, 140.0f ) );
		AW::KeyBox( XorStr( "Menu" ), &Config::Binds->Menu );
		AW::KeyBox( XorStr( "Eject" ), &Config::Binds->Eject );
		AW::KeyBox( XorStr( "Panic" ), &Config::Binds->Panic );
		AW::Checkbox( XorStr( "Icons" ), &Config::Misc->icons );
		AW::EndPanel();
	}

	void Menu::DrawSettingsTab()
	{
		ImVec2 pos = ImGui::GetWindowPos();
		AW::BeginPanel( XorStr( "Configuration" ), ImVec2( pos.x + 14.0f, pos.y + 70.0f ), ImVec2( 782.0f, 400.0f ) );
		float cx = AW::g_flContentX;
		float cy = ImGui::GetCursorScreenPos().y;
		WIN32_FIND_DATAA ffd;
		LARGE_INTEGER filesize;
		CHAR szDir[ MAX_PATH ];
		HANDLE hFind = INVALID_HANDLE_VALUE;
		std::string strPath = Config::GetPath();
		strncpy_s( szDir, strPath.c_str(), MAX_PATH );
		strncat_s( szDir, "*", MAX_PATH );
		hFind = FindFirstFileA( szDir, &ffd );
		std::vector< std::string > files;
		int index = 0;
		AW::g_flContentX = cx;
		AW::g_flContentW = 300.0f;
		ImGui::SetCursorScreenPos( ImVec2( cx, cy ) );

		if( hFind != INVALID_HANDLE_VALUE )
		{
			do
			{
				auto ext = GetExtension( ffd.cFileName );

				if( !( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ext.compare( "cfg" ) == 0 )
				{
					filesize.LowPart = ffd.nFileSizeLow;
					filesize.HighPart = ffd.nFileSizeHigh;
					char szText[ MAX_PATH ];
					sprintf_s( szText, sizeof( szText ), XorStr( "%s [%lld bytes]" ), ffd.cFileName, filesize.QuadPart );

					if( AW::ListRow( index, szText, index == m_iConfig ) )
					{
						m_iConfig = index;
						strncpy_s( m_szConfigName, ffd.cFileName, sizeof( m_szConfigName ) );

						if( ImGui::IsMouseDoubleClicked( 0 ) )
						{
							Shared::m_strConfig = files[ m_iConfig ];
							Shared::m_bLoad = true;
						}
					}

					index++;
					files.push_back( ffd.cFileName );
				}
			}
			while( FindNextFileA( hFind, &ffd ) != 0 );

			FindClose( hFind );
		}

		if( m_iConfig >= index )
			m_iConfig = -1;

		float bx = cx + 330.0f;
		ImGui::SetCursorScreenPos( ImVec2( bx, cy + 2.0f ) );
		ImGui::PushItemWidth( 200.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );
		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 1, 1, 1, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.12f, 0.12f, 0.12f, 1 ) );
		ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.65f, 0.65f, 0.65f, 1 ) );
		ImGui::InputText( "##cfgname", m_szConfigName, sizeof( m_szConfigName ) );
		ImGui::PopStyleColor( 3 );
		ImGui::PopStyleVar();
		ImGui::PopItemWidth();
		AW::g_flContentX = bx;
		AW::g_flContentW = 200.0f;
		ImGui::SetCursorScreenPos( ImVec2( bx, cy + 32.0f ) );

		if( AW::Button( XorStr( "Save" ), 200.0f, 30.0f ) && !std::string( m_szConfigName ).empty() )
		{
			Shared::m_strConfig = m_szConfigName;
			Shared::m_bSave = true;
		}

		ImGui::SetCursorScreenPos( ImVec2( bx, cy + 68.0f ) );

		if( AW::Button( XorStr( "Load" ), 200.0f, 30.0f ) && m_iConfig != -1 )
		{
			Shared::m_strConfig = files[ m_iConfig ];
			Shared::m_bLoad = true;
		}

		ImGui::SetCursorScreenPos( ImVec2( bx, cy + 104.0f ) );

		if( AW::Button( XorStr( "Delete" ), 200.0f, 30.0f ) && m_iConfig != -1 )
			Config::Delete( m_szConfigName );

		char count[ 64 ];
		sprintf_s( count, sizeof( count ), XorStr( "%d configs" ), index );
		AW::Text( ImVec2( bx, cy + 146.0f ), AW::Col( 110, 110, 110 ), count );
		AW::Text( ImVec2( bx, cy + 162.0f ), AW::Col( 110, 110, 110 ), XorStr( "Double-click a config to load" ) );
		AW::EndPanel();
		AW::BeginPanel( XorStr( "Misc" ), ImVec2( pos.x + 14.0f, pos.y + 478.0f ), ImVec2( 380.0f, 174.0f ) );

		if( AW::Button( XorStr( "Unload Cheat" ), 200.0f, 32.0f ) )
			Shared::m_bEject = true;

		AW::EndPanel();
	}

	void Menu::DrawPlayersBlock()
	{
		auto local = C_CSPlayer::GetLocalPlayer();

		if( !local )
		{
			AW::Notice( XorStr( "No players" ) );
			return;
		}

		int size = Source::m_pEngine->GetMaxClients();

		for( int i = 0; i <= size; i++ )
		{
			auto player = ToCSPlayer( Source::m_pEntList->GetBaseEntity( i ) );

			if( !player )
				continue;

			auto player_from_list = Source::m_pPlayerList->GetPlayer( i );

			if( !player_from_list )
				continue;

			int team = player->m_iTeamNum();

			if( team > 3 )
				team = 0;

			player_info_t data;

			if( !Source::m_pEngine->GetPlayerInfo( i, &data ) )
				continue;

			ImVec2 p = AW::RowReserve( 48.0f );
			ImDrawList* d = ImGui::GetWindowDrawList();
			d->AddLine( ImVec2( p.x, p.y + 47.0f ), ImVec2( p.x + AW::g_flContentW, p.y + 47.0f ), AW::Col( 208, 208, 208 ) );
			ImGui::PushClipRect( ImVec2( p.x, p.y ), ImVec2( p.x + 165.0f, p.y + 22.0f ), true );
			AW::Text( ImVec2( p.x, p.y + 3.0f ), AW::Col( 28, 28, 28 ), data.name );
			ImGui::PopClipRect();
			AW::Text( ImVec2( p.x + 175.0f, p.y + 3.0f ), AW::Col( 66, 66, 66 ), TeamList[ team ] );
			ImGui::PushClipRect( ImVec2( p.x + 225.0f, p.y ), ImVec2( p.x + AW::g_flContentW, p.y + 22.0f ), true );
			AW::Text( ImVec2( p.x + 225.0f, p.y + 3.0f ), AW::Col( 66, 66, 66 ), data.guid );
			ImGui::PopClipRect();
			AW::Text( ImVec2( p.x, p.y + 28.0f ), AW::Col( 28, 28, 28 ), XorStr( "Pitch" ) );
			AW::Text( ImVec2( p.x + 190.0f, p.y + 28.0f ), AW::Col( 28, 28, 28 ), XorStr( "Yaw" ) );
			ImGui::PushID( i );
			AW::ComboRaw( "##p", &player_from_list->m_pitch, PitchModList, ARRAYSIZE( PitchModList ), ImVec2( p.x + 40.0f, p.y + 25.0f ), ImVec2( p.x + 180.0f, p.y + 45.0f ) );
			AW::ComboRaw( "##yw", &player_from_list->m_yaw, YawModList, ARRAYSIZE( YawModList ), ImVec2( p.x + 225.0f, p.y + 25.0f ), ImVec2( p.x + AW::g_flContentW, p.y + 45.0f ) );
			ImGui::PopID();
			ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + 48.0f ) );
		}
	}

	void Menu::SetColors()
	{
		for( int i = 0; i < 4; i++ )
		{
			m_flColors[ COL_T_ESP_NORMAL ][ i ]		= Config::Colors->T_ESP_Normal[ i ] / 255.0f;
			m_flColors[ COL_T_ESP_COLORED ][ i ]	= Config::Colors->T_ESP_Colored[ i ] / 255.0f;
			m_flColors[ COL_T_CHAMS_NORMAL ][ i ]	= Config::Colors->T_Chams_Normal[ i ] / 255.0f;
			m_flColors[ COL_T_CHAMS_COLORED ][ i ]	= Config::Colors->T_Chams_Colored[ i ] / 255.0f;
			m_flColors[ COL_CT_ESP_NORMAL ][ i ]	= Config::Colors->CT_ESP_Normal[ i ] / 255.0f;
			m_flColors[ COL_CT_ESP_COLORED ][ i ]	= Config::Colors->CT_ESP_Colored[ i ] / 255.0f;
			m_flColors[ COL_CT_CHAMS_NORMAL ][ i ]	= Config::Colors->CT_Chams_Normal[ i ] / 255.0f;
			m_flColors[ COL_CT_CHAMS_COLORED ][ i ]	= Config::Colors->CT_Chams_Colored[ i ] / 255.0f;
			m_flColors[ COL_CROSSHAIR ][ i ] = Config::Colors->Crosshair[ i ] / 255.0f;
			m_flColors[ COL_CHAMSOUTLINEDC ][ i ] = Config::Colors->ChamsOutlinedC[ i ] / 255.0f;
		}
	}

	void Menu::ApplyColors()
	{
		for( int i = 0; i < 4; i++ )
		{
			Config::Colors->T_ESP_Normal[ i ]		= ( int )( m_flColors[ COL_T_ESP_NORMAL ][ i ] * 255.0f );
			Config::Colors->T_ESP_Colored[ i ]		= ( int )( m_flColors[ COL_T_ESP_COLORED ][ i ] * 255.0f );
			Config::Colors->T_Chams_Normal[ i ]		= ( int )( m_flColors[ COL_T_CHAMS_NORMAL ][ i ] * 255.0f );
			Config::Colors->T_Chams_Colored[ i ]	= ( int )( m_flColors[ COL_T_CHAMS_COLORED ][ i ] * 255.0f );
			Config::Colors->CT_ESP_Normal[ i ]		= ( int )( m_flColors[ COL_CT_ESP_NORMAL ][ i ] * 255.0f );
			Config::Colors->CT_ESP_Colored[ i ]		= ( int )( m_flColors[ COL_CT_ESP_COLORED ][ i ] * 255.0f );
			Config::Colors->CT_Chams_Normal[ i ]	= ( int )( m_flColors[ COL_CT_CHAMS_NORMAL ][ i ] * 255.0f );
			Config::Colors->CT_Chams_Colored[ i ]	= ( int )( m_flColors[ COL_CT_CHAMS_COLORED ][ i ] * 255.0f );
			Config::Colors->Crosshair[ i ] = ( int )( m_flColors[ COL_CROSSHAIR ][ i ] * 255.0f );
			Config::Colors->ChamsOutlinedC[ i ] = ( int )( m_flColors[ COL_CHAMSOUTLINEDC ][ i ] * 255.0f );
		}
	}

	void Menu::ResetColors()
	{
		Config::Colors->T_ESP_Normal.R = 255;
		Config::Colors->T_ESP_Normal.G = 0;
		Config::Colors->T_ESP_Normal.B = 0;
		Config::Colors->T_ESP_Normal.A = 255;
		Config::Colors->T_ESP_Colored.R = 255;
		Config::Colors->T_ESP_Colored.G = 255;
		Config::Colors->T_ESP_Colored.B = 0;
		Config::Colors->T_ESP_Colored.A = 255;
		Config::Colors->T_Chams_Normal.R = 255;
		Config::Colors->T_Chams_Normal.G = 0;
		Config::Colors->T_Chams_Normal.B = 0;
		Config::Colors->T_Chams_Normal.A = 255;
		Config::Colors->T_Chams_Colored.R = 255;
		Config::Colors->T_Chams_Colored.G = 255;
		Config::Colors->T_Chams_Colored.B = 0;
		Config::Colors->T_Chams_Colored.A = 255;
		Config::Colors->CT_ESP_Normal.R = 0;
		Config::Colors->CT_ESP_Normal.G = 128;
		Config::Colors->CT_ESP_Normal.B = 255;
		Config::Colors->CT_ESP_Normal.A = 255;
		Config::Colors->CT_ESP_Colored.R = 0;
		Config::Colors->CT_ESP_Colored.G = 255;
		Config::Colors->CT_ESP_Colored.B = 0;
		Config::Colors->CT_ESP_Colored.A = 255;
		Config::Colors->CT_Chams_Normal.R = 0;
		Config::Colors->CT_Chams_Normal.G = 128;
		Config::Colors->CT_Chams_Normal.B = 255;
		Config::Colors->CT_Chams_Normal.A = 255;
		Config::Colors->CT_Chams_Colored.R = 0;
		Config::Colors->CT_Chams_Colored.G = 255;
		Config::Colors->CT_Chams_Colored.B = 0;
		Config::Colors->CT_Chams_Colored.A = 255;
		Config::Colors->Crosshair.R = 0;
		Config::Colors->Crosshair.G = 0;
		Config::Colors->Crosshair.B = 0;
		Config::Colors->Crosshair.A = 255;
		Config::Colors->ChamsOutlinedC.R = 255;
		Config::Colors->ChamsOutlinedC.G = 255;
		Config::Colors->ChamsOutlinedC.B = 255;
		Config::Colors->ChamsOutlinedC.A = 255;
		SetColors();
	}
}
