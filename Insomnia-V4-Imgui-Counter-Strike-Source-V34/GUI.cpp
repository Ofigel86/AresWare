// BUILD MARKER r46 (2026-09-19): pixel-perfect icons from the reference shots injected into the font atlas + drawn in the V3 tab bar and weapon strip.
// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r44 (2026-09-19): V3 (AIMWARE) menu skin - orange header, icon tab strip, 8 pages over the same cvars, separate player-list window.
// BUILD MARKER r43 (2026-09-19): legit AA fake is now actually visible - auto-fakelag (Fake Choke Ticks, def 6) + fake goes out on the flush tick too.
// BUILD MARKER r42 (2026-09-19): legit AA merged - fake-on-choke widgets moved into the LEGIT ANTI-AIM section (GUI only, cvars unchanged).
// BUILD MARKER r41 (2026-09-19): slow walk default bind = SHIFT (combo index 6 -> VK 0x10).
// BUILD MARKER r40 (2026-09-19): lag records - per-record anti-jitter resolve (phase prediction + miss brute).
// BUILD MARKER r38 (2026-09-18): slophook directional autostrafe port (wall avoidance + key offsets + 90/100 step).
// BUILD MARKER r36 (2026-09-18): audit-2 fix - Snap-mode legit FOV slider restored (lost in r29 when dual FOV replaced the single slider).
// BUILD MARKER r34 (2026-09-18): ClientMod Emulator user toggle (MISC) + boot-time default.ini load.
// BUILD MARKER r33 (2026-09-18): menu schematic fix (FAKEDUCK block was under the DOUBLE TAP header) + lua 2.4 (cmd get/set_sendpacket, input.cursor).
// BUILD MARKER r31 (2026-09-18): reverted r30 Humanize+ per user request - legit anti-snap back to r29 state. Awaiting user-supplied anti-detect material.
// BUILD MARKER r29 (2026-09-18): legit max - dual FOV (near/far by distance) + visible-only target lock + anti-snap on target switch + dual FOV circles.
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r24 (2026-09-18): ported Fakeduck (deep 12/3 slow cycle) + Micromoves (zero-net micro-jitter, position pinned).
// BUILD MARKER r21 (2026-09-18): Show Fake Pose toggle (pin off on demand) + AIC punished-combo ban + AI resolver recency/soft-ban.
// BUILD MARKER r20 (2026-09-18): removed Yaw 13/14/15 (Jitter Back / Random Back / Fake 0) - Yaw list back to 0-12 (Server Hold last).
// BUILD MARKER r19 (2026-09-18): Freeze Model (ModelZero) feature fully removed.
// BUILD MARKER r17 (2026-09-18): rage Yaw 15 "Fake 0" - fake (choked) always 0 deg, real untouched.
// BUILD MARKER r16 (2026-09-18): Freeze Model (0 deg) - old-school local look: frozen body, free skeleton overlay.
// BUILD MARKER r15 (2026-09-18): legit AA (real view kept, fake yaw 0 on choked ticks) + skeleton drawn per-segment (no more vanish).
// BUILD MARKER r13 (2026-09-18): Aimware V4 menu look (Menu Style combo: Classic | Aimware V4, dark theme, V4 banner/tabs/footer/headers).
// BUILD MARKER r12 (2026-09-18): best-of-256 ForceSeed picker + AutoWall corner points + menu polish (rounding/banner gradient/section accents).
// BUILD MARKER r8 (2026-09-18): aimspot sanitize + Yaw 13/14 fakes + ServerHold menu fix + Honest Shot resolver (bullet_impact).
#include "Main.h"
#include "GUI.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <vector>
#include <string>

GUI g_GUI;

bool GUI::ShouldDisableInput( void )
{
	return bMouse;
}

static void ColorToFloat3( const Color& c, float out[ 3 ] )
{
	out[ 0 ] = c[ 0 ] / 255.0f;
	out[ 1 ] = c[ 1 ] / 255.0f;
	out[ 2 ] = c[ 2 ] / 255.0f;
}

static Color Float3ToColor( const float in[ 3 ] )
{
	return Color(
		( int )( in[ 0 ] * 255.0f ),
		( int )( in[ 1 ] * 255.0f ),
		( int )( in[ 2 ] * 255.0f ),
		255
	);
}

static bool ImGuiColorEdit( const char* label, Color& col )
{
	float clr[ 3 ];
	ColorToFloat3( col, clr );
	if( ImGui::ColorEdit3( label, clr, ImGuiColorEditFlags_NoInputs ) )
	{
		col = Float3ToColor( clr );
		return true;
	}
	return false;
}

static int GetHitboxIndex( int hitbox )
{
	if( hitbox == 12 ) return 0;
	if( hitbox == 11 ) return 1;
	if( hitbox == 10 ) return 2;
	if( hitbox == 9 ) return 3;
	return 0;
}

static int HitboxFromIndex( int idx )
{
	if( idx == 0 ) return 12;
	if( idx == 1 ) return 11;
	if( idx == 2 ) return 10;
	if( idx == 3 ) return 9;
	return 12;
}

// ---- aimware v4 palette ----
static const ImVec4 AW_RED        ( 0.702f, 0.180f, 0.180f, 1.00f ); // checkbox / slider fill / progress
static const ImVec4 AW_RED_BRIGHT ( 0.769f, 0.235f, 0.235f, 1.00f ); // hovers
static const ImVec4 AW_RED_BANNER ( 0.612f, 0.157f, 0.157f, 1.00f ); // top banner base
static const ImVec4 AW_RED_LOGO   ( 0.690f, 0.243f, 0.243f, 1.00f ); // logo block / active tab
static const ImVec4 AW_RED_STRIP  ( 0.541f, 0.137f, 0.137f, 1.00f ); // tab strip background
static const ImVec4 AW_WHITE      ( 1.00f, 1.00f, 1.00f, 1.00f );
static const ImVec4 AW_TEXT       ( 0.13f, 0.13f, 0.13f, 1.00f );
static const ImVec4 AW_TEXT_DIM   ( 0.55f, 0.55f, 0.55f, 1.00f );
static const ImVec4 AW_TRACK      ( 0.79f, 0.79f, 0.79f, 1.00f );
static const ImVec4 AW_FRAME_HOV  ( 0.955f, 0.955f, 0.955f, 1.00f );

// aimware-style section header: plain dark title
static void SectionHeader( const char* label )
{
	if( g_CVars.Miscellaneous.MenuMode == 1 )
	{
		// aimware group label (V4): red bullet + light text + thin red underline
		ImDrawList* dl = ImGui::GetWindowDrawList( );
		ImVec2 p = ImGui::GetCursorScreenPos( );
		float fs = ImGui::GetFontSize( );
		ImVec2 ts = ImGui::CalcTextSize( label );
		dl->AddRectFilled( ImVec2( p.x, p.y + fs * 0.15f ), ImVec2( p.x + 3.0f, p.y + fs * 0.95f ), IM_COL32( 186, 35, 43, 255 ), 0 );
		dl->AddText( ImVec2( p.x + 9.0f, p.y ), IM_COL32( 235, 235, 235, 255 ), label );
		dl->AddRectFilled( ImVec2( p.x + 9.0f, p.y + fs * 1.15f ), ImVec2( p.x + 9.0f + ts.x + 4.0f, p.y + fs * 1.15f + 1.0f ), IM_COL32( 186, 35, 43, 190 ), 0 );
		ImGui::Dummy( ImVec2( 0, fs * 1.4f ) );
		return;
	}
	ImGui::TextColored( AW_TEXT, "%s", label ); // r12: accent underline
	ImVec2 hp = ImGui::GetItemRectMin( );
	float hw = ImGui::GetContentRegionAvail( ).x;
	float hy = ImGui::GetItemRectMax( ).y + 3.0f;
	ImGui::GetWindowDrawList( )->AddRectFilled( ImVec2( hp.x, hy ), ImVec2( hp.x + hw, hy + 2.0f ), ImGui::GetColorU32( ImVec4( AW_RED.x, AW_RED.y, AW_RED.z, AW_RED.w * 0.80f ) ), 1.0f );
	ImGui::Dummy( ImVec2( 0, 4 ) );
}

// ---- aimware-style widgets ----
static void AwCheckbox( const char* label, bool* v )
{
	const bool cbDark = ( g_CVars.Miscellaneous.MenuMode == 1 ); // r13
	ImGui::PushStyleColor( ImGuiCol_FrameBg, *v ? AW_RED : ( cbDark ? ImVec4( 0.16f, 0.16f, 0.17f, 1.00f ) : AW_WHITE ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, *v ? AW_RED_BRIGHT : AW_FRAME_HOV );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, *v ? AW_RED_BRIGHT : ImVec4( 0.90f, 0.90f, 0.90f, 1.00f ) );
	ImGui::PushStyleColor( ImGuiCol_CheckMark, AW_WHITE );
	ImGui::Checkbox( label, v );
	ImGui::PopStyleColor( 4 );
}

static void AwSliderLabel( const char* label, char* out )
{
	int n = 0;
	for( ; label[ n ] && label[ n ] != '#' && n < 90; n++ ) out[ n ] = label[ n ];
	out[ n ] = 0;
	ImGui::TextUnformatted( out );
}

static void AwSliderDraw( float frac, const char* buf )
{
	ImVec2 rmin = ImGui::GetItemRectMin( ), rmax = ImGui::GetItemRectMax( );
	float cy = ( rmin.y + rmax.y ) * 0.5f;
	if( frac < 0.0f ) frac = 0.0f; if( frac > 1.0f ) frac = 1.0f;
	ImDrawList* dl = ImGui::GetWindowDrawList( );
	const bool slDark = ( g_CVars.Miscellaneous.MenuMode == 1 ); // r13
	ImU32 track = ImGui::GetColorU32( slDark ? ImVec4( 0.28f, 0.28f, 0.30f, 1.00f ) : AW_TRACK );
	ImU32 red = ImGui::GetColorU32( AW_RED );
	dl->AddRectFilled( ImVec2( rmin.x, cy - 2 ), ImVec2( rmax.x, cy + 2 ), track, 0 );
	dl->AddRectFilled( ImVec2( rmin.x, cy - 2 ), ImVec2( rmin.x + ( rmax.x - rmin.x ) * frac, cy + 2 ), red, 0 );
	float gx = rmin.x + ( rmax.x - rmin.x ) * frac;
	dl->AddRectFilled( ImVec2( gx - 2, cy - 6 ), ImVec2( gx + 2, cy + 6 ), red, 0 );

	ImVec2 ts = ImGui::CalcTextSize( buf );
	dl->AddText( ImVec2( rmin.x + ( ( rmax.x - rmin.x ) - ts.x ) * 0.5f, rmax.y + 2 ), slDark ? ImGui::GetColorU32( ImVec4( 0.90f, 0.90f, 0.92f, 1.00f ) ) : ImGui::GetColorU32( AW_TEXT ), buf ); // r13
	ImGui::Dummy( ImVec2( 0, ts.y + 3 ) ); // room for the value under the track
}

static void AwSliderFloat( const char* label, float* v, float mn, float mx, const char* fmt = "%.1f" )
{
	char disp[ 96 ]; AwSliderLabel( label, disp );

	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrabActive, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushID( label );
	ImGui::SliderFloat( "##v", v, mn, mx, "" );
	ImGui::PopID( );
	ImGui::PopStyleColor( 5 );

	char buf[ 32 ];
	sprintf( buf, fmt, *v );
	AwSliderDraw( ( mx > mn ) ? ( ( *v - mn ) / ( mx - mn ) ) : 0.0f, buf );
}

static void AwSliderInt( const char* label, int* v, int mn, int mx, const char* fmt = "%d" )
{
	char disp[ 96 ]; AwSliderLabel( label, disp );

	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrabActive, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushID( label );
	ImGui::SliderInt( "##v", v, mn, mx, "" );
	ImGui::PopID( );
	ImGui::PopStyleColor( 5 );

	char buf[ 32 ];
	sprintf( buf, fmt, *v );
	AwSliderDraw( ( mx > mn ) ? ( ( float )( *v - mn ) / ( float )( mx - mn ) ) : 0.0f, buf );
}

// top banner: aimware-style red header with logo block and live status
static void MenuBanner( void )
{
	float w = ImGui::GetContentRegionAvail( ).x;
	ImDrawList* dl = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );
	float h = 52.0f;
	float fs = ImGui::GetFontSize( );
	ImFont* font = ImGui::GetFont( );

	if( g_CVars.Miscellaneous.MenuMode == 2 )
	{
		// r44: AIMWARE V3 header - orange gradient + full product title
		dl->AddRectFilled( p, ImVec2( p.x + w, p.y + 5.0f ), IM_COL32( 195, 58, 0, 255 ), 0 );
		dl->AddRectFilledMultiColor( p, ImVec2( p.x + w, p.y + h ), IM_COL32( 255, 138, 0, 255 ), IM_COL32( 226, 71, 0, 255 ), IM_COL32( 163, 28, 0, 255 ), IM_COL32( 255, 138, 0, 255 ) );
		dl->AddText( font, fs * 1.05f, ImVec2( p.x + 14.0f, p.y + ( h - fs * 1.05f ) * 0.5f ), IM_COL32_WHITE, "Awesware for Counter-Strike: Source V34" );
		ImGui::Dummy( ImVec2( w, h + 2.0f ) );
		return;
	}
	if( g_CVars.Miscellaneous.MenuMode == 1 )
	{
		// aimware v4 look: translucent red header + 4px bright line + soft shadow
		dl->AddRectFilled( p, ImVec2( p.x + w, p.y + h ), IM_COL32( 170, 30, 26, 150 ), 0 );
		dl->AddRectFilled( p, ImVec2( p.x + 200.0f, p.y + h ), ImGui::GetColorU32( AW_RED_LOGO ), 0 );
		dl->AddRectFilled( ImVec2( p.x, p.y + h ), ImVec2( p.x + w, p.y + h + 4.0f ), IM_COL32( 186, 35, 43, 255 ), 0 );
		dl->AddRectFilledMultiColor( ImVec2( p.x, p.y + h + 4.0f ), ImVec2( p.x + w, p.y + h + 10.0f ), IM_COL32( 0, 0, 0, 150 ), IM_COL32( 0, 0, 0, 150 ), IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ) );
	}
	else
	{
		dl->AddRectFilledMultiColor( p, ImVec2( p.x + w, p.y + h ), // r12: gradient instead of flat banner
			ImGui::GetColorU32( AW_RED_BANNER ), ImGui::GetColorU32( ImVec4( 0.33f, 0.085f, 0.085f, 1.00f ) ),
			ImGui::GetColorU32( ImVec4( 0.33f, 0.085f, 0.085f, 1.00f ) ), ImGui::GetColorU32( AW_RED_BANNER ) );
		dl->AddRectFilled( p, ImVec2( p.x + 200.0f, p.y + h ), ImGui::GetColorU32( AW_RED_LOGO ), 0 );
		dl->AddRectFilled( p, ImVec2( p.x + w, p.y + h + 2.0f ), ImGui::GetColorU32( AW_RED_BRIGHT ), 0 ); // r12: bright strip
	}

	dl->AddText( font, fs * 1.55f, ImVec2( p.x + 14.0f, p.y + 6.0f ), IM_COL32_WHITE, "awesware" );
	dl->AddText( font, fs * 0.72f, ImVec2( p.x + 15.0f, p.y + 6.0f + fs * 1.62f ), IM_COL32( 255, 255, 255, 190 ), "ONE STEP AHEAD OF THE GAME" );

	const char* gameLine = "Counter-Strike: Source v34  |  build " __DATE__;
	ImVec2 s2 = ImGui::CalcTextSize( gameLine );
	dl->AddText( font, fs * 0.72f, ImVec2( p.x + w - s2.x * 0.72f - 12.0f, p.y + h - fs * 1.15f ), IM_COL32( 255, 255, 255, 170 ), gameLine );

	char stat[ 64 ];
	sprintf( stat, "choke %s  |  choked: %d", g_NetchanHooked ? "ON" : "off", g_iChokedTicks );
	ImVec2 s1 = ImGui::CalcTextSize( stat );
	dl->AddText( font, fs * 0.80f, ImVec2( p.x + w - s1.x * 0.80f - 12.0f, p.y + 9.0f ), IM_COL32( 255, 255, 255, 220 ), stat );

	ImGui::Dummy( ImVec2( w, h + ( ( g_CVars.Miscellaneous.MenuMode == 1 ) ? 10.0f : 2.0f ) ) );
}

void GUI::SetupStyle( void )
{
	ImGuiStyle& style = ImGui::GetStyle( );
	ImVec4* colors = style.Colors;

	// aimware v4 light theme, red accents
	colors[ ImGuiCol_Text ]                  = AW_TEXT;
	colors[ ImGuiCol_TextDisabled ]          = AW_TEXT_DIM;
	colors[ ImGuiCol_WindowBg ]              = ImVec4( 0.949f, 0.949f, 0.949f, 1.00f );
	colors[ ImGuiCol_ChildBg ]               = AW_WHITE;
	colors[ ImGuiCol_PopupBg ]               = AW_WHITE;
	colors[ ImGuiCol_Border ]                = ImVec4( 0.85f, 0.85f, 0.85f, 1.00f );
	colors[ ImGuiCol_BorderShadow ]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_FrameBg ]               = AW_WHITE;
	colors[ ImGuiCol_FrameBgHovered ]        = AW_FRAME_HOV;
	colors[ ImGuiCol_FrameBgActive ]         = ImVec4( 0.90f, 0.90f, 0.90f, 1.00f );
	colors[ ImGuiCol_TitleBg ]               = AW_RED_BANNER;
	colors[ ImGuiCol_TitleBgActive ]         = AW_RED_BANNER;
	colors[ ImGuiCol_TitleBgCollapsed ]      = AW_RED_BANNER;
	colors[ ImGuiCol_MenuBarBg ]             = AW_WHITE;
	colors[ ImGuiCol_ScrollbarBg ]           = ImVec4( 0.92f, 0.92f, 0.92f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrab ]         = ImVec4( 0.78f, 0.78f, 0.78f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrabHovered ]  = ImVec4( 0.68f, 0.68f, 0.68f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrabActive ]   = AW_RED;
	colors[ ImGuiCol_CheckMark ]             = AW_RED;
	colors[ ImGuiCol_SliderGrab ]            = AW_RED;
	colors[ ImGuiCol_SliderGrabActive ]      = AW_RED_BRIGHT;
	colors[ ImGuiCol_Button ]                = AW_WHITE;
	colors[ ImGuiCol_ButtonHovered ]         = AW_FRAME_HOV;
	colors[ ImGuiCol_ButtonActive ]          = ImVec4( 0.88f, 0.88f, 0.88f, 1.00f );
	colors[ ImGuiCol_Header ]                = ImVec4( 0.91f, 0.91f, 0.91f, 1.00f );
	colors[ ImGuiCol_HeaderHovered ]         = ImVec4( 0.87f, 0.87f, 0.87f, 1.00f );
	colors[ ImGuiCol_HeaderActive ]          = ImVec4( 0.83f, 0.83f, 0.83f, 1.00f );
	colors[ ImGuiCol_Separator ]             = ImVec4( 0.87f, 0.87f, 0.87f, 1.00f );
	colors[ ImGuiCol_SeparatorHovered ]      = AW_RED;
	colors[ ImGuiCol_SeparatorActive ]       = AW_RED;
	colors[ ImGuiCol_ResizeGrip ]            = ImVec4( 0.85f, 0.85f, 0.85f, 0.50f );
	colors[ ImGuiCol_ResizeGripHovered ]     = AW_RED;
	colors[ ImGuiCol_ResizeGripActive ]      = AW_RED_BRIGHT;
	colors[ ImGuiCol_Tab ]                   = ImVec4( 0.90f, 0.90f, 0.90f, 1.00f );
	colors[ ImGuiCol_TabHovered ]            = AW_RED;
	colors[ ImGuiCol_TabActive ]             = AW_RED;
	colors[ ImGuiCol_TabUnfocused ]          = ImVec4( 0.90f, 0.90f, 0.90f, 1.00f );
	colors[ ImGuiCol_TabUnfocusedActive ]    = ImVec4( 0.84f, 0.84f, 0.84f, 1.00f );
	colors[ ImGuiCol_PlotLines ]             = AW_RED;
	colors[ ImGuiCol_PlotLinesHovered ]      = AW_RED_BRIGHT;
	colors[ ImGuiCol_PlotHistogram ]         = AW_RED;
	colors[ ImGuiCol_PlotHistogramHovered ]  = AW_RED_BRIGHT;
	colors[ ImGuiCol_TableHeaderBg ]         = ImVec4( 0.92f, 0.92f, 0.92f, 1.00f );
	colors[ ImGuiCol_TableBorderStrong ]     = ImVec4( 0.80f, 0.80f, 0.80f, 1.00f );
	colors[ ImGuiCol_TableBorderLight ]      = ImVec4( 0.88f, 0.88f, 0.88f, 1.00f );
	colors[ ImGuiCol_TableRowBg ]            = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_TableRowBgAlt ]         = AW_WHITE;
	colors[ ImGuiCol_TextSelectedBg ]        = ImVec4( 0.702f, 0.180f, 0.180f, 0.35f );
	colors[ ImGuiCol_DragDropTarget ]        = AW_RED;
	colors[ ImGuiCol_NavHighlight ]          = AW_RED;
	colors[ ImGuiCol_ModalWindowDimBg ]      = ImVec4( 0.00f, 0.00f, 0.00f, 0.40f );

	style.WindowPadding     = ImVec2( 10, 10 );
	style.FramePadding      = ImVec2( 6, 4 );
	style.ItemSpacing       = ImVec2( 8, 7 );
	style.ItemInnerSpacing  = ImVec2( 6, 4 );
	style.ScrollbarSize     = 10.0f;
	style.GrabMinSize       = 8.0f;
	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.PopupBorderSize   = 1.0f;
	style.FrameBorderSize   = 1.0f; // aimware checkboxes / combos have thin outlines
	style.TabBorderSize     = 0.0f;
	style.WindowRounding    = 0.0f;
	style.ChildRounding     = 0.0f;
	style.FrameRounding     = 0.0f;
	style.PopupRounding     = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding      = 0.0f;
	style.TabRounding       = 0.0f;
}

//===============================================================================================
// r44: V3 (AIMWARE) skin - orange header chrome, icon tab strip, V3 panel layout.
// Pages rearrange the existing widgets around the same g_CVars; no new feature state.
//===============================================================================================
static void ApplyRagePreset( void );
static void ApplyLegitPreset( void );

static float V3_HalfWidth( void )
{
	return ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;
}

static void V3_PanelHeader( const char* title )
{
	ImDrawList* dl = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );
	float w = ImGui::GetContentRegionAvail( ).x;
	float h = 22.0f;
	float fs = ImGui::GetFontSize( );
	dl->AddRectFilledMultiColor( p, ImVec2( p.x + w, p.y + h ), IM_COL32( 74, 74, 74, 255 ), IM_COL32( 74, 74, 74, 255 ), IM_COL32( 22, 22, 22, 255 ), IM_COL32( 22, 22, 22, 255 ) );
	dl->AddText( ImGui::GetFont( ), fs * 0.92f, ImVec2( p.x + 8.0f, p.y + ( h - fs * 0.92f ) * 0.5f ), IM_COL32_WHITE, title );
	ImGui::Dummy( ImVec2( 0, h ) );
	ImGui::Spacing( );
}

