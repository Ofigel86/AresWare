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

static void SectionHeader( const char* label )
{
	ImGui::Spacing( );
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.62f, 0.44f, 1.00f, 1.00f ) );
	ImGui::TextUnformatted( label );
	ImGui::PopStyleColor( );
	ImGui::Separator( );
}

void GUI::SetupStyle( void )
{
	ImGuiStyle& style = ImGui::GetStyle( );
	ImVec4* colors = style.Colors;

	// Dark purple theme with violet accents
	colors[ ImGuiCol_Text ]                  = ImVec4( 0.93f, 0.94f, 0.96f, 1.00f );
	colors[ ImGuiCol_TextDisabled ]          = ImVec4( 0.49f, 0.47f, 0.54f, 1.00f );
	colors[ ImGuiCol_WindowBg ]              = ImVec4( 0.078f, 0.071f, 0.094f, 0.97f );
	colors[ ImGuiCol_ChildBg ]               = ImVec4( 0.102f, 0.094f, 0.125f, 0.92f );
	colors[ ImGuiCol_PopupBg ]               = ImVec4( 0.094f, 0.086f, 0.117f, 0.97f );
	colors[ ImGuiCol_Border ]                = ImVec4( 0.240f, 0.210f, 0.300f, 0.60f );
	colors[ ImGuiCol_BorderShadow ]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_FrameBg ]               = ImVec4( 0.145f, 0.129f, 0.180f, 0.85f );
	colors[ ImGuiCol_FrameBgHovered ]        = ImVec4( 0.192f, 0.169f, 0.251f, 0.85f );
	colors[ ImGuiCol_FrameBgActive ]         = ImVec4( 0.243f, 0.208f, 0.337f, 0.92f );
	colors[ ImGuiCol_TitleBg ]               = ImVec4( 0.071f, 0.063f, 0.086f, 1.00f );
	colors[ ImGuiCol_TitleBgActive ]         = ImVec4( 0.106f, 0.094f, 0.133f, 1.00f );
	colors[ ImGuiCol_TitleBgCollapsed ]      = ImVec4( 0.071f, 0.063f, 0.086f, 0.75f );
	colors[ ImGuiCol_MenuBarBg ]             = ImVec4( 0.102f, 0.094f, 0.125f, 1.00f );
	colors[ ImGuiCol_ScrollbarBg ]           = ImVec4( 0.071f, 0.063f, 0.086f, 0.60f );
	colors[ ImGuiCol_ScrollbarGrab ]         = ImVec4( 0.208f, 0.188f, 0.251f, 0.80f );
	colors[ ImGuiCol_ScrollbarGrabHovered ]  = ImVec4( 0.263f, 0.231f, 0.337f, 0.80f );
	colors[ ImGuiCol_ScrollbarGrabActive ]   = ImVec4( 0.325f, 0.286f, 0.427f, 0.90f );
	colors[ ImGuiCol_CheckMark ]             = ImVec4( 0.630f, 0.420f, 1.000f, 1.00f );
	colors[ ImGuiCol_SliderGrab ]            = ImVec4( 0.570f, 0.360f, 0.980f, 0.90f );
	colors[ ImGuiCol_SliderGrabActive ]      = ImVec4( 0.680f, 0.500f, 1.000f, 1.00f );
	colors[ ImGuiCol_Button ]                = ImVec4( 0.184f, 0.165f, 0.231f, 0.85f );
	colors[ ImGuiCol_ButtonHovered ]         = ImVec4( 0.251f, 0.212f, 0.345f, 0.92f );
	colors[ ImGuiCol_ButtonActive ]          = ImVec4( 0.500f, 0.310f, 0.950f, 1.00f );
	colors[ ImGuiCol_Header ]                = ImVec4( 0.212f, 0.184f, 0.290f, 0.75f );
	colors[ ImGuiCol_HeaderHovered ]         = ImVec4( 0.278f, 0.235f, 0.400f, 0.85f );
	colors[ ImGuiCol_HeaderActive ]          = ImVec4( 0.420f, 0.310f, 0.780f, 0.95f );
	colors[ ImGuiCol_Separator ]             = ImVec4( 0.235f, 0.212f, 0.275f, 0.60f );
	colors[ ImGuiCol_SeparatorHovered ]      = ImVec4( 0.480f, 0.340f, 0.900f, 0.80f );
	colors[ ImGuiCol_SeparatorActive ]       = ImVec4( 0.580f, 0.420f, 1.000f, 1.00f );
	colors[ ImGuiCol_ResizeGrip ]            = ImVec4( 0.184f, 0.165f, 0.231f, 0.50f );
	colors[ ImGuiCol_ResizeGripHovered ]     = ImVec4( 0.480f, 0.340f, 0.900f, 0.70f );
	colors[ ImGuiCol_ResizeGripActive ]      = ImVec4( 0.580f, 0.420f, 1.000f, 0.90f );
	colors[ ImGuiCol_Tab ]                   = ImVec4( 0.129f, 0.117f, 0.153f, 0.85f );
	colors[ ImGuiCol_TabHovered ]            = ImVec4( 0.251f, 0.212f, 0.345f, 0.92f );
	colors[ ImGuiCol_TabActive ]             = ImVec4( 0.212f, 0.184f, 0.290f, 1.00f );
	colors[ ImGuiCol_TabUnfocused ]          = ImVec4( 0.102f, 0.094f, 0.125f, 0.85f );
	colors[ ImGuiCol_TabUnfocusedActive ]    = ImVec4( 0.145f, 0.129f, 0.180f, 1.00f );
	colors[ ImGuiCol_TableHeaderBg ]         = ImVec4( 0.145f, 0.129f, 0.180f, 1.00f );
	colors[ ImGuiCol_TableBorderStrong ]     = ImVec4( 0.235f, 0.212f, 0.275f, 1.00f );
	colors[ ImGuiCol_TableBorderLight ]      = ImVec4( 0.184f, 0.165f, 0.231f, 0.60f );
	colors[ ImGuiCol_TableRowBg ]            = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_TableRowBgAlt ]         = ImVec4( 1.00f, 1.00f, 1.00f, 0.03f );

	style.WindowPadding     = ImVec2( 12, 12 );
	style.FramePadding      = ImVec2( 7, 4 );
	style.ItemSpacing       = ImVec2( 8, 6 );
	style.ItemInnerSpacing  = ImVec2( 6, 4 );
	style.ScrollbarSize     = 13.0f;
	style.GrabMinSize       = 11.0f;
	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.PopupBorderSize   = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.TabBorderSize     = 0.0f;
	style.WindowRounding    = 7.0f;
	style.ChildRounding     = 6.0f;
	style.FrameRounding     = 4.0f;
	style.PopupRounding     = 4.0f;
	style.ScrollbarRounding = 7.0f;
	style.GrabRounding      = 3.0f;
	style.TabRounding       = 4.0f;
}

