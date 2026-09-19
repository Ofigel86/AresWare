// BUILD MARKER r15 (2026-09-18): legit AA (real view kept, fake yaw 0 on choked ticks) + skeleton drawn per-segment (no more vanish).
#include "Main.h"
#include <cstdio>
#include <cstdarg>

Drawing g_Drawing;

void Drawing::FontInit( )
{
	static bool Init = false;
	if( !Init )
	{
		Init = true;
		g_Font = g_pSurface->FontCreate( );
		g_MenuFontNormal = g_pSurface->FontCreate( );
		g_MenuFontBold = g_pSurface->FontCreate( );
		g_OldFont = g_pSurface->FontCreate( );
		g_WelcomeFont = g_pSurface->FontCreate( );
		
		g_pSurface->SetFontGlyphSet( g_Font, /*Verdana Bold*/XorStr<0xF2,13,0x2BBD3F55>("\xA4\x96\x86\x91\x97\x99\x99\xD9\xB8\x94\x90\x99"+0x2BBD3F55).s, 12, 400, 0, 0, Valve::vgui::EFontFlags::FONTFLAG_NONE );
		g_pSurface->SetFontGlyphSet( g_MenuFontNormal, /*Verdana Bold*/XorStr<0x0B,13,0x5787A4CC>("\x5D\x69\x7F\x6A\x6E\x7E\x70\x32\x51\x7B\x79\x72"+0x5787A4CC).s, 12, 400, 0, 0, Valve::vgui::EFontFlags::FONTFLAG_ANTIALIAS );
		g_pSurface->SetFontGlyphSet( g_MenuFontBold, /*Calibri*/XorStr<0x86,8,0xAA58CF56>("\xC5\xE6\xE4\xE0\xE8\xF9\xE5"+0xAA58CF56).s, 14, 900, 0, 0, Valve::vgui::EFontFlags::FONTFLAG_ANTIALIAS );
		g_pSurface->SetFontGlyphSet( g_OldFont, /*Arial*/XorStr<0x52,6,0xF1721C60>("\x13\x21\x3D\x34\x3A"+0xF1721C60).s, 12, 400, 0, 0, Valve::vgui::EFontFlags::FONTFLAG_OUTLINE );

		g_pSurface->SetFontGlyphSet( g_WelcomeFont, /*Helvetica Neue*/XorStr<0x62,15,0x0ED93AF5>("\x2A\x06\x08\x13\x03\x13\x01\x0A\x0B\x4B\x22\x08\x1B\x0A"+0x0ED93AF5).s, 92, 400, 0, 0, Valve::vgui::EFontFlags::FONTFLAG_ANTIALIAS );
	}
}

void Drawing::DrawPolygon( int count, Valve::vgui::Vertex_t* Vertexs, Color color )
{
	static int Texture = g_pSurface->CreateNewTextureID( true );
	unsigned char buffer[ 4 ] = { 255, 255, 255, 255 };
	
	g_pSurface->DrawSetTextureRGBA( Texture, buffer, 1, 1 );
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawSetTexture( Texture );

	g_pSurface->DrawTexturedPolygon( count, Vertexs );
}

void Drawing::DrawRectRainbow( int x, int y, int width, int height, float flSpeed, float &flRainbow )
{
	Color colColor( 0, 0, 0 );

	flRainbow += flSpeed;
	if( flRainbow > 1.f ) flRainbow = 0.f;

	for( int i = 0; i < width; i++ )
	{
		float hue = ( 1.f / ( float ) width ) * i;
		hue -= flRainbow;
		if( hue < 0.f ) hue += 1.f;

		Color colRainbow = colColor.FromHSB( hue, 1.f, 1.f );
		FilledRect( x + i, y, 1, height, colRainbow );
	}
}

void Drawing::DrawCircle( int x, int y, int radius, Color color )
{
	int a = radius;
	int b = 0;
	int err = 0;

	while( a >= b )
	{
		FilledRect( x + a, y + b, 1, 1, color );
		FilledRect( x + a, y + b, 1, 1, color );
		FilledRect( x - a, y + b, 1, 1, color );
		FilledRect( x - a, y + b, 1, 1, color );
		FilledRect( x - a, y - b, 1, 1, color );
		FilledRect( x - a, y - b, 1, 1, color );
		FilledRect( x + a, y - b, 1, 1, color );
		FilledRect( x + a, y - b, 1, 1, color );

		
		if( err > 0 )
		{
			a -= 1;
			err -= a * 2 + 1;
		}
		
		if( err <= 0 )
		{
			b += 1;
			err += b * 2 + 1;
		}
	}
}