// r46: pixel-perfect icons ripped from the AIMWARE V3 reference screenshots.
// Injected into the ImGui font atlas once at init; drawn with AddImage(atlas UVs).
// r46: pixel-perfect icons ripped from the AIMWARE V3 reference screenshots (0RRGGBBAA per pixel).
static const int awIconW[ 12 ] = {
	19, 32, 32, 32, 32, 32, 32, 65, 65, 65, 65, 65
};
static const int awIconH[ 12 ] = {
	18, 18, 18, 18, 18, 18, 18, 28, 28, 28, 28, 28
};
// r47: awIconData was unsigned int[12][] (ill-formed: inner bounds must be set);
// now 12 named arrays + a pointer table. Same data, same order.
static const unsigned int awIconData0[ 343 ] = { // legitbot 19x18
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD287D, 0x3CCD28E1,
		0x3CCD28A5, 0x3CCD283C, 0x3CCD280F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x3CCD280F, 0x3CCD2828, 0x3CCD2855, 0x3CCD28AF, 0x3CCD28FA, 0x3CCD28D7, 0x3CCD288C, 0x3CCD2873, 0x3CCD2855, 0x3CCD2819,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2823, 0x3CCD283C, 0x3CCD2887, 0x3CCD28AA, 0x3CCD28C3,
		0x3CCD28FA, 0x3CCD28FF, 0x3CCD28EB, 0x3CCD28B9, 0x3CCD28C8, 0x3CCD28C8, 0x3CCD285A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x3CCD2819, 0x3CCD2869, 0x3CCD2891, 0x3CCD28AF, 0x3CCD28B9, 0x3CCD28C8, 0x3CCD28FF, 0x3CCD28FF, 0x3CCD28D7, 0x3CCD2887, 0x3CCD28B4,
		0x3CCD28E1, 0x3CCD2891, 0x3CCD282D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD283C, 0x3CCD28A0, 0x3CCD2891, 0x3CCD286E,
		0x3CCD2850, 0x3CCD285F, 0x3CCD2882, 0x3CCD28FF, 0x3CCD28A0, 0x3CCD2819, 0x3CCD2846, 0x3CCD2891, 0x3CCD28A0, 0x3CCD2891, 0x3CCD2841, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x3CCD285F, 0x3CCD28BE, 0x3CCD2887, 0x3CCD2832, 0x00000000, 0x00000000, 0x3CCD2873, 0x3CCD28CD, 0x3CCD2855,
		0x00000000, 0x00000000, 0x3CCD2837, 0x3CCD2896, 0x3CCD28CD, 0x3CCD286E, 0x00000000, 0x00000000, 0x00000000, 0x3CCD281E, 0x3CCD2882, 0x3CCD28CD,
		0x3CCD2887, 0x3CCD282D, 0x00000000, 0x00000000, 0x3CCD2823, 0x3CCD285A, 0x3CCD2814, 0x00000000, 0x00000000, 0x3CCD2814, 0x3CCD2855, 0x3CCD28C8,
		0x3CCD287D, 0x00000000, 0x3CCD2828, 0x3CCD2855, 0x3CCD2882, 0x3CCD28B9, 0x3CCD28DC, 0x3CCD28B9, 0x3CCD2878, 0x3CCD2841, 0x3CCD280F, 0x3CCD280F,
		0x3CCD280F, 0x00000000, 0x00000000, 0x3CCD2814, 0x3CCD284B, 0x3CCD28A5, 0x3CCD28F5, 0x3CCD28C8, 0x3CCD281E, 0x3CCD2869, 0x3CCD28AA, 0x3CCD28D7,
		0x3CCD28F0, 0x3CCD28FA, 0x3CCD28FA, 0x3CCD28E6, 0x3CCD28AA, 0x3CCD2864, 0x3CCD282D, 0x3CCD280A, 0x3CCD280A, 0x3CCD2828, 0x3CCD2869, 0x3CCD28BE,
		0x3CCD28FF, 0x3CCD28FF, 0x3CCD28FF, 0x3CCD2819, 0x3CCD2864, 0x3CCD28A0, 0x3CCD28CD, 0x3CCD28E6, 0x3CCD28EB, 0x3CCD28EB, 0x3CCD28D2, 0x3CCD289B,
		0x3CCD2855, 0x3CCD2823, 0x3CCD2805, 0x3CCD280A, 0x3CCD2832, 0x3CCD2873, 0x3CCD28C3, 0x3CCD28FF, 0x3CCD28FF, 0x3CCD28FF, 0x00000000, 0x3CCD2819,
		0x3CCD2841, 0x3CCD286E, 0x3CCD28AA, 0x3CCD28C8, 0x3CCD289B, 0x3CCD2850, 0x3CCD281E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x3CCD2823, 0x3CCD2855, 0x3CCD285F, 0x3CCD28FF, 0x3CCD28E1, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2823, 0x3CCD2887, 0x3CCD28C8, 0x3CCD286E,
		0x3CCD2805, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2832, 0x3CCD280F, 0x00000000, 0x00000000, 0x3CCD2828, 0x3CCD285A, 0x3CCD28E1, 0x3CCD2891,
		0x00000000, 0x00000000, 0x00000000, 0x3CCD281E, 0x3CCD2891, 0x3CCD28E1, 0x3CCD2887, 0x3CCD2814, 0x00000000, 0x00000000, 0x3CCD284B, 0x3CCD288C,
		0x3CCD2841, 0x00000000, 0x3CCD280A, 0x3CCD284B, 0x3CCD2891, 0x3CCD28B4, 0x3CCD286E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2846,
		0x3CCD28D2, 0x3CCD28A5, 0x3CCD285F, 0x3CCD2855, 0x3CCD285F, 0x3CCD288C, 0x3CCD28EB, 0x3CCD289B, 0x3CCD2841, 0x3CCD2846, 0x3CCD2873, 0x3CCD2878,
		0x3CCD286E, 0x3CCD2832, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD282D, 0x3CCD2869, 0x3CCD28AF, 0x3CCD28AF, 0x3CCD28B9, 0x3CCD28C8,
		0x3CCD28FF, 0x3CCD28FF, 0x3CCD28F0, 0x3CCD28AA, 0x3CCD2891, 0x3CCD2882, 0x3CCD2855, 0x3CCD2828, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x3CCD2841, 0x3CCD2846, 0x3CCD289B, 0x3CCD28B4, 0x3CCD28CD, 0x3CCD28FF, 0x3CCD28FF, 0x3CCD28FF, 0x3CCD28B9, 0x3CCD288C,
		0x3CCD285F, 0x3CCD282D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2814, 0x3CCD2832,
		0x3CCD284B, 0x3CCD2873, 0x3CCD28C8, 0x3CCD28FF, 0x3CCD28C3, 0x3CCD285F, 0x3CCD2837, 0x3CCD281E, 0x3CCD280A, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3CCD2823, 0x3CCD2887, 0x3CCD28D7, 0x3CCD2878,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData1[ 577 ] = { // ragebot 32x18
		0x292A2B17, 0x292A2B17, 0x292A2B17, 0x292A2B17, 0x292A2B17, 0x292A2B17, 0x292A2B17, 0x282A2B17, 0x26292A15, 0x29292A15, 0x2B282A17, 0x2D292B1B,
		0x2D272A1B, 0x34252A28, 0x00000000, 0x3A1D1E33, 0x70514D95, 0x31151022, 0x331B1626, 0x351A1529, 0x3819152F, 0x4C252453, 0x5D323272, 0x5B38326F,
		0x3D231838, 0x301A1320, 0x00000000, 0x3A1F1B33, 0x28060112, 0x5935356B, 0x502E335B, 0x00000000, 0x262A2815, 0x262A2815, 0x262A2815, 0x262A2815,
		0x262A2815, 0x262A2815, 0x262A2815, 0x262A2815, 0x24292614, 0x26282612, 0x2A272615, 0x2B272617, 0x322B2B24, 0x30212420, 0x21070C05, 0x885C5BC1,
		0xD89D95FF, 0x814740B4, 0x420E0841, 0x62302B7B, 0xA4716DF4, 0xC28B89FF, 0xBA7F7FFF, 0xC38E89FF, 0xC5958CFF, 0xA7766FF9, 0x6F3B3793, 0x42090441,
		0x6D302A8F, 0xC88F8EFF, 0xBA8B8FFF, 0x331A1D26, 0x24292414, 0x24292414, 0x24292414, 0x24292414, 0x24292414, 0x24292414, 0x25292414, 0x26282412,
		0x27272310, 0x28262312, 0x2A262315, 0x2A272415, 0x2A272415, 0x2D28271B, 0x210F0F05, 0x67332F84, 0xD48279FF, 0xDE8A81FF, 0xB0635DFF, 0xCA837EFF,
		0xDB9A97FF, 0xB07370FF, 0xA26763F0, 0xA2645EF0, 0xB7736CFF, 0xD68A84FF, 0xD88681FF, 0xB9645DFF, 0xCD7B73FF, 0xD79491FF, 0x865659BD, 0x2A121315,
		0x24292214, 0x24292214, 0x24292214, 0x24292214, 0x24292214, 0x25292214, 0x26292214, 0x28282212, 0x29262114, 0x2A262115, 0x2A262115, 0x28282212,
		0x2A2C2619, 0x26282412, 0x2A201D15, 0x420E0741, 0x8C2E24C8, 0xED847DFF, 0xEF8A86FF, 0xD77F7DFF, 0x833B3AB7, 0x440D0845, 0x370A012D, 0x48120A4C,
		0x57110A67, 0x842824B9, 0xDB6F6DFF, 0xF88885FF, 0xF48E88FF, 0x974E4BDC, 0x32090924, 0x321F1F24, 0x26272110, 0x26272110, 0x26272110, 0x26272110,
		0x26272110, 0x26272110, 0x26272110, 0x27272110, 0x29262114, 0x2A262115, 0x2A262115, 0x28282212, 0x25292214, 0x282B2617, 0x2E26221D, 0x400E073D,
		0x8D3027CA, 0xED7F79FF, 0xE97A79FF, 0xE58787FF, 0x813C3BB4, 0x31070022, 0x371F122D, 0x42201441, 0x470E064A, 0x741B199C, 0xE97477FF, 0xFC8284FF,
		0xF68786FF, 0x954B4AD8, 0x29070614, 0x3627272B, 0x27272310, 0x27272310, 0x27272310, 0x27272310, 0x27272310, 0x27272310, 0x27272310, 0x27272310,
		0x28262312, 0x2A262315, 0x2A262315, 0x26272310, 0x24272310, 0x2525240C, 0x2F21211E, 0x48120D4C, 0xC5685FFF, 0xD7736DFF, 0x983A38DE, 0xBC6968FF,
		0xC68482FF, 0x6E4B4491, 0x00000000, 0x2F13091E, 0x5A241C6D, 0xC17270FF, 0xDE7A7EFF, 0xB5484BFF, 0xD16968FF, 0xC77F7EFF, 0x4F2C2B59, 0x28191812,
		0x27262610, 0x27262610, 0x27262610, 0x27262610, 0x27262610, 0x26272610, 0x26272610, 0x27262610, 0x28262612, 0x2A262615, 0x2A252615, 0x26272610,
		0x2326240E, 0x2C262919, 0x2F161B1E, 0x6E2E2C91, 0xD8786FFF, 0x9E4E45E9, 0x3C040036, 0x6A2C278A, 0xB67571FF, 0x8E6965CC, 0x00000000, 0x2602000E,
		0x894945C2, 0xBC7977FF, 0x7F3D3BB0, 0x4E000057, 0x9A4340E1, 0xC98381FF, 0x794D4CA5, 0x00000000, 0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E,
		0x2525260E, 0x2625260E, 0x2625260E, 0x27252610, 0x27252610, 0x28242612, 0x29242614, 0x28262712, 0x28252612, 0x35252B29, 0x26020B0E, 0x834041B7,
		0xD78179FF, 0x72392F99, 0x2E150B1D, 0x2B050017, 0x56201B66, 0x502F2C5B, 0x00000000, 0x3819162F, 0x57241E67, 0x5930296B, 0x2B0B0417, 0x46130C48,
		0x5F181276, 0xC07C7AFF, 0x8D5959CA, 0x260D0C0E, 0x2424250C, 0x2424250C, 0x2424250C, 0x2424250C, 0x2424250C, 0x2524250C, 0x2624250E, 0x2623250E,
		0x2623250E, 0x27232510, 0x2622230E, 0x29242614, 0x2A212315, 0x361E262B, 0x2F06101E, 0x824446B6, 0xCD857DFF, 0x5B2F266F, 0x24150C0A, 0x381E172F,
		0x39110E31, 0x21070705, 0x2F24261E, 0x2D1E181B, 0x2E170A1D, 0x00000000, 0x2D1E151B, 0x4017103D, 0x500E085B, 0xB97572FF, 0x9F6767EA, 0x2A0F0F15,
		0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x2423230A, 0x2522230C, 0x2423230A, 0x2423230A, 0x2522230C, 0x2621220E, 0x29212314,
		0x2E20231D, 0x341B2228, 0x2A030C15, 0x85484BBB, 0xCF8681FF, 0x63312C7D, 0x30171320, 0x27040310, 0x4D1F2055, 0x46262B48, 0x00000000, 0x29171414,
		0x2C140819, 0x50372B5B, 0x34160E28, 0x43110843, 0x5E1A1174, 0xC0807AFF, 0x986764DE, 0x28100F12, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x22222207, 0x22222207, 0x21222207, 0x20222207, 0x23212209, 0x27202210, 0x281F2112, 0x2A1E2115, 0x2F1C221E, 0x34151C28, 0x67282B84,
		0xDF8685FF, 0x8D4645CA, 0x2E00001D, 0x63292C7D, 0xBE7B80FF, 0xA1737BEE, 0x29101814, 0x2603040E, 0x87524CBF, 0xC38680FF, 0x874540BF, 0x45000047,
		0x782C20A3, 0xC68C82FF, 0x714D4797, 0x00000000, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x20212105, 0x1F212105,
		0x20222207, 0x23212209, 0x261F210E, 0x261F210E, 0x251F210C, 0x241F220A, 0x2C1F2219, 0x420C0B41, 0xB85B58FF, 0xCA6E6EFF, 0x843339B9, 0xB8696FFF,
		0xC98185FF, 0x764C50A0, 0x220E1207, 0x34121228, 0x6227227B, 0xC1726DFF, 0xDB7D78FF, 0xA34238F2, 0xB05A4AFF, 0xB57F71FF, 0x3D282038, 0x00000000,
		0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F202003, 0x20212105, 0x23202109, 0x241E200A, 0x211F2005,
		0x1D202003, 0x19212105, 0x20212105, 0x370F092D, 0x77251AA2, 0xDC7675FF, 0xE47C83FF, 0xD78285FF, 0x6F353293, 0x290E0B14, 0x00000000, 0x2C1B1519,
		0x350C0329, 0x68201886, 0xCC6963FF, 0xE27A6EFF, 0xD97D6BFF, 0x7F4A3AB0, 0x00000000, 0x23221D09, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01,
		0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F201F03, 0x21212005, 0x23202009, 0x221F1F07, 0x1F201F03, 0x1C211F05, 0x1A202003, 0x2521220C, 0x2E04001D,
		0x7C2A1FAB, 0xD8736FFF, 0xDC7376FF, 0xC7706FFF, 0x6A2C268A, 0x3E16103A, 0x20050003, 0x2506000C, 0x3B0D0534, 0x560E0866, 0xC0625DFF, 0xDB796FFF,
		0xCB7365FF, 0x80493CB2, 0x220E0307, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F2003, 0x1F1F1F01, 0x1F201F03,
		0x21211F05, 0x22201F07, 0x211F1E05, 0x21211F05, 0x00000000, 0x241B1D0A, 0x00000000, 0x561B1B66, 0xB85E5AFF, 0xC26662FF, 0xA44F4CF4, 0xAD5A56FF,
		0xB2615EFF, 0xA15653EE, 0x96504EDA, 0x904B49CF, 0x995250E0, 0xAC5E5DFF, 0xB35F5EFF, 0x9F4D4AEA, 0xA75C57F9, 0xAE736CFF, 0x69433A88, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x20201D03, 0x201F1D03, 0x201E1C03, 0x00000000,
		0x211E1C05, 0x2A191B15, 0x2B070C17, 0x82474BB6, 0xB46969FF, 0x6C2D298E, 0x3E0E053A, 0x622A247B, 0x8E4A47CC, 0x994F4DE0, 0xAD5F5FFF, 0xA95F60FD,
		0xA35D5FF2, 0x8B4B4CC6, 0x6B30318C, 0x3F07083C, 0x5923246B, 0x885855C1, 0x99726DE0, 0x2D1B171B, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x1F1E1D01, 0x1F1D1B01, 0x201E1C03, 0x1F1D1B01, 0x00000000, 0x00000000, 0x281A1A12, 0x26090B0E, 0x552E3064,
		0x5F353576, 0x21050005, 0x291B1114, 0x00000000, 0x32181124, 0x3D191538, 0x4D222155, 0x491F1F4E, 0x4923244E, 0x310F0F22, 0x21030305, 0x2F0E0F1E,
		0x22000107, 0x3C1F1F36, 0x513D3C5C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x1F1D1B01, 0x1F1D1B01, 0x1F1D1B01, 0x00000000, 0x00000000, 0x241B1A0A, 0x2613130E, 0x00000000, 0x00000000, 0x00000000, 0x181F1801, 0x18211905,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x251F200C, 0x21161805, 0x27171A10, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData2[ 577 ] = { // visuals 32x18
		0x27292B17, 0x27292B17, 0x27292B17, 0x27292B17, 0x27292B17, 0x26292A15, 0x252A2915, 0x242A2915, 0x232A2915, 0x232A2915, 0x21282612, 0x252B2B17,
		0x21282712, 0x0E24130A, 0x00000000, 0x00000000, 0x211D2207, 0x27212710, 0x221F2007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x2223250C, 0x25252710, 0x25252710, 0x26282A15, 0x26282A15, 0x26282A15, 0x26282A15,
		0x26282A15, 0x25282914, 0x24292714, 0x23292714, 0x22292714, 0x22292714, 0x20272510, 0x272B2D1B, 0x1E26240E, 0x0025000C, 0x6CB46FFF, 0x8BB490FF,
		0x00000000, 0x231F2A15, 0x1C1A2105, 0xC4C2C2FF, 0xFBF9F3FF, 0xFBFAF7FF, 0xFEFEFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFDFCFFFF, 0xC3C2C4FF,
		0x00000000, 0x2424260E, 0x27262812, 0x27262812, 0x25272710, 0x25272710, 0x25272710, 0x25272710, 0x25272710, 0x24272610, 0x23282512, 0x22282412,
		0x21282512, 0x21282512, 0x20272310, 0x28292C19, 0x1D25210C, 0x0026000E, 0x51C555FF, 0x72B877FF, 0x00000000, 0x1F1C260E, 0xC3C2CAFF, 0xFFFDFEFF,
		0xCECCC4FF, 0x8A8884C4, 0x8F8E8FCD, 0x888788C1, 0x898788C2, 0xE7E5E6FF, 0xFEFCFDFF, 0xFDFBFCFF, 0x00000000, 0x27252610, 0x28262712, 0x28262712,
		0x25272610, 0x25272610, 0x25272610, 0x25272610, 0x25272610, 0x24272510, 0x23282412, 0x22282412, 0x21282412, 0x21282412, 0x20272310, 0x2A282D1B,
		0x1E25210C, 0x002C0019, 0x32CC3BFF, 0x62BE66FF, 0x00000000, 0xC1BEC5FF, 0xF9F9FFFF, 0xBFBDBFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0xC4C1C1FF, 0xFFFCFCFF, 0xE5E2E3FF, 0xFFFFFFFF, 0x00000000, 0x29262614, 0x29262614, 0x29262614, 0x24272610, 0x24272610, 0x24272610, 0x24272610,
		0x24272610, 0x24272610, 0x24272610, 0x23272610, 0x22282612, 0x22282612, 0x22282612, 0x2B262E1D, 0x1F24220A, 0x002E001D, 0x1ECB2EFF, 0x54BB5AFF,
		0x00000000, 0xFDF9FDFF, 0xFFFFFFFF, 0xFAF9FDFF, 0xF6F4F5FF, 0xFBF8F9FF, 0xFFFEFFFF, 0xFFFDFEFF, 0xFFFFFFFF, 0xC7C4C5FF, 0x8E8B8CCC, 0xFEFBFBFF,
		0x00000000, 0x2C292A19, 0x29262614, 0x29262614, 0x25272610, 0x25272610, 0x25272610, 0x25272610, 0x25272610, 0x25272710, 0x25272710, 0x24272710,
		0x23272710, 0x23272710, 0x23272710, 0x2D252F1E, 0x2024230A, 0x00300020, 0x12CB25FF, 0x4DBB54FF, 0x00000000, 0xFCF8FBFF, 0x828588C1, 0x87878CC8,
		0x827F83B7, 0x827F82B6, 0x838181B7, 0x878586BF, 0xF4F2F2FF, 0x00000000, 0x8C8A8BC8, 0xFBF9F9FF, 0x00000000, 0x2624240E, 0x27252510, 0x27252510,
		0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2426250E, 0x2326250E, 0x2326250E, 0x24272710, 0x2E252F1E,
		0x2124230A, 0x00310022, 0x09C81BFF, 0x47B74EFF, 0x00000000, 0xFBF7FBFF, 0x7E8284B9, 0x00000000, 0x23202209, 0x23212209, 0x1F1E1E01, 0x7A7A7AA7,
		0xE9E9E9FF, 0x00000000, 0x7E7E7EAE, 0xF6F6F6FF, 0x00000000, 0x2524240C, 0x2625250E, 0x2625250E, 0x2625250E, 0x2625250E, 0x2625250E, 0x2625250E,
		0x2625250E, 0x2625250E, 0x2625250E, 0x2525250C, 0x2426250E, 0x2426250E, 0x25272610, 0x2F252F1E, 0x22232309, 0x00310022, 0x08C718FF, 0x45B44CFF,
		0x00000000, 0xFAF5FBFF, 0x797E7EAE, 0x00000000, 0x2423230A, 0x27262610, 0x00000000, 0x7B7C7BAB, 0xEAEAEAFF, 0x00000000, 0x7E7E7EAE, 0xF3F3F3FF,
		0x00000000, 0x2424240A, 0x2525250C, 0x2525250C, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A,
		0x2424240A, 0x2424240A, 0x2525250C, 0x2F232E1E, 0x22222207, 0x00300020, 0x09C316FF, 0x44B049FF, 0x00000000, 0xF8F3F8FF, 0x797E7DAE, 0x00000000,
		0x20222207, 0x26272610, 0x00000000, 0x7E7E7EAE, 0xECECECFF, 0x00000000, 0x7E7E7EAE, 0xF3F3F3FF, 0x00000000, 0x2324240A, 0x2325240C, 0x2225240C,
		0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x2424240A, 0x2424240A, 0x2E222D1D,
		0x22222207, 0x00300020, 0x0CBF14FF, 0x47AD48FF, 0x00000000, 0xFAF3F7FF, 0x7E8281B6, 0x00000000, 0x1F212105, 0x21222207, 0x00000000, 0x808080B2,
		0xF0F0F0FF, 0x00000000, 0x7E7E7EAE, 0xF3F3F3FF, 0x00000000, 0x2324240A, 0x2124230A, 0x2124230A, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x23232309, 0x23232309, 0x2424240A, 0x2C212C19, 0x22222207, 0x002F001E, 0x0FBC12FF, 0x4BAB47FF,
		0x00000000, 0xFEF3F8FF, 0x7F8281B6, 0x00000000, 0x20212105, 0x22222207, 0x00000000, 0x828282B6, 0xF2F2F2FF, 0x00000000, 0x828282B6, 0xF2F2F2FF,
		0x00000000, 0x22232309, 0x2124230A, 0x2124230A, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x23232309, 0x23232309, 0x2A202B17, 0x21212105, 0x002F001E, 0x10BA0FFF, 0x4DA945FD, 0x00000000, 0xFFF3F9FF, 0x7F8281B6, 0x00000000,
		0x21212005, 0x22222207, 0x00000000, 0x828283B7, 0xF3F3F3FF, 0x00000000, 0x7A7A7AA7, 0xF1F1F1FF, 0x00000000, 0x21222207, 0x20222207, 0x20222207,
		0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x21212105, 0x21212105, 0x21212105, 0x22222207, 0x22222207, 0x291F2A15,
		0x21212105, 0x002F001E, 0x0FB70BFF, 0x4BA642F7, 0x00000000, 0xFBEFF6FF, 0x7E8281B6, 0x00000000, 0x1F212005, 0x21222207, 0x00000000, 0x848484B9,
		0xF4F4F5FF, 0x00000000, 0x7B7B7CAB, 0xE8E8E8FF, 0x00000000, 0x00000000, 0x1F212105, 0x1E212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105,
		0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x22222207, 0x22222207, 0x291F2914, 0x21212105, 0x00300020, 0x0FB50EFF, 0x49A544F5,
		0x00000000, 0xF8EEF6FF, 0x7E8282B6, 0x00000000, 0x1E212207, 0x21222309, 0x00000000, 0x848483B9, 0xF4F5F4FF, 0x00000000, 0xAFAFAEFF, 0xE8E8E7FF,
		0x00000000, 0x20212005, 0x1F212105, 0x1F212105, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x21212105, 0x21212105, 0x22222207, 0x281E2712, 0x1F1F1F01, 0x002D001B, 0x0CAC12FF, 0x419B40E3, 0x00000000, 0xE3DBE2FF, 0x747779A5, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x7373729A, 0xE0E0DEFF, 0xACACAAFF, 0xDCDCDAFF, 0x7475739E, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x261D250E,
		0x1E1E1F01, 0x00280012, 0x159D21E7, 0x3A8A3FC4, 0x00000000, 0xD5D0D4FF, 0x7172739A, 0x7375749E, 0x747674A0, 0x7475739E, 0x7474749C, 0x72727199,
		0xD7D8D5FF, 0xCCCCC9FF, 0x9C9C9AE5, 0x00000000, 0x1F201D03, 0x00000000, 0x1F1E1E01, 0x1F1E1E01, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x211A2005, 0x00000000, 0x00000000, 0x2F923CD3, 0x43814AB4,
		0x00000000, 0xBCBDBAFF, 0xC2C3BEFF, 0xC3C3BAFF, 0xC7C8BBFF, 0xCCCDC4FF, 0xC6C6C4FF, 0xC5C5C3FF, 0xCBCBC8FF, 0x70706E95, 0x00000000, 0x00000000,
		0x1F201D03, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00220007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData3[ 577 ] = { // misc 32x18
		0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817,
		0x2B282817, 0x00000000, 0x625F5F7B, 0xA29F9FF0, 0x7571729E, 0x27232410, 0x00000000, 0x2C292C19, 0x2B282A17, 0x2B282817, 0x2B282817, 0x2B282817,
		0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2B282817, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915,
		0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x2A282915, 0x00000000, 0x7E7C7CAE,
		0xA3A1A2F2, 0x9E9D9DE9, 0x27252610, 0x211F2105, 0x29272914, 0x29272714, 0x29272714, 0x29272714, 0x29272714, 0x29272714, 0x29272714, 0x29272714,
		0x29272714, 0x29272714, 0x29272714, 0x29272714, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612,
		0x28282612, 0x2A2A2815, 0x2625240E, 0x00000000, 0x2625240E, 0x2424230A, 0x00000000, 0x00000000, 0x80807EB2, 0x999997E0, 0x7C7B7BAB, 0x00000000,
		0x2E2D2D1D, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612, 0x28282612,
		0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x2526230E, 0x00000000, 0x52535060,
		0x00000000, 0x26272410, 0x1F201D03, 0x00000000, 0x858582BB, 0x929390D5, 0x949493D6, 0x00000000, 0x26272510, 0x26272410, 0x26272410, 0x26272410,
		0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x27282512, 0x27282512, 0x27282512, 0x27282512,
		0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x27282512, 0x2526230E, 0x00000000, 0x8A8B88C6, 0x66676484, 0x00000000, 0x00000000, 0x4747454A,
		0x969694DA, 0x8F8F8DCD, 0x8F8F8ECD, 0x00000000, 0x2525240C, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272410,
		0x26272410, 0x26272410, 0x26272410, 0x26272410, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510,
		0x26272510, 0x2525240C, 0x00000000, 0x6D6E6C91, 0x8E8E8CCC, 0x6D6E6C91, 0x777775A2, 0x888887C1, 0x8B8B89C6, 0x8D8D8BCA, 0x878786BF, 0x3636362B,
		0x00000000, 0x28282612, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510, 0x26272510,
		0x2626260E, 0x2626260E, 0x2626260E, 0x2626260E, 0x2626260E, 0x2626260E, 0x2626260E, 0x2626260E, 0x2525250C, 0x2525250C, 0x22222107, 0x29292814,
		0x7373739A, 0x7F7F7FB0, 0x7E7E7DAE, 0x7E7E7DAE, 0x808080B2, 0x7F7F7FB0, 0x888888C1, 0x858585BB, 0x3838382F, 0x00000000, 0x2525250C, 0x2626260E,
		0x29292814, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E,
		0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E, 0x26262710, 0x00000000, 0x28282812, 0x54545462, 0x6161617A, 0x6161617A,
		0x66666784, 0x70707095, 0x7A7A7AA7, 0x828282B6, 0x818181B4, 0x32323224, 0x00000000, 0x28282812, 0x2424240A, 0x2525260E, 0x2525260E, 0x2525260E,
		0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A,
		0x2424240A, 0x2424240A, 0x22222207, 0x2424240A, 0x1F1F2003, 0x00000000, 0x00000000, 0x00000000, 0x2424240A, 0x5E5E5E74, 0x69696988, 0x72727299,
		0x7B7B7BA9, 0x7E7E7EAE, 0x2F2F2F1E, 0x00000000, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2424240A, 0x2424240A, 0x2424240A,
		0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x22222207,
		0x22222207, 0x23232309, 0x22222207, 0x00000000, 0x00000000, 0x1F1F1F01, 0x5A5A5A6D, 0x6464647F, 0x6F6F6F93, 0x7474749C, 0x777777A2, 0x2B2B2B17,
		0x00000000, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2424240A, 0x2424240A, 0x2424240A, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x00000000, 0x1F1F1F01, 0x53535360, 0x5B5B5B6F, 0x68686886, 0x6F6F6F93, 0x6C6C6C8E, 0x2B2B2B17, 0x00000000, 0x00000000, 0x2525250C,
		0x22222207, 0x2424240A, 0x2424240A, 0x2424240A, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x22222207, 0x22222207, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x00000000, 0x00000000, 0x2424240A,
		0x4D4D4D55, 0x57575767, 0x5D5D5D72, 0x66666683, 0x68686886, 0x28282812, 0x00000000, 0x21212105, 0x2525250C, 0x22222207, 0x22222207, 0x22222207,
		0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x1F1F1F01,
		0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x21212105, 0x2525250C, 0x22222207, 0x00000000, 0x1F1F1F01, 0x4747474A, 0x5252525E, 0x57575767,
		0x5D5D5D72, 0x5E5E5E74, 0x2B2B2B17, 0x00000000, 0x22222207, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105,
		0x21212105, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01,
		0x21212105, 0x21212105, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000, 0x3D3D3D38, 0x4D4D4D55, 0x5050505B, 0x5B5B5B6F, 0x5959596B, 0x2525250C,
		0x00000000, 0x1F1F1F01, 0x21212105, 0x21212105, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01,
		0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x3E3E3E3A, 0x42424241, 0x4949494E, 0x54545462, 0x4F4F4F59, 0x29292914, 0x00000000, 0x21212105, 0x1F1F1F01,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x3838382F, 0x3A3A3A33, 0x3A3A3A33, 0x3636362B, 0x4848484C, 0x00000000, 0x21212105, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x34343428, 0x29292914, 0x00000000,
		0x3A3A3A33, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x2F2F2F1E, 0x2F2F2F1E, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData4[ 577 ] = { // colors 32x18
		0x2B282A17, 0x2B282A17, 0x2A282B17, 0x29282B17, 0x29282C19, 0x29282D1B, 0x29282D1B, 0x28272B17, 0x28272B17, 0x27262A15, 0x26262A15, 0x2D2F3224,
		0x2D2C3020, 0x2D292E1D, 0x302F3428, 0x5355596B, 0x7D7C80B2, 0xACA9ACFF, 0x6262627B, 0x272B2917, 0x262A2915, 0x28292A15, 0x2E29281D, 0x33252226,
		0x4D2B2655, 0x693C3588, 0x3B231F34, 0x28292914, 0x262A2915, 0x28292914, 0x28292914, 0x28292914, 0x2A282A15, 0x2A282B17, 0x29282B17, 0x28292B17,
		0x28292C19, 0x28292D1B, 0x28282D1B, 0x27272C19, 0x27272C19, 0x26262B17, 0x25252914, 0x2B292F1E, 0x352D3931, 0x8C8090CF, 0xBEB5C1FF, 0xB8B5BAFF,
		0xA3A6A8FB, 0xADB3B5FF, 0x60646581, 0x292A2C19, 0x27292B17, 0x27292A15, 0x2D29271B, 0x3828222F, 0x633C337D, 0x885349C1, 0x4126203F, 0x25282812,
		0x22292914, 0x24292814, 0x24292814, 0x24292814, 0x29292914, 0x29292A15, 0x28292A15, 0x27292914, 0x27292A15, 0x26282B17, 0x27292C19, 0x25272914,
		0x24272914, 0x25272914, 0x29272A15, 0x4D454B55, 0xC7B2C5FF, 0xC1A3BFFF, 0xA28D9FF0, 0xA1999EEE, 0x949899E0, 0xA3AFAFFF, 0x282C3020, 0x2D272F1E,
		0x2B262F1E, 0x29262E1D, 0x211D2105, 0x2E25231D, 0x846863B9, 0x845B54B9, 0x3E29253A, 0x24292814, 0x21292914, 0x23292814, 0x23292814, 0x23292814,
		0x27282612, 0x27282712, 0x26282712, 0x25282712, 0x25282812, 0x25282914, 0x25282914, 0x25282914, 0x24272812, 0x23252812, 0x4843484C, 0xC8B7C2FF,
		0xAF84A3FF, 0x915583D1, 0x906984CF, 0x998E92E0, 0xA1A5A3F5, 0x747F7DB0, 0x25272914, 0x2F262E1E, 0x2D242F1E, 0x2B243020, 0x2A262E1D, 0x7471749C,
		0x776867A2, 0x634B497D, 0x30252320, 0x23282612, 0x22282712, 0x24272610, 0x24272610, 0x24272610, 0x28282612, 0x27282712, 0x26282712, 0x25282712,
		0x25282914, 0x25282A15, 0x25282914, 0x27292A15, 0x272A2C19, 0x3132362B, 0xAEA7AEFF, 0xAC95A5FF, 0x965A83DA, 0x9A4680E1, 0x824E71B6, 0x7D7276AD,
		0xABADA9FF, 0x4548434C, 0x27262410, 0x2D26281B, 0x2C232A19, 0x2E252F1E, 0x28252C19, 0x24252710, 0x39353531, 0x30272620, 0x27242210, 0x24272610,
		0x25272710, 0x27262610, 0x27262610, 0x27262610, 0x28282712, 0x28282812, 0x27282812, 0x26282812, 0x26282914, 0x26282A15, 0x26282C19, 0x21232812,
		0x26282A15, 0x7C7B7CAB, 0xA29C9DF0, 0x97858DDC, 0x946582D6, 0x813E6BB4, 0x744A669C, 0x8A8186C4, 0xA3A29FF2, 0x34322E28, 0x28262212, 0x2B282517,
		0x2F2A2D1E, 0x2E272F1E, 0x53525869, 0x5357596B, 0x25282812, 0x27282712, 0x27282712, 0x25272610, 0x27262610, 0x29262614, 0x29262614, 0x29262614,
		0x2626260E, 0x2626260E, 0x2626260E, 0x26272610, 0x26262812, 0x26262914, 0x26262E1D, 0x25242F1E, 0x2E2D2F1E, 0x99988FE0, 0x807E72B2, 0x817C71B4,
		0x908385CF, 0x8A7685C4, 0x857681BB, 0x8E888BCC, 0x979496DC, 0x2E2D2D1D, 0x26272410, 0x2526230E, 0x2422250C, 0x2A262E1D, 0xA5A6ADFF, 0x9DA3A6F7,
		0x1E232309, 0x25272510, 0x28292814, 0x2525250C, 0x28272812, 0x27262610, 0x29272814, 0x28272812, 0x26262710, 0x2626260E, 0x27262610, 0x27262610,
		0x27262710, 0x27262710, 0x27262B17, 0x27262B17, 0x4C4B4153, 0x98977CDE, 0x8B8B68C6, 0x8D8F6ACD, 0x797E65AE, 0x7E8579BB, 0x7E7F7EB0, 0x7F7B80B2,
		0x928F96DA, 0x30313529, 0x2124240A, 0x272B2817, 0x2223250C, 0x3A383F3C, 0x9C9EA4F4, 0x262E3122, 0x2125250C, 0x26272410, 0x2424230A, 0x2424240A,
		0x23232309, 0x26272610, 0x27282812, 0x27282812, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525250C, 0x2525240C, 0x2526220E, 0x2526210E, 0x25261D0E,
		0x6161487A, 0x848359B9, 0xA19F6AEE, 0xACAA72FF, 0x908E67CF, 0x7573619E, 0x797673A5, 0x79767CAB, 0x83828AC4, 0x62646A8A, 0x1C202105, 0x2026220E,
		0x2324250C, 0x5C5A5F76, 0x5254596B, 0x1D23260E, 0x1F222309, 0x2625230E, 0x2524230C, 0x2B2B2B17, 0x2424240A, 0x23232309, 0x2626260E, 0x2626260E,
		0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x23232109, 0x23231E09, 0x23241D0A, 0x26271C10, 0x6A6B538C, 0x777650A2, 0x908E5CCF, 0x9A945FE1,
		0x867F5BBD, 0x746B5E9C, 0x787270A3, 0x757376A0, 0x737376A0, 0x818384B9, 0x3E424041, 0x1F232009, 0x20212105, 0x5C595E74, 0x1E1E2309, 0x21242812,
		0x2323240A, 0x2622210E, 0x33312F26, 0x3E3E3E3A, 0x2D2D2E1D, 0x21212105, 0x2424240A, 0x2424240A, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222007, 0x22221F07, 0x22222107, 0x2525250C, 0x6B6C658E, 0x69695A88, 0x72705799, 0x777254A2, 0x726C5B99, 0x6E686691, 0x6A67668A, 0x6F6F6A93,
		0x6B6C658E, 0x6E706895, 0x73756F9E, 0x40423F41, 0x2421230A, 0x1F191F01, 0x24212710, 0x24252A15, 0x322F3124, 0x69636288, 0x7E7B7AAE, 0x787878A3,
		0x1F202003, 0x00000000, 0x21212105, 0x21212105, 0x22222207, 0x22222207, 0x22222207, 0x22222207, 0x22222107, 0x22222007, 0x2222240A, 0x1C1B2207,
		0x5453596B, 0x6D6D7095, 0x62656581, 0x5F63637D, 0x61636A8A, 0x61626E91, 0x63646683, 0x63645B7F, 0x65655B81, 0x65655C81, 0x69686288, 0x787775A3,
		0x64626683, 0x4847505B, 0x4D4F5869, 0x6C6F759E, 0x7471749C, 0x746C6B9C, 0x6E696891, 0x54545462, 0x26272710, 0x2525250C, 0x2424240A, 0x2424240A,
		0x21212005, 0x21212105, 0x21212207, 0x21202309, 0x21212005, 0x21211F05, 0x21202207, 0x1F1F250C, 0x3C3C4241, 0x686A7095, 0x58606F93, 0x536179A5,
		0x58657DAD, 0x5A6477A2, 0x585E637D, 0x5F605A78, 0x62615B7B, 0x625F5E7B, 0x5F5B5A76, 0x5F5D5B76, 0x62636784, 0x6569739A, 0x62687299, 0x5F656D8F,
		0x5B595C70, 0x69605F88, 0x716B6A97, 0x3E3E3E3A, 0x00000000, 0x2525250C, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F2105, 0x1F1F2105,
		0x1F201F03, 0x1F201D03, 0x21212105, 0x24242812, 0x26262914, 0x61636784, 0x4F597197, 0x56668FCD, 0x5C6C90CF, 0x5A687FB0, 0x545C6581, 0x56585569,
		0x5C5B5A70, 0x5A565A6D, 0x53515260, 0x615E5C7A, 0x68545386, 0x765354A0, 0x70545495, 0x66555583, 0x6155557A, 0x5D545472, 0x6C68688E, 0x2A2A2A15,
		0x20212105, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1E1E1F01, 0x1E1E1F01, 0x00000000, 0x1F201D03, 0x201F2003, 0x00000000,
		0x00000000, 0x4548484C, 0x54597197, 0x4F5784B9, 0x59648AC4, 0x515D739A, 0x4B545E74, 0x4B50505B, 0x4C4F4D59, 0x4446474A, 0x4A4F4E59, 0x524F4B5E,
		0x7C473DAB, 0xA64536F7, 0xA45041F4, 0x844E42B9, 0x664A4583, 0x5F585976, 0x4747484C, 0x21212105, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1E1E2003, 0x1E1F1D01, 0x20222207, 0x53546581, 0x5454749C,
		0x4B4F6A8A, 0x49536078, 0x4452525E, 0x3C4E4157, 0x51675284, 0x657C64AB, 0x495C4E70, 0x4847424C, 0x7F3F33B0, 0xAC3623FF, 0xA33D2BF2, 0x7F3F31B0,
		0x684A4586, 0x5E5B5D74, 0x1C1F2003, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x21222207, 0x1E1E2003, 0x302E3529, 0x53505B6F, 0x4749525E, 0x3D48494E, 0x2F463748, 0x395B3B6F,
		0x6A9867DE, 0x75A46DF4, 0x5B7B5BA9, 0x3A3E383A, 0x693B3688, 0x8B362CC6, 0x7B342CA9, 0x683F3986, 0x604F4D78, 0x2B2C2D1B, 0x1C1F2003, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x1F1E1F01, 0x00000000, 0x00000000, 0x2B282517, 0x4E4E4C57, 0x474F4C59, 0x2D41323F, 0x35563966, 0x5B8759BF, 0x5E8D57CA, 0x4F6D4D8F, 0x3D433E43,
		0x503A3C5B, 0x663D4083, 0x5C3D4070, 0x5D505172, 0x2C292919, 0x00000000, 0x1C1F1E01, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData5[ 577 ] = { // gui 32x18
		0x2A272815, 0x2A272815, 0x2A272915, 0x2A272915, 0x29272914, 0x302E3020, 0x2B282917, 0x211E1E05, 0x53515360, 0x52515564, 0x52515564, 0x52515462,
		0x53515360, 0x54515262, 0x54515262, 0x55515164, 0x54514F62, 0x53514E60, 0x51524E5E, 0x4F524F5E, 0x4F524F5E, 0x4F514D5C, 0x4F4F4C59, 0x58565469,
		0x00000000, 0x27232410, 0x2C262719, 0x2E27281D, 0x2E27281D, 0x2E27281D, 0x2D26271B, 0x2D26281B, 0x27262610, 0x28262612, 0x28262612, 0x27262510,
		0x29272714, 0x2C292919, 0x29242414, 0x5A54526D, 0xFFFBF6FF, 0xFEFBF4FF, 0xFDFCF4FF, 0xFBFCF5FF, 0xF8FDF8FF, 0xF6FDFBFF, 0xF5FDFCFF, 0xF5FDFDFF,
		0xF5FDFEFF, 0xF6FDFFFF, 0xF7FCFFFF, 0xF9FCFEFF, 0xFDFBFCFF, 0xFCF7F7FF, 0xFFFDFCFF, 0xFFFEFDFF, 0x5E585974, 0x322C2E24, 0x29232514, 0x2D26291B,
		0x2E272A1D, 0x2E272A1D, 0x2D26281B, 0x2D26281B, 0x25272610, 0x25272510, 0x24272410, 0x24282312, 0x27272310, 0x2B272417, 0x2C252419, 0x574F4D67,
		0xF6F0E2FF, 0xFDFAE2FF, 0xFDFDE6FF, 0xF7FCE9FF, 0xEAF7EAFF, 0xE4F8F2FF, 0xE0F9F5FF, 0xDFF9F8FF, 0xE0F8FEFF, 0xE5F7FFFF, 0xE4EDFCFF, 0xE6E5F4FF,
		0xF1E7F3FF, 0xFDEDF5FF, 0xFDEFF4FF, 0xF4EAEEFF, 0x5A53576D, 0x2C262B19, 0x2B262B17, 0x2C262B19, 0x2C262B19, 0x2C262B19, 0x2B252A17, 0x2B252A17,
		0x22282512, 0x1F292414, 0x1F292214, 0x22282112, 0x26272110, 0x2C282319, 0x2B242317, 0x544C4C62, 0xF1EDDAFF, 0xF8F8D5FF, 0xFAFCDBFF, 0xF3F9E0FF,
		0xE4F4E5FF, 0xDBF6F0FF, 0xD4F8F3FF, 0xD1F8F6FF, 0xD5F6FFFF, 0xDAEFFFFF, 0xDDE4FFFF, 0xE3DBF3FF, 0xEFDCF1FF, 0xF8DDEEFF, 0xF8E0EFFF, 0xF0DEEBFF,
		0x554A5364, 0x2B262D1B, 0x2B262C19, 0x2B262D1B, 0x2A252B17, 0x2A252B17, 0x2A252C19, 0x2A252B17, 0x21282512, 0x192B2417, 0x1C2A2315, 0x22282212,
		0x26272110, 0x2D2A241B, 0x2B242317, 0x4F484859, 0xEAE7D4FF, 0xF0F3CCFF, 0xF3F7D3FF, 0xEDF3D9FF, 0xDEEFE0FF, 0xD3F1EBFF, 0xCBF3ECFF, 0xC8F4EEFF,
		0xD0F0FFFF, 0xCFDEFFFF, 0xD4D4F8FF, 0xDACBEAFF, 0xE5CAE5FF, 0xF6D3EAFF, 0xF9D7EEFF, 0xF4D8EEFF, 0x56465466, 0x2B262C19, 0x2A262C19, 0x2A252B17,
		0x2A252B17, 0x2A252B17, 0x29242A15, 0x29242A15, 0x22282612, 0x1C2A2515, 0x1E292414, 0x24282212, 0x27272110, 0x2D28221B, 0x2F231E1E, 0x53433960,
		0xEEE3C4FF, 0xF3F1BEFF, 0xEDF5C0FF, 0xE1F5C5FF, 0xCEF2CDFF, 0xC3F4DCFF, 0xBDF5E7FF, 0xBCF4F2FF, 0xC6EFFFFF, 0xC4D9FFFF, 0xCBCCFFFF, 0xD5C1F6FF,
		0xE6C0F2FF, 0xF5C3EDFF, 0xF8C7E8FF, 0xF5CCE3FF, 0x533D4A60, 0x2B262C19, 0x2A272B17, 0x2A252A15, 0x29242914, 0x29242914, 0x29242914, 0x29242914,
		0x2525250C, 0x2426250E, 0x2526240E, 0x2625230E, 0x27252210, 0x2A241F15, 0x35221529, 0x5C3E2370, 0xE5C99AFF, 0xF3E3A0FF, 0xE9F6A6FF, 0xCEFBA7FF,
		0xADEFA6FF, 0xA1F1BBFF, 0xA4F5DDFF, 0xACF4FBFF, 0xB2ECFFFF, 0xB1D6FFFF, 0xADB5F9FF, 0xB8A2F5FF, 0xDBAAFBFF, 0xF1AEF2FF, 0xF3B2DBFF, 0xE6B2C1FF,
		0x593D446B, 0x2A252915, 0x28262914, 0x29242814, 0x28232712, 0x28232712, 0x28232712, 0x28232712, 0x29242514, 0x2C232519, 0x2B242517, 0x28252412,
		0x28252312, 0x2A231E15, 0x3A210F33, 0x6139147A, 0xE9C589FF, 0xF3E08FFF, 0xE2F794FF, 0xBEFD92FF, 0x9AF292FF, 0x8CF2AAFF, 0x90F5D5FF, 0x9AF3FDFF,
		0xA3EAFFFF, 0x9DCCFFFF, 0x9CA9FDFF, 0xAC94FEFF, 0xD49BFFFF, 0xF2A3FAFF, 0xF3A8DBFF, 0xE7ABBBFF, 0x57383D67, 0x2A252815, 0x26252710, 0x28232612,
		0x28232612, 0x28232612, 0x28232612, 0x28232612, 0x29242514, 0x2C222419, 0x2A222415, 0x27232410, 0x27242210, 0x2A231C15, 0x3A200D33, 0x5D340F72,
		0xE0BF80FF, 0xE8DC85FF, 0xD8F38BFF, 0xB5F88CFF, 0x90EE8DFF, 0x7DEAA2FF, 0x81EFCCFF, 0x89ECF2FF, 0x95E2FFFF, 0x93C2FFFF, 0x94A0FDFF, 0xA48BFCFF,
		0xCC91FFFF, 0xE392F0FF, 0xE798D4FF, 0xDFA1BAFF, 0x5130395C, 0x28232412, 0x2423230A, 0x27222310, 0x27222310, 0x27222310, 0x27222310, 0x28232412,
		0x28232412, 0x2B222517, 0x28232612, 0x2423240A, 0x23232109, 0x29241B14, 0x3B1E0B34, 0x5E310C74, 0xDCBC77FF, 0xE1DC7AFF, 0xD1F181FF, 0xB0F785FF,
		0x8AED89FF, 0x73E99FFF, 0x73EDC7FF, 0x7BECEEFF, 0x89E0FFFF, 0x83B6FFFF, 0x8893FEFF, 0x9A7EF9FF, 0xC483FBFF, 0xE289F0FF, 0xE790D6FF, 0xDD99BCFF,
		0x4E2B3957, 0x27222310, 0x22222007, 0x2521220C, 0x2521220C, 0x2521220C, 0x2621220E, 0x2521220C, 0x2521230C, 0x28202412, 0x2321250C, 0x2024250C,
		0x2024210A, 0x27231810, 0x3B1B0634, 0x612C057A, 0xD4AF63FF, 0xE5DF70FF, 0xD5F77AFF, 0xA7F175FF, 0x7FE97EFF, 0x67E799FF, 0x65EBC2FF, 0x6CEAEAFF,
		0x7BDDFFFF, 0x78B3FFFF, 0x7A87FEFF, 0x8B6BF3FF, 0xB970F4FF, 0xE07CEDFF, 0xEA89D7FF, 0xD083ADFF, 0x542C3D62, 0x27222310, 0x22222007, 0x2621220E,
		0x2621220E, 0x2621220E, 0x2621220E, 0x2621220E, 0x2521230C, 0x27202410, 0x2022250C, 0x1A24240A, 0x1B24200A, 0x2422170A, 0x3D1B0538, 0x6227007B,
		0xD1A652FF, 0xE0D65AFF, 0xCFEE64FF, 0x9EE761FF, 0x72E06DFF, 0x56DD8AFF, 0x52E2B6FF, 0x58E1E1FF, 0x68D3FFFF, 0x67A9FFFF, 0x6B7CFBFF, 0x815EEEFF,
		0xB464EEFF, 0xDC6EE4FF, 0xE67BCEFF, 0xCD77A5FF, 0x5226395E, 0x2521220C, 0x20211F05, 0x2420210A, 0x2420210A, 0x2420210A, 0x2420210A, 0x2420210A,
		0x241F210A, 0x261F210E, 0x1E212207, 0x17232109, 0x18231F09, 0x20201603, 0x3D1B0638, 0x5E210074, 0xC5953DFF, 0xD7C644FF, 0xC5DE4EFF, 0x93D94EFF,
		0x66D25BFF, 0x44CC77FF, 0x40D2A6FF, 0x45D0D1FF, 0x53C3F6FF, 0x5298F8FF, 0x586BEAFF, 0x6F4DDCFF, 0xA552DBFF, 0xCA57CCFF, 0xD161B4FF, 0xBF6694FF,
		0x4B1F3251, 0x23202109, 0x1E201E03, 0x221F1F07, 0x221F1F07, 0x221F1F07, 0x221F1F07, 0x221F1F07, 0x231E1F09, 0x251E1F0C, 0x1E202003, 0x18221F07,
		0x1C211E05, 0x251C150C, 0x431A0943, 0x5B1D006F, 0xBC8C36FF, 0xC4B331FF, 0xB3CB3CFF, 0x82C43CFF, 0x56BC4BFF, 0x34B565FF, 0x30BA90FF, 0x35B9B9FF,
		0x41ADDEFF, 0x3E83E2FF, 0x4356D3FF, 0x5938C5FF, 0x8F3DC5FF, 0xBF4DC1FF, 0xC755A8FF, 0xB95E8CFF, 0x491B2E4E, 0x22202107, 0x1D201E03, 0x211F1F05,
		0x211F1F05, 0x211F1F05, 0x211F1F05, 0x211F1F05, 0x221F1F07, 0x241E200A, 0x1F1F2003, 0x1D202003, 0x251E1D0C, 0x31181422, 0x49180D4E, 0x58190069,
		0xA4792AF4, 0xABA225FF, 0x97B22DFF, 0x6EAE32FF, 0x3E9D38E7, 0x219650DA, 0x1F9C75E5, 0x239B97E3, 0x2E8FBCFF, 0x2F6DC8FF, 0x3041B9FF, 0x4F30B8FF,
		0x782CB1FF, 0xA338ABFF, 0xB84A9CFF, 0xA24674F0, 0x46182B48, 0x201E1E03, 0x1C1F1D01, 0x201E1E03, 0x201E1E03, 0x201E1E03, 0x201E1E03, 0x201E1E03,
		0x1F1E1E01, 0x201E1E03, 0x1D1F1E01, 0x1C1F1E01, 0x271C1C10, 0x33171526, 0x47170E4A, 0x561D0466, 0x95722DD8, 0x969327DA, 0x839E2DE9, 0x5A942CD6,
		0x31832FB7, 0x1D8045B2, 0x1D8563BB, 0x238581BB, 0x2A7BA0EC, 0x2559A6F7, 0x1F2C95D8, 0x381B95D8, 0x601C95D8, 0x8D2F99E0, 0x993785E0, 0x8E3B69CC,
		0x3C132636, 0x201E1E03, 0x1C1F1D01, 0x201E1E03, 0x201E1E03, 0x201E1E03, 0x201E1E03, 0x201E1E03, 0x00000000, 0x00000000, 0x181F1B01, 0x181F1A01,
		0x201D1A03, 0x301E1C20, 0x37140A2D, 0x45190047, 0x795C24A5, 0x85812CBB, 0x70872EBF, 0x4E7F2DB0, 0x3275319E, 0x1D6C3B8E, 0x1F705695, 0x25717097,
		0x296885BB, 0x1B457FB0, 0x1C2578A3, 0x270D7197, 0x480E739A, 0x6F217BA9, 0x7F2F73B0, 0x76345EA0, 0x3817292F, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x141F1701, 0x131F1601, 0x00000000, 0x00000000, 0x2E1C131D, 0x21080005,
		0x301D0020, 0x312B0022, 0x22310022, 0x0D300020, 0x002C0019, 0x00270510, 0x002B1917, 0x002C2C19, 0x0126372D, 0x011A3D38, 0x0D114547, 0x1D08494E,
		0x2F064A50, 0x3A054445, 0x410E3F3F, 0x340B2B28, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData6[ 577 ] = { // settings 32x18
		0x29272A15, 0x29272A15, 0x29272A15, 0x29272A15, 0x29272A15, 0x29272A15, 0x3534372D, 0x42414343, 0x4F4E505B, 0x302E3122, 0x2B292C19, 0x45434648,
		0xB5B4B7FF, 0xC7C5C8FF, 0xC8C7C9FF, 0xB2B0B3FF, 0x4745484C, 0x2E2D3020, 0x2A282B17, 0x49484A50, 0x42414343, 0x29272A15, 0x29272A15, 0x29272A15,
		0x29272A15, 0x2A282B17, 0x2A282B17, 0x2A282B17, 0x2A282B17, 0x2A282B17, 0x2A282A15, 0x2A292815, 0x28262914, 0x28262914, 0x28262914, 0x28262914,
		0x28262914, 0x302E3122, 0x423F4241, 0x9C999CE5, 0xA7A5A7F9, 0x5F5C5F76, 0x423F4241, 0x767477A2, 0xBDBBBEFF, 0xC0BDC0FF, 0xC0BDC0FF, 0xBAB7BAFF,
		0x787578A3, 0x45434648, 0x56535666, 0xA8A6A9FD, 0x959295D8, 0x4946494E, 0x2D2A2D1B, 0x2D2A2D1B, 0x2D2A2D1B, 0x2A272A15, 0x2B282B17, 0x2A272A15,
		0x2A272A15, 0x2A272A15, 0x2A272915, 0x2A272915, 0x27262914, 0x27262914, 0x27262914, 0x26252710, 0x27262914, 0x37363931, 0x979698DE, 0xC4C3C6FF,
		0xCAC9CCFF, 0xABAAACFF, 0x99989BE3, 0xC1C0C2FF, 0xB7B6B9FF, 0xB7B6B9FF, 0xB4B3B5FF, 0xB9B8BAFF, 0xBEBDC0FF, 0x989799E0, 0xA3A1A4F4, 0xC5C4C7FF,
		0xC7C6C8FF, 0x969597DC, 0x33323428, 0x26252710, 0x26252710, 0x28272A15, 0x28272A15, 0x28272A15, 0x28272A15, 0x28272A15, 0x28272A15, 0x27262A15,
		0x25252812, 0x25252812, 0x26262914, 0x25252812, 0x2323250C, 0x3B3B3E3A, 0xA9A9ABFF, 0xBABABDFF, 0xB1B1B3FF, 0xB5B5B8FF, 0xB9B9BBFF, 0xAEAEB1FF,
		0xA9A9ABFF, 0xA6A6A9FD, 0x9F9FA2F0, 0xA5A5A8FB, 0xB1B1B3FF, 0xB7B7B9FF, 0xB7B7B9FF, 0xB4B4B7FF, 0xB9B9BBFF, 0xA4A4A6F7, 0x39393B34, 0x25252812,
		0x25252812, 0x26262914, 0x27272A15, 0x26262914, 0x26262914, 0x27272A15, 0x27272B17, 0x26262B17, 0x25252812, 0x25252812, 0x25252812, 0x2424260E,
		0x2323250C, 0x29292B17, 0x50505360, 0xA2A2A4F4, 0xAFAEB1FF, 0xAAAAACFF, 0xA5A5A8FB, 0x9C9C9EE9, 0x9A999CE5, 0xA1A0A3F2, 0x9D9DA0EC, 0x9B9B9DE7,
		0x9D9DA0EC, 0xA5A5A8FB, 0xACACAFFF, 0xB2B2B5FF, 0xA3A3A5F5, 0x54545666, 0x29292B17, 0x25252812, 0x25252812, 0x25252812, 0x26262914, 0x26262914,
		0x26262914, 0x26262914, 0x26262A15, 0x26262B17, 0x25252710, 0x25252710, 0x25252710, 0x25252710, 0x25252710, 0x2A2A2C19, 0x2424260E, 0x929193D5,
		0xB1B1B3FF, 0xA4A4A6F7, 0x9A9A9BE3, 0x949496DA, 0x5F5F617A, 0x3635372D, 0x3635372D, 0x63636581, 0x979799E0, 0x99989AE1, 0xA3A3A5F5, 0xABABADFF,
		0x949496DA, 0x29292A15, 0x26262812, 0x25252710, 0x25252710, 0x25252710, 0x26262812, 0x25252710, 0x25252710, 0x26262812, 0x26262914, 0x26262A15,
		0x2525260E, 0x2525260E, 0x2525260E, 0x2525260E, 0x00000000, 0x21212105, 0x45454547, 0xB0B0B0FF, 0xA4A4A5F5, 0x9D9D9EE9, 0x969697DC, 0x34343529,
		0x1E1E1F01, 0x1F1F2003, 0x21212105, 0x00000000, 0x34343529, 0x999899E0, 0xA3A3A3F2, 0xA4A4A5F5, 0xADADAEFF, 0x4D4D4D55, 0x1E1E1F01, 0x23232309,
		0x1E1E1F01, 0x2F2F2F1E, 0x28282812, 0x2424250C, 0x2525260E, 0x2525260E, 0x25252710, 0x25252812, 0x2525250C, 0x2525250C, 0x2525250C, 0x00000000,
		0x5F605F78, 0x7E7E7DAE, 0x9C9C9CE5, 0xA8A8A7FB, 0x999998E0, 0xA3A3A3F2, 0x5959586B, 0x00000000, 0x2B2B2B17, 0x2424240A, 0x29292914, 0x00000000,
		0x00000000, 0x5A5A596D, 0xA3A3A3F2, 0x9C9C9CE5, 0xABABABFF, 0x969696DA, 0x7F7F7FB0, 0x5C5C5C70, 0x00000000, 0x22222207, 0x22222207, 0x2424240A,
		0x2424240A, 0x2525250C, 0x2525260E, 0x25252710, 0x23232309, 0x23232309, 0x23232309, 0x1F1F1F01, 0x9D9D9DE7, 0x9B9B9BE3, 0xA2A2A2F0, 0x9B9B9BE3,
		0xA0A09FEC, 0xACACACFF, 0x3838382F, 0x23232309, 0x2424240A, 0x2626260E, 0x2424240A, 0x22222207, 0x00000000, 0x33333326, 0xAAAAAAFF, 0xA0A09FEC,
		0x9E9E9EE9, 0x999998E0, 0x979797DC, 0x9A9A9AE1, 0x00000000, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A, 0x2424240A,
		0x22222207, 0x22222207, 0x22222207, 0x00000000, 0x939393D5, 0x939393D5, 0x959595D8, 0x969696DA, 0x9C9C9CE5, 0xB0B0B0FF, 0x33333326, 0x21212105,
		0x2424240A, 0x29292914, 0x2424240A, 0x21212105, 0x00000000, 0x31313122, 0xAAAAAAFF, 0x969696DA, 0x999999E0, 0x8E8E8ECC, 0x969696DA, 0x969696DA,
		0x00000000, 0x23232309, 0x2424240A, 0x2424240A, 0x23232309, 0x23232309, 0x23232309, 0x23232309, 0x22222207, 0x22222207, 0x22222207, 0x00000000,
		0x6262627B, 0x787878A3, 0x8B8B8BC6, 0x929292D3, 0x999999E0, 0xB0B0B0FF, 0x54545462, 0x21212105, 0x2424240A, 0x21212105, 0x2424240A, 0x22222207,
		0x1F1F1F01, 0x5A5A5A6D, 0xB0B0B0FF, 0x959595D8, 0x929292D3, 0x888888C1, 0x767676A0, 0x6464647F, 0x00000000, 0x22222207, 0x23232309, 0x2424240A,
		0x23232309, 0x23232309, 0x23232309, 0x22222207, 0x22222207, 0x22222207, 0x21212105, 0x00000000, 0x00000000, 0x00000000, 0x45454547, 0x929292D3,
		0x999999E0, 0x9D9D9DE7, 0xA0A0A0EC, 0x4E4E4E57, 0x2525250C, 0x2424240A, 0x00000000, 0x2424240A, 0x4E4E4E57, 0xA2A2A2F0, 0xA2A2A2F0, 0x909090CF,
		0x8E8E8ECC, 0x4747474A, 0x00000000, 0x00000000, 0x00000000, 0x21212105, 0x22222207, 0x23232309, 0x22222207, 0x22222207, 0x22222207, 0x22222207,
		0x22222207, 0x23232309, 0x2424240A, 0x2525250C, 0x2525250C, 0x1F1F1F01, 0x00000000, 0x7373739A, 0x8B8B8BC6, 0x979797DC, 0xACACACFF, 0xA6A6A6F7,
		0x5F5F5F76, 0x4848484C, 0x4C4C4C53, 0x6262627B, 0xA4A4A4F4, 0xA8A8A8FB, 0x999999E0, 0x939393D5, 0x70707095, 0x00000000, 0x00000000, 0x00000000,
		0x1F1F1F01, 0x2424240A, 0x28282812, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x21212105, 0x1F1F1F01, 0x1F1F1F01, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x57575767, 0x7D7D7DAD, 0x868686BD, 0x969696DA, 0xAAAAAAFF, 0xB8B8B8FF, 0xB3B3B3FF, 0xB4B4B4FF, 0xB4B4B4FF,
		0xA1A1A1EE, 0x969696DA, 0x828282B6, 0x808080B2, 0x6161617A, 0x1F1F1F01, 0x00000000, 0x21212105, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01,
		0x1F1F1F01, 0x21212105, 0x21212105, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000, 0x21212105, 0x00000000, 0x00000000, 0x4C4C4C53, 0x6F6F6F93,
		0x69696988, 0x6F6F6F93, 0x848484B9, 0x8C8C8CC8, 0x8F8F8FCD, 0x969696DA, 0x9A9A9AE1, 0x8D8D8DCA, 0x8D8D8DCA, 0x818181B4, 0x7373739A, 0x6262627B,
		0x69696988, 0x4F4F4F59, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01, 0x1F1F1F01,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4949494E, 0x5F5F5F76, 0x5F5F5F76, 0x5C5C5C70, 0x6262627B, 0x7474749C,
		0x787878A3, 0x72727299, 0x7373739A, 0x767676A0, 0x777777A2, 0x6262627B, 0x5959596B, 0x5C5C5C70, 0x5C5C5C70, 0x44444445, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1F1F1F01, 0x1F1F1F01, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x22222207,
		0x00000000, 0x00000000, 0x1F1F1F01, 0x4747474A, 0x4949494E, 0x00000000, 0x00000000, 0x34343428, 0x57575767, 0x66666683, 0x5D5D5D72, 0x5F5F5F76,
		0x32323224, 0x00000000, 0x22222207, 0x4F4F4F59, 0x4747474A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x42424241, 0x55555564, 0x4F4F4F59, 0x4848484C, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData7[ 1821 ] = { // pistol 65x28
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0x38383803, 0x38383803, 0x38383803,
		0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803,
		0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803,
		0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803,
		0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x38383803, 0x39393906, 0x39393906, 0x3A3A3A09,
		0x3B3B3B0C, 0x3F3F3F19, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x47474733, 0x828282EF, 0x3E3E3E16, 0x3E3E3E16,
		0x3E3E3E16, 0x3D3D3D13, 0x3A3A3A09, 0x39393906, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5C5C5C75, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4040401C,
		0x56565662, 0xB7B7B7FF, 0xFAFAFAFF, 0xFAFAFAFF, 0xFBFBFBFF, 0xFDFDFDFF, 0xFEFEFEFF, 0xFEFEFEFF, 0xF0F0F0FF, 0xF0F0F0FF, 0xEFEFEFFF, 0xEEEEEEFF,
		0xEAEAEAFF, 0xEAEAEAFF, 0xF0F0F0FF, 0xEFEFEFFF, 0xECECECFF, 0xEAEAEAFF, 0xE8E8E8FF, 0xE7E7E7FF, 0xE4E4E4FF, 0xFBFBFBFF, 0xA5A5A5FF, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xA2A2A2FF, 0xF4F4F4FF, 0xFAFAFAFF, 0xFBFBFBFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFEFEFEFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFEFEFEFF, 0xFDFDFDFF, 0xFEFEFEFF, 0xFEFEFEFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF,
		0xFDFDFDFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xB2B2B2FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6D6D6DAC, 0xFFFFFFFF, 0xF8F8F8FF,
		0xFFFFFFFF, 0xFEFEFEFF, 0xFEFEFEFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF8F8F8FF, 0xF3F3F3FF,
		0xECECECFF, 0xE9E9E9FF, 0xE5E5E5FF, 0xE3E3E3FF, 0xE1E1E1FF, 0xE0E0E0FF, 0xE5E5E5FF, 0xEBEBEBFF, 0x7F7F7FE5, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x5B5B5B72, 0xC9C9C9FF, 0xFAFAFAFF, 0xFBFBFBFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFEFEFEFF, 0xF4F4F4FF,
		0xE3E3E3FF, 0xD2D2D2FF, 0xC8C8C8FF, 0xB4B4B4FF, 0xB4B4B4FF, 0xB4B4B4FF, 0xB3B3B3FF, 0xB3B3B3FF, 0xB3B3B3FF, 0xB2B2B2FF, 0xB2B2B2FF, 0xB7B7B7FF,
		0xB2B2B2FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x898989FF, 0xC1C1C1FF, 0xD5D5D5FF, 0xF9F9F9FF, 0xFAFAFAFF, 0xFBFBFBFF,
		0xFBFBFBFF, 0xFFFFFFFF, 0xF1F1F1FF, 0x8F8F8FFF, 0x5C5C5C75, 0x7E7E7EE2, 0xB1B1B1FF, 0x52525256, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x39393906, 0xE3E3E3FF, 0xFFFFFFFF, 0xF6F6F6FF, 0xF2F2F2FF, 0xE5E5E5FF, 0xC7C7C7FF, 0x3B3B3B0C, 0x00000000, 0x00000000, 0x737373BF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xCDCDCDFF, 0xE9E9E9FF, 0xEFEFEFFF, 0xE0E0E0FF, 0xCFCFCFFF,
		0xC2C2C2FF, 0x797979D2, 0x4D4D4D46, 0x6868689C, 0x49494939, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4E4E4E49,
		0xDBDBDBFF, 0xE3E3E3FF, 0xD0D0D0FF, 0xCCCCCCFF, 0xBFBFBFFF, 0x7F7F7FE5, 0x52525256, 0x4D4D4D46, 0x4040401C, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xB1B1B1FF, 0xD9D9D9FF, 0xD3D3D3FF, 0xCBCBCBFF, 0xC2C2C2FF, 0xA3A3A3FF, 0x3B3B3B0C, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4E4E4E49, 0xCFCFCFFF, 0xCCCCCCFF, 0xCCCCCCFF,
		0xBBBBBBFF, 0xBEBEBEFF, 0x9C9C9CFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData8[ 1821 ] = { // smg 65x28
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF, 0xEEEEEEFF,
		0xEEEEEEFF, 0xEEEEEEFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0x47474733, 0x47474733, 0x47474733,
		0x47474733, 0x47474733, 0x47474733, 0x47474733, 0x47474733, 0x47474733, 0x47474733, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x47474733, 0xACACACFF, 0xAAAAAAFF, 0x9A9A9AFF, 0x787878CF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3E3E3E16, 0xBFBFBFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66666695, 0xE1E1E1FF, 0xEAEAEAFF, 0xE5E5E5FF, 0xF0F0F0FF, 0xEEEEEEFF,
		0xF1F1F1FF, 0xFFFFFFFF, 0xFAFAFAFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xE2E2E2FF, 0xE3E3E3FF, 0xE3E3E3FF, 0xE3E3E3FF, 0xE3E3E3FF, 0xE7E7E7FF, 0xA3A3A3FF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x39393906, 0x44444429, 0x4040401C, 0x5050504F, 0x56565662, 0x6363638C, 0x6A6A6AA2, 0x979797FF, 0xEFEFEFFF,
		0xFFFFFFFF, 0xFBFBFBFF, 0xFDFDFDFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFBFBFBFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF,
		0xFEFEFEFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFAFAFAFF, 0xE9E9E9FF, 0xABABABFF, 0x777777CC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xD9D9D9FF, 0xBABABAFF, 0xC1C1C1FF, 0xC0C0C0FF, 0xB0B0B0FF, 0xB7B7B7FF, 0xA9A9A9FF,
		0x878787FF, 0x868686FB, 0xCDCDCDFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFDFDFDFF, 0xFFFFFFFF, 0xFBFBFBFF,
		0xFAFAFAFF, 0xFAFAFAFF, 0xFAFAFAFF, 0xFAFAFAFF, 0xEBEBEBFF, 0xEBEBEBFF, 0xEBEBEBFF, 0xEBEBEBFF, 0xCBCBCBFF, 0xD5D5D5FF, 0x868686FB, 0x3B3B3B0C,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8E8E8EFF, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4B4B4B3F, 0x7A7A7AD5, 0xB8B8B8FF, 0xB3B3B3FF, 0xB6B6B6FF, 0xF6F6F6FF, 0xE9E9E9FF,
		0xB7B7B7FF, 0x858585F8, 0xA1A1A1FF, 0xBFBFBFFF, 0xC9C9C9FF, 0xCBCBCBFF, 0xCBCBCBFF, 0xCCCCCCFF, 0xCDCDCDFF, 0xCDCDCDFF, 0xCECECEFF, 0xCFCFCFFF,
		0xCFCFCFFF, 0xCFCFCFFF, 0xD4D4D4FF, 0x5454545C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x8E8E8EFF, 0x00000000, 0x00000000, 0x00000000, 0x4F4F4F4C, 0x868686FB, 0x9E9E9EFF, 0x7D7D7DDF, 0x4545452C,
		0x00000000, 0x00000000, 0x00000000, 0xC9C9C9FF, 0xA9A9A9FF, 0x3B3B3B0C, 0x00000000, 0x39393906, 0x52525256, 0x6D6D6DAC, 0x848484F5, 0x848484F5,
		0x858585F8, 0x858585F8, 0x868686FB, 0x868686FB, 0x878787FF, 0x878787FF, 0x929292FF, 0xA6A6A6FF, 0x3F3F3F19, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xAAAAAAFF, 0x5959596C, 0x929292FF, 0x929292FF,
		0x727272BC, 0x3A3A3A09, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4D4D4D46, 0xACACACFF, 0x3B3B3B0C, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x9C9C9CFF, 0x6D6D6DAC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x787878CF, 0x929292FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x9E9E9EFF, 0x65656592, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData9[ 1821 ] = { // rifle 65x28
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x4F4F4F4C, 0x8C8C8CFF, 0x959595FF, 0x969696FF, 0x909090FF, 0x949494FF, 0x8F8F8FFF, 0x9B9B9BFF, 0xABABABFF,
		0x9C9C9CFF, 0x9E9E9EFF, 0x9E9E9EFF, 0x959595FF, 0x7D7D7DDF, 0x5A5A5A6F, 0x47474733, 0x00000000, 0x00000000, 0x4040401C, 0x52525256, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x38383803, 0x3D3D3D13, 0x49494939, 0x5C5C5C75, 0x6969699F, 0x4B4B4B3F, 0x747474C2, 0xD6D6D6FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFBFBFBFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFAFAFAFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xF9F9F9FF, 0xF0F0F0FF, 0xE5E5E5FF,
		0xA9A9A9FF, 0x818181EB, 0xC5C5C5FF, 0xA8A8A8FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xDEDEDEFF, 0xFBFBFBFF, 0xFEFEFEFF, 0xFAFAFAFF, 0xFFFFFFFF, 0xFAFAFAFF,
		0xFBFBFBFF, 0xFFFFFFFF, 0xFDFDFDFF, 0xF0F0F0FF, 0xECECECFF, 0xF6F6F6FF, 0xFFFFFFFF, 0xEBEBEBFF, 0xD5D5D5FF, 0xD2D2D2FF, 0xB1B1B1FF, 0x898989FF,
		0x858585F8, 0x878787FF, 0x6D6D6DAC, 0x6464648F, 0x6464648F, 0x707070B5, 0x747474C2, 0x6969699F, 0x4D4D4D46, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xD0D0D0FF,
		0xF2F2F2FF, 0xF2F2F2FF, 0xEAEAEAFF, 0xE2E2E2FF, 0xB4B4B4FF, 0x4F4F4F4C, 0x6F6F6FB2, 0xE1E1E1FF, 0x969696FF, 0x4141411F, 0xB0B0B0FF, 0xE2E2E2FF,
		0xB7B7B7FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xBFBFBFFF, 0xD4D4D4FF, 0xB6B6B6FF, 0x7D7D7DDF, 0x00000000, 0x00000000, 0x00000000, 0x878787FF,
		0x909090FF, 0x00000000, 0x00000000, 0x00000000, 0xB2B2B2FF, 0xC4C4C4FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6363638C, 0x5555555F, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x3F3F3F19, 0x8F8F8FFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5A5A5A6F, 0xB2B2B2FF, 0x818181EB,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData10[ 1821 ] = { // shotgun 65x28
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x7F7F7FE5, 0x66666695, 0x47474733, 0x4646462F, 0x42424223, 0x4141411F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4F4F4F4C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x888888FF, 0xF7F7F7FF, 0xF2F2F2FF, 0xEFEFEFFF, 0xEEEEEEFF, 0xEAEAEAFF, 0xE9E9E9FF,
		0xDADADAFF, 0xDADADAFF, 0xDADADAFF, 0xDADADAFF, 0xDBDBDBFF, 0xDBDBDBFF, 0xDBDBDBFF, 0xDBDBDBFF, 0xD9D9D9FF, 0xE4E4E4FF, 0xE2E2E2FF, 0xFDFDFDFF,
		0xDBDBDBFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4C4C4C42, 0xCECECEFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFDFDFDFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFEFEFEFF, 0xFDFDFDFF, 0xFDFDFDFF, 0xFDFDFDFF,
		0xFDFDFDFF, 0xF6F6F6FF, 0x939393FF, 0x6B6B6BA5, 0x707070B5, 0x5F5F5F7F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3E3E3E16, 0x5F5F5F7F, 0x61616185, 0x8E8E8EFF, 0xBABABAFF, 0xC9C9C9FF, 0xDCDCDCFF,
		0xCFCFCFFF, 0xF1F1F1FF, 0xFBFBFBFF, 0xFFFFFFFF, 0xF6F6F6FF, 0xF1F1F1FF, 0xE3E3E3FF, 0xE2E2E2FF, 0xDEDEDEFF, 0xDEDEDEFF, 0xE0E0E0FF, 0xE0E0E0FF,
		0xE0E0E0FF, 0xDEDEDEFF, 0xDCDCDCFF, 0xDBDBDBFF, 0xDADADAFF, 0xDADADAFF, 0xD3D3D3FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xC0C0C0FF, 0xF3F3F3FF,
		0xF3F3F3FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF9F9F9FF, 0xECECECFF, 0x909090FF, 0x9A9A9AFF, 0x797979D2, 0x767676C8, 0x00000000,
		0x00000000, 0x3A3A3A09, 0x717171B8, 0xA6A6A6FF, 0x9B9B9BFF, 0x858585F8, 0x797979D2, 0x62626289, 0x62626289, 0x62626289, 0x62626289, 0x3F3F3F19,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x9A9A9AFF, 0xFDFDFDFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFEFEFEFF, 0xF2F2F2FF, 0xE3E3E3FF, 0xD4D4D4FF, 0x8C8C8CFF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6B6B6BA5, 0xEAEAEAFF, 0xE7E7E7FF, 0xDEDEDEFF,
		0xD2D2D2FF, 0xADADADFF, 0x5A5A5A6F, 0xA9A9A9FF, 0x3D3D3D13, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int awIconData11[ 1821 ] = { // sniper 65x28
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF,
		0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0xECECECFF, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F, 0x4646462F,
		0x4646462F, 0x4646462F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x868686FB, 0x6363638C, 0x56565662, 0x6F6F6FB2, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x929292FF, 0xDEDEDEFF, 0xF0F0F0FF, 0xFFFFFFFF, 0xF6F6F6FF, 0xD5D5D5FF, 0xDCDCDCFF, 0x5E5E5E7C,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5A5A5A6F, 0x6868689C, 0x5959596C, 0x00000000, 0x00000000, 0x5D5D5D79, 0xCFCFCFFF, 0xEFEFEFFF,
		0xFBFBFBFF, 0xF7F7F7FF, 0xDBDBDBFF, 0xDCDCDCFF, 0xD7D7D7FF, 0xCECECEFF, 0xC6C6C6FF, 0xBFBFBFFF, 0xBBBBBBFF, 0xBABABAFF, 0xBABABAFF, 0xBABABAFF,
		0xBABABAFF, 0xBDBDBDFF, 0xB9B9B9FF, 0xBEBEBEFF, 0x808080E8, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xA9A9A9FF, 0xF7F7F7FF, 0xFEFEFEFF, 0xFDFDFDFF,
		0xF0F0F0FF, 0xE5E5E5FF, 0xF3F3F3FF, 0xFFFFFFFF, 0xF8F8F8FF, 0xF1F1F1FF, 0xEFEFEFFF, 0xECECECFF, 0xE1E1E1FF, 0xE4E4E4FF, 0xEAEAEAFF, 0xEBEBEBFF,
		0xA8A8A8FF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0xB2B2B2FF, 0xF7F7F7FF, 0xF4F4F4FF, 0xF3F3F3FF, 0xF2F2F2FF, 0xE8E8E8FF, 0xD2D2D2FF, 0x828282EF, 0xC9C9C9FF, 0xC1C1C1FF, 0x818181EB,
		0x00000000, 0x4646462F, 0xA2A2A2FF, 0x909090FF, 0x959595FF, 0x818181EB, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xADADADFF, 0xDADADAFF, 0x8D8D8DFF, 0x6363638C, 0xC0C0C0FF, 0xC4C4C4FF,
		0x7E7E7EE2, 0x00000000, 0x57575766, 0x5D5D5D79, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4040401C,
		0x3E3E3E16, 0x00000000, 0x00000000, 0x3B3B3B0C, 0x6B6B6BA5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};
static const unsigned int* const awIconData[ 12 ] = {
awIconData0, awIconData1, awIconData2, awIconData3, awIconData4, awIconData5, awIconData6, awIconData7, awIconData8, awIconData9, awIconData10, awIconData11
};

static ImFontAtlasRectId g_V3IconId[ 12 ] = { ImFontAtlasRectId_Invalid };
static ImFontAtlasRect g_V3Icon[ 12 ];
static bool g_V3IconReady[ 12 ] = { false };

void GUI_BuildIconsV3( void )
{
	ImFontAtlas* atlas = ImGui::GetIO( ).Fonts;
	for( int i = 0; i < 12; i++ )
	{
		if( g_V3IconId[ i ] == ImFontAtlasRectId_Invalid )
			g_V3IconId[ i ] = atlas->AddCustomRect( awIconW[ i ], awIconH[ i ] );
		if( g_V3IconId[ i ] == ImFontAtlasRectId_Invalid ) return;
	}
	atlas->TexPixelsUseColors = true;
	unsigned char* pix = NULL; int pw = 0, ph = 0, pbpp = 0;
	atlas->GetTexDataAsRGBA32( &pix, &pw, &ph, &pbpp );
	if( !pix || pbpp != 4 ) return;
	for( int i = 0; i < 12; i++ )
	{
		ImFontAtlasRect r;
		if( !atlas->GetCustomRect( g_V3IconId[ i ], &r ) ) continue;
		g_V3Icon[ i ] = r;
		const unsigned int* src = awIconData[ i ];
		for( int py = 0; py < awIconH[ i ]; py++ )
			for( int px = 0; px < awIconW[ i ]; px++ )
			{
				unsigned int v = src[ py * awIconW[ i ] + px ];
				unsigned char* d = pix + ( ( ( int )r.y + py ) * pw + ( ( int )r.x + px ) ) * 4;
				d[ 0 ] = ( unsigned char )( ( v >> 24 ) & 0xFF ); d[ 1 ] = ( unsigned char )( ( v >> 16 ) & 0xFF );
				d[ 2 ] = ( unsigned char )( ( v >> 8 ) & 0xFF ); d[ 3 ] = ( unsigned char )( v & 0xFF );
			}
		g_V3IconReady[ i ] = true;
	}
}

static void V3_DrawIcon( ImDrawList* dl, ImVec2 center, int idx )
{
	if( idx < 0 || idx >= 12 ) return;
	if( !g_V3IconReady[ idx ] ) return;
	ImFontAtlas* atlas = ImGui::GetIO( ).Fonts;
	float iw = ( float )g_V3Icon[ idx ].w, ih = ( float )g_V3Icon[ idx ].h;
	ImVec2 imin = ImVec2( center.x - iw * 0.5f, center.y - ih * 0.5f );
	dl->AddImage( atlas->TexRef, imin, ImVec2( imin.x + iw, imin.y + ih ), g_V3Icon[ idx ].uv0, g_V3Icon[ idx ].uv1 );
}

static void V3_TabIcon( ImDrawList* dl, ImVec2 c, float s, int id )
{
	switch( id )
	{
		case 0: // legitbot: green crosshair
		{
			ImU32 gc = IM_COL32( 60, 181, 33, 255 );
			dl->AddCircle( c, s * 0.62f, gc, 24, 1.6f );
			dl->AddLine( ImVec2( c.x, c.y - s ), ImVec2( c.x, c.y - s * 0.35f ), gc, 1.6f );
			dl->AddLine( ImVec2( c.x, c.y + s * 0.35f ), ImVec2( c.x, c.y + s ), gc, 1.6f );
			dl->AddLine( ImVec2( c.x - s, c.y ), ImVec2( c.x - s * 0.35f, c.y ), gc, 1.6f );
			dl->AddLine( ImVec2( c.x + s * 0.35f, c.y ), ImVec2( c.x + s, c.y ), gc, 1.6f );
			dl->AddCircleFilled( c, s * 0.16f, gc );
			break;
		}
		case 1: // ragebot: red crosshair with diagonals
		{
			ImU32 rc = IM_COL32( 232, 38, 31, 255 );
			dl->AddCircle( c, s * 0.55f, rc, 24, 1.6f );
			dl->AddLine( ImVec2( c.x, c.y - s ), ImVec2( c.x, c.y - s * 0.3f ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x, c.y + s * 0.3f ), ImVec2( c.x, c.y + s ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x - s, c.y ), ImVec2( c.x - s * 0.3f, c.y ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x + s * 0.3f, c.y ), ImVec2( c.x + s, c.y ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x - s * 0.66f, c.y - s * 0.66f ), ImVec2( c.x - s * 0.3f, c.y - s * 0.3f ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x + s * 0.3f, c.y + s * 0.3f ), ImVec2( c.x + s * 0.66f, c.y + s * 0.66f ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x + s * 0.66f, c.y - s * 0.66f ), ImVec2( c.x + s * 0.3f, c.y - s * 0.3f ), rc, 1.6f );
			dl->AddLine( ImVec2( c.x - s * 0.66f, c.y + s * 0.66f ), ImVec2( c.x - s * 0.3f, c.y + s * 0.3f ), rc, 1.6f );
			break;
		}
		case 2: // visuals: three green bars
			dl->AddRectFilled( ImVec2( c.x - s * 0.75f, c.y + s * 0.05f ), ImVec2( c.x - s * 0.4f, c.y + s * 0.85f ), IM_COL32( 60, 181, 33, 255 ), 1.0f );
			dl->AddRectFilled( ImVec2( c.x - s * 0.18f, c.y - s * 0.75f ), ImVec2( c.x + s * 0.17f, c.y + s * 0.85f ), IM_COL32( 60, 181, 33, 255 ), 1.0f );
			dl->AddRectFilled( ImVec2( c.x + s * 0.4f, c.y - s * 0.35f ), ImVec2( c.x + s * 0.75f, c.y + s * 0.85f ), IM_COL32( 60, 181, 33, 255 ), 1.0f );
			break;
		case 3: // misc: wrench-ish ring + handle
			dl->AddCircle( ImVec2( c.x + s * 0.3f, c.y - s * 0.3f ), s * 0.5f, IM_COL32( 207, 207, 207, 255 ), 20, 2.2f );
			dl->AddLine( ImVec2( c.x - s * 0.1f, c.y + s * 0.1f ), ImVec2( c.x - s * 0.7f, c.y + s * 0.7f ), IM_COL32( 207, 207, 207, 255 ), 2.6f );
			break;
		case 4: // colors: palette with dots
			dl->AddCircleFilled( c, s * 0.85f, IM_COL32( 201, 201, 201, 255 ) );
			dl->AddCircleFilled( ImVec2( c.x - s * 0.3f, c.y - s * 0.25f ), s * 0.14f, IM_COL32( 232, 38, 31, 255 ) );
			dl->AddCircleFilled( ImVec2( c.x + s * 0.05f, c.y - s * 0.4f ), s * 0.14f, IM_COL32( 247, 179, 43, 255 ) );
			dl->AddCircleFilled( ImVec2( c.x + s * 0.38f, c.y - s * 0.15f ), s * 0.14f, IM_COL32( 60, 181, 33, 255 ) );
			dl->AddCircleFilled( ImVec2( c.x - s * 0.05f, c.y + s * 0.2f ), s * 0.14f, IM_COL32( 46, 124, 230, 255 ) );
			break;
		case 5: // gui: four color bars
			dl->AddRectFilled( ImVec2( c.x - s * 0.8f, c.y - s * 0.75f ), ImVec2( c.x + s * 0.8f, c.y - s * 0.45f ), IM_COL32( 232, 38, 31, 255 ), 1.0f );
			dl->AddRectFilled( ImVec2( c.x - s * 0.8f, c.y - s * 0.3f ), ImVec2( c.x + s * 0.8f, c.y ), IM_COL32( 247, 179, 43, 255 ), 1.0f );
			dl->AddRectFilled( ImVec2( c.x - s * 0.8f, c.y + s * 0.15f ), ImVec2( c.x + s * 0.8f, c.y + s * 0.45f ), IM_COL32( 60, 181, 33, 255 ), 1.0f );
			dl->AddRectFilled( ImVec2( c.x - s * 0.8f, c.y + s * 0.6f ), ImVec2( c.x + s * 0.8f, c.y + s * 0.9f ), IM_COL32( 46, 124, 230, 255 ), 1.0f );
			break;
		case 6: // settings: gear-ish
		{
			dl->AddCircle( c, s * 0.62f, IM_COL32( 207, 207, 207, 255 ), 20, 2.4f );
			for( int k = 0; k < 4; k++ )
			{
				float a = k * 0.785398f;
				dl->AddLine( ImVec2( c.x + cos( a ) * s * 0.62f, c.y + sin( a ) * s * 0.62f ), ImVec2( c.x + cos( a ) * s * 0.95f, c.y + sin( a ) * s * 0.95f ), IM_COL32( 207, 207, 207, 255 ), 2.0f );
			}
			break;
		}
		default: // scripts: braces
			dl->AddText( ImVec2( c.x - s * 0.75f, c.y - s * 0.9f ), IM_COL32( 207, 207, 207, 255 ), "{ }" );
			break;
	}
}

static void V3_Page_Legitbot( void )
{
	float halfWidth = V3_HalfWidth( );

	ImGui::BeginChild( "V3LegL", ImVec2( halfWidth, 0 ), true );
	{
		V3_PanelHeader( "Accuracy" );
		AwCheckbox( "Enabled", &g_CVars.Legit.Active );
		static const char* legitAimTypeNames[] = { "Snap", "Smooth", "Adaptive" };
		ImGui::Combo( "Aim Type", &g_CVars.Legit.AimType, legitAimTypeNames, IM_ARRAYSIZE( legitAimTypeNames ) );
		static const char* v3LegKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		ImGui::Combo( "Aim Key", &g_CVars.Legit.Key, v3LegKeyNames, IM_ARRAYSIZE( v3LegKeyNames ) );
		AwSliderInt( "Smoothing", &g_CVars.Legit.Smoothing, 1, 30 );
		AwSliderInt( "Reaction Ms", &g_CVars.Legit.ReactionMs, 0, 400 );
		AwSliderInt( "Kill Delay Ms", &g_CVars.Legit.KillDelayMs, 0, 1000 );
		AwCheckbox( "Prediction", &g_CVars.Legit.Prediction );
		AwSliderInt( "RCS %", &g_CVars.Legit.RCS, 0, 100 );
		AwCheckbox( "Standalone RCS", &g_CVars.Legit.RCSStandalone );
		AwSliderInt( "Aim FOV (Snap mode)", &g_CVars.Legit.AimFOV, 1, 30 );
		AwCheckbox( "FOV Circle", &g_CVars.Legit.FovCircle );
		AwCheckbox( "Flash Check", &g_CVars.Legit.FlashCheck );
		AwCheckbox( "Scoped Check (AWP)", &g_CVars.Legit.ScopedCheck );
		AwCheckbox( "Auto Scope (AWP)", &g_CVars.Legit.AutoScope );
		AwCheckbox( "Desync Resolver", &g_CVars.Legit.DesyncResolver );

		ImGui::Spacing( );
		V3_PanelHeader( "Target" );
		AwSliderInt( "FOV Near", &g_CVars.Legit.FovNear, 1, 30 );
		AwSliderInt( "FOV Far", &g_CVars.Legit.FovFar, 1, 30 );
		AwSliderInt( "FOV Switch Dist (0 = off)", &g_CVars.Legit.FovSwitchDist, 0, 1500 );
		static const char* legitTargetNames[] = { "Distance", "Health", "Next Shot", "Random", "Crosshair" };
		if( g_CVars.Legit.TargetSelection < 0 || g_CVars.Legit.TargetSelection > 4 ) g_CVars.Legit.TargetSelection = 4;
		ImGui::Combo( "Target", &g_CVars.Legit.TargetSelection, legitTargetNames, IM_ARRAYSIZE( legitTargetNames ) );
		static const char* legitHitboxNames[] = { "Head", "Neck", "Chest", "Stomach" };
		int legitHitboxIdx = GetHitboxIndex( g_CVars.Legit.Hitbox );
		if( ImGui::Combo( "Hitbox", &legitHitboxIdx, legitHitboxNames, IM_ARRAYSIZE( legitHitboxNames ) ) )
			g_CVars.Legit.Hitbox = HitboxFromIndex( legitHitboxIdx );
		AwSliderInt( "Backtrack", &g_CVars.Legit.BacktrackTicks, 0, 12 );
		AwCheckbox( "Aim Lock", &g_CVars.Legit.AimLock );
		AwCheckbox( "Auto Shoot", &g_CVars.Legit.AutoShoot );
		AwCheckbox( "Auto Stop", &g_CVars.Legit.AutoStop );
		AwCheckbox( "Auto Pistol", &g_CVars.Legit.AutoPistol );

		ImGui::Spacing( );
		V3_PanelHeader( "Filter (Hitbox Groups)" );
		static const char* legitGroupNames[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs" };
		static const char* legitGroupModeNames[] = { "Off", "Scan", "Priority" };
		for( int lg = 0; lg < 7; lg++ )
		{
			int lmode = g_CVars.Legit.HitboxGroup[ lg ];
			if( lmode < 0 || lmode > 2 ) lmode = 1;
			ImGui::PushID( 300 + lg );
			ImGui::Combo( legitGroupNames[ lg ], &lmode, legitGroupModeNames, IM_ARRAYSIZE( legitGroupModeNames ) );
			g_CVars.Legit.HitboxGroup[ lg ] = lmode;
			ImGui::PopID( );
		}
		if( ImGui::Button( "All Scan", ImVec2( 80, 0 ) ) )
			for( int lg = 0; lg < 7; lg++ ) g_CVars.Legit.HitboxGroup[ lg ] = 1;
		ImGui::SameLine( );
		if( ImGui::Button( "Upper", ImVec2( 80, 0 ) ) )
		{
			for( int lg = 0; lg < 7; lg++ ) g_CVars.Legit.HitboxGroup[ lg ] = 0;
			g_CVars.Legit.HitboxGroup[ 0 ] = 2;
			g_CVars.Legit.HitboxGroup[ 1 ] = 1;
			g_CVars.Legit.HitboxGroup[ 2 ] = 1;
			g_CVars.Legit.HitboxGroup[ 3 ] = 1;
		}
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "V3LegR", ImVec2( 0, 0 ), true );
	{
		V3_PanelHeader( "Aimbot" );
		static const char* v3LegAimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		ImGui::Combo( "Aim Key##V3LA", &g_CVars.Legit.Key, v3LegAimKeyNames, IM_ARRAYSIZE( v3LegAimKeyNames ) );
		AwCheckbox( "Silent Aim", &g_CVars.Legit.Silent );

		ImGui::Spacing( );
		V3_PanelHeader( "Triggerbot" );
		AwCheckbox( "Triggerbot Active", &g_CVars.Triggerbot.Active );
		AwCheckbox( "Seed Check", &g_CVars.Triggerbot.Seed );
		AwCheckbox( "Spread Check", &g_CVars.Triggerbot.Spread );
		AwCheckbox( "Recoil Check", &g_CVars.Triggerbot.Recoil );
		AwSliderInt( "Trigger Delay Ms", &g_CVars.Triggerbot.Delay, 0, 300 );
		static const char* triggerStrengthNames[] = { "Low", "Medium", "High", "Extra" };
		if( g_CVars.Triggerbot.Strength < 0 || g_CVars.Triggerbot.Strength >= 4 ) g_CVars.Triggerbot.Strength = 0;
		ImGui::Combo( "Strength", &g_CVars.Triggerbot.Strength, triggerStrengthNames, IM_ARRAYSIZE( triggerStrengthNames ) );
		static const char* triggerHitboxNames[] = { "Head", "Upper Body", "Lower Body", "Full Body" };
		if( g_CVars.Triggerbot.Hitbox < 0 || g_CVars.Triggerbot.Hitbox >= 4 ) g_CVars.Triggerbot.Hitbox = 0;
		ImGui::Combo( "Trigger Hitbox", &g_CVars.Triggerbot.Hitbox, triggerHitboxNames, IM_ARRAYSIZE( triggerHitboxNames ) );
		static const char* v3TrigKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Triggerbot.Key < 0 || g_CVars.Triggerbot.Key >= 6 ) g_CVars.Triggerbot.Key = 0;
		ImGui::Combo( "Trigger Key", &g_CVars.Triggerbot.Key, v3TrigKeyNames, IM_ARRAYSIZE( v3TrigKeyNames ) );

		ImGui::Spacing( );
		V3_PanelHeader( "Legit Anti-Aim" );
		AwCheckbox( "Legit AA", &g_CVars.Legit.LegitAA );
		static const char* legitAAKeyNames[] = { "Always", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Legit.LegitAAKey < 0 || g_CVars.Legit.LegitAAKey >= 6 ) g_CVars.Legit.LegitAAKey = 0;
		ImGui::Combo( "Legit AA Key", &g_CVars.Legit.LegitAAKey, legitAAKeyNames, IM_ARRAYSIZE( legitAAKeyNames ) );
		static const char* legitAAInvertNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Legit.LegitAAInvertKey < 0 || g_CVars.Legit.LegitAAInvertKey >= 6 ) g_CVars.Legit.LegitAAInvertKey = 0;
		ImGui::Combo( "Legit AA Inverter", &g_CVars.Legit.LegitAAInvertKey, legitAAInvertNames, IM_ARRAYSIZE( legitAAInvertNames ) );
		AwSliderInt( "Legit AA Angle", &g_CVars.Legit.LegitAAAngle, 0, 45, "%d deg" );
		ImGui::TextDisabled( "Silent yaw padding, skipped while shooting." );
		AwCheckbox( "Fake On Choke", &g_CVars.Legit.DesyncAA );
		AwSliderInt( "Fake Yaw", &g_CVars.Legit.DesyncYaw, -180, 180 );
		if( g_CVars.Legit.DesyncChoke < 1 || g_CVars.Legit.DesyncChoke > 14 ) g_CVars.Legit.DesyncChoke = 6;
		AwSliderInt( "Fake Choke Ticks", &g_CVars.Legit.DesyncChoke, 1, 14, "%d ticks" );
		ImGui::TextDisabled( "Auto-fakelag keeps the burst so the fake is seen." );

		ImGui::Spacing( );
		V3_PanelHeader( "Extra" );
		AwCheckbox( "Snap Limiter", &g_CVars.Legit.SnapLimiter );
		AwSliderInt( "Angle Limit", &g_CVars.Legit.AngleLimit, 0, 180 );
		AwSliderFloat( "Angle Limit Tens", &g_CVars.Legit.AngleLimitTens, 0.0f, 1.0f, "%.2f" );
		ImGui::Spacing( );
		if( ImGui::Button( "Apply Legit", ImVec2( 110, 0 ) ) ) ApplyLegitPreset( );
		ImGui::SameLine( );
		if( ImGui::Button( "Apply Rage", ImVec2( 110, 0 ) ) ) ApplyRagePreset( );
	}
	ImGui::EndChild( );
}

static void V3_Page_Ragebot( void )
{
	float halfWidth = V3_HalfWidth( );
	static int v3RageTarget = 0; // 0 Main / 1 Hitbox
	static int v3RageMode = 0;   // 0 Weapons / 1 Anti-Aim
	static int v3Weap = 0;

	ImGui::BeginChild( "V3RageL", ImVec2( halfWidth, 0 ), true );
	{
		V3_PanelHeader( "Aimbot" );
		static const char* aimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Aimbot.Key < 0 || g_CVars.Aimbot.Key >= 6 ) g_CVars.Aimbot.Key = 0;
		ImGui::Combo( "Aim Key", &g_CVars.Aimbot.Key, aimKeyNames, IM_ARRAYSIZE( aimKeyNames ) );
		AwSliderInt( "Aim FOV (0 = 360)", &g_CVars.Aimbot.AimFOV, 0, 180 );
		AwSliderInt( "Long Range (0=off)", &g_CVars.Aimbot.LongRangeDist, 0, 2000 );
		AwCheckbox( "Silent Aim", &g_CVars.Aimbot.Silent );
		AwCheckbox( "Perfect Silent", &g_CVars.Aimbot.PerfectSilent );
		AwCheckbox( "Multi Spot", &g_CVars.Aimbot.MultiSpot );
		AwCheckbox( "Hit Scan", &g_CVars.Aimbot.HitScan );
		AwCheckbox( "Perfect Auto Wall", &g_CVars.Aimbot.AutoWall );
		AwCheckbox( "Anti SMAC", &g_CVars.Aimbot.AntiSMAC );
		AwCheckbox( "Friendly Fire", &g_CVars.Aimbot.FriendlyFire );
		AwCheckbox( "Body AWP", &g_CVars.Aimbot.BodyAWP );

		ImGui::Spacing( );
		V3_PanelHeader( "Target" );
		if( v3RageTarget < 0 || v3RageTarget > 1 ) v3RageTarget = 0;
		if( ImGui::Button( "Main", ImVec2( halfWidth * 0.5f - 2.0f, 0 ) ) ) v3RageTarget = 0;
		ImGui::SameLine( );
		if( ImGui::Button( "Hitbox", ImVec2( halfWidth * 0.5f - 2.0f, 0 ) ) ) v3RageTarget = 1;

		if( v3RageTarget == 0 )
		{
			static const char* targetSelectionNames[] = { "Distance", "Health", "Next Shot", "Random", "Crosshair" };
			if( g_CVars.Aimbot.TargetSelection < 0 || g_CVars.Aimbot.TargetSelection >= 5 ) g_CVars.Aimbot.TargetSelection = 0;
			ImGui::Combo( "Selection", &g_CVars.Aimbot.TargetSelection, targetSelectionNames, IM_ARRAYSIZE( targetSelectionNames ) );
			AwSliderInt( "Min Damage", &g_CVars.Aimbot.MinDamage, 0, 100 );
			AwCheckbox( "Hit Chance", &g_CVars.Aimbot.HitChance );
			AwSliderInt( "Min Hit Chance", &g_CVars.Aimbot.HitChanceValue, 0, 100 );
			static const char* forceKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
			if( g_CVars.Aimbot.ForceBodyKey < 0 || g_CVars.Aimbot.ForceBodyKey >= 6 ) g_CVars.Aimbot.ForceBodyKey = 0;
			ImGui::Combo( "Force Body Key", &g_CVars.Aimbot.ForceBodyKey, forceKeyNames, IM_ARRAYSIZE( forceKeyNames ) );
			if( g_CVars.Aimbot.ForceMinDmgKey < 0 || g_CVars.Aimbot.ForceMinDmgKey >= 6 ) g_CVars.Aimbot.ForceMinDmgKey = 0;
			ImGui::Combo( "Force MinDmg Key", &g_CVars.Aimbot.ForceMinDmgKey, forceKeyNames, IM_ARRAYSIZE( forceKeyNames ) );
			AwSliderInt( "Force Min Damage", &g_CVars.Aimbot.ForceMinDmgValue, 1, 100 );
			const char* posAdjustmentNames[] = { "Off", "On", "On + History" };
			ImGui::Combo( "Pos Adjustment", &g_CVars.Aimbot.Interpolation.LagPrediction, posAdjustmentNames, IM_ARRAYSIZE( posAdjustmentNames ) );
			AwSliderInt( "Backtrack Ticks", &g_CVars.Aimbot.BacktrackTicks, 0, 12 );
		}
		else
		{
			static const char* primHbNames[] = { "Head", "Neck", "Chest", "Stomach" };
			int primSel = g_CVars.Aimbot.Hitbox;
			if( primSel < 9 || primSel > 12 ) primSel = 12;
			int primIdx = 12 - primSel;
			if( ImGui::Combo( "Primary Hitbox", &primIdx, primHbNames, 4 ) )
				g_CVars.Aimbot.Hitbox = 12 - primIdx;
			static const char* fbHbNames[] = { "Off (group scan)", "Head", "Neck", "Chest", "Stomach" };
			int fbSel = g_CVars.Aimbot.FallbackHitbox;
			if( fbSel < 0 ) fbSel = 0;
			int fbIdx = ( fbSel >= 9 && fbSel <= 12 ) ? ( 13 - fbSel ) : 0;
			if( ImGui::Combo( "Fallback Hitbox", &fbIdx, fbHbNames, 5 ) )
				g_CVars.Aimbot.FallbackHitbox = ( fbIdx > 0 ) ? ( 13 - fbIdx ) : 0;
			AwCheckbox( "Strict Primary", &g_CVars.Aimbot.StrictPrimary );
			AwCheckbox( "Best Damage", &g_CVars.Aimbot.BestDamage );
			static const char* priorityNames[] = { "Off", "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs" };
			int prioIdx = 0;
			for( int hg = 0; hg < 7; hg++ )
				if( g_CVars.Aimbot.HitboxGroup[ hg ] == 2 ) prioIdx = hg + 1;
			if( prioIdx < 0 || prioIdx > 7 ) prioIdx = 0;
			if( ImGui::Combo( "Hitbox Priority", &prioIdx, priorityNames, IM_ARRAYSIZE( priorityNames ) ) )
			{
				for( int hg = 0; hg < 7; hg++ )
					g_CVars.Aimbot.HitboxGroup[ hg ] = ( prioIdx == 0 ) ? 1 : ( ( hg == prioIdx - 1 ) ? 2 : 1 );
			}
			AwSliderFloat( "Point Scale", &g_CVars.Aimbot.PointScale, 0.0f, 1.0f, "%.2f" );
			static const char* heightModeNames[] = { "Auto", "Origin", "Center", "Center Fixed", "Highest" };
			if( g_CVars.Aimbot.HitboxMode < 0 || g_CVars.Aimbot.HitboxMode >= 5 ) g_CVars.Aimbot.HitboxMode = 0;
			ImGui::Combo( "Height Mode", &g_CVars.Aimbot.HitboxMode, heightModeNames, IM_ARRAYSIZE( heightModeNames ) );
		}

		ImGui::Spacing( );
		V3_PanelHeader( "Accuracy" );
		AwCheckbox( "Remove Recoil / Spread", &g_CVars.Accuracy.PerfectAccuracy );
		AwCheckbox( "Force Seed", &g_CVars.Accuracy.ForceSeed );
		static const char* spreadModeNames[] = { "NULL", "Classic", "Iterative", "Rotation" };
		if( g_CVars.Accuracy.NoSpreadMode < 0 || g_CVars.Accuracy.NoSpreadMode >= 4 ) g_CVars.Accuracy.NoSpreadMode = 0;
		ImGui::Combo( "NoSpread Mode", &g_CVars.Accuracy.NoSpreadMode, spreadModeNames, IM_ARRAYSIZE( spreadModeNames ) );

		ImGui::Spacing( );
		V3_PanelHeader( "Resolver" );
		AwCheckbox( "Resolver Active", &g_CVars.Aimbot.Resolver.Active );
		static const char* resolverModeNames[] = { "Everyone", "Selected" };
		if( g_CVars.Aimbot.Resolver.Mode < 0 || g_CVars.Aimbot.Resolver.Mode >= 2 ) g_CVars.Aimbot.Resolver.Mode = 0;
		ImGui::Combo( "Resolver Target", &g_CVars.Aimbot.Resolver.Mode, resolverModeNames, IM_ARRAYSIZE( resolverModeNames ) );
		static const char* resolverTypeNames[] = { "Spin", "Back Twitch", "Alternative", "2 bullets", "Anim Test", "AI Learn", "Honest Shot" };
		if( g_CVars.Aimbot.Resolver.Type < 0 || g_CVars.Aimbot.Resolver.Type >= 7 ) g_CVars.Aimbot.Resolver.Type = 0;
		ImGui::Combo( "Resolver Type", &g_CVars.Aimbot.Resolver.Type, resolverTypeNames, IM_ARRAYSIZE( resolverTypeNames ) );
		AwCheckbox( "Lag Records (Anti-Jitter)", &g_CVars.Aimbot.Resolver.LagRecords );
		AwCheckbox( "Smart Resolver", &g_CVars.Aimbot.Resolver.Smart );

		ImGui::Spacing( );
		V3_PanelHeader( "Snap Limiter" );
		AwCheckbox( "Snap Limiter Active", &g_CVars.Aimbot.SnapLimiter );
		AwCheckbox( "Disable Enemy Interpolation", &g_CVars.Aimbot.Interpolation.DisableInterp );
		AwSliderInt( "Angle Limit", &g_CVars.Aimbot.AngleLimit, 0, 180 );
		AwSliderFloat( "Angle Limit Tens", &g_CVars.Aimbot.AngleLimitTens, 0.0f, 1.0f, "%.2f" );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "V3RageR", ImVec2( 0, 0 ), true );
	{
		if( v3RageMode < 0 || v3RageMode > 1 ) v3RageMode = 0;
		if( ImGui::Button( "Weapons", ImVec2( halfWidth * 0.5f - 2.0f, 0 ) ) ) v3RageMode = 0;
		ImGui::SameLine( );
		if( ImGui::Button( "Anti-Aim", ImVec2( halfWidth * 0.5f - 2.0f, 0 ) ) ) v3RageMode = 1;
		ImGui::Spacing( );

		if( v3RageMode == 0 )
		{
			// r44: weapon strip (visual grouping - rage settings are global in this build)
			static const char* weapNames[] = { "Pistol", "SMG", "Rifle", "Shotgun", "Sniper" };
			if( v3Weap < 0 || v3Weap > 4 ) v3Weap = 0;
			{
				// r46: V3 weapon strip - dark cells + ripped silhouettes + label
				ImDrawList* wdl = ImGui::GetWindowDrawList( );
				float cw = ( ImGui::GetContentRegionAvail( ).x - 4.0f * 5.0f ) / 5.0f, ch = 44.0f;
				ImVec2 sp = ImGui::GetCursorScreenPos( );
				for( int wi = 0; wi < 5; wi++ )
				{
					ImVec2 cmin = ImVec2( sp.x + wi * ( cw + 4.0f ), sp.y ), cmax = ImVec2( cmin.x + cw, sp.y + ch );
					bool wActive = ( v3Weap == wi );
					ImGui::PushID( 700 + wi );
					ImGui::SetCursorScreenPos( cmin );
					ImGui::InvisibleButton( "v3weap", ImVec2( cw, ch ) );
					if( ImGui::IsItemClicked( ) ) v3Weap = wi;
					ImGui::PopID( );
					wdl->AddRectFilled( cmin, cmax, wActive ? IM_COL32( 40, 40, 40, 255 ) : IM_COL32( 21, 21, 21, 255 ), 0 );
					wdl->AddRect( cmin, cmax, IM_COL32( 0, 0, 0, 255 ), 0 );
					if( g_V3IconReady[ 7 + wi ] )
					{
						ImFontAtlas* watlas = ImGui::GetIO( ).Fonts;
						float iw = ( float )g_V3Icon[ 7 + wi ].w, ih = ( float )g_V3Icon[ 7 + wi ].h;
						float k = ( iw > cw - 8.0f ) ? ( cw - 8.0f ) / iw : 1.0f; // native pixels when it fits
						ImVec2 icn = ImVec2( ( cmin.x + cmax.x ) * 0.5f - iw * k * 0.5f, sp.y + 3.0f );
						wdl->AddImage( watlas->TexRef, icn, ImVec2( icn.x + iw * k, icn.y + ih * k ), g_V3Icon[ 7 + wi ].uv0, g_V3Icon[ 7 + wi ].uv1 );
					}
					ImVec2 ts = ImGui::CalcTextSize( weapNames[ wi ] );
					wdl->AddText( ImVec2( ( cmin.x + cmax.x ) * 0.5f - ts.x * 0.5f, sp.y + ch - 15.0f ), IM_COL32( 235, 235, 235, 255 ), weapNames[ wi ] );
				}
				ImGui::Dummy( ImVec2( 0, ch ) );
			}
			// r45: per-weapon-group rage profiles
			AwCheckbox( "Use weapon groups", &g_CVars.Aimbot.RageGroups );
			ImGui::TextDisabled( "On: active weapon uses its group values. Off: global only." );
			char wtitle[ 32 ];
			sprintf( wtitle, "%s Group", weapNames[ v3Weap ] );
			V3_PanelHeader( wtitle );
			AwCheckbox( "Auto Shoot##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].AutoShoot );
			AwCheckbox( "Auto Stop##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].AutoStop );
			AwCheckbox( "Perfect Auto Wall##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].AutoWall );
			AwCheckbox( "Multi Spot##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].MultiSpot );
			AwCheckbox( "Hit Scan##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].HitScan );
			AwCheckbox( "Body Aim vs Jump##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].BodyVsJump );
			AwCheckbox( "Body AWP##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].BodyAWP );
			AwCheckbox( "Anti SMAC##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].AntiSMAC );
			static const char* v3GrpPrim[] = { "Head", "Neck", "Chest", "Stomach" };
			int grpPrim = g_CVars.Aimbot.RageGroup[ v3Weap ].Hitbox;
			if( grpPrim < 9 || grpPrim > 12 ) grpPrim = 12;
			int grpPrimIdx = 12 - grpPrim;
			if( ImGui::Combo( "Primary Hitbox##G", &grpPrimIdx, v3GrpPrim, 4 ) )
				g_CVars.Aimbot.RageGroup[ v3Weap ].Hitbox = 12 - grpPrimIdx;
			static const char* v3GrpFb[] = { "Off (group scan)", "Head", "Neck", "Chest", "Stomach" };
			int grpFb = g_CVars.Aimbot.RageGroup[ v3Weap ].FallbackHitbox;
			if( grpFb < 0 ) grpFb = 0;
			int grpFbIdx = ( grpFb >= 9 && grpFb <= 12 ) ? ( 13 - grpFb ) : 0;
			if( ImGui::Combo( "Fallback Hitbox##G", &grpFbIdx, v3GrpFb, 5 ) )
				g_CVars.Aimbot.RageGroup[ v3Weap ].FallbackHitbox = ( grpFbIdx > 0 ) ? ( 13 - grpFbIdx ) : 0;
			AwCheckbox( "Strict Primary##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].StrictPrimary );
			AwCheckbox( "Best Damage##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].BestDamage );
			AwSliderInt( "Min Damage##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].MinDamage, 0, 100 );
			AwCheckbox( "Hit Chance##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].HitChance );
			AwSliderInt( "Min Hit Chance##G", &g_CVars.Aimbot.RageGroup[ v3Weap ].HitChanceValue, 0, 100 );

			ImGui::Spacing( );
			V3_PanelHeader( "Rage - Double Tap" );
			AwCheckbox( "Double Tap", &g_CVars.Miscellaneous.DoubleTap );
			AwCheckbox( "DT Only On Ground", &g_CVars.Miscellaneous.DoubleTapOnlyGround );
			AwCheckbox( "DT Delay Shot", &g_CVars.Miscellaneous.DoubleTapDelayShot );
			AwSliderInt( "DT Ticks", &g_CVars.Miscellaneous.DoubleTapTicks, 2, 16 );
			AwCheckbox( "DT Auto Ticks", &g_CVars.Miscellaneous.DoubleTapAuto );
			if( g_CVars.Miscellaneous.DoubleTapMode < 0 || g_CVars.Miscellaneous.DoubleTapMode > 1 ) g_CVars.Miscellaneous.DoubleTapMode = 0;
			static const char* dtModeNames[] = { "Offensive", "Defensive (fakelag)" };
			ImGui::Combo( "DT Mode", &g_CVars.Miscellaneous.DoubleTapMode, dtModeNames, IM_ARRAYSIZE( dtModeNames ) );
		}
		else
		{
			V3_PanelHeader( "Anti-Aim (HVH)" );
			AwCheckbox( "Anti-Aim Active", &g_CVars.Miscellaneous.AntiAim.Active );
			ImGui::Text( "Choke hook: %s", g_NetchanHooked ? "ACTIVE" : "waiting..." );
			static const char* pitchNames[] = { "Off", "Normal", "Inverse Normal", "Safe", "Fake Down", "Lisp Down", "Lisp Up", "Lag Down", "Lag Up", "Down 89", "Up -89" };
			if( g_CVars.Miscellaneous.AntiAim.Pitch < 0 || g_CVars.Miscellaneous.AntiAim.Pitch >= 11 ) g_CVars.Miscellaneous.AntiAim.Pitch = 0;
			ImGui::Combo( "Pitch", &g_CVars.Miscellaneous.AntiAim.Pitch, pitchNames, IM_ARRAYSIZE( pitchNames ) );
			static const char* yawNames[] = { "Forwards", "Backwards", "Sideways", "Jitter", "Static", "Static Reversed", "Lisp", "Custom", "Jitter X", "AI", "Defensive", "AI Custom", "Server Hold" };
			if( g_CVars.Miscellaneous.AntiAim.Yaw < 0 || g_CVars.Miscellaneous.AntiAim.Yaw >= 13 ) g_CVars.Miscellaneous.AntiAim.Yaw = 0;
			ImGui::Combo( "Yaw", &g_CVars.Miscellaneous.AntiAim.Yaw, yawNames, IM_ARRAYSIZE( yawNames ) );

			std::vector< const char* > yawVariations;
			int yawSel = g_CVars.Miscellaneous.AntiAim.Yaw;
			if( yawSel == 3 ) yawVariations = { "Normal", "Synced", "Static", "Static Synced" };
			else if( yawSel == 6 ) yawVariations = { "m3nly", "m3nly #2", "Jitter", "1337" };
			else if( yawSel == 7 ) yawVariations = { "Additional", "Static" };
			else if( yawSel == 8 ) yawVariations = { "Wide", "Sway", "Random", "Spin" };
			else if( yawSel == 9 ) yawVariations = { "Auto Learn" };
			else if( yawSel == 10 ) yawVariations = { "Spin + Flick", "Flicker", "Sway Spin", "Laggy", "Max Desync" };
			else if( yawSel == 11 ) yawVariations = { "Full Auto" };
			else yawVariations = { "Normal", "Fake Side 1", "Fake Side 2", "Random" };
			if( g_CVars.Miscellaneous.AntiAim.Variation >= ( int )yawVariations.size( ) )
				g_CVars.Miscellaneous.AntiAim.Variation = 0;
			ImGui::Combo( "Yaw Mode", &g_CVars.Miscellaneous.AntiAim.Variation, yawVariations.data( ), ( int )yawVariations.size( ) );

			if( yawSel == 9 )
			{
				static const char* aiStyleNames[] = { "Back", "Side", "Wide", "Spin", "Spin+Flick", "Flicker", "Sway", "Laggy" };
				static const char* aiStateNames[] = { "stand", "move", "air" };
				int aiSt = AIAA_GetStyle( ); if( aiSt < 0 || aiSt > 7 ) aiSt = 0;
				int aiMv = AIAA_GetState( ); if( aiMv < 0 || aiMv > 2 ) aiMv = 0;
				ImGui::Text( "AI Style: %s (%s)", aiStyleNames[ aiSt ], aiStateNames[ aiMv ] );
			}
			else if( yawSel == 11 )
			{
				static const char* aicRNames[] = { "Back", "Side", "Spin", "Spin+Flick", "Flicker", "Laggy" };
				static const char* aicFNames[] = { "Fwd", "Side+", "Side-", "RevSpin" };
				static const char* aicPNames[] = { "Down", "FakeDown", "Up", "Lisp" };
				static const char* aiStateNames2[] = { "stand", "move", "air" };
				int info = AIC_GetInfo( );
				int r = info % 6, f = ( info / 6 ) % 4, pp = ( info / 24 ) % 4, mv = info / 96;
				if( r < 0 || r > 5 ) r = 0; if( f < 0 || f > 3 ) f = 0;
				if( pp < 0 || pp > 3 ) pp = 0; if( mv < 0 || mv > 2 ) mv = 0;
				ImGui::Text( "AI Custom: %s / %s / %s (%s)", aicRNames[ r ], aicFNames[ f ], aicPNames[ pp ], aiStateNames2[ mv ] );
			}

			AwSliderFloat( "Custom Real Yaw", &g_CVars.Miscellaneous.AntiAim.RealValue, 0.0f, 360.0f, "%.1f deg" );
			AwSliderFloat( "Custom Fake Yaw", &g_CVars.Miscellaneous.AntiAim.FakeValue, 0.0f, 360.0f, "%.1f deg" );

			ImGui::Spacing( );
			V3_PanelHeader( "Flick" );
			AwCheckbox( "Flick Enable", &g_CVars.Miscellaneous.AntiAim.FlickEnable );
			AwSliderInt( "Flick Every", &g_CVars.Miscellaneous.AntiAim.FlickTicks, 2, 30, "%d ticks" );
			AwSliderFloat( "Flick Angle", &g_CVars.Miscellaneous.AntiAim.FlickAngle, 0.0f, 180.0f, "%.1f deg" );
			if( g_CVars.Miscellaneous.AntiAim.FlickSide < 0 || g_CVars.Miscellaneous.AntiAim.FlickSide > 2 ) g_CVars.Miscellaneous.AntiAim.FlickSide = 1;
			static const char* flickSideNames[] = { "Real", "Fake", "Both" };
			ImGui::Combo( "Flick Side", &g_CVars.Miscellaneous.AntiAim.FlickSide, flickSideNames, IM_ARRAYSIZE( flickSideNames ) );
			AwCheckbox( "Flick Random", &g_CVars.Miscellaneous.AntiAim.FlickRandom );
			AwCheckbox( "Flick On Shot", &g_CVars.Miscellaneous.AntiAim.FlickOnShot );
			AwCheckbox( "InAttack Pitch", &g_CVars.Miscellaneous.AntiAim.Static );
			AwCheckbox( "Wall Detection", &g_CVars.Miscellaneous.AntiAim.WallDetection );
			static const char* wallDtcModes[] = { "Normal", "Fake", "Fake Out", "Jitter" };
			ImGui::Combo( "Wall DTC Mode", &g_CVars.Miscellaneous.AntiAim.WallDetectionMode, wallDtcModes, IM_ARRAYSIZE( wallDtcModes ) );
			AwCheckbox( "At Targets", &g_CVars.Miscellaneous.AntiAim.AtTargets );
			AwCheckbox( "Duck In Air", &g_CVars.Miscellaneous.AntiAim.DuckInAir );
			AwCheckbox( "Enemy Check", &g_CVars.Miscellaneous.AntiAim.TurnOff );

			ImGui::Spacing( );
			V3_PanelHeader( "Fakeduck / Micromoves" );
			AwCheckbox( "Fakeduck (hold key)", &g_CVars.Miscellaneous.Fakeduck );
			static const char* fdKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
			if( g_CVars.Miscellaneous.FakeduckKey < 0 || g_CVars.Miscellaneous.FakeduckKey >= 6 ) g_CVars.Miscellaneous.FakeduckKey = 0;
			ImGui::Combo( "Fakeduck Key", &g_CVars.Miscellaneous.FakeduckKey, fdKeyNames, IM_ARRAYSIZE( fdKeyNames ) );
			AwCheckbox( "Micromoves (standing)", &g_CVars.Miscellaneous.Micromoves );
		}
	}
	ImGui::EndChild( );
}

static void V3_Page_Visuals( void )
{
	float thirdW = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x * 2.0f ) / 3.0f;

	ImGui::BeginChild( "V3VisL", ImVec2( thirdW, 0 ), true );
	{
		V3_PanelHeader( "ESP" );
		AwCheckbox( "Bounding Box", &g_CVars.Visuals.ESP.Box );
		static const char* boxStyleNames[] = { "Full", "Corner", "3D" };
		ImGui::Combo( "Box Style", &g_CVars.Visuals.ESP.BoxStyle, boxStyleNames, IM_ARRAYSIZE( boxStyleNames ) );
		AwCheckbox( "Player Name", &g_CVars.Visuals.ESP.Name );
		AwCheckbox( "Health Bar / Text", &g_CVars.Visuals.ESP.Health );
		static const char* hpStyleNames[] = { "Bottom", "Left", "Top" };
		ImGui::Combo( "Bar Style", &g_CVars.Visuals.ESP.HealthStyle, hpStyleNames, IM_ARRAYSIZE( hpStyleNames ) );
		AwCheckbox( "Armor Bar", &g_CVars.Visuals.ESP.Armor );
		AwCheckbox( "Ammo Counter", &g_CVars.Visuals.ESP.Ammo );
		AwCheckbox( "Weapon Name", &g_CVars.Visuals.ESP.Weapon );
		AwCheckbox( "Skeleton / Bone", &g_CVars.Visuals.ESP.Bone );
		AwCheckbox( "Aim Spot", &g_CVars.Visuals.ESP.AimSpot );
		AwCheckbox( "Hitmarker", &g_CVars.Visuals.ESP.Hit );
		AwCheckbox( "Ground ESP", &g_CVars.Visuals.ESP.Ground );
		AwCheckbox( "Enemy Only", &g_CVars.Visuals.ESP.EnemyOnly );
		AwCheckbox( "Dormant ESP", &g_CVars.Visuals.ESP.Dormant );
		AwCheckbox( "Offscreen Arrows", &g_CVars.Visuals.ESP.OOF );
		AwCheckbox( "Fake Skeleton (local)", &g_CVars.Visuals.ESP.Fake );
		AwCheckbox( "Show Fake Pose (ThirdPerson)", &g_CVars.Visuals.ESP.ShowFake );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "V3VisM", ImVec2( thirdW, 0 ), true );
	{
		V3_PanelHeader( "Filter / Logs" );
		AwCheckbox( "Event Log", &g_CVars.Visuals.EventLog );
		AwCheckbox( "Shot Log", &g_CVars.Visuals.ShotLog );
		AwCheckbox( "Spectator List", &g_CVars.Visuals.SpectatorList );
		AwCheckbox( "Indicators (FL/DT/AA)", &g_CVars.Visuals.Indicators );

		ImGui::Spacing( );
		V3_PanelHeader( "Chams & Models" );
		AwCheckbox( "Player Chams", &g_CVars.Visuals.Chams.Active );
		AwCheckbox( "Weapon Chams", &g_CVars.Visuals.Chams.Weapons );
		static const char* chamStyleNames[] = { "Flat", "Lit", "Wireframe", "Glow" };
		ImGui::Combo( "Cham Style", &g_CVars.Visuals.Chams.Style, chamStyleNames, IM_ARRAYSIZE( chamStyleNames ) );
		AwCheckbox( "Model Outline", &g_CVars.Visuals.Chams.Outline );
		AwCheckbox( "Hands Outline", &g_CVars.Visuals.Chams.HandsOutline );
		AwCheckbox( "Chams Enemy Only", &g_CVars.Visuals.Chams.EnemyOnly );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "V3VisR", ImVec2( 0, 0 ), true );
	{
		V3_PanelHeader( "World & Screen" );
		AwCheckbox( "Draw Radar", &g_CVars.Visuals.Radar );
		AwCheckbox( "No Sky", &g_CVars.Visuals.NoSky );
		AwCheckbox( "No Smoke", &g_CVars.Visuals.NoSmoke );
		AwCheckbox( "No Flash", &g_CVars.Visuals.NoFlash );
		AwCheckbox( "No Hands", &g_CVars.Visuals.NoHands );
		AwCheckbox( "No Visual Recoil", &g_CVars.Visuals.NoVisualRecoil );
		AwSliderFloat( "ASUS Walls", &g_CVars.Visuals.ASUS, 0.0f, 1.0f, "%.2f" );
		static const char* crosshairTypeNames[] = { "Off", "Cross", "Dot", "Round" };
		ImGui::Combo( "Crosshair Type", &g_CVars.Visuals.Crosshair.Type, crosshairTypeNames, IM_ARRAYSIZE( crosshairTypeNames ) );
		AwCheckbox( "Dynamic Crosshair", &g_CVars.Visuals.Crosshair.Dynamic );
		AwCheckbox( "Third Person View", &g_CVars.Miscellaneous.ThirdPerson );
		static const char* tpKeyNames[] = { "Off", "Mouse 4", "Mouse 5", "V", "C", "T", "F" };
		static const int tpKeyVK[] = { 0, 0x05, 0x06, 0x56, 0x43, 0x54, 0x46 };
		int curTpKey = 0;
		for( int k = 0; k < 7; k++ ) if( g_CVars.Miscellaneous.ThirdPersonKey == tpKeyVK[ k ] ) curTpKey = k;
		if( ImGui::Combo( "TP Key", &curTpKey, tpKeyNames, IM_ARRAYSIZE( tpKeyNames ) ) )
			g_CVars.Miscellaneous.ThirdPersonKey = tpKeyVK[ curTpKey ];
		AwSliderInt( "TP Distance", &g_CVars.Miscellaneous.ThirdPersonDist, 50, 250 );
	}
	ImGui::EndChild( );
}

static void V3_Page_Misc( void )
{
	static int v3MiscPart = 0;
	float halfWidth = V3_HalfWidth( );

	if( v3MiscPart < 0 || v3MiscPart > 1 ) v3MiscPart = 0;
	if( ImGui::Button( "Part 1", ImVec2( 110, 0 ) ) ) v3MiscPart = 0;
	ImGui::SameLine( );
	if( ImGui::Button( "Part 2", ImVec2( 110, 0 ) ) ) v3MiscPart = 1;
	ImGui::Spacing( );

	if( v3MiscPart == 0 )
	{
		ImGui::BeginChild( "V3Ms1L", ImVec2( halfWidth, 0 ), true );
		{
			V3_PanelHeader( "Menu" );
			static const char* menuKeyNames[] = { "INSERT", "DELETE", "HOME", "END", "F8", "F9" };
			static const int menuKeyVK[] = { 0x2D, 0x2E, 0x24, 0x23, 0x77, 0x78 };
			int curMenuKey = 0;
			for( int k = 0; k < 6; k++ ) if( g_CVars.Miscellaneous.MenuKey == menuKeyVK[ k ] ) curMenuKey = k;
			if( ImGui::Combo( "Menu Key", &curMenuKey, menuKeyNames, IM_ARRAYSIZE( menuKeyNames ) ) )
				g_CVars.Miscellaneous.MenuKey = menuKeyVK[ curMenuKey ];

			ImGui::Spacing( );
			V3_PanelHeader( "Movement" );
			AwCheckbox( "Bunny Hop", &g_CVars.Miscellaneous.BunnyHop );
			AwCheckbox( "Auto Strafe", &g_CVars.Miscellaneous.AutoStrafe );
			static const char* asModeNames[] = { "Classic (hold space)", "Directional (slophook)" };
			ImGui::Combo( "Strafe Mode", &g_CVars.Miscellaneous.AutoStrafeMode, asModeNames, IM_ARRAYSIZE( asModeNames ) );
			AwSliderInt( "Strafe Avoid Dist", &g_CVars.Miscellaneous.StrafeAvoidDist, 16, 256 );
			AwCheckbox( "Circle Strafe (hold V)", &g_CVars.Miscellaneous.CircleStrafe );
			AwCheckbox( "Slow Walk", &g_CVars.Miscellaneous.SlowWalk );
			static const char* slowKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5", "SHIFT" };
			if( g_CVars.Miscellaneous.SlowWalkKey < 0 || g_CVars.Miscellaneous.SlowWalkKey >= 7 ) g_CVars.Miscellaneous.SlowWalkKey = 6;
			ImGui::Combo( "Slow Walk Key", &g_CVars.Miscellaneous.SlowWalkKey, slowKeyNames, IM_ARRAYSIZE( slowKeyNames ) );
			AwSliderInt( "Slow Walk Speed", &g_CVars.Miscellaneous.SlowWalkSpeed, 50, 200 );
			AwCheckbox( "Air Stuck (press F)", &g_CVars.Miscellaneous.AirStuck );
			AwCheckbox( "Auto Knife", &g_CVars.Miscellaneous.AutoKnife );

			ImGui::Spacing( );
			V3_PanelHeader( "Other" );
			AwCheckbox( "Round Say", &g_CVars.Miscellaneous.RoundSay );
			AwCheckbox( "sv_cheats Bypass", &g_CVars.Miscellaneous.CheatsBypass );
		}
		ImGui::EndChild( );

		ImGui::SameLine( );

		ImGui::BeginChild( "V3Ms1R", ImVec2( 0, 0 ), true );
		{
			V3_PanelHeader( "Extra" );
			AwCheckbox( "Player List (separate window)", &g_CVars.Visuals.PlayerList );
			ImGui::TextDisabled( "Friend / Pitch / Yaw overrides in a floating window." );
			ImGui::Spacing( );
			if( ImGui::Button( "Open Configs Folder", ImVec2( 180, 0 ) ) ) WinExec( "explorer.exe C:\\Awesware\\configs", SW_SHOWNORMAL );
			if( ImGui::Button( "Open Scripts Folder", ImVec2( 180, 0 ) ) ) WinExec( "explorer.exe C:\\Awesware\\scripts", SW_SHOWNORMAL );

			ImGui::Spacing( );
			V3_PanelHeader( "Clientmod Emulator" );
			AwCheckbox( "ClientMod Emulator", &g_CVars.Miscellaneous.ClientModEmulator );
			ImGui::TextDisabled( "Anti-detect for clientmod-checked servers. Applies on next inject." );
		}
		ImGui::EndChild( );
	}
	else
	{
		ImGui::BeginChild( "V3Ms2L", ImVec2( halfWidth, 0 ), true );
		{
			V3_PanelHeader( "Speedhack" );
			AwCheckbox( "Speedhack", &g_CVars.Miscellaneous.Speedhack );
			AwSliderInt( "Speedhack Factor", &g_CVars.Miscellaneous.SpeedhackValue, 0, 13 );
			static const char* speedKeyNames[] = { "E", "Mouse 4", "Mouse 5", "ALT", "SHIFT", "X" };
			ImGui::Combo( "Speedhack Key", &g_CVars.Miscellaneous.SpeedhackKey, speedKeyNames, IM_ARRAYSIZE( speedKeyNames ) );

			ImGui::Spacing( );
			V3_PanelHeader( "Fake Lag" );
			AwCheckbox( "Fake Lag Active", &g_CVars.Miscellaneous.Fakelag.Active );
			AwCheckbox( "Fake Lag In Attack", &g_CVars.Miscellaneous.Fakelag.InAttack );
			AwCheckbox( "Fake Lag Air Only", &g_CVars.Miscellaneous.Fakelag.AirOnly );
			AwSliderInt( "Choke Ticks", &g_CVars.Miscellaneous.Fakelag.Value, 0, 14 );
			static const char* fakelagModes[] = { "Factor", "Switch", "Adaptive", "AI Smart" };
			if( g_CVars.Miscellaneous.Fakelag.Mode < 0 || g_CVars.Miscellaneous.Fakelag.Mode >= 4 ) g_CVars.Miscellaneous.Fakelag.Mode = 0;
			ImGui::Combo( "Fake Lag Mode", &g_CVars.Miscellaneous.Fakelag.Mode, fakelagModes, IM_ARRAYSIZE( fakelagModes ) );
		}
		ImGui::EndChild( );

		ImGui::SameLine( );

		ImGui::BeginChild( "V3Ms2R", ImVec2( 0, 0 ), true );
		{
			V3_PanelHeader( "AI Memory" );
			if( ImGui::Button( "Reset AI Memory", ImVec2( 180, 0 ) ) ) { AIResolver_Reset( ); AIAA_Reset( ); AIC_Reset( ); g_Drawing.AddLog( Color( 255, 220, 120, 255 ), "AI memory reset" ); }
			ImGui::TextDisabled( "Clears resolver + AA learning." );
		}
		ImGui::EndChild( );
	}
}

static void V3_Page_Colors( void )
{
	float halfWidth = V3_HalfWidth( );

	ImGui::BeginChild( "V3ColL", ImVec2( halfWidth, 0 ), true );
	{
		V3_PanelHeader( "ESP Colors" );
		ImGuiColorEdit( "CT ESP", g_CVars.ColorSelector.ESP.CT );
		ImGuiColorEdit( "T ESP", g_CVars.ColorSelector.ESP.TT );
		ImGuiColorEdit( "Weapon ESP", g_CVars.ColorSelector.ESP.Wpn );

		ImGui::Spacing( );
		V3_PanelHeader( "Chams Visible" );
		ImGuiColorEdit( "CT Visible", g_CVars.ColorSelector.Chams.CTVis );
		ImGuiColorEdit( "T Visible", g_CVars.ColorSelector.Chams.TTVis );
		ImGuiColorEdit( "Wpn Visible", g_CVars.ColorSelector.Chams.WpnVis );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "V3ColR", ImVec2( 0, 0 ), true );
	{
		V3_PanelHeader( "Chams Hidden" );
		ImGuiColorEdit( "CT Hidden", g_CVars.ColorSelector.Chams.CTInvis );
		ImGuiColorEdit( "T Hidden", g_CVars.ColorSelector.Chams.TTInvis );
		ImGuiColorEdit( "Wpn Hidden", g_CVars.ColorSelector.Chams.WpnInvis );

		ImGui::Spacing( );
		V3_PanelHeader( "Chams Outline" );
		ImGuiColorEdit( "CT Outline", g_CVars.ColorSelector.Chams.CTOutline );
		ImGuiColorEdit( "T Outline", g_CVars.ColorSelector.Chams.TTOutline );
		ImGuiColorEdit( "Wpn Outline", g_CVars.ColorSelector.Chams.WpnOutline );
	}
	ImGui::EndChild( );
}

static void V3_Page_GUI( void )
{
	ImGui::BeginChild( "V3Gui", ImVec2( 0, 0 ), true );
	{
		V3_PanelHeader( "Menu" );
		static const char* v3MenuModeNames[] = { "Classic", "Aimware V4", "V3 (orange)" };
		if( g_CVars.Miscellaneous.MenuMode < 0 || g_CVars.Miscellaneous.MenuMode > 2 ) g_CVars.Miscellaneous.MenuMode = 0;
		ImGui::Combo( "Menu Style", &g_CVars.Miscellaneous.MenuMode, v3MenuModeNames, IM_ARRAYSIZE( v3MenuModeNames ) );
		ImGui::TextDisabled( "Switch styles right here - V3 (orange) is this skin." );
	}
	ImGui::EndChild( );
}

static void RenderPlayerListWindowV3( void )
{
	ImGui::SetNextWindowSize( ImVec2( 430, 240 ), ImGuiCond_FirstUseEver );
	if( ImGui::Begin( "Player List", nullptr, ImGuiWindowFlags_NoCollapse ) )
	{
		g_GUI.RenderPlayerListTab( );
	}
	ImGui::End( );
}

static void RenderTopBarV3( void )
{
	static int section = 0;
	static const char* v3Names[] = { "Legitbot", "Ragebot", "Visuals", "Misc", "Colors", "GUI", "Settings", "Scripts" };
	if( section < 0 || section > 7 ) section = 0;

	float w = ImGui::GetContentRegionAvail( ).x;
	ImDrawList* dl = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );
	float h = 52.0f;
	float tw = w / 8.0f;
	float fs = ImGui::GetFontSize( );
	ImFont* font = ImGui::GetFont( );

	dl->AddRectFilled( p, ImVec2( p.x + w, p.y + h ), IM_COL32( 7, 7, 7, 255 ), 0 );

	for( int b = 0; b < 8; b++ )
	{
		ImVec2 tmin = ImVec2( p.x + tw * b, p.y );
		ImVec2 tmax = ImVec2( p.x + tw * ( b + 1 ), p.y + h );
		bool active = ( section == b );

		ImGui::PushID( 1000 + b );
		ImGui::SetCursorScreenPos( tmin );
		ImGui::InvisibleButton( "v3tab", ImVec2( tw, h ) );
		bool hovered = ImGui::IsItemHovered( );
		if( ImGui::IsItemClicked( ) ) section = b;
		ImGui::PopID( );

		if( active )
			dl->AddRectFilledMultiColor( tmin, tmax, IM_COL32( 255, 138, 0, 255 ), IM_COL32( 255, 138, 0, 255 ), IM_COL32( 194, 42, 0, 255 ), IM_COL32( 194, 42, 0, 255 ) );
		else if( hovered )
			dl->AddRectFilled( tmin, tmax, IM_COL32( 24, 24, 24, 255 ), 0 );

		if( g_V3IconReady[ b ] )
			V3_DrawIcon( dl, ImVec2( ( tmin.x + tmax.x ) * 0.5f, p.y + 15.0f ), b ); // r46: ripped pixels
		else
		{
			ImVec2 ic = ImVec2( ( tmin.x + tmax.x ) * 0.5f, p.y + 15.0f );
			V3_TabIcon( dl, ic, 8.0f, b );
		}

		ImVec2 ts = ImGui::CalcTextSize( v3Names[ b ] );
		dl->AddText( font, fs * 0.82f, ImVec2( ( tmin.x + tmax.x ) * 0.5f - ts.x * 0.41f, p.y + 32.0f ),
			active ? IM_COL32_WHITE : IM_COL32( 214, 214, 214, 255 ), v3Names[ b ] );
	}

	ImGui::Dummy( ImVec2( 0, h ) );

	ImGui::BeginChild( "TabContentV3", ImVec2( 0, 0 ), false );
	{
		switch( section )
		{
			case 0: V3_Page_Legitbot( ); break;
			case 1: V3_Page_Ragebot( ); break;
			case 2: V3_Page_Visuals( ); break;
			case 3: V3_Page_Misc( ); break;
			case 4: V3_Page_Colors( ); break;
			case 5: V3_Page_GUI( ); break;
			case 6: g_GUI.RenderConfigsTab( ); break;
			default: g_GUI.RenderScriptsTab( ); break;
		}
	}
	ImGui::EndChild( );
}

// aimware-style horizontal tab strip + dispatched content
static void RenderTopBar( void )
{
	static int section = 0;
	static const char* sectionNames[] = { "RAGE", "LEGIT", "VISUALS", "MISC", "PLAYERS", "CONFIG", "SCRIPTS" };

	// r44: V3 (AIMWARE) skin owns the whole chrome when selected
	if( g_CVars.Miscellaneous.MenuMode == 2 )
	{
		RenderTopBarV3( );
		return;
	}

	if( section < 0 || section > 6 ) section = 0;

	float w = ImGui::GetContentRegionAvail( ).x;
	ImDrawList* dl = ImGui::GetWindowDrawList( );
	ImVec2 p = ImGui::GetCursorScreenPos( );
	float h = 28.0f;
	float tw = w / 7.0f;
	float fs = ImGui::GetFontSize( );
	ImFont* font = ImGui::GetFont( );

	dl->AddRectFilled( p, ImVec2( p.x + w, p.y + h ), ImGui::GetColorU32( AW_RED_STRIP ), 0 );

	for( int b = 0; b < 7; b++ )
	{
		ImVec2 tmin = ImVec2( p.x + tw * b, p.y );
		ImVec2 tmax = ImVec2( p.x + tw * ( b + 1 ), p.y + h );
		bool active = ( section == b );

		ImGui::PushID( b );
		ImGui::SetCursorScreenPos( tmin );
		ImGui::InvisibleButton( "tab", ImVec2( tw, h ) );
		bool hovered = ImGui::IsItemHovered( );
		if( ImGui::IsItemClicked( ) ) section = b;
		ImGui::PopID( );

		if( active && g_CVars.Miscellaneous.MenuMode == 1 )
			dl->AddRectFilledMultiColor( tmin, tmax, IM_COL32( 230, 35, 43, 200 ), IM_COL32( 230, 35, 43, 200 ), IM_COL32( 186, 35, 43, 255 ), IM_COL32( 186, 35, 43, 255 ) );
		else if( active || hovered )
			dl->AddRectFilled( tmin, tmax, ImGui::GetColorU32( active ? AW_RED_LOGO : AW_RED_BANNER ), 0 );

		ImVec2 ts = ImGui::CalcTextSize( sectionNames[ b ] );
		dl->AddText( font, fs * 0.85f, ImVec2( tmin.x + ( tw - ts.x * 0.85f ) * 0.5f, p.y + ( h - fs * 0.85f ) * 0.5f ),
			active ? IM_COL32_WHITE : IM_COL32( 255, 255, 255, 150 ), sectionNames[ b ] );
	}

	ImGui::Dummy( ImVec2( 0, h ) );

	bool awMode = ( g_CVars.Miscellaneous.MenuMode == 1 ); // r13
	ImGui::BeginChild( "TabContent", ImVec2( 0, awMode ? -19.0f : 0.0f ), false );
	{
		switch( section )
		{
			case 0: g_GUI.RenderAimbotTab( ); break;
			case 1: g_GUI.RenderLegitTab( ); break;
			case 2: g_GUI.RenderVisualsTab( ); break;
			case 3: g_GUI.RenderMiscTab( ); break;
			case 4: g_GUI.RenderPlayerListTab( ); break;
			case 5: g_GUI.RenderConfigsTab( ); break;
			default: g_GUI.RenderScriptsTab( ); break;
		}
	}
	ImGui::EndChild( );
}

void GUI::DrawImGui( void )
{
	if( !bMouse )
		return;

	ImGui::SetNextWindowSize( ImVec2( 740, 600 ), ImGuiCond_FirstUseEver );
	if( g_CVars.Miscellaneous.MenuMode == 2 ) ImGui::SetNextWindowSize( ImVec2( 796, 686 ), ImGuiCond_FirstUseEver ); // r44: V3 is taller
	// r12 polish: rounded frames/windows, tighter spacing, soft border (r13: V4 = dark)
	const bool awDark = ( g_CVars.Miscellaneous.MenuMode == 1 );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 5.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_GrabRounding, 3.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 6 ) );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, awDark ? ImVec4( 0.075f, 0.075f, 0.078f, 0.985f ) : ImVec4( 0.965f, 0.965f, 0.970f, 1.00f ) );
	ImGui::PushStyleColor( ImGuiCol_Border, awDark ? ImVec4( 0.25f, 0.25f, 0.27f, 0.60f ) : ImVec4( 0.75f, 0.75f, 0.78f, 0.60f ) );
	ImGui::PushStyleColor( ImGuiCol_Separator, awDark ? ImVec4( 0.25f, 0.25f, 0.27f, 1.00f ) : ImVec4( 0.80f, 0.80f, 0.83f, 1.00f ) );
	if( awDark )
	{
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.90f, 0.90f, 0.92f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.090f, 0.090f, 0.095f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.16f, 0.16f, 0.17f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.20f, 0.20f, 0.21f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.24f, 0.24f, 0.25f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_PopupBg, ImVec4( 0.10f, 0.10f, 0.105f, 0.99f ) );
		ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, ImVec4( 0.09f, 0.09f, 0.095f, 1.00f ) );
		ImGui::PushStyleColor( ImGuiCol_TitleBg, ImVec4( 0.08f, 0.08f, 0.085f, 1.00f ) );
	}
	if( ImGui::Begin( "Awesware | Counter-Strike: Source v34", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
	{
		MenuBanner( );
		RenderTopBar( );

		if( g_CVars.Miscellaneous.MenuMode == 1 )
		{
			// aimware v4 footer: dark strip, build line left, brand right
			ImDrawList* fdl = ImGui::GetWindowDrawList( );
			ImVec2 wp = ImGui::GetWindowPos( ), ws = ImGui::GetWindowSize( );
			ImVec2 fmin = ImVec2( wp.x, wp.y + ws.y - 19.0f ), fmax = ImVec2( wp.x + ws.x, wp.y + ws.y );
			fdl->AddRectFilled( fmin, fmax, IM_COL32( 29, 29, 27, 200 ), 0 );
			float ffs = ImGui::GetFontSize( );
			const char* fleft = "V4 for Counter-Strike: Source";
			fdl->AddText( ImGui::GetFont( ), ffs * 0.78f, ImVec2( fmin.x + 6.0f, fmin.y + ( 19.0f - ffs * 0.78f ) * 0.5f ), IM_COL32( 180, 180, 180, 255 ), fleft );
			const char* fright = "awesware";
			ImVec2 frs = ImGui::CalcTextSize( fright );
			fdl->AddText( ImGui::GetFont( ), ffs * 0.78f, ImVec2( fmax.x - 6.0f - frs.x * 0.78f, fmin.y + ( 19.0f - ffs * 0.78f ) * 0.5f ), IM_COL32( 180, 180, 180, 255 ), fright );
		}
	}
	ImGui::End( );
	ImGui::PopStyleColor( awDark ? 11 : 3 );
	ImGui::PopStyleVar( 7 );

	// r44: V3 - separate player-list window (checkbox in Misc / Extra)
	if( g_CVars.Miscellaneous.MenuMode == 2 && g_CVars.Visuals.PlayerList )
		RenderPlayerListWindowV3( );
}

static void ApplyRagePreset( void )
{
	g_CVars.Aimbot.Active = true;
	g_CVars.Aimbot.AutoShoot = true;
	g_CVars.Aimbot.Silent = true;
	g_CVars.Aimbot.MultiSpot = true;
	g_CVars.Aimbot.HitScan = true;
	g_CVars.Aimbot.AutoWall = true;
	g_CVars.Aimbot.SnapLimiter = false;
	g_CVars.Aimbot.AimFOV = 90;
	g_CVars.Aimbot.HitChance = false;
	g_CVars.Aimbot.StrictPrimary = false;
	g_CVars.Aimbot.PointScale = 0.75f;
	g_CVars.Aimbot.BestDamage = true;
	g_CVars.Triggerbot.Active = false;
	g_CVars.Legit.Active = false; // exclusive: rage on, legit off
}

static void ApplyLegitPreset( void )
{
	g_CVars.Aimbot.Active = false; // exclusive: legit on, rage off
	g_CVars.Legit.Active = true;
	g_CVars.Legit.AimType = 2;
	g_CVars.Legit.Smoothing = 8;
	g_CVars.Legit.ReactionMs = 120;
	g_CVars.Legit.KillDelayMs = 250;
	g_CVars.Legit.Prediction = true;
	g_CVars.Legit.RCS = 70;
	g_CVars.Legit.RCSStandalone = true;
	g_CVars.Legit.AimLock = true;
	g_CVars.Legit.DesyncResolver = true;
	g_CVars.Legit.DesyncAA = true; // r15 legit preset: fake on choke
	g_CVars.Legit.DesyncYaw = 0;
	g_CVars.Legit.FlashCheck = true;
	g_CVars.Legit.TargetSelection = 4;
	g_CVars.Legit.AutoShoot = false;
	g_CVars.Legit.Silent = false;
	g_CVars.Legit.SnapLimiter = true;
	g_CVars.Legit.AngleLimit = 8;
	g_CVars.Legit.AngleLimitTens = 0.f;
	g_CVars.Legit.Key = 0;
	g_CVars.Legit.AimFOV = 4;
	g_CVars.Legit.FovNear = 14; // r29
	g_CVars.Legit.FovFar = 5; // r29
	g_CVars.Legit.FovSwitchDist = 450; // r29
	g_CVars.Legit.Hitbox = 12;
	g_CVars.Legit.AutoStop = false;
	g_CVars.Legit.AutoPistol = true;
	g_CVars.Legit.FovCircle = true;
	g_CVars.Legit.BacktrackTicks = 6;
	g_CVars.Triggerbot.Active = true;
	g_CVars.Triggerbot.Delay = 90;
}

void GUI::RenderAimbotTab( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	// Left Column
	ImGui::BeginChild( "Aimbot_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "AIMBOT MAIN" );
		AwCheckbox( "Active", &g_CVars.Aimbot.Active );
		AwCheckbox( "Auto Shoot", &g_CVars.Aimbot.AutoShoot );
		AwCheckbox( "Silent Aim", &g_CVars.Aimbot.Silent );
		AwCheckbox( "Perfect Silent", &g_CVars.Aimbot.PerfectSilent );
		AwCheckbox( "Multi Spot", &g_CVars.Aimbot.MultiSpot );
		AwCheckbox( "Body AWP", &g_CVars.Aimbot.BodyAWP );
		AwCheckbox( "Auto Stop", &g_CVars.Aimbot.AutoStop );
		AwCheckbox( "Body Aim vs Jump", &g_CVars.Aimbot.BodyVsJump );
		AwCheckbox( "Hit Scan", &g_CVars.Aimbot.HitScan );
		AwCheckbox( "Perfect Auto Wall", &g_CVars.Aimbot.AutoWall );
		AwCheckbox( "Anti SMAC", &g_CVars.Aimbot.AntiSMAC );
		AwCheckbox( "Friendly Fire", &g_CVars.Aimbot.FriendlyFire );

		ImGui::Spacing( );
		SectionHeader( "FORCE KEYS (HOLD)" );
		static const char* forceKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Aimbot.ForceBodyKey < 0 || g_CVars.Aimbot.ForceBodyKey >= 6 ) g_CVars.Aimbot.ForceBodyKey = 0;
		ImGui::Combo( "Force Body Key", &g_CVars.Aimbot.ForceBodyKey, forceKeyNames, IM_ARRAYSIZE( forceKeyNames ) );
		if( g_CVars.Aimbot.ForceMinDmgKey < 0 || g_CVars.Aimbot.ForceMinDmgKey >= 6 ) g_CVars.Aimbot.ForceMinDmgKey = 0;
		ImGui::Combo( "Force MinDmg Key", &g_CVars.Aimbot.ForceMinDmgKey, forceKeyNames, IM_ARRAYSIZE( forceKeyNames ) );
		AwSliderInt( "Force Min Damage", &g_CVars.Aimbot.ForceMinDmgValue, 1, 100 );

		ImGui::Spacing( );
		SectionHeader( "FAKEDUCK / MICROMOVES" ); // r24 (r33: own header, was glued under DOUBLE TAP)
		AwCheckbox( "Fakeduck (hold key)", &g_CVars.Miscellaneous.Fakeduck );
		static const char* fdKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Miscellaneous.FakeduckKey < 0 || g_CVars.Miscellaneous.FakeduckKey >= 6 ) g_CVars.Miscellaneous.FakeduckKey = 0;
		ImGui::Combo( "Fakeduck Key", &g_CVars.Miscellaneous.FakeduckKey, fdKeyNames, IM_ARRAYSIZE( fdKeyNames ) );
		AwCheckbox( "Micromoves (standing)", &g_CVars.Miscellaneous.Micromoves );
		ImGui::Spacing( );
		SectionHeader( "RAGE - DOUBLE TAP" ); // r33: header returned to its own block

		AwCheckbox( "Double Tap", &g_CVars.Miscellaneous.DoubleTap );
		AwCheckbox( "DT Only On Ground", &g_CVars.Miscellaneous.DoubleTapOnlyGround );
		AwCheckbox( "DT Delay Shot", &g_CVars.Miscellaneous.DoubleTapDelayShot );
		AwSliderInt( "DT Ticks", &g_CVars.Miscellaneous.DoubleTapTicks, 2, 16 );
		AwCheckbox( "DT Auto Ticks", &g_CVars.Miscellaneous.DoubleTapAuto );
		if( g_CVars.Miscellaneous.DoubleTapMode < 0 || g_CVars.Miscellaneous.DoubleTapMode > 1 ) g_CVars.Miscellaneous.DoubleTapMode = 0;
		const char* dtModeNames[] = { "Offensive", "Defensive (fakelag)" };
		ImGui::Combo( "DT Mode", &g_CVars.Miscellaneous.DoubleTapMode, dtModeNames, IM_ARRAYSIZE( dtModeNames ) );
		int dtNeed = g_CVars.Miscellaneous.DoubleTapAuto ? g_DTTicks : g_CVars.Miscellaneous.DoubleTapTicks;
		if( dtNeed < 2 ) dtNeed = 2; if( dtNeed > 16 ) dtNeed = 16;
		if( g_CVars.Miscellaneous.DoubleTap )
		{
			float dtFrac = ( dtNeed > 0 ) ? ( ( float )g_DTCharge / ( float )dtNeed ) : 0.f;
			if( dtFrac < 0.f ) dtFrac = 0.f; if( dtFrac > 1.f ) dtFrac = 1.f;
			ImGui::ProgressBar( dtFrac, ImVec2( -1, 0 ), ( g_DTCharge >= dtNeed ) ? "DT READY" : "DT charging" );
		}

		ImGui::Spacing( );
		SectionHeader( "ANTI-AIM (HVH)" );
		AwCheckbox( "Anti-Aim Active", &g_CVars.Miscellaneous.AntiAim.Active );
		ImGui::Text( "Choke hook: %s", g_NetchanHooked ? "ACTIVE" : "waiting..." );

		const char* pitchNames[] = { "Off", "Normal", "Inverse Normal", "Safe", "Fake Down", "Lisp Down", "Lisp Up", "Lag Down", "Lag Up", "Down 89", "Up -89" };
		if( g_CVars.Miscellaneous.AntiAim.Pitch < 0 || g_CVars.Miscellaneous.AntiAim.Pitch >= 11 ) g_CVars.Miscellaneous.AntiAim.Pitch = 0;
		ImGui::Combo( "Pitch", &g_CVars.Miscellaneous.AntiAim.Pitch, pitchNames, IM_ARRAYSIZE( pitchNames ) );

		const char* yawNames[] = { "Forwards", "Backwards", "Sideways", "Jitter", "Static", "Static Reversed", "Lisp", "Custom", "Jitter X", "AI", "Defensive", "AI Custom", "Server Hold" };
		if( g_CVars.Miscellaneous.AntiAim.Yaw < 0 || g_CVars.Miscellaneous.AntiAim.Yaw >= 13 ) g_CVars.Miscellaneous.AntiAim.Yaw = 0; // FIX r8: old clamp 12 reset Server Hold to Forwards
		ImGui::Combo( "Yaw", &g_CVars.Miscellaneous.AntiAim.Yaw, yawNames, IM_ARRAYSIZE( yawNames ) );

		std::vector< const char* > yawVariations;
		if( g_CVars.Miscellaneous.AntiAim.Yaw == 3 )
		{
			yawVariations = { "Normal", "Synced", "Static", "Static Synced" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 6 )
		{
			yawVariations = { "m3nly", "m3nly #2", "Jitter", "1337" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 7 )
		{
			yawVariations = { "Additional", "Static" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 8 )
		{
			yawVariations = { "Wide", "Sway", "Random", "Spin" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 9 )
		{
			yawVariations = { "Auto Learn" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 10 )
		{
			yawVariations = { "Spin + Flick", "Flicker", "Sway Spin", "Laggy", "Max Desync" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 11 )
		{
			yawVariations = { "Full Auto" };
		}
		else
		{
			yawVariations = { "Normal", "Fake Side 1", "Fake Side 2", "Random" };
		}

		if( g_CVars.Miscellaneous.AntiAim.Variation >= ( int )yawVariations.size( ) )
			g_CVars.Miscellaneous.AntiAim.Variation = 0;

		ImGui::Combo( "Yaw Mode", &g_CVars.Miscellaneous.AntiAim.Variation, yawVariations.data( ), ( int )yawVariations.size( ) );
		if( g_CVars.Miscellaneous.AntiAim.Yaw == 9 )
		{
			static const char* aiStyleNames[] = { "Back", "Side", "Wide", "Spin", "Spin+Flick", "Flicker", "Sway", "Laggy" };
			static const char* aiStateNames[] = { "stand", "move", "air" };
			int aiSt = AIAA_GetStyle( ); if( aiSt < 0 || aiSt > 7 ) aiSt = 0;
			int aiMv = AIAA_GetState( ); if( aiMv < 0 || aiMv > 2 ) aiMv = 0;
			ImGui::Text( "AI Style: %s (%s)", aiStyleNames[ aiSt ], aiStateNames[ aiMv ] );
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 11 )
		{
			static const char* aicRNames[] = { "Back", "Side", "Spin", "Spin+Flick", "Flicker", "Laggy" };
			static const char* aicFNames[] = { "Fwd", "Side+", "Side-", "RevSpin" };
			static const char* aicPNames[] = { "Down", "FakeDown", "Up", "Lisp" };
			static const char* aiStateNames2[] = { "stand", "move", "air" };
			int info = AIC_GetInfo( );
			int r = info % 6, f = ( info / 6 ) % 4, p = ( info / 24 ) % 4, mv = info / 96;
			if( r < 0 || r > 5 ) r = 0; if( f < 0 || f > 3 ) f = 0;
			if( p < 0 || p > 3 ) p = 0; if( mv < 0 || mv > 2 ) mv = 0;
			ImGui::Text( "AI Custom: %s / %s / %s (%s)", aicRNames[ r ], aicFNames[ f ], aicPNames[ p ], aiStateNames2[ mv ] );
		}

		AwSliderFloat( "Custom Real Yaw", &g_CVars.Miscellaneous.AntiAim.RealValue, 0.0f, 360.0f, "%.1f deg" );
		AwSliderFloat( "Custom Fake Yaw", &g_CVars.Miscellaneous.AntiAim.FakeValue, 0.0f, 360.0f, "%.1f deg" );

		ImGui::Spacing( );
		ImGui::TextDisabled( "FLICK" );
		AwCheckbox( "Flick Enable", &g_CVars.Miscellaneous.AntiAim.FlickEnable );
		AwSliderInt( "Flick Every", &g_CVars.Miscellaneous.AntiAim.FlickTicks, 2, 30, "%d ticks" );
		AwSliderFloat( "Flick Angle", &g_CVars.Miscellaneous.AntiAim.FlickAngle, 0.0f, 180.0f, "%.1f deg" );
		if( g_CVars.Miscellaneous.AntiAim.FlickSide < 0 || g_CVars.Miscellaneous.AntiAim.FlickSide > 2 ) g_CVars.Miscellaneous.AntiAim.FlickSide = 1;
		const char* flickSideNames[] = { "Real", "Fake", "Both" };
		ImGui::Combo( "Flick Side", &g_CVars.Miscellaneous.AntiAim.FlickSide, flickSideNames, IM_ARRAYSIZE( flickSideNames ) );
		AwCheckbox( "Flick Random", &g_CVars.Miscellaneous.AntiAim.FlickRandom );
		AwCheckbox( "Flick On Shot", &g_CVars.Miscellaneous.AntiAim.FlickOnShot );

		AwCheckbox( "InAttack Pitch", &g_CVars.Miscellaneous.AntiAim.Static );
		AwCheckbox( "Wall Detection", &g_CVars.Miscellaneous.AntiAim.WallDetection );

		const char* wallDtcModes[] = { "Normal", "Fake", "Fake Out", "Jitter" };
		if( g_CVars.Miscellaneous.AntiAim.WallDetectionMode < 0 || g_CVars.Miscellaneous.AntiAim.WallDetectionMode >= 4 ) g_CVars.Miscellaneous.AntiAim.WallDetectionMode = 0;
		ImGui::Combo( "Wall DTC Mode", &g_CVars.Miscellaneous.AntiAim.WallDetectionMode, wallDtcModes, IM_ARRAYSIZE( wallDtcModes ) );

		AwCheckbox( "At Targets", &g_CVars.Miscellaneous.AntiAim.AtTargets );
		AwCheckbox( "Duck In Air", &g_CVars.Miscellaneous.AntiAim.DuckInAir );
		AwCheckbox( "Enemy Check", &g_CVars.Miscellaneous.AntiAim.TurnOff );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	// Right Column
	ImGui::BeginChild( "Aimbot_Right", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "TARGETING & ADJUSTMENTS" );

		const char* aimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Aimbot.Key < 0 || g_CVars.Aimbot.Key >= 6 ) g_CVars.Aimbot.Key = 0;
		ImGui::Combo( "Aim Key", &g_CVars.Aimbot.Key, aimKeyNames, IM_ARRAYSIZE( aimKeyNames ) );

		// r25: direct hitbox selectors (raw ids 9-12) - replaced the old base
		// combo that mapped through GetHitboxIndex and could desync from configs
		static const char* primHbNames[] = { "Head", "Neck", "Chest", "Stomach" };
		int primSel = g_CVars.Aimbot.Hitbox;
		if( primSel < 9 || primSel > 12 ) primSel = 12;
		int primIdx = 12 - primSel; // 12->0 Head, 11->1 Neck, 10->2 Chest, 9->3 Stomach
		if( ImGui::Combo( "Primary Hitbox", &primIdx, primHbNames, 4 ) )
			g_CVars.Aimbot.Hitbox = 12 - primIdx;
		static const char* fbHbNames[] = { "Off (group scan)", "Head", "Neck", "Chest", "Stomach" };
		int fbSel = g_CVars.Aimbot.FallbackHitbox;
		if( fbSel < 0 ) fbSel = 0;
		int fbIdx = ( fbSel >= 9 && fbSel <= 12 ) ? ( 13 - fbSel ) : 0; // 12->1 .. 9->4
		if( ImGui::Combo( "Fallback Hitbox", &fbIdx, fbHbNames, 5 ) )
			g_CVars.Aimbot.FallbackHitbox = ( fbIdx > 0 ) ? ( 13 - fbIdx ) : 0;
		AwCheckbox( "Strict Primary", &g_CVars.Aimbot.StrictPrimary );
		AwCheckbox( "Best Damage", &g_CVars.Aimbot.BestDamage );

		ImGui::Spacing( );
		SectionHeader( "HITBOX PRIORITY" );
		static const char* priorityNames[] = { "Off", "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs" };
		int prioIdx = 0;
		for( int hg = 0; hg < 7; hg++ )
			if( g_CVars.Aimbot.HitboxGroup[ hg ] == 2 ) prioIdx = hg + 1;
		if( prioIdx < 0 || prioIdx > 7 ) prioIdx = 0;
		if( ImGui::Combo( "Hitbox Priority", &prioIdx, priorityNames, IM_ARRAYSIZE( priorityNames ) ) )
		{
			for( int hg = 0; hg < 7; hg++ )
				g_CVars.Aimbot.HitboxGroup[ hg ] = ( prioIdx == 0 ) ? 1 : ( ( hg == prioIdx - 1 ) ? 2 : 1 );
		}

		AwSliderFloat( "Point Scale", &g_CVars.Aimbot.PointScale, 0.0f, 1.0f, "%.2f" );

		const char* heightModeNames[] = { "Auto", "Origin", "Center", "Center Fixed", "Highest" };
		if( g_CVars.Aimbot.HitboxMode < 0 || g_CVars.Aimbot.HitboxMode >= 5 ) g_CVars.Aimbot.HitboxMode = 0;
		ImGui::Combo( "Height Mode", &g_CVars.Aimbot.HitboxMode, heightModeNames, IM_ARRAYSIZE( heightModeNames ) );

		const char* targetSelectionNames[] = { "Distance", "Health", "Next Shot", "Random", "Crosshair" };
		if( g_CVars.Aimbot.TargetSelection < 0 || g_CVars.Aimbot.TargetSelection >= 5 ) g_CVars.Aimbot.TargetSelection = 0;
		ImGui::Combo( "Target Selection", &g_CVars.Aimbot.TargetSelection, targetSelectionNames, IM_ARRAYSIZE( targetSelectionNames ) );

		AwSliderInt( "Min Damage", &g_CVars.Aimbot.MinDamage, 0, 100 );
		AwCheckbox( "Hit Chance", &g_CVars.Aimbot.HitChance );
		AwSliderInt( "Min Hit Chance", &g_CVars.Aimbot.HitChanceValue, 0, 100 );

		const char* posAdjustmentNames[] = { "Off", "On", "On + History" };
		ImGui::Combo( "Pos Adjustment", &g_CVars.Aimbot.Interpolation.LagPrediction, posAdjustmentNames, IM_ARRAYSIZE( posAdjustmentNames ) );
		AwSliderInt( "Backtrack Ticks", &g_CVars.Aimbot.BacktrackTicks, 0, 12 );
		AwSliderInt( "Aim FOV (0 = 360)", &g_CVars.Aimbot.AimFOV, 0, 180 );
		AwSliderInt( "Long Range (0=off)", &g_CVars.Aimbot.LongRangeDist, 0, 2000 );

		ImGui::Spacing( );
		SectionHeader( "ACCURACY" );
		AwCheckbox( "Remove Recoil / Spread", &g_CVars.Accuracy.PerfectAccuracy );
		AwCheckbox( "Force Seed", &g_CVars.Accuracy.ForceSeed );

		const char* spreadModeNames[] = { "NULL", "Classic", "Iterative", "Rotation" };
		if( g_CVars.Accuracy.NoSpreadMode < 0 || g_CVars.Accuracy.NoSpreadMode >= 4 ) g_CVars.Accuracy.NoSpreadMode = 0;
		ImGui::Combo( "NoSpread Mode", &g_CVars.Accuracy.NoSpreadMode, spreadModeNames, IM_ARRAYSIZE( spreadModeNames ) );

		ImGui::Spacing( );
		SectionHeader( "SNAP LIMITER" );
		AwCheckbox( "Snap Limiter Active", &g_CVars.Aimbot.SnapLimiter );
		AwCheckbox( "Disable Enemy Interpolation", &g_CVars.Aimbot.Interpolation.DisableInterp ); // r25: Segregation bypass
		AwSliderInt( "Angle Limit", &g_CVars.Aimbot.AngleLimit, 0, 180 );
		AwSliderFloat( "Angle Limit Tens", &g_CVars.Aimbot.AngleLimitTens, 0.0f, 1.0f, "%.2f" );

		ImGui::Spacing( );
		SectionHeader( "RESOLVER" );
		AwCheckbox( "Resolver Active", &g_CVars.Aimbot.Resolver.Active );

		const char* resolverModeNames[] = { "Everyone", "Selected" };
		if( g_CVars.Aimbot.Resolver.Mode < 0 || g_CVars.Aimbot.Resolver.Mode >= 2 ) g_CVars.Aimbot.Resolver.Mode = 0;
		ImGui::Combo( "Resolver Target", &g_CVars.Aimbot.Resolver.Mode, resolverModeNames, IM_ARRAYSIZE( resolverModeNames ) );

		const char* resolverTypeNames[] = { "Spin", "Back Twitch", "Alternative", "2 bullets", "Anim Test", "AI Learn", "Honest Shot" };
		if( g_CVars.Aimbot.Resolver.Type < 0 || g_CVars.Aimbot.Resolver.Type >= 7 ) g_CVars.Aimbot.Resolver.Type = 0;
		ImGui::Combo( "Resolver Type", &g_CVars.Aimbot.Resolver.Type, resolverTypeNames, IM_ARRAYSIZE( resolverTypeNames ) );
		AwCheckbox( "Lag Records (Anti-Jitter)", &g_CVars.Aimbot.Resolver.LagRecords ); // r40

		AwCheckbox( "Smart Resolver", &g_CVars.Aimbot.Resolver.Smart );

		ImGui::Spacing( );
		SectionHeader( "FAKE LAG" );
		AwCheckbox( "Fake Lag Active", &g_CVars.Miscellaneous.Fakelag.Active );
		AwCheckbox( "Fake Lag In Attack", &g_CVars.Miscellaneous.Fakelag.InAttack );
		AwCheckbox( "Fake Lag Air Only", &g_CVars.Miscellaneous.Fakelag.AirOnly );
		AwSliderInt( "Choke Ticks", &g_CVars.Miscellaneous.Fakelag.Value, 0, 14 );

		const char* fakelagModes[] = { "Factor", "Switch", "Adaptive", "AI Smart" };
		if( g_CVars.Miscellaneous.Fakelag.Mode < 0 || g_CVars.Miscellaneous.Fakelag.Mode >= 4 ) g_CVars.Miscellaneous.Fakelag.Mode = 0;
		ImGui::Combo( "Fake Lag Mode", &g_CVars.Miscellaneous.Fakelag.Mode, fakelagModes, IM_ARRAYSIZE( fakelagModes ) );

		ImGui::Spacing( );
		SectionHeader( "AI MEMORY" );
		if( ImGui::Button( "Reset AI Memory", ImVec2( 180, 0 ) ) ) { AIResolver_Reset( ); AIAA_Reset( ); AIC_Reset( ); g_Drawing.AddLog( Color( 255, 220, 120, 255 ), "AI memory reset" ); }
		ImGui::TextDisabled( "Clears resolver + AA learning." );

	}
	ImGui::EndChild( );
}

void GUI::RenderLegitTab( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	// Left Column
	ImGui::BeginChild( "Legit_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "LEGIT AIM" );
		AwCheckbox( "Enabled##Legit", &g_CVars.Legit.Active );

		const char* legitAimTypeNames[] = { "Snap", "Smooth", "Adaptive" };
		if( g_CVars.Legit.AimType < 0 || g_CVars.Legit.AimType > 2 ) g_CVars.Legit.AimType = 2;
		ImGui::Combo( "Aim Type##Legit", &g_CVars.Legit.AimType, legitAimTypeNames, IM_ARRAYSIZE( legitAimTypeNames ) );

		const char* legitAimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Legit.Key < 0 || g_CVars.Legit.Key >= 6 ) g_CVars.Legit.Key = 0;
		ImGui::Combo( "Aim Key##Legit", &g_CVars.Legit.Key, legitAimKeyNames, IM_ARRAYSIZE( legitAimKeyNames ) );

		AwSliderInt( "FOV Near##Legit", &g_CVars.Legit.FovNear, 1, 30 ); // r29: close-range cone
		AwSliderInt( "FOV Far##Legit", &g_CVars.Legit.FovFar, 1, 30 ); // r29: long-range cone
		AwSliderInt( "FOV Switch Dist", &g_CVars.Legit.FovSwitchDist, 0, 1500 ); // r29: 0 = single FOV mode
		AwSliderInt( "Aim FOV (Snap mode)", &g_CVars.Legit.AimFOV, 1, 30 ); // r36: Snap (AimType 0) cone - slider lost in r29, restored

		const char* legitTargetNames[] = { "Distance", "Health", "Next Shot", "Random", "Crosshair" };
		if( g_CVars.Legit.TargetSelection < 0 || g_CVars.Legit.TargetSelection > 4 ) g_CVars.Legit.TargetSelection = 4;
		ImGui::Combo( "Target##Legit", &g_CVars.Legit.TargetSelection, legitTargetNames, IM_ARRAYSIZE( legitTargetNames ) );

		const char* legitHitboxNames[] = { "Head", "Neck", "Chest", "Stomach" };
		int legitHitboxIdx = GetHitboxIndex( g_CVars.Legit.Hitbox );
		if( ImGui::Combo( "Hitbox##Legit", &legitHitboxIdx, legitHitboxNames, IM_ARRAYSIZE( legitHitboxNames ) ) )
		{
			g_CVars.Legit.Hitbox = HitboxFromIndex( legitHitboxIdx );
		}

		AwCheckbox( "Aim Lock##Legit", &g_CVars.Legit.AimLock );
		AwCheckbox( "Auto Shoot##Legit", &g_CVars.Legit.AutoShoot );
		AwCheckbox( "Auto Stop##Legit", &g_CVars.Legit.AutoStop );
		AwCheckbox( "Auto Pistol##Legit", &g_CVars.Legit.AutoPistol );
		AwCheckbox( "FOV Circle##Legit", &g_CVars.Legit.FovCircle );
		AwSliderInt( "Backtrack##Legit", &g_CVars.Legit.BacktrackTicks, 0, 12 );

		ImGui::Spacing( );
		SectionHeader( "SMOOTHING && HUMANIZE" );
		AwSliderInt( "Smoothing##Legit", &g_CVars.Legit.Smoothing, 1, 30 );
		AwSliderInt( "Reaction Ms##Legit", &g_CVars.Legit.ReactionMs, 0, 400 );
		AwSliderInt( "Kill Delay Ms##Legit", &g_CVars.Legit.KillDelayMs, 0, 1000 );
		AwCheckbox( "Prediction##Legit", &g_CVars.Legit.Prediction );
		AwSliderInt( "RCS %##Legit", &g_CVars.Legit.RCS, 0, 100 );
		AwCheckbox( "Standalone RCS##Legit", &g_CVars.Legit.RCSStandalone );
		AwCheckbox( "Flash Check##Legit", &g_CVars.Legit.FlashCheck );
		AwCheckbox( "Scoped Check (AWP)##Legit", &g_CVars.Legit.ScopedCheck );
		AwCheckbox( "Auto Scope (AWP)##Legit", &g_CVars.Legit.AutoScope );
		AwCheckbox( "Desync Resolver##Legit", &g_CVars.Legit.DesyncResolver );
		AwCheckbox( "Snap Limiter##Legit", &g_CVars.Legit.SnapLimiter );
		AwSliderInt( "Angle Limit##Legit", &g_CVars.Legit.AngleLimit, 0, 180 );
		AwSliderFloat( "Angle Limit Tens##Legit", &g_CVars.Legit.AngleLimitTens, 0.0f, 1.0f, "%.2f" );
		AwCheckbox( "Silent Aim##Legit", &g_CVars.Legit.Silent );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	// Right Column
	ImGui::BeginChild( "Legit_Right", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "HITBOX GROUPS" );
		static const char* legitGroupNames[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs" };
		static const char* legitGroupModeNames[] = { "Off", "Scan", "Priority" };
		for( int lg = 0; lg < 7; lg++ )
		{
			int lmode = g_CVars.Legit.HitboxGroup[ lg ];
			if( lmode < 0 || lmode > 2 ) lmode = 1;
			ImGui::PushID( 100 + lg );
			ImGui::Combo( legitGroupNames[ lg ], &lmode, legitGroupModeNames, IM_ARRAYSIZE( legitGroupModeNames ) );
			g_CVars.Legit.HitboxGroup[ lg ] = lmode;
			ImGui::PopID( );
		}
		if( ImGui::Button( "All Scan##Legit", ImVec2( 80, 0 ) ) )
			for( int lg = 0; lg < 7; lg++ ) g_CVars.Legit.HitboxGroup[ lg ] = 1;
		ImGui::SameLine( );
		if( ImGui::Button( "Upper##Legit", ImVec2( 80, 0 ) ) )
		{
			for( int lg = 0; lg < 7; lg++ ) g_CVars.Legit.HitboxGroup[ lg ] = 0;
			g_CVars.Legit.HitboxGroup[ 0 ] = 2;
			g_CVars.Legit.HitboxGroup[ 1 ] = 1;
			g_CVars.Legit.HitboxGroup[ 2 ] = 1;
			g_CVars.Legit.HitboxGroup[ 3 ] = 1;
		}
		ImGui::Spacing( );
		SectionHeader( "TRIGGERBOT" );
		AwCheckbox( "Triggerbot Active", &g_CVars.Triggerbot.Active );
		AwCheckbox( "Seed Check", &g_CVars.Triggerbot.Seed );
		AwCheckbox( "Spread Check", &g_CVars.Triggerbot.Spread );
		AwCheckbox( "Recoil Check", &g_CVars.Triggerbot.Recoil );
		AwSliderInt( "Trigger Delay Ms", &g_CVars.Triggerbot.Delay, 0, 300 );

		const char* triggerStrengthNames[] = { "Low", "Medium", "High", "Extra" };
		if( g_CVars.Triggerbot.Strength < 0 || g_CVars.Triggerbot.Strength >= 4 ) g_CVars.Triggerbot.Strength = 0;
		ImGui::Combo( "Strength", &g_CVars.Triggerbot.Strength, triggerStrengthNames, IM_ARRAYSIZE( triggerStrengthNames ) );

		const char* triggerHitboxNames[] = { "Head", "Upper Body", "Lower Body", "Full Body" };
		if( g_CVars.Triggerbot.Hitbox < 0 || g_CVars.Triggerbot.Hitbox >= 4 ) g_CVars.Triggerbot.Hitbox = 0;
		ImGui::Combo( "Trigger Hitbox", &g_CVars.Triggerbot.Hitbox, triggerHitboxNames, IM_ARRAYSIZE( triggerHitboxNames ) );

		const char* keyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Triggerbot.Key < 0 || g_CVars.Triggerbot.Key >= 6 ) g_CVars.Triggerbot.Key = 0;
		ImGui::Combo( "Trigger Key", &g_CVars.Triggerbot.Key, keyNames, IM_ARRAYSIZE( keyNames ) );

		ImGui::Spacing( );
		ImGui::Spacing( );
		SectionHeader( "LEGIT STRAFE (HOLD SPACE)" );
		AwCheckbox( "Strafe Active##Legit", &g_CVars.Legit.StrafeActive );
		AwSliderInt( "Strafe Power##Legit", &g_CVars.Legit.StrafePower, 1, 10 );

		ImGui::Spacing( );
		SectionHeader( "LEGIT ANTI-AIM" );
		AwCheckbox( "Legit AA##Legit", &g_CVars.Legit.LegitAA );
		static const char* legitAAKeyNames[] = { "Always", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Legit.LegitAAKey < 0 || g_CVars.Legit.LegitAAKey >= 6 ) g_CVars.Legit.LegitAAKey = 0;
		ImGui::Combo( "Legit AA Key##Legit", &g_CVars.Legit.LegitAAKey, legitAAKeyNames, IM_ARRAYSIZE( legitAAKeyNames ) );
		static const char* legitAAInvertNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		if( g_CVars.Legit.LegitAAInvertKey < 0 || g_CVars.Legit.LegitAAInvertKey >= 6 ) g_CVars.Legit.LegitAAInvertKey = 0;
		ImGui::Combo( "Legit AA Inverter##Legit", &g_CVars.Legit.LegitAAInvertKey, legitAAInvertNames, IM_ARRAYSIZE( legitAAInvertNames ) );
		AwSliderInt( "Legit AA Angle##Legit", &g_CVars.Legit.LegitAAAngle, 0, 45, "%d deg" );
		ImGui::TextDisabled( "Silent yaw padding, skipped while shooting." );
		AwCheckbox( "Fake On Choke", &g_CVars.Legit.DesyncAA ); // r42: moved here from SMOOTHING && HUMANIZE (renamed)
		AwSliderInt( "Fake Yaw", &g_CVars.Legit.DesyncYaw, -180, 180 ); // r15: 0 = fake faces 0 deg
		if( g_CVars.Legit.DesyncChoke < 1 || g_CVars.Legit.DesyncChoke > 14 ) g_CVars.Legit.DesyncChoke = 6; // r43
		AwSliderInt( "Fake Choke Ticks", &g_CVars.Legit.DesyncChoke, 1, 14, "%d ticks" ); // r43: auto-fakelag so the fake is seen

		ImGui::Spacing( );
		SectionHeader( "PRESETS" );
		if( ImGui::Button( "Apply Legit", ImVec2( 120, 0 ) ) ) ApplyLegitPreset( );
		ImGui::SameLine( );
		if( ImGui::Button( "Apply Rage", ImVec2( 120, 0 ) ) ) ApplyRagePreset( );
	}
	ImGui::EndChild( );
}

void GUI::RenderVisualsTab( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	// Left Column
	ImGui::BeginChild( "Visuals_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "ESP (SURFACE RENDER)" );
		AwCheckbox( "Bounding Box", &g_CVars.Visuals.ESP.Box );
		const char* boxStyleNames[] = { "Full", "Corner", "3D" };
		if( g_CVars.Visuals.ESP.BoxStyle < 0 || g_CVars.Visuals.ESP.BoxStyle > 2 ) g_CVars.Visuals.ESP.BoxStyle = 1;
		ImGui::Combo( "Box Style", &g_CVars.Visuals.ESP.BoxStyle, boxStyleNames, IM_ARRAYSIZE( boxStyleNames ) );
		AwCheckbox( "Player Name", &g_CVars.Visuals.ESP.Name );
		AwCheckbox( "Health Bar / Text", &g_CVars.Visuals.ESP.Health );
		const char* hpStyleNames[] = { "Bottom", "Left", "Top" };
		if( g_CVars.Visuals.ESP.HealthStyle < 0 || g_CVars.Visuals.ESP.HealthStyle > 2 ) g_CVars.Visuals.ESP.HealthStyle = 1;
		ImGui::Combo( "Bar Style", &g_CVars.Visuals.ESP.HealthStyle, hpStyleNames, IM_ARRAYSIZE( hpStyleNames ) );
		AwCheckbox( "Armor Bar", &g_CVars.Visuals.ESP.Armor );
		AwCheckbox( "Ammo Counter", &g_CVars.Visuals.ESP.Ammo );
		AwCheckbox( "Fake Skeleton (local)", &g_CVars.Visuals.ESP.Fake );
		AwCheckbox( "Show Fake Pose (ThirdPerson)", &g_CVars.Visuals.ESP.ShowFake ); // r21: pin off -> live real/fake flicker visible
		AwCheckbox( "Weapon Name", &g_CVars.Visuals.ESP.Weapon );
		AwCheckbox( "Skeleton / Bone", &g_CVars.Visuals.ESP.Bone );
		AwCheckbox( "Aim Spot", &g_CVars.Visuals.ESP.AimSpot );
		AwCheckbox( "Hitmarker", &g_CVars.Visuals.ESP.Hit );
		AwCheckbox( "Ground ESP", &g_CVars.Visuals.ESP.Ground );
		AwCheckbox( "Enemy Only", &g_CVars.Visuals.ESP.EnemyOnly );
		AwCheckbox( "Dormant ESP", &g_CVars.Visuals.ESP.Dormant );
		AwCheckbox( "Offscreen Arrows", &g_CVars.Visuals.ESP.OOF );
		AwCheckbox( "Event Log", &g_CVars.Visuals.EventLog );
		AwCheckbox( "Shot Log", &g_CVars.Visuals.ShotLog );
		AwCheckbox( "Spectator List", &g_CVars.Visuals.SpectatorList ); // r25
		AwCheckbox( "Indicators (FL/DT/AA)", &g_CVars.Visuals.Indicators );

		ImGui::Spacing( );
		SectionHeader( "CHAMS & MODELS" );
		AwCheckbox( "Player Chams", &g_CVars.Visuals.Chams.Active );
		AwCheckbox( "Weapon Chams", &g_CVars.Visuals.Chams.Weapons );
		const char* chamStyleNames[] = { "Flat", "Lit", "Wireframe", "Glow" };
		if( g_CVars.Visuals.Chams.Style < 0 || g_CVars.Visuals.Chams.Style > 3 ) g_CVars.Visuals.Chams.Style = 0;
		ImGui::Combo( "Cham Style", &g_CVars.Visuals.Chams.Style, chamStyleNames, IM_ARRAYSIZE( chamStyleNames ) );
		AwCheckbox( "Model Outline", &g_CVars.Visuals.Chams.Outline );
		AwCheckbox( "Hands Outline", &g_CVars.Visuals.Chams.HandsOutline );
		AwCheckbox( "Chams Enemy Only", &g_CVars.Visuals.Chams.EnemyOnly );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	// Right Column
	ImGui::BeginChild( "Visuals_Right", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "WORLD & SCREEN" );
		AwCheckbox( "Draw Radar", &g_CVars.Visuals.Radar );
		AwCheckbox( "No Sky", &g_CVars.Visuals.NoSky );
		AwCheckbox( "No Smoke", &g_CVars.Visuals.NoSmoke );
		AwCheckbox( "No Flash", &g_CVars.Visuals.NoFlash );
		AwCheckbox( "No Hands", &g_CVars.Visuals.NoHands );
		AwCheckbox( "No Visual Recoil", &g_CVars.Visuals.NoVisualRecoil );
		AwSliderFloat( "ASUS Walls", &g_CVars.Visuals.ASUS, 0.0f, 1.0f, "%.2f" );

		const char* crosshairTypeNames[] = { "Off", "Cross", "Dot", "Round" };
		if( g_CVars.Visuals.Crosshair.Type < 0 || g_CVars.Visuals.Crosshair.Type >= 4 ) g_CVars.Visuals.Crosshair.Type = 0;
		ImGui::Combo( "Crosshair Type", &g_CVars.Visuals.Crosshair.Type, crosshairTypeNames, IM_ARRAYSIZE( crosshairTypeNames ) );
		AwCheckbox( "Dynamic Crosshair", &g_CVars.Visuals.Crosshair.Dynamic );

		ImGui::Spacing( );
		SectionHeader( "CUSTOM COLORS" );

		ImGui::Text( "ESP Colors:" );
		ImGuiColorEdit( "CT ESP", g_CVars.ColorSelector.ESP.CT );
		ImGuiColorEdit( "T ESP", g_CVars.ColorSelector.ESP.TT );
		ImGuiColorEdit( "Weapon ESP", g_CVars.ColorSelector.ESP.Wpn );

		ImGui::Spacing( );
		ImGui::Text( "Chams Colors:" );
		ImGuiColorEdit( "CT Visible", g_CVars.ColorSelector.Chams.CTVis );
		ImGuiColorEdit( "CT Hidden", g_CVars.ColorSelector.Chams.CTInvis );
		ImGuiColorEdit( "CT Outline", g_CVars.ColorSelector.Chams.CTOutline );

		ImGuiColorEdit( "T Visible", g_CVars.ColorSelector.Chams.TTVis );
		ImGuiColorEdit( "T Hidden", g_CVars.ColorSelector.Chams.TTInvis );
		ImGuiColorEdit( "T Outline", g_CVars.ColorSelector.Chams.TTOutline );

		ImGuiColorEdit( "Wpn Visible", g_CVars.ColorSelector.Chams.WpnVis );
		ImGuiColorEdit( "Wpn Hidden", g_CVars.ColorSelector.Chams.WpnInvis );
		ImGuiColorEdit( "Wpn Outline", g_CVars.ColorSelector.Chams.WpnOutline );
	}
	ImGui::EndChild( );
}

void GUI::RenderMiscTab( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	// Left Column
	ImGui::BeginChild( "Misc_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "MENU" );
		static const char* menuKeyNames[] = { "INSERT", "DELETE", "HOME", "END", "F8", "F9" };
		static const int menuKeyVK[] = { 0x2D, 0x2E, 0x24, 0x23, 0x77, 0x78 };
		int curMenuKey = 0;
		for( int k = 0; k < 6; k++ ) if( g_CVars.Miscellaneous.MenuKey == menuKeyVK[ k ] ) curMenuKey = k;
		if( ImGui::Combo( "Menu Key", &curMenuKey, menuKeyNames, IM_ARRAYSIZE( menuKeyNames ) ) )
			g_CVars.Miscellaneous.MenuKey = menuKeyVK[ curMenuKey ];
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	// Right Column
	ImGui::BeginChild( "Misc_Right", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "MOVEMENT & EXPLOITS" );
		AwCheckbox( "Bunny Hop", &g_CVars.Miscellaneous.BunnyHop );
		AwCheckbox( "Auto Strafe", &g_CVars.Miscellaneous.AutoStrafe );
		static const char* asModeNames[] = { "Classic (hold space)", "Directional (slophook)" }; // r38
		if( g_CVars.Miscellaneous.AutoStrafeMode < 0 || g_CVars.Miscellaneous.AutoStrafeMode > 1 ) g_CVars.Miscellaneous.AutoStrafeMode = 0;
		ImGui::Combo( "Strafe Mode", &g_CVars.Miscellaneous.AutoStrafeMode, asModeNames, IM_ARRAYSIZE( asModeNames ) ); // r38
		AwSliderInt( "Strafe Avoid Dist", &g_CVars.Miscellaneous.StrafeAvoidDist, 16, 256 ); // r38: directional wall probe
		AwCheckbox( "Circle Strafe (hold V)", &g_CVars.Miscellaneous.CircleStrafe );
		AwCheckbox( "Slow Walk", &g_CVars.Miscellaneous.SlowWalk );
		static const char* slowKeyNames[] = { "Off", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5", "SHIFT" }; // r41: + SHIFT
		if( g_CVars.Miscellaneous.SlowWalkKey < 0 || g_CVars.Miscellaneous.SlowWalkKey >= 7 ) g_CVars.Miscellaneous.SlowWalkKey = 6; // r41: default SHIFT
		ImGui::Combo( "Slow Walk Key", &g_CVars.Miscellaneous.SlowWalkKey, slowKeyNames, IM_ARRAYSIZE( slowKeyNames ) );
		AwSliderInt( "Slow Walk Speed", &g_CVars.Miscellaneous.SlowWalkSpeed, 50, 200 );
		AwCheckbox( "Air Stuck (press F)", &g_CVars.Miscellaneous.AirStuck );
		AwCheckbox( "Auto Knife", &g_CVars.Miscellaneous.AutoKnife );
		AwCheckbox( "Speedhack", &g_CVars.Miscellaneous.Speedhack );
		AwSliderInt( "Speedhack Factor", &g_CVars.Miscellaneous.SpeedhackValue, 0, 13 );
		static const char* speedKeyNames[] = { "E", "Mouse 4", "Mouse 5", "ALT", "SHIFT", "X" };
		if( g_CVars.Miscellaneous.SpeedhackKey < 0 || g_CVars.Miscellaneous.SpeedhackKey >= 6 ) g_CVars.Miscellaneous.SpeedhackKey = 0;
		ImGui::Combo( "Speedhack Key", &g_CVars.Miscellaneous.SpeedhackKey, speedKeyNames, IM_ARRAYSIZE( speedKeyNames ) );

		ImGui::Spacing( );
		SectionHeader( "OTHER" );
		AwCheckbox( "Round Say", &g_CVars.Miscellaneous.RoundSay );
		AwCheckbox( "sv_cheats Bypass", &g_CVars.Miscellaneous.CheatsBypass );
		AwCheckbox( "Third Person View", &g_CVars.Miscellaneous.ThirdPerson );
		static const char* tpKeyNames[] = { "Off", "Mouse 4", "Mouse 5", "V", "C", "T", "F" };
		static const int tpKeyVK[] = { 0, 0x05, 0x06, 'V', 'C', 'T', 'F' };
		int curTpKey = 0;
		for( int k = 0; k < 7; k++ ) if( g_CVars.Miscellaneous.ThirdPersonKey == tpKeyVK[ k ] ) curTpKey = k;
		if( ImGui::Combo( "TP Key", &curTpKey, tpKeyNames, IM_ARRAYSIZE( tpKeyNames ) ) )
			g_CVars.Miscellaneous.ThirdPersonKey = tpKeyVK[ curTpKey ];
		AwSliderInt( "TP Distance", &g_CVars.Miscellaneous.ThirdPersonDist, 50, 250 );

		ImGui::Spacing( );
		SectionHeader( "CLIENTMOD EMULATOR" ); // r34
		AwCheckbox( "ClientMod Emulator", &g_CVars.Miscellaneous.ClientModEmulator );
		ImGui::TextDisabled( "Anti-detect for clientmod-checked servers. Applies on next inject." );
	}
	ImGui::EndChild( );
}

void GUI::RenderPlayerListTab( void )
{
	if( !g_pEngineClient || !g_pEngineClient->IsInGame( ) || !g_pGlobals )
	{
		ImGui::TextDisabled( "Join a game server to view active players." );
		return;
	}

	if( ImGui::BeginTable( "PlayerListTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY ) )
	{
		ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthFixed, 30.0f );
		ImGui::TableSetupColumn( "Player Name", ImGuiTableColumnFlags_WidthStretch );
		ImGui::TableSetupColumn( "Friend", ImGuiTableColumnFlags_WidthFixed, 60.0f );
		ImGui::TableSetupColumn( "Pitch Override", ImGuiTableColumnFlags_WidthFixed, 120.0f );
		ImGui::TableSetupColumn( "Yaw Override", ImGuiTableColumnFlags_WidthFixed, 130.0f );
		ImGui::TableHeadersRow( );

		player_info_t pInfo;
		const char* pitchNames[] = { "Auto", "Down", "Up", "Off" };
		const char* yawNames[] = { "Auto", "Resolver", "Half Left", "Half Right", "Inverse", "Off" };

		for( int i = 1; i <= g_pGlobals->maxClients && i < 64; i++ ) // bounds: PlayerList arrays are [64]
		{
			if( i == g_pEngineClient->GetLocalPlayer( ) )
				continue;

			BasePlayer* pEnt = ( BasePlayer* )g_pClientEntityList->GetClientEntity( i );
			if( !pEnt || !g_pEngineClient->GetPlayerInfo( i, &pInfo ) )
				continue;

			ImGui::TableNextRow( );
			ImGui::PushID( i );

			// ID
			ImGui::TableSetColumnIndex( 0 );
			ImGui::Text( "%d", i );

			// Name
			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( "%s", pInfo.name );

			// Friend
			ImGui::TableSetColumnIndex( 2 );
			bool isFriend = g_CVars.PlayerList.Friend[ i ];
			AwCheckbox( "##Friend", &isFriend );
			g_CVars.PlayerList.Friend[ i ] = isFriend;

			// Pitch Override
			ImGui::TableSetColumnIndex( 3 );
			int curPitch = g_CVars.PlayerList.Pitch[ i ];
			if( curPitch < 0 || curPitch > 3 ) curPitch = 0;
			ImGui::SetNextItemWidth( -1 );
			if( ImGui::Combo( "##Pitch", &curPitch, pitchNames, IM_ARRAYSIZE( pitchNames ) ) )
			{
				g_CVars.PlayerList.Pitch[ i ] = curPitch;
			}

			// Yaw Override
			ImGui::TableSetColumnIndex( 4 );
			int curYaw = g_CVars.PlayerList.Yaw[ i ];
			if( curYaw < 0 || curYaw > 5 ) curYaw = 0;
			ImGui::SetNextItemWidth( -1 );
			if( ImGui::Combo( "##Yaw", &curYaw, yawNames, IM_ARRAYSIZE( yawNames ) ) )
			{
				g_CVars.PlayerList.Yaw[ i ] = curYaw;
			}

			ImGui::PopID( );
		}

		ImGui::EndTable( );
	}
}

void GUI::RenderConfigsTab( void )
{
	ImGui::BeginChild( "ConfigsChild", ImVec2( 0, 0 ), true );
	{
		SectionHeader( "CONFIG MANAGER" );
		static const char* menuModeNames[] = { "Classic", "Aimware V4", "V3 (orange)" }; // r13, r44
		if( g_CVars.Miscellaneous.MenuMode < 0 || g_CVars.Miscellaneous.MenuMode > 2 ) g_CVars.Miscellaneous.MenuMode = 0; // r44: + V3
		ImGui::Combo( "Menu Style", &g_CVars.Miscellaneous.MenuMode, menuModeNames, IM_ARRAYSIZE( menuModeNames ) );
		static std::vector<std::string> s_list;
		static int s_sel = -1;
		static char s_name[ 64 ] = "myconfig";
		static std::string s_status = "Configs: C:\\Awesware\\configs";
		static bool s_dirty = true;

		if( s_dirty )
		{
			g_Config.List( s_list );
			if( s_sel >= ( int )s_list.size( ) ) s_sel = s_list.size( ) ? 0 : -1;
			s_dirty = false;
		}

		ImGui::BeginListBox( "##cfglist", ImVec2( -1, 300 ) );
		for( int i = 0; i < ( int )s_list.size( ); i++ )
		{
			if( ImGui::Selectable( s_list[ i ].c_str( ), s_sel == i ) )
			{
				s_sel = i;
				size_t k = 0;
				for( ; s_list[ i ].c_str( )[ k ] && k < 63; k++ ) s_name[ k ] = s_list[ i ].c_str( )[ k ];
				s_name[ k ] = 0;
			}
		}
		ImGui::EndListBox( );

		ImGui::InputText( "Name", s_name, 64 );
		if( ImGui::Button( "Save", ImVec2( 110, 0 ) ) )
		{
			if( g_Config.SaveAs( s_name ) ) { s_status = std::string( "Saved: " ) + s_name; s_dirty = true; }
			else s_status = "Save failed";
		}
		ImGui::SameLine( );
		if( ImGui::Button( "Load", ImVec2( 110, 0 ) ) )
		{
			if( s_sel >= 0 && g_Config.LoadFrom( s_list[ s_sel ].c_str( ) ) ) s_status = std::string( "Loaded: " ) + s_list[ s_sel ];
			else s_status = "Load failed";
		}
		ImGui::SameLine( );
		if( ImGui::Button( "Delete", ImVec2( 110, 0 ) ) )
		{
			if( s_sel >= 0 && g_Config.Delete( s_list[ s_sel ].c_str( ) ) ) { s_status = std::string( "Deleted: " ) + s_list[ s_sel ]; s_dirty = true; }
			else s_status = "Delete failed";
		}
		if( ImGui::Button( "Refresh", ImVec2( 110, 0 ) ) ) s_dirty = true;
		ImGui::SameLine( );
		if( ImGui::Button( "Open Folder", ImVec2( 110, 0 ) ) ) WinExec( "explorer.exe C:\\Awesware\\configs", SW_SHOWNORMAL );

		ImGui::Spacing( );
		ImGui::TextDisabled( "%s", s_status.c_str( ) );

		ImGui::Spacing( ); ImGui::Separator( );
		ImGui::Text( "Hotkeys:" );
		ImGui::BulletText( "INSERT: Toggle Menu & Mouse" );
		ImGui::BulletText( "F12: Unhook & Eject DLL" );
		ImGui::BulletText( "F6 - F11: Movement Recorder Controls" );
	}
	ImGui::EndChild( );
}

// scripts tab: script list on the left, the selected script's own ui.* menu on the right
void GUI::RenderScriptsTab( void )
{
	static int s_sel = -1;
	int n = LuaAPI::ScriptCount( );
	if( s_sel >= n ) s_sel = n ? 0 : -1;

	// left: script list + controls
	ImGui::BeginChild( "Scripts_List", ImVec2( 240, 0 ), true );
	{
		SectionHeader( "SCRIPTS" );
		ImGui::BeginListBox( "##scripts", ImVec2( -1, 300 ) );
		for( int i = 0; i < n; i++ )
		{
			const char* st = LuaAPI::ScriptStatus( i );
			bool ok = ( strcmp( st, "OK" ) == 0 );
			char label[ 160 ];
			sprintf( label, "%s%s%s", LuaAPI::ScriptName( i ), ok ? "" : " [!]", ( s_sel == i ) ? " *" : "" );
			if( ImGui::Selectable( label, s_sel == i ) ) s_sel = i;
			if( ImGui::IsItemHovered( ) && !ok ) ImGui::SetTooltip( "%s", st );
		}
		ImGui::EndListBox( );

		if( ImGui::Button( "Reload Scripts", ImVec2( -1, 0 ) ) ) LuaAPI::Reload( );
		if( ImGui::Button( "Open Folder", ImVec2( -1, 0 ) ) ) WinExec( "explorer.exe C:\\Awesware\\scripts", SW_SHOWNORMAL );
		ImGui::Spacing( );
		ImGui::TextDisabled( "C:\\Awesware\\scripts\\*.lua" );
		ImGui::TextDisabled( "%d script(s), [!] = error", n );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	// right: the selected script's own menu (ui.* elements drawn by its on_menu)
	ImGui::BeginChild( "Scripts_Menu", ImVec2( 0, 0 ), true );
	{
		if( s_sel < 0 || s_sel >= n )
		{
			ImGui::TextDisabled( "Select a script on the left." );
		}
		else
		{
			SectionHeader( LuaAPI::ScriptName( s_sel ) );
			const char* st = LuaAPI::ScriptStatus( s_sel );
			if( strcmp( st, "OK" ) != 0 )
			{
				ImGui::TextColored( ImVec4( 0.85f, 0.30f, 0.30f, 1.00f ), "error:" );
				ImGui::TextWrapped( "%s", st );
			}
			else if( !LuaAPI::ScriptHasMenu( s_sel ) )
			{
				ImGui::TextDisabled( "This script has no on_menu()." );
				ImGui::Spacing( );
				ImGui::TextWrapped( "Add one to build its settings here: ui.checkbox, ui.slider_int, ui.slider_float, ui.combo, ui.button, ui.text, ui.header, ui.separator, ui.same_line. Read values anywhere with ui.get(label, default)." );
			}
			else
			{
				LuaAPI::Menu( s_sel );
			}
		}
	}
	ImGui::EndChild( );
}