void GUI::DrawImGui( void )
{
	if( !bMouse )
		return;

	static int selectedPage = 0;

	ImGui::SetNextWindowSize( ImVec2( 780, 580 ), ImGuiCond_FirstUseEver );
	if( ImGui::Begin( "Insomnia Hook | Counter-Strike: Source v34", nullptr, ImGuiWindowFlags_NoCollapse ) )
	{
		// Left navigation sidebar
		ImGui::BeginChild( "NavSidebar", ImVec2( 150.0f, 0.0f ), false );
		{
			ImGui::Spacing( );
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.62f, 0.44f, 1.00f, 1.00f ) );
			ImGui::TextUnformatted( "INSOMNIA" );
			ImGui::PopStyleColor( );
			ImGui::TextDisabled( "css v34 hook" );
			ImGui::Spacing( );
			ImGui::Separator( );
			ImGui::Spacing( );

			const char* pages[] = { "Ragebot", "Visuals", "Miscellaneous", "Player List", "Config" };
			const int numPages = ( int )( sizeof( pages ) / sizeof( pages[ 0 ] ) );
			for( int i = 0; i < numPages; i++ )
			{
				if( ImGui::Selectable( pages[ i ], selectedPage == i, ImGuiSelectableFlags_None, ImVec2( 0.0f, 26.0f ) ) )
					selectedPage = i;
			}
		}
		ImGui::EndChild( );

		ImGui::SameLine( );

		// Page content
		ImGui::BeginChild( "PageContent", ImVec2( 0.0f, 0.0f ), false );
		{
			switch( selectedPage )
			{
			case 0: RenderRagebotTab( );  break;
			case 1: RenderVisualsTab( );    break;
			case 2: RenderMiscTab( );       break;
			case 3: RenderPlayerListTab( ); break;
			case 4: RenderConfigsTab( );    break;
			default: break;
			}
		}
		ImGui::EndChild( );
	}
	ImGui::End( );
}