void Drawing::MenuStringNormal( bool centerx, bool centery, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_MenuFontNormal );
	
	int wide, tall;
	g_pSurface->GetTextSize( g_MenuFontNormal, wbuf, wide, tall );
	g_flMenuFontNormalSize = wide + 2;
	if( centerx ) x -= wide / 2;
	if( centery ) y -= tall / 2;

	g_pSurface->DrawSetTextColor( Color( 0, 0, 0, color.a( ) ) );
	g_pSurface->DrawSetTextPos( x + 1, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( color );
	g_pSurface->DrawSetTextPos( x, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

void Drawing::MenuStringBold( bool centerx, bool centery, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_MenuFontBold );
	
	int wide, tall;
	g_pSurface->GetTextSize( g_MenuFontBold, wbuf, wide, tall );
	g_flMenuFontBoldSize = wide + 2;
	if( centerx ) x -= wide / 2;
	if( centery ) y -= tall / 2;

	g_pSurface->DrawSetTextColor( Color( 0, 0, 0, color.a( ) ) );
	g_pSurface->DrawSetTextPos( x + 1, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( color );
	g_pSurface->DrawSetTextPos( x, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

void Drawing::MenuString2( bool centerx, bool centery, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_Font );
	g_pSurface->DrawSetTextColor( color );

	int wide, tall;
	g_pSurface->GetTextSize( g_Font, wbuf, wide, tall );
	if( centerx ) x -= wide / 2;
	if( centery ) y -= tall / 2;

	g_pSurface->DrawSetTextPos( x, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

void Drawing::String( bool center, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_Font );	

	if( center )
	{
		int wide, tall;
		g_pSurface->GetTextSize( g_Font, wbuf, wide, tall );
		x -= wide / 2;
	}

	Color outline = Color( 0, 0, 0, color.a( ) );

	// === 1337 outline fix for chars ===

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x - 1, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x - 1, y - 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x - 1, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x, y - 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x + 1, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x + 1, y - 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	g_pSurface->DrawSetTextColor( outline );
	g_pSurface->DrawSetTextPos( x + 1, y + 1 );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );

	// ==================================

	g_pSurface->DrawSetTextColor( color );
	g_pSurface->DrawSetTextPos( x, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

void Drawing::StringNormal( bool center, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_Font );
	g_pSurface->DrawSetTextColor( color );

	if( center )
	{
		int wide, tall;
		g_pSurface->GetTextSize( g_Font, wbuf, wide, tall );
		x -= wide / 2;
	}

	g_pSurface->DrawSetTextPos( x, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

void Drawing::BigString( bool center, int x, int y, Color color, const char* input, ... )
{
	va_list va_alist;
	char buf[ 1024 ];
	wchar_t wbuf[ 1024 ];

	va_start( va_alist, input );
	vsprintf( buf, input, va_alist );
	va_end( va_alist );

	MultiByteToWideChar( CP_UTF8, 0, buf, 256, wbuf, 256 );

	g_pSurface->DrawSetTextFont( g_WelcomeFont );
	g_pSurface->DrawSetTextColor( color );

	if( center )
	{
		int wide, tall;
		g_pSurface->GetTextSize( g_WelcomeFont, wbuf, wide, tall );
		x -= wide / 2;
		y -= tall / 2;
	}

	g_pSurface->DrawSetTextPos( x, y );
	g_pSurface->DrawPrintText( wbuf, wcslen( wbuf ), Valve::vgui::FontDrawType_t::FONT_DRAW_DEFAULT );
}

int Drawing::Pulse( int min, int max, float speed )
{
	return min + abs( sin( DEG2RAD( g_pGlobals->curtime * 180 * speed ) ) * ( ( float )max - ( float )min ) );
}

void Drawing::Line( int x0, int y0, int x1, int y1, Color color )
{
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawLine( x0, y0, x1, y1 );
}

void Drawing::FilledRect( int x, int y, int w, int h, Color color )
{
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawFilledRect( x, y, x + w, y + h );
}

void Drawing::OutlinedRect( int x, int y, int w, int h, Color color )
{
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawOutlinedRect( x, y, x + w, y + h );
}

void Drawing::Circle( int x, int y, int radius, int segments, Color color )
{
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawOutlinedCircle( x, y, radius, segments );
}

void Drawing::OutlinedBox( int x, int y, int w, int h, Color color, Color outline )
{
	OutlinedRect( x - 1, y - 1, w + 2, h + 2, outline );
	OutlinedRect( x + 1, y + 1, w - 2, h - 2, outline );
	OutlinedRect( x, y, w, h, color );
}

void Drawing::GradientHorizontal( int x, int y, int w, int h, Color from, Color to, int alpha )
{
	float diff_r, diff_g, diff_b;
	float val_r, val_g, val_b;
	Color color;

	diff_r = from.r( ) - to.r( );
	diff_g = from.g( ) - to.g( );
	diff_b = from.b( ) - to.b( );

	if( from.r( ) > to.r( ) && diff_r < 0 ) diff_r = -diff_r;
	if( from.g( ) > to.g( ) && diff_g < 0 ) diff_g = -diff_g;
	if( from.b( ) > to.b( ) && diff_b < 0 ) diff_b = -diff_b;

	val_r = diff_r / h;
	val_g = diff_g / h;
	val_b = diff_b / h;

	for( int i = 0; i < h; i++ )
	{
		color = Color( 
			from.r( ) - ( val_r * i ),
			from.g( ) - ( val_g * i ), 
			from.b( ) - ( val_b * i ), 
			alpha );
		//Line( x, y + i, x + w, y + i, color );
		FilledRect( x, y + i, w, 1, color );
	}
}

void Drawing::GradientVertical( int x, int y, int w, int h, Color from, Color to, int alpha )
{
	int diff_r, diff_g, diff_b;
	int val_r, val_g, val_b;
	Color color;

	diff_r = from.r( ) - to.r( );
	diff_g = from.g( ) - to.g( );
	diff_b = from.b( ) - to.b( );

	if( from.r( ) > to.r( ) && diff_r < 0 ) diff_r = -diff_r;
	if( from.g( ) > to.g( ) && diff_g < 0 ) diff_g = -diff_g;
	if( from.b( ) > to.b( ) && diff_b < 0 ) diff_b = -diff_b;

	val_r = diff_r / w;
	val_g = diff_g / w;
	val_b = diff_b / w;

	for( int i = 0; i < w; i++ )
	{
		color = Color( 
			from.r( ) - ( val_r * i ),
			from.g( ) - ( val_g * i ), 
			from.b( ) - ( val_b * i ), 
			alpha );
		//Line( x + i, y, x + i, y + h, color );
		FilledRect( x + i, y, 1, h, color );
	}
}

void Drawing::ColoredFilledRect( int x, int y, int w, int h, Color color )
{
	int r = color.r( );
	int g = color.g( );
	int b = color.b( );

	if( r < h ) r = h;
	if( g < h ) g = h;
	if( b < h ) b = h;

	Color fixed = Color( r, g, b );

	Color darker = Color( int( fixed.r( ) * 0.65 ), int( fixed.g( ) * 0.65 ), int( fixed.b( ) * 0.65 ), 255 );
	GradientHorizontal( x, y, w, h, darker, fixed, 255 );
	OutlinedRect( x, y, w, h, fixed );
}

void Drawing::Box3D( Vector* points, Color color )
{
	Vector startPos, endPos, centerPos;

	g_pSurface->DrawSetColor( color );

	for( int i = 0; i < 3; i++ )
	{
		if( g_Stuff.WorldToScreen( points[ i ], startPos ) && g_Stuff.WorldToScreen( points[ i + 1 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	}

	if( g_Stuff.WorldToScreen( points[ 0 ], startPos ) && g_Stuff.WorldToScreen( points[ 3 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );

	for( int i = 4; i < 7; i++ )
	{
		if( g_Stuff.WorldToScreen( points[ i ], startPos ) && g_Stuff.WorldToScreen( points[ i + 1 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	}

	if( g_Stuff.WorldToScreen( points[ 4 ], startPos ) && g_Stuff.WorldToScreen( points[ 7 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	if( g_Stuff.WorldToScreen( points[ 0 ], startPos ) && g_Stuff.WorldToScreen( points[ 6 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	if( g_Stuff.WorldToScreen( points[ 1 ], startPos ) && g_Stuff.WorldToScreen( points[ 5 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	if( g_Stuff.WorldToScreen( points[ 2 ], startPos ) && g_Stuff.WorldToScreen( points[ 4 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	if( g_Stuff.WorldToScreen( points[ 3 ], startPos ) && g_Stuff.WorldToScreen( points[ 7 ], endPos ) ) g_pSurface->DrawLine( startPos.x, startPos.y, endPos.x, endPos.y );
	if( g_Stuff.WorldToScreen( points[ 8 ], centerPos ) ) g_Drawing.FilledRect( centerPos.x, centerPos.y, 1, 1, color );
}

// FPS: per-frame bone cache shared by DrawBones/DrawHitbox/DrawAimSpot.
// One SetupBones per entity per frame instead of one per ESP element.
static matrix3x4_t espBoneMatrix[ 128 ];
static int espBoneEnt = -1, espBoneFrame = -1;
static mstudiohitboxset_t* espBoneSet = nullptr;

static mstudiohitboxset_t* ESP_GetBones( BasePlayer* Ent, matrix3x4_t** outMatrix )
{
	int frame = g_pGlobals->framecount;
	if( Ent->entindex( ) == espBoneEnt && frame == espBoneFrame && espBoneSet )
	{
		*outMatrix = espBoneMatrix;
		return espBoneSet;
	}
	espBoneEnt = -1; espBoneSet = nullptr;
	if( !( Ent->SetupBones( espBoneMatrix, 128, 0x100, Ent->m_flSimulationTime( ) ) ) ) return nullptr;
	void* pModel = Ent->GetModel( );
	if( !pModel ) return nullptr;
	studiohdr_t* studiohdr = g_pModelInfo->GetStudiomodel( pModel );
	if( !studiohdr ) return nullptr;
	espBoneSet = studiohdr->pHitboxSet( Ent->m_nHitboxSet( ) );
	if( !espBoneSet ) return nullptr;
	espBoneEnt = Ent->entindex( );
	espBoneFrame = frame;
	*outMatrix = espBoneMatrix;
	return espBoneSet;
}

// Event log: ring buffer of formatted lines with per-line color and timestamp.
struct EventLogEntry
{
	char text[ 192 ];
	int r, g, b;
	float time;
};
static EventLogEntry eventLog[ 12 ];
static int eventLogCount = 0;

void Drawing::AddLog( Color color, const char* fmt, ... )
{
	if( !g_CVars.Visuals.EventLog ) return; // master switch: no screen log, no console spam
	EventLogEntry e;
	va_list va;
	va_start( va, fmt );
	vsnprintf( e.text, sizeof( e.text ), fmt, va );
	va_end( va );
	e.text[ sizeof( e.text ) - 1 ] = 0;
	e.r = color.r( ); e.g = color.g( ); e.b = color.b( );
	e.time = g_pGlobals->curtime;
	eventLog[ eventLogCount % 12 ] = e;
	eventLogCount++;

	// mirror to game console (sanitized: player names may contain ; " etc.)
	char clean[ 192 ];
	int ci = 0;
	for( int i = 0; i < 191 && e.text[ i ] != 0; i++ )
	{
		char c = e.text[ i ];
		clean[ ci++ ] = ( c == '"' || c == ';' || c == '\n' || c == '\r' ) ? ' ' : c;
	}
	clean[ ci ] = 0;
	char cmd[ 256 ];
	snprintf( cmd, sizeof( cmd ), "echo [Awesware] %s", clean );
	cmd[ sizeof( cmd ) - 1 ] = 0;
	g_pEngineClient->ExecuteClientCmd( cmd );
}

void Drawing::DrawEventLog( void )
{
	float now = g_pGlobals->curtime;
	int shown = 0;
	for( int n = eventLogCount - 1; n >= 0 && shown < 8; n-- )
	{
		if( n < eventLogCount - 12 ) break; // slot overwritten by newer line
		EventLogEntry& e = eventLog[ n % 12 ];
		float age = now - e.time;
		if( age < 0.f || age > 5.f ) continue;
		int alpha = ( age < 4.f ) ? 220 : ( int )( 220.f * ( 5.f - age ) );
		int y = ( int )( screen_y * 0.35f ) + shown * 16;
		MenuStringNormal( true, false, 12, y, Color( e.r, e.g, e.b, alpha ), "%s", e.text );
		shown++;
	}
}

static void DrawSkeletonPairs( Vector p[ 19 ], bool ok[ 19 ], Color color )
{
#define SKL( a, b ) if( ok[ a ] && ok[ b ] ) g_Drawing.Line( ( int )p[ a ].x, ( int )p[ a ].y, ( int )p[ b ].x, ( int )p[ b ].y, color )
	SKL( 4, 3 ); SKL( 3, 2 ); SKL( 2, 1 ); SKL( 1, 0 ); // left leg
	SKL( 8, 7 ); SKL( 7, 6 ); SKL( 6, 5 ); SKL( 5, 0 ); // right leg
	SKL( 0, 9 ); SKL( 9, 10 ); SKL( 10, 11 ); SKL( 11, 12 ); // spine to head
	SKL( 11, 16 ); SKL( 16, 17 ); SKL( 17, 18 ); // right arm
	SKL( 11, 13 ); SKL( 13, 14 ); SKL( 14, 15 ); // left arm
#undef SKL
}

void Drawing::DrawBones( BasePlayer* Ent, Color color )
{
	Vector points[ 19 ];
	Vector pointstransformed[ 19 ];
	Vector pointsw2s[ 19 ];
	bool ok[ 19 ];

	matrix3x4_t* espMatrix;
	mstudiohitboxset_t* studiohitboxset = ESP_GetBones( Ent, &espMatrix );
	if( !studiohitboxset ) return;
	matrix3x4_t* matrix = espMatrix;

	for( int i = 0; i <= 18; i++ )
	{
		mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( i );
		if( !studiobbox ) { ok[ i ] = false; continue; } // r15: skip this bone, keep the rest
		points[ i ] = ( studiobbox->bbmax + studiobbox->bbmin ) * .5f;
		VectorTransform( points[ i ], matrix[ studiobbox->bone ], pointstransformed[ i ] );

		ok[ i ] = g_Stuff.WorldToScreen( pointstransformed[ i ], pointsw2s[ i ] ); // r15: per-point (skeleton no longer vanishes when one bone is off-screen)
	}

	DrawSkeletonPairs( pointsw2s, ok, color );
}

void Drawing::DrawBonesRotated( BasePlayer* Ent, Color color, float yawDeg )
{
	// same hitbox centers as DrawBones, rotated around entity origin (fake-angle viz)
	matrix3x4_t* espMatrix;
	mstudiohitboxset_t* set = ESP_GetBones( Ent, &espMatrix );
	if( !set ) return;
	Vector origin = Ent->GetAbsOrigin( );
	float rad = yawDeg * 0.01745329252f;
	float c = cos( rad ), si = sin( rad );
	Vector w2s[ 19 ];
	bool ok[ 19 ];
	for( int i = 0; i <= 18; i++ )
	{
		mstudiobbox_t* b = set->pHitbox( i );
		if( !b ) { ok[ i ] = false; continue; } // r15
		Vector mid = ( b->bbmax + b->bbmin ) * .5f;
		Vector t; VectorTransform( mid, espMatrix[ b->bone ], t );
		t.x -= origin.x; t.y -= origin.y;
		float rx = t.x * c - t.y * si, ry = t.x * si + t.y * c;
		t.x = rx + origin.x; t.y = ry + origin.y;
		ok[ i ] = g_Stuff.WorldToScreen( t, w2s[ i ] ); // r15: per-point
	}
	DrawSkeletonPairs( w2s, ok, color );
}

void Drawing::CornerBox( int x, int y, int w, int h, Color color, Color outline )
{
	int len = ( w < h ) ? w / 4 : h / 4;
	if( len < 4 ) len = 4; if( len > 14 ) len = 14;
	// outline (behind, slightly larger)
	FilledRect( x - 1, y - 1, len + 2, 1, outline ); FilledRect( x - 1, y - 1, 1, len + 2, outline );
	FilledRect( x + w - len - 1, y - 1, len + 2, 1, outline ); FilledRect( x + w, y - 1, 1, len + 2, outline );
	FilledRect( x - 1, y + h - len - 1, 1, len + 2, outline ); FilledRect( x - 1, y + h, len + 2, 1, outline );
	FilledRect( x + w, y + h - len - 1, 1, len + 2, outline ); FilledRect( x + w - len - 1, y + h, len + 2, 1, outline );
	// corners
	FilledRect( x, y, len, 1, color ); FilledRect( x, y, 1, len, color );
	FilledRect( x + w - len, y, len, 1, color ); FilledRect( x + w - 1, y, 1, len, color );
	FilledRect( x, y + h - len, 1, len, color ); FilledRect( x, y + h - 1, len, 1, color );
	FilledRect( x + w - 1, y + h - len, 1, len, color ); FilledRect( x + w - len, y + h - 1, len, 1, color );
}

void Drawing::DrawHitbox( BasePlayer* Ent, int iHitbox, Color color )
{
	matrix3x4_t* espMatrix;
	mstudiohitboxset_t* studiohitboxset = ESP_GetBones( Ent, &espMatrix );
	if( !studiohitboxset ) return;
	matrix3x4_t* matrix = espMatrix;
	mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( iHitbox );
	if( !studiobbox ) return;

	Vector points[ 9 ];

	float scalecenter = g_pGlobals->interval_per_tick;
	points[ 8 ] = ( studiobbox->bbmax + studiobbox->bbmin ) * 0.5f;
	points[ 0 ] = studiobbox->bbmin + points[ 0 ] * scalecenter;

	points[ 1 ].x = studiobbox->bbmin.x + points[ 0 ].x * scalecenter;
	points[ 1 ].y = studiobbox->bbmax.y - points[ 0 ].y * scalecenter;
	points[ 1 ].z = studiobbox->bbmin.z + points[ 0 ].z * scalecenter;

	points[ 2 ].x = studiobbox->bbmax.x - points[ 0 ].x * scalecenter;
	points[ 2 ].y = studiobbox->bbmax.y - points[ 0 ].y * scalecenter;
	points[ 2 ].z = studiobbox->bbmin.z + points[ 0 ].z * scalecenter;

	points[ 3 ].x = studiobbox->bbmax.x - points[ 0 ].x * scalecenter;
	points[ 3 ].y = studiobbox->bbmin.y + points[ 0 ].y * scalecenter;
	points[ 3 ].z = studiobbox->bbmin.z + points[ 0 ].z * scalecenter;

	points[ 4 ] = studiobbox->bbmax - points[ 0 ] * scalecenter;

	points[ 5 ].x = studiobbox->bbmin.x + points[ 0 ].x * scalecenter;
	points[ 5 ].y = studiobbox->bbmax.y - points[ 0 ].y * scalecenter;
	points[ 5 ].z = studiobbox->bbmax.z - points[ 0 ].z * scalecenter;
		      
	points[ 6 ].x = studiobbox->bbmin.x + points[ 0 ].x * scalecenter;
	points[ 6 ].y = studiobbox->bbmin.y + points[ 0 ].y * scalecenter;
	points[ 6 ].z = studiobbox->bbmax.z - points[ 0 ].z * scalecenter;

	points[ 7 ].x = studiobbox->bbmax.x - points[ 0 ].x * scalecenter;
	points[ 7 ].y = studiobbox->bbmin.y + points[ 0 ].y * scalecenter;
	points[ 7 ].z = studiobbox->bbmax.z - points[ 0 ].z * scalecenter;

	Vector pointsTransformed[ 9 ];
	for( int i = 0; i <= 8; i++ ) VectorTransform( points[ i ], matrix[ studiobbox->bone ], pointsTransformed[ i ] );

	Box3D( pointsTransformed, color );
}

void Drawing::DrawAimSpot( BasePlayer* Ent, int iHitbox, Color color )
{
	matrix3x4_t* espMatrix;
	mstudiohitboxset_t* studiohitboxset = ESP_GetBones( Ent, &espMatrix );
	if( !studiohitboxset ) return;
	matrix3x4_t* matrix = espMatrix;
	mstudiobbox_t* studiobbox = studiohitboxset->pHitbox( iHitbox );
	if( !studiobbox ) return;

	float scalecenter = g_pGlobals->interval_per_tick;

	Vector points[ ] = { ( ( studiobbox->bbmin + studiobbox->bbmax ) * .5f ),
		Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmin.z + ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmax.y - ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmin.x + ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ),
	  Vector( studiobbox->bbmax.x - ( studiobbox->bbmax.x * ( 1 - g_CVars.Aimbot.PointScale ) * .5f ), studiobbox->bbmin.y + ( studiobbox->bbmax.y * ( 1 - g_CVars.Aimbot.PointScale ) ), studiobbox->bbmax.z - ( studiobbox->bbmax.z * ( 1 - g_CVars.Aimbot.PointScale ) ) ) };

	Vector pointsTransformed[ 9 ], pts[ 9 ];
	VectorTransform( points[ 0 ], matrix[ studiobbox->bone ], pointsTransformed[ 0 ] );

	if( g_Stuff.WorldToScreen( pointsTransformed[ 0 ], pts[ 0 ] ) )
	{
		g_Drawing.FilledRect( pts[ 0 ].x - 2, pts[ 0 ].y - 2, 5, 5, color );
		g_Drawing.OutlinedRect( pts[ 0 ].x - 2, pts[ 0 ].y - 2, 5, 5, Color( 0, 0, 0, 160 ) );
	}
}