static void RageGeneral( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	ImGui::BeginChild( "General_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "AIMBOT MAIN" );
		ImGui::Checkbox( "Active", &g_CVars.Aimbot.Active );
		ImGui::Checkbox( "Auto Shoot", &g_CVars.Aimbot.AutoShoot );
		ImGui::Checkbox( "Silent Aim", &g_CVars.Aimbot.Silent );
		ImGui::Checkbox( "Perfect Silent", &g_CVars.Aimbot.PerfectSilent );
		ImGui::Checkbox( "Multi Spot", &g_CVars.Aimbot.MultiSpot );
		ImGui::Checkbox( "Body AWP", &g_CVars.Aimbot.BodyAWP );
		ImGui::Checkbox( "Hit Scan", &g_CVars.Aimbot.HitScan );
		ImGui::Checkbox( "Perfect Auto Wall", &g_CVars.Aimbot.AutoWall );
		ImGui::Checkbox( "Friendly Fire", &g_CVars.Aimbot.FriendlyFire );

		ImGui::Spacing( );
		SectionHeader( "TRIGGERBOT" );
		ImGui::Checkbox( "Triggerbot Active", &g_CVars.Triggerbot.Active );
		ImGui::Checkbox( "Seed Check", &g_CVars.Triggerbot.Seed );
		ImGui::Checkbox( "Spread Check", &g_CVars.Triggerbot.Spread );
		ImGui::Checkbox( "Recoil Check", &g_CVars.Triggerbot.Recoil );

		const char* triggerStrengthNames[] = { "Low", "Medium", "High", "Extra" };
		ImGui::Combo( "Strength", &g_CVars.Triggerbot.Strength, triggerStrengthNames, IM_ARRAYSIZE( triggerStrengthNames ) );

		const char* triggerHitboxNames[] = { "Head", "Upper Body", "Lower Body", "Full Body" };
		ImGui::Combo( "Trigger Hitbox", &g_CVars.Triggerbot.Hitbox, triggerHitboxNames, IM_ARRAYSIZE( triggerHitboxNames ) );

		const char* keyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		ImGui::Combo( "Trigger Key", &g_CVars.Triggerbot.Key, keyNames, IM_ARRAYSIZE( keyNames ) );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "General_R", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "RESOLVER" );
		ImGui::Checkbox( "Resolver Active", &g_CVars.Aimbot.Resolver.Active );

		ImGui::Checkbox( "Smart Resolver", &g_CVars.Aimbot.Resolver.Smart );
		ImGui::TextDisabled( "Adaptive: resolves all enemies by their history" );
	}
	ImGui::EndChild( );
}

static void RageTargeting( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	ImGui::BeginChild( "Targeting_Left", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "TARGETING & ADJUSTMENTS" );

		const char* aimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		ImGui::Combo( "Aim Key", &g_CVars.Aimbot.Key, aimKeyNames, IM_ARRAYSIZE( aimKeyNames ) );

		const char* hitboxNames[] = { "Head", "Neck", "Chest", "Stomach" };
		int currentHitboxIdx = GetHitboxIndex( g_CVars.Aimbot.Hitbox );
		if( ImGui::Combo( "Hitbox", &currentHitboxIdx, hitboxNames, IM_ARRAYSIZE( hitboxNames ) ) )
		{
			g_CVars.Aimbot.Hitbox = HitboxFromIndex( currentHitboxIdx );
		}

		ImGui::SliderFloat( "Point Scale", &g_CVars.Aimbot.PointScale, 0.0f, 1.0f, "%.2f" );

		const char* heightModeNames[] = { "Auto", "Origin", "Center", "Center Fixed", "Highest" };
		ImGui::Combo( "Height Mode", &g_CVars.Aimbot.HitboxMode, heightModeNames, IM_ARRAYSIZE( heightModeNames ) );

		const char* targetSelectionNames[] = { "Distance", "Health", "Next Shot", "Random" };
		ImGui::Combo( "Target Selection", &g_CVars.Aimbot.TargetSelection, targetSelectionNames, IM_ARRAYSIZE( targetSelectionNames ) );

		ImGui::SliderInt( "Min Damage", &g_CVars.Aimbot.MinDamage, 0, 100 );

		const char* posAdjustmentNames[] = { "Off", "On", "On + History" };
		ImGui::Combo( "Pos Adjustment", &g_CVars.Aimbot.Interpolation.LagPrediction, posAdjustmentNames, IM_ARRAYSIZE( posAdjustmentNames ) );

		ImGui::Spacing( );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "Targeting_R", ImVec2( halfWidth, 0 ), true );
	{
		SectionHeader( "SNAP LIMITER" );
		ImGui::Checkbox( "Snap Limiter Active", &g_CVars.Aimbot.SnapLimiter );
		ImGui::SliderInt( "Angle Limit", &g_CVars.Aimbot.AngleLimit, 0, 180 );
		ImGui::SliderFloat( "Angle Limit Tens", &g_CVars.Aimbot.AngleLimitTens, 0.0f, 1.0f, "%.2f" );

		ImGui::Spacing( );
	}
	ImGui::EndChild( );
}

static void RageAccuracy( void )
{
	ImGui::BeginChild( "Rage_Accuracy", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "ACCURACY" );
		ImGui::Checkbox( "Remove Recoil / Spread", &g_CVars.Accuracy.PerfectAccuracy );
		ImGui::TextDisabled( "NoSpread: Classic mode (best, fixed)" );

		ImGui::Spacing( );
		const char* autoStopNames[] = { "Off", "Min Speed", "Full Stop" };
		ImGui::Combo( "Auto Stop", &g_CVars.Miscellaneous.AutoStop, autoStopNames, IM_ARRAYSIZE( autoStopNames ) );
	}
	ImGui::EndChild( );
}

static void RageAntiAim( void )
{
	ImGui::BeginChild( "Rage_AntiAim", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "ANTI-AIM (HVH)" );
		ImGui::Checkbox( "Anti-Aim Active", &g_CVars.Miscellaneous.AntiAim.Active );

		const char* pitchNames[] = { "Off", "Normal", "Inverse Normal", "Safe", "Fake Down", "Down", "Up", "Lag Down", "Lag Up" };
		ImGui::Combo( "Pitch", &g_CVars.Miscellaneous.AntiAim.Pitch, pitchNames, IM_ARRAYSIZE( pitchNames ) );

		const char* yawNames[] = { "Forwards", "Backwards", "Sideways", "Jitter", "Static", "Static Reversed", "Lisp", "Custom" };
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
		else
		{
			yawVariations = { "Normal", "Fake Side 1", "Fake Side 2", "Random" };
		}

		if( g_CVars.Miscellaneous.AntiAim.Variation >= ( int )yawVariations.size( ) )
			g_CVars.Miscellaneous.AntiAim.Variation = 0;

		ImGui::Combo( "Yaw Mode", &g_CVars.Miscellaneous.AntiAim.Variation, yawVariations.data( ), ( int )yawVariations.size( ) );

		ImGui::SliderFloat( "Custom Real Yaw", &g_CVars.Miscellaneous.AntiAim.RealValue, 0.0f, 360.0f, "%.1f deg" );
		ImGui::SliderFloat( "Custom Fake Yaw", &g_CVars.Miscellaneous.AntiAim.FakeValue, 0.0f, 360.0f, "%.1f deg" );

		ImGui::Checkbox( "InAttack Pitch", &g_CVars.Miscellaneous.AntiAim.Static );
		ImGui::Checkbox( "Wall Detection", &g_CVars.Miscellaneous.AntiAim.WallDetection );

		const char* wallDtcModes[] = { "Normal", "Fake", "Fake Out", "Jitter" };
		ImGui::Combo( "Wall DTC Mode", &g_CVars.Miscellaneous.AntiAim.WallDetectionMode, wallDtcModes, IM_ARRAYSIZE( wallDtcModes ) );

		ImGui::Checkbox( "At Targets", &g_CVars.Miscellaneous.AntiAim.AtTargets );
		ImGui::Checkbox( "Duck In Air", &g_CVars.Miscellaneous.AntiAim.DuckInAir );
		ImGui::Checkbox( "Enemy Check", &g_CVars.Miscellaneous.AntiAim.TurnOff );
	}
	ImGui::EndChild( );
}

void GUI::RenderRagebotTab( void )
{
	if( ImGui::BeginTabBar( "RagebotSubTabs", ImGuiTabBarFlags_None ) )
	{
		if( ImGui::BeginTabItem( "General" ) )
		{
			RageGeneral( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "Targeting" ) )
		{
			RageTargeting( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "Accuracy" ) )
		{
			RageAccuracy( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "Anti-Aim" ) )
		{
			RageAntiAim( );
			ImGui::EndTabItem( );
		}
		ImGui::EndTabBar( );
	}
}

static void VisESP( void )
{
	ImGui::BeginChild( "Vis_ESP", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "ESP (SURFACE RENDER)" );
		ImGui::Checkbox( "Bounding Box", &g_CVars.Visuals.ESP.Box );
		ImGui::Checkbox( "Player Name", &g_CVars.Visuals.ESP.Name );
		ImGui::Checkbox( "Health Bar / Text", &g_CVars.Visuals.ESP.Health );
		ImGui::Checkbox( "Weapon Name", &g_CVars.Visuals.ESP.Weapon );
		ImGui::Checkbox( "Skeleton / Bone", &g_CVars.Visuals.ESP.Bone );
		ImGui::Checkbox( "Aim Spot", &g_CVars.Visuals.ESP.AimSpot );
		ImGui::Checkbox( "Hitmarker", &g_CVars.Visuals.ESP.Hit );
		ImGui::Checkbox( "Ground ESP", &g_CVars.Visuals.ESP.Ground );
		ImGui::Checkbox( "Enemy Only", &g_CVars.Visuals.ESP.EnemyOnly );

		ImGui::Spacing( );
		ImGui::Spacing( );
		SectionHeader( "ESP COLORS" );
		ImGuiColorEdit( "CT ESP", g_CVars.ColorSelector.ESP.CT );
		ImGuiColorEdit( "T ESP", g_CVars.ColorSelector.ESP.TT );
		ImGuiColorEdit( "Weapon ESP", g_CVars.ColorSelector.ESP.Wpn );

		ImGui::Spacing( );
		ImGui::Text( "Chams Colors:" );

	}
	ImGui::EndChild( );
}

static void VisChams( void )
{
	ImGui::BeginChild( "Vis_Chams", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "CHAMS & MODELS" );
		ImGui::Checkbox( "Player Chams", &g_CVars.Visuals.Chams.Active );
		ImGui::Checkbox( "Weapon Chams", &g_CVars.Visuals.Chams.Weapons );
		ImGui::Checkbox( "Draw Shadows", &g_CVars.Visuals.Chams.Shadows );
		ImGui::Checkbox( "Model Outline", &g_CVars.Visuals.Chams.Outline );
		ImGui::Checkbox( "Hands Outline", &g_CVars.Visuals.Chams.HandsOutline );
		ImGui::Checkbox( "Chams Enemy Only", &g_CVars.Visuals.Chams.EnemyOnly );
		ImGui::Spacing( );
		SectionHeader( "CHAMS COLORS" );
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

static void VisWorld( void )
{
	ImGui::BeginChild( "Vis_World", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "WORLD & SCREEN" );
		ImGui::Checkbox( "Draw Radar", &g_CVars.Visuals.Radar );
		ImGui::Checkbox( "No Sky", &g_CVars.Visuals.NoSky );
		ImGui::Checkbox( "No Smoke", &g_CVars.Visuals.NoSmoke );
		ImGui::Checkbox( "No Flash", &g_CVars.Visuals.NoFlash );
		ImGui::Checkbox( "No Hands", &g_CVars.Visuals.NoHands );
		ImGui::Checkbox( "No Visual Recoil", &g_CVars.Visuals.NoVisualRecoil );
		ImGui::SliderFloat( "ASUS Walls", &g_CVars.Visuals.ASUS, 0.0f, 1.0f, "%.2f" );

		const char* crosshairTypeNames[] = { "Off", "Cross", "Dot", "Round" };
		ImGui::Combo( "Crosshair Type", &g_CVars.Visuals.Crosshair.Type, crosshairTypeNames, IM_ARRAYSIZE( crosshairTypeNames ) );
		ImGui::Checkbox( "Dynamic Crosshair", &g_CVars.Visuals.Crosshair.Dynamic );

		ImGui::Spacing( );
	}
	ImGui::EndChild( );
}

void GUI::RenderVisualsTab( void )
{
	if( ImGui::BeginTabBar( "VisualsSubTabs", ImGuiTabBarFlags_None ) )
	{
		if( ImGui::BeginTabItem( "ESP" ) )
		{
			VisESP( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "Chams" ) )
		{
			VisChams( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "World" ) )
		{
			VisWorld( );
			ImGui::EndTabItem( );
		}
		ImGui::EndTabBar( );
	}
}

static void MiscMovement( void )
{
	ImGui::BeginChild( "Misc_Movement", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "MOVEMENT & EXPLOITS" );
		ImGui::Checkbox( "Bunny Hop", &g_CVars.Miscellaneous.BunnyHop );
		ImGui::Checkbox( "Auto Strafe", &g_CVars.Miscellaneous.AutoStrafe );
		ImGui::Checkbox( "Circle Strafe (hold V)", &g_CVars.Miscellaneous.CircleStrafe );
		ImGui::Checkbox( "Air Stuck (press F)", &g_CVars.Miscellaneous.AirStuck );
		ImGui::Checkbox( "Auto Knife", &g_CVars.Miscellaneous.AutoKnife );
		ImGui::Checkbox( "Speedhack (hold E)", &g_CVars.Miscellaneous.Speedhack );
		ImGui::SliderInt( "Speedhack Factor", &g_CVars.Miscellaneous.SpeedhackValue, 0, 13 );

		ImGui::Spacing( );
	}
	ImGui::EndChild( );
}

static void MiscOther( void )
{
	ImGui::BeginChild( "Misc_Other", ImVec2( 440, 0 ), true );
	{
		SectionHeader( "OTHER" );
		ImGui::Checkbox( "Round Say", &g_CVars.Miscellaneous.RoundSay );
		ImGui::Checkbox( "sv_cheats Bypass", &g_CVars.Miscellaneous.CheatsBypass );
		ImGui::Checkbox( "Third Person View", &g_CVars.Miscellaneous.ThirdPerson );
		ImGui::Checkbox( "Anti SMAC", &g_CVars.Aimbot.AntiSMAC );
		ImGui::Spacing( );
		SectionHeader( "FAKE LAG" );
		ImGui::Checkbox( "Fake Lag Active", &g_CVars.Miscellaneous.Fakelag.Active );
		ImGui::Checkbox( "Fake Lag In Attack", &g_CVars.Miscellaneous.Fakelag.InAttack );
		ImGui::Checkbox( "Fake Lag Air Only", &g_CVars.Miscellaneous.Fakelag.AirOnly );
		ImGui::SliderInt( "Choke Ticks", &g_CVars.Miscellaneous.Fakelag.Value, 0, 14 );

		const char* fakelagModes[] = { "Factor", "Switch", "Adaptive" };
		ImGui::Combo( "Fake Lag Mode", &g_CVars.Miscellaneous.Fakelag.Mode, fakelagModes, IM_ARRAYSIZE( fakelagModes ) );

		ImGui::Spacing( );
	}
	ImGui::EndChild( );
}

void GUI::RenderMiscTab( void )
{
	if( ImGui::BeginTabBar( "MiscSubTabs", ImGuiTabBarFlags_None ) )
	{
		if( ImGui::BeginTabItem( "Movement" ) )
		{
			MiscMovement( );
			ImGui::EndTabItem( );
		}
		if( ImGui::BeginTabItem( "Other" ) )
		{
			MiscOther( );
			ImGui::EndTabItem( );
		}
		ImGui::EndTabBar( );
	}
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

		for( int i = 1; i <= g_pGlobals->maxClients; i++ )
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
			if( ImGui::Checkbox( "##Friend", &isFriend ) )
			{
				g_CVars.PlayerList.Friend[ i ] = isFriend;
			}

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
	ImGui::BeginChild( "ConfigsChild", ImVec2( 0, 0 ), false );
	{
		SectionHeader( "CONFIGURATION MANAGER" );

		ImGui::TextWrapped( "Config settings are saved to / loaded from your cheat configuration directory." );
		ImGui::Spacing( );

		if( ImGui::Button( "Save Configuration", ImVec2( 180, 32 ) ) )
		{
			g_Config.Save( );
		}

		ImGui::SameLine( );

		if( ImGui::Button( "Load Configuration", ImVec2( 180, 32 ) ) )
		{
			g_Config.Load( );
		}

		ImGui::Spacing( );
		ImGui::Separator( );
		ImGui::Text( "Hotkeys:" );
		ImGui::BulletText( "INSERT: Toggle Menu & Mouse" );
		ImGui::BulletText( "F12: Unhook & Eject DLL" );
		ImGui::BulletText( "F6 - F11: Movement Recorder Controls" );
	}
	ImGui::EndChild( );
}