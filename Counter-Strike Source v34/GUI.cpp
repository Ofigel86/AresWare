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

void GUI::SetupStyle( void )
{
	ImGuiStyle& style = ImGui::GetStyle( );
	ImVec4* colors = style.Colors;

	colors[ ImGuiCol_Text ]                  = ImVec4( 0.92f, 0.92f, 0.92f, 1.00f );
	colors[ ImGuiCol_TextDisabled ]          = ImVec4( 0.45f, 0.45f, 0.45f, 1.00f );
	colors[ ImGuiCol_WindowBg ]              = ImVec4( 0.12f, 0.12f, 0.14f, 0.96f );
	colors[ ImGuiCol_ChildBg ]               = ImVec4( 0.15f, 0.15f, 0.17f, 0.70f );
	colors[ ImGuiCol_PopupBg ]               = ImVec4( 0.14f, 0.14f, 0.16f, 0.96f );
	colors[ ImGuiCol_Border ]                = ImVec4( 0.25f, 0.25f, 0.28f, 0.65f );
	colors[ ImGuiCol_BorderShadow ]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_FrameBg ]               = ImVec4( 0.18f, 0.18f, 0.21f, 0.80f );
	colors[ ImGuiCol_FrameBgHovered ]        = ImVec4( 0.24f, 0.24f, 0.28f, 0.80f );
	colors[ ImGuiCol_FrameBgActive ]         = ImVec4( 0.28f, 0.28f, 0.33f, 0.90f );
	colors[ ImGuiCol_TitleBg ]               = ImVec4( 0.10f, 0.10f, 0.12f, 1.00f );
	colors[ ImGuiCol_TitleBgActive ]         = ImVec4( 0.14f, 0.14f, 0.16f, 1.00f );
	colors[ ImGuiCol_TitleBgCollapsed ]      = ImVec4( 0.10f, 0.10f, 0.12f, 0.75f );
	colors[ ImGuiCol_MenuBarBg ]             = ImVec4( 0.14f, 0.14f, 0.16f, 1.00f );
	colors[ ImGuiCol_ScrollbarBg ]           = ImVec4( 0.10f, 0.10f, 0.12f, 0.60f );
	colors[ ImGuiCol_ScrollbarGrab ]         = ImVec4( 0.24f, 0.24f, 0.28f, 0.80f );
	colors[ ImGuiCol_ScrollbarGrabHovered ]  = ImVec4( 0.30f, 0.30f, 0.35f, 0.80f );
	colors[ ImGuiCol_ScrollbarGrabActive ]   = ImVec4( 0.35f, 0.35f, 0.40f, 0.90f );
	colors[ ImGuiCol_CheckMark ]             = ImVec4( 0.55f, 0.85f, 0.15f, 1.00f );
	colors[ ImGuiCol_SliderGrab ]            = ImVec4( 0.50f, 0.80f, 0.12f, 0.90f );
	colors[ ImGuiCol_SliderGrabActive ]      = ImVec4( 0.60f, 0.92f, 0.18f, 1.00f );
	colors[ ImGuiCol_Button ]                = ImVec4( 0.20f, 0.20f, 0.24f, 0.80f );
	colors[ ImGuiCol_ButtonHovered ]         = ImVec4( 0.28f, 0.28f, 0.33f, 0.90f );
	colors[ ImGuiCol_ButtonActive ]          = ImVec4( 0.35f, 0.60f, 0.15f, 1.00f );
	colors[ ImGuiCol_Header ]                = ImVec4( 0.22f, 0.22f, 0.26f, 0.70f );
	colors[ ImGuiCol_HeaderHovered ]         = ImVec4( 0.30f, 0.30f, 0.35f, 0.80f );
	colors[ ImGuiCol_HeaderActive ]          = ImVec4( 0.35f, 0.35f, 0.40f, 0.90f );
	colors[ ImGuiCol_Separator ]             = ImVec4( 0.25f, 0.25f, 0.28f, 0.60f );
	colors[ ImGuiCol_SeparatorHovered ]      = ImVec4( 0.40f, 0.70f, 0.15f, 0.80f );
	colors[ ImGuiCol_SeparatorActive ]       = ImVec4( 0.50f, 0.80f, 0.20f, 1.00f );
	colors[ ImGuiCol_ResizeGrip ]            = ImVec4( 0.20f, 0.20f, 0.24f, 0.50f );
	colors[ ImGuiCol_ResizeGripHovered ]     = ImVec4( 0.50f, 0.80f, 0.15f, 0.70f );
	colors[ ImGuiCol_ResizeGripActive ]      = ImVec4( 0.60f, 0.90f, 0.20f, 0.90f );
	colors[ ImGuiCol_Tab ]                   = ImVec4( 0.15f, 0.15f, 0.18f, 0.80f );
	colors[ ImGuiCol_TabHovered ]            = ImVec4( 0.28f, 0.28f, 0.33f, 0.90f );
	colors[ ImGuiCol_TabActive ]             = ImVec4( 0.22f, 0.22f, 0.26f, 1.00f );
	colors[ ImGuiCol_TabUnfocused ]          = ImVec4( 0.12f, 0.12f, 0.14f, 0.80f );
	colors[ ImGuiCol_TabUnfocusedActive ]    = ImVec4( 0.16f, 0.16f, 0.20f, 1.00f );
	colors[ ImGuiCol_TableHeaderBg ]         = ImVec4( 0.16f, 0.16f, 0.19f, 1.00f );
	colors[ ImGuiCol_TableBorderStrong ]     = ImVec4( 0.25f, 0.25f, 0.28f, 0.80f );
	colors[ ImGuiCol_TableBorderLight ]      = ImVec4( 0.20f, 0.20f, 0.23f, 0.50f );
	colors[ ImGuiCol_TableRowBg ]            = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ ImGuiCol_TableRowBgAlt ]         = ImVec4( 1.00f, 1.00f, 1.00f, 0.03f );

	style.WindowPadding     = ImVec2( 10, 10 );
	style.FramePadding      = ImVec2( 6, 4 );
	style.ItemSpacing       = ImVec2( 8, 6 );
	style.ItemInnerSpacing  = ImVec2( 6, 4 );
	style.ScrollbarSize     = 12.0f;
	style.GrabMinSize       = 10.0f;
	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.PopupBorderSize   = 1.0f;
	style.FrameBorderSize   = 1.0f;
	style.TabBorderSize     = 1.0f;
	style.WindowRounding    = 4.0f;
	style.ChildRounding     = 4.0f;
	style.FrameRounding     = 3.0f;
	style.PopupRounding     = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding      = 2.0f;
	style.TabRounding       = 3.0f;
}

void GUI::DrawImGui( void )
{
	if( !bMouse )
		return;

	ImGui::SetNextWindowSize( ImVec2( 700, 560 ), ImGuiCond_FirstUseEver );
	if( ImGui::Begin( "Insomnia Hook | Counter-Strike: Source v34", nullptr, ImGuiWindowFlags_NoCollapse ) )
	{
		if( ImGui::BeginTabBar( "MainTabBar", ImGuiTabBarFlags_None ) )
		{
			if( ImGui::BeginTabItem( "Legit Bot" ) )
			{
				RenderLegitTab( );
				ImGui::EndTabItem( );
			}

			if( ImGui::BeginTabItem( "Rage Bot" ) )
			{
				RenderRageTab( );
				ImGui::EndTabItem( );
			}

			if( ImGui::BeginTabItem( "Visuals" ) )
			{
				RenderVisualsTab( );
				ImGui::EndTabItem( );
			}

			if( ImGui::BeginTabItem( "Miscellaneous" ) )
			{
				RenderMiscTab( );
				ImGui::EndTabItem( );
			}

			if( ImGui::BeginTabItem( "Player List" ) )
			{
				RenderPlayerListTab( );
				ImGui::EndTabItem( );
			}

			if( ImGui::BeginTabItem( "Config" ) )
			{
				RenderConfigsTab( );
				ImGui::EndTabItem( );
			}

			ImGui::EndTabBar( );
		}
	}
	ImGui::End( );
}

void GUI::RenderAimbotTab( void )
{
	RenderRageTab( );
}

void GUI::RenderLegitTab( void )
{
	// Legit = humanized: FOV, Smooth, Hitbox, Triggerbot, SnapLimiter
	auto& profile = g_CVars.Legit;
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	ImGui::BeginChild( "Legit_Left", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "LEGIT MODE" );
		ImGui::Separator( );

		if( g_CVars.AimbotProfile == 0 )
			ImGui::TextColored( ImVec4( 0.55f, 0.85f, 0.15f, 1.0f ), "Active: Legit" );
		else
			ImGui::TextDisabled( "Active: Rage (switch via key or Rage tab)" );

		const char* profileKeyNames[] = { "Off", "Insert", "Home", "End", "Page Up", "Page Down" };
		const int profileKeyVK[] = { 0, VK_INSERT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT };
		int nProfileKeyIdx = 0;
		for( int k = 1; k < IM_ARRAYSIZE( profileKeyVK ); k++ )
		{
			if( profileKeyVK[ k ] == g_CVars.AimbotProfileKey ) { nProfileKeyIdx = k; break; }
		}
		if( ImGui::Combo( "Switch Key", &nProfileKeyIdx, profileKeyNames, IM_ARRAYSIZE( profileKeyNames ) ) )
			g_CVars.AimbotProfileKey = profileKeyVK[ nProfileKeyIdx ];

		if( ImGui::Button( "Use Legit Profile", ImVec2( -1, 0 ) ) )
			g_Stuff.SwitchAimbotProfile( 0 );

		ImGui::Spacing( );
		ImGui::TextDisabled( "AIMBOT MAIN (HUMANIZED)" );
		ImGui::Separator( );
		if( ImGui::Checkbox( "Active", &profile.Active ) )
		{
			if( profile.Active )
			{
				// if legit enabled, disable rage
				g_CVars.Rage.Active = false;
				g_CVars.AimbotProfile = 0;
			}
		}
		ImGui::Checkbox( "Friendly Fire", &profile.FriendlyFire );
		ImGui::Checkbox( "Vis Only (no wall)", &profile.VisOnly );
		ImGui::Checkbox( "On Key", &profile.OnKey );

		ImGui::Spacing( );
		ImGui::TextDisabled( "TARGETING (LEGIT)" );
		ImGui::Separator( );

		const char* aimKeyNames[] = { "Auto", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5" };
		ImGui::Combo( "Aim Key", &profile.Key, aimKeyNames, IM_ARRAYSIZE( aimKeyNames ) );

		const char* hitboxNames[] = { "Head", "Neck", "Chest", "Stomach" };
		int currentHitboxIdx = GetHitboxIndex( profile.Hitbox );
		if( ImGui::Combo( "Priority Hitbox", &currentHitboxIdx, hitboxNames, IM_ARRAYSIZE( hitboxNames ) ) )
			profile.Hitbox = HitboxFromIndex( currentHitboxIdx );

		ImGui::TextDisabled( "HITBOX GROUPS (LEGIT)" );
		if( ImGui::Checkbox( "Head (12)", &profile.HitboxGroup[0] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Neck (11)", &profile.HitboxGroup[1] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Chest (9,10,5)", &profile.HitboxGroup[2] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Stomach (0,1)", &profile.HitboxGroup[3] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Arms (13,14,16,17)", &profile.HitboxGroup[4] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Legs (2,3,4,15,6,7,8,18)", &profile.HitboxGroup[5] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Button( "Reset to Head+Chest" ) ) { for(int i=0;i<6;i++) profile.HitboxGroup[i]=false; profile.HitboxGroup[0]=true; profile.HitboxGroup[2]=true; profile.HitboxGroupsMask=(1<<0)|(1<<2); }

		ImGui::Spacing( );
		ImGui::TextDisabled( "GROUP PRIORITY (LEGIT)" );
		const char* groupNames[] = { "Head", "Neck", "Chest", "Stomach", "Arms", "Legs", "None (use order)" };
		int prioIdx = profile.HitboxPriorityGroup;
		if( prioIdx < -1 || prioIdx >= 6 ) prioIdx = 6;
		int comboPrio = (prioIdx==-1)?6:prioIdx;
		if( ImGui::Combo( "Primary Group", &comboPrio, groupNames, IM_ARRAYSIZE(groupNames) ) )
		{
			profile.HitboxPriorityGroup = (comboPrio==6)?-1:comboPrio;
		}
		ImGui::TextDisabled( "Custom Order:" );
		for( int orderPos=0; orderPos<6; orderPos++ )
		{
			int gId = profile.HitboxGroupOrder[orderPos];
			if( gId<0||gId>=6 ) gId=orderPos;
			ImGui::PushID( orderPos );
			ImGui::Text( "%d. %s", orderPos+1, groupNames[gId] );
			ImGui::SameLine( );
			if( ImGui::SmallButton( "^" ) && orderPos>0 )
			{
				int tmp = profile.HitboxGroupOrder[orderPos-1];
				profile.HitboxGroupOrder[orderPos-1]=profile.HitboxGroupOrder[orderPos];
				profile.HitboxGroupOrder[orderPos]=tmp;
			}
			ImGui::SameLine( );
			if( ImGui::SmallButton( "v" ) && orderPos<5 )
			{
				int tmp = profile.HitboxGroupOrder[orderPos+1];
				profile.HitboxGroupOrder[orderPos+1]=profile.HitboxGroupOrder[orderPos];
				profile.HitboxGroupOrder[orderPos]=tmp;
			}
			ImGui::PopID( );
		}
		if( ImGui::Button( "Reset Order" ) ) { for(int i=0;i<6;i++) profile.HitboxGroupOrder[i]=i; }

		ImGui::SliderFloat( "Point Scale", &profile.PointScale, 0.0f, 1.0f, "%.2f" );

		const char* heightModeNames[] = { "Auto", "Origin", "Center", "Center Fixed", "Highest" };
		ImGui::Combo( "Height Mode", &profile.HitboxMode, heightModeNames, IM_ARRAYSIZE( heightModeNames ) );

		if( ImGui::SliderFloat( "FOV", &profile.FOV, 1.0f, 180.0f, "%.0f" ) )
		{
			if( profile.FOV > 180.0f ) profile.FOV = 180.0f;
			if( profile.FOV < 1.0f ) profile.FOV = 1.0f;
		}
		ImGui::SliderFloat( "Smooth", &profile.Smooth, 1.0f, 20.0f, "%.1f" );
		ImGui::SliderFloat( "Reaction ms", &profile.ReactionTime, 0.f, 300.f, "%.0f ms" );
		ImGui::SliderFloat( "AutoDelay ms", &profile.AutoDelayTime, 0.f, 300.f, "%.0f ms" );
		ImGui::Checkbox( "Auto Delay", &profile.AutoDelay );
		ImGui::Checkbox( "Humanize", &profile.Humanize );
		ImGui::SliderFloat( "Humanize Rand", &profile.HumanizeRandom, 0.f, 1.f, "%.2f" );

		ImGui::Spacing( );
		ImGui::TextDisabled( "RCS (RECOIL CONTROL)" );
		ImGui::Separator( );
		ImGui::Checkbox( "RCS Active", &profile.RCS );
		const char* rcsModeNames[] = { "Always", "While Shooting" };
		ImGui::Combo( "RCS Mode", &profile.RCSMode, rcsModeNames, IM_ARRAYSIZE( rcsModeNames ) );
		ImGui::SliderFloat( "RCS X", &profile.RCSAmountX, 0.f, 2.f, "%.2f" );
		ImGui::SliderFloat( "RCS Y", &profile.RCSAmountY, 0.f, 2.f, "%.2f" );
		ImGui::SliderFloat( "RCS Scale", &profile.RCSScale, 0.f, 2.f, "%.2f" );

		ImGui::Spacing( );
		ImGui::TextDisabled( "SNAP LIMITER" );
		ImGui::Separator( );
		ImGui::Checkbox( "Snap Limiter Active", &profile.SnapLimiter );
		ImGui::SliderInt( "Angle Limit", &profile.AngleLimit, 0, 180 );
		ImGui::SliderFloat( "Angle Limit Tens", &profile.AngleLimitTens, 0.0f, 1.0f, "%.2f" );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "Legit_Right", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "TRIGGERBOT (LEGIT)" );
		ImGui::Separator( );
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

		ImGui::Spacing( );
		ImGui::TextDisabled( "HUMANIZED LEGIT" );
		ImGui::Separator( );
		ImGui::TextWrapped( "Max humanized legit bot: smooth curves, random, RCS, reaction delay, vis only, auto delay." );
		ImGui::BulletText( "FOV 1-25 = legit, 25-45 = semi" );
		ImGui::BulletText( "Smooth 8-15 = humanized glide" );
		ImGui::BulletText( "RCS compensates recoil gradually" );
		ImGui::BulletText( "Humanize adds random to smooth/RCS" );
		ImGui::BulletText( "Reaction ms = delay before aiming new target" );
		ImGui::BulletText( "AutoDelay = delay before shooting" );
		ImGui::BulletText( "VisOnly = no autowall, legit only" );
		ImGui::BulletText( "Snap Limiter prevents obvious snaps" );
	}
	ImGui::EndChild( );

	if( g_CVars.AimbotProfile == 0 )
		g_Stuff.LoadAimbotSettings( profile );
}

void GUI::RenderRageTab( void )
{
	// Rage = automated: AutoShoot, Silent, MultiSpot, HitScan, AutoWall, Resolver, PosAdjustment, MinDamage
	auto& profile = g_CVars.Rage;
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	ImGui::BeginChild( "Rage_Left", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "RAGE MODE" );
		ImGui::Separator( );

		if( g_CVars.AimbotProfile == 1 )
			ImGui::TextColored( ImVec4( 0.55f, 0.85f, 0.15f, 1.0f ), "Active: Rage" );
		else
			ImGui::TextDisabled( "Active: Legit (switch via key or Legit tab)" );

		const char* profileKeyNames[] = { "Off", "Insert", "Home", "End", "Page Up", "Page Down" };
		const int profileKeyVK[] = { 0, VK_INSERT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT };
		int nProfileKeyIdx = 0;
		for( int k = 1; k < IM_ARRAYSIZE( profileKeyVK ); k++ )
		{
			if( profileKeyVK[ k ] == g_CVars.AimbotProfileKey ) { nProfileKeyIdx = k; break; }
		}
		if( ImGui::Combo( "Switch Key", &nProfileKeyIdx, profileKeyNames, IM_ARRAYSIZE( profileKeyNames ) ) )
			g_CVars.AimbotProfileKey = profileKeyVK[ nProfileKeyIdx ];

		if( ImGui::Button( "Use Rage Profile", ImVec2( -1, 0 ) ) )
			g_Stuff.SwitchAimbotProfile( 1 );

		ImGui::Spacing( );
		ImGui::TextDisabled( "AIMBOT MAIN (RAGE)" );
		ImGui::Separator( );
		if( ImGui::Checkbox( "Active", &profile.Active ) )
		{
			if( profile.Active )
			{
				// if rage enabled, disable legit
				g_CVars.Legit.Active = false;
				g_CVars.AimbotProfile = 1;
			}
		}
		ImGui::Checkbox( "Auto Shoot", &profile.AutoShoot );
		ImGui::Checkbox( "Silent Aim", &profile.Silent );
		ImGui::Checkbox( "Perfect Silent", &profile.PerfectSilent );
		ImGui::Checkbox( "Multi Spot", &profile.MultiSpot );
		ImGui::Checkbox( "Body AWP", &profile.BodyAWP );
		if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "If ON, AWP forces body (old). If OFF (recommended), head is priority with lower MinDamage for AWP. Fixed head shooting." );
		ImGui::Checkbox( "Hit Scan", &profile.HitScan );
		ImGui::Checkbox( "Perfect Auto Wall", &profile.AutoWall );
		ImGui::Checkbox( "Friendly Fire", &profile.FriendlyFire );

		ImGui::Spacing( );
		ImGui::TextDisabled( "TARGETING (RAGE)" );
		ImGui::Separator( );

		const char* targetSelectionNames[] = { "Distance", "Health", "Next Shot", "Random" };
		ImGui::Combo( "Target Selection", &profile.TargetSelection, targetSelectionNames, IM_ARRAYSIZE( targetSelectionNames ) );

		ImGui::SliderInt( "Min Damage", &profile.MinDamage, 0, 100 );

		const char* posAdjustmentNames[] = { "Off", "On", "On + History" };
		ImGui::Combo( "Pos Adjustment", &profile.Interpolation.LagPrediction, posAdjustmentNames, IM_ARRAYSIZE( posAdjustmentNames ) );

		ImGui::Checkbox( "Disable Interpolation (from sega)", &profile.Interpolation.Disable );
		if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "From Segregation - disables interpolation for enemies (like sega Interpolate.hpp return 1). Better hitreg, less lag, enemy positions more accurate." );
		ImGui::Checkbox( "Lethal Body Aim", &profile.Interpolation.LethalBody );
		if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "If body shot is lethal (health <= damage), aim body for higher hit chance. If low health <=50, prioritize body." );

		ImGui::Spacing( );
		ImGui::TextDisabled( "HITBOX GROUPS (RAGE)" );
		ImGui::TextWrapped( "Select groups to shoot at - aimbot scans enabled groups by priority" );
		if( ImGui::Checkbox( "Head", &profile.HitboxGroup[0] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Neck", &profile.HitboxGroup[1] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Chest", &profile.HitboxGroup[2] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Stomach", &profile.HitboxGroup[3] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Arms", &profile.HitboxGroup[4] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Checkbox( "Legs", &profile.HitboxGroup[5] ) ) { profile.HitboxGroupsMask = 0; for(int i=0;i<6;i++) if(profile.HitboxGroup[i]) profile.HitboxGroupsMask|=(1<<i); }
		if( ImGui::Button( "All Groups" ) ) { for(int i=0;i<6;i++) profile.HitboxGroup[i]=true; profile.HitboxGroupsMask=63; }
		ImGui::SameLine( );
		if( ImGui::Button( "Head Only" ) ) { for(int i=0;i<6;i++) profile.HitboxGroup[i]=false; profile.HitboxGroup[0]=true; profile.HitboxGroupsMask=1; }
		ImGui::SameLine( );
		if( ImGui::Button( "Body Only" ) ) { for(int i=0;i<6;i++) profile.HitboxGroup[i]=false; profile.HitboxGroup[2]=true; profile.HitboxGroup[3]=true; profile.HitboxGroupsMask=(1<<2)|(1<<3); }

		ImGui::Spacing( );
		ImGui::TextDisabled( "GROUP PRIORITY (RAGE)" );
		const char* groupNamesRage[] = { "Head", "Neck", "Chest", "Stomach", "Arms", "Legs", "None (use order)" };
		int prioIdxRage = profile.HitboxPriorityGroup;
		if( prioIdxRage < -1 || prioIdxRage >= 6 ) prioIdxRage = 6;
		int comboPrioRage = (prioIdxRage==-1)?6:prioIdxRage;
		if( ImGui::Combo( "Primary Group", &comboPrioRage, groupNamesRage, IM_ARRAYSIZE(groupNamesRage) ) )
		{
			profile.HitboxPriorityGroup = (comboPrioRage==6)?-1:comboPrioRage;
		}
		ImGui::TextDisabled( "Custom Order:" );
		for( int orderPos=0; orderPos<6; orderPos++ )
		{
			int gId = profile.HitboxGroupOrder[orderPos];
			if( gId<0||gId>=6 ) gId=orderPos;
			ImGui::PushID( orderPos+100 );
			ImGui::Text( "%d. %s", orderPos+1, groupNamesRage[gId] );
			ImGui::SameLine( );
			if( ImGui::SmallButton( "^" ) && orderPos>0 )
			{
				int tmp = profile.HitboxGroupOrder[orderPos-1];
				profile.HitboxGroupOrder[orderPos-1]=profile.HitboxGroupOrder[orderPos];
				profile.HitboxGroupOrder[orderPos]=tmp;
			}
			ImGui::SameLine( );
			if( ImGui::SmallButton( "v" ) && orderPos<5 )
			{
				int tmp = profile.HitboxGroupOrder[orderPos+1];
				profile.HitboxGroupOrder[orderPos+1]=profile.HitboxGroupOrder[orderPos];
				profile.HitboxGroupOrder[orderPos]=tmp;
			}
			ImGui::PopID( );
		}
		if( ImGui::Button( "Reset Order" ) ) { for(int i=0;i<6;i++) profile.HitboxGroupOrder[i]=i; }
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "Rage_Right", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "ACCURACY (RAGE)" );
		ImGui::Separator( );
		ImGui::Checkbox( "Remove Recoil / Spread", &g_CVars.Accuracy.PerfectAccuracy );
		ImGui::Checkbox( "Force Seed", &g_CVars.Accuracy.ForceSeed );

		const char* spreadModeNames[] = { "NULL", "Classic", "Iterative", "Rotation" };
		ImGui::Combo( "NoSpread Mode", &g_CVars.Accuracy.NoSpreadMode, spreadModeNames, IM_ARRAYSIZE( spreadModeNames ) );

		ImGui::Spacing( );
		ImGui::TextDisabled( "RESOLVER (RAGE) - IMPROVED" );
		ImGui::Separator( );
		ImGui::Checkbox( "Resolver Active", &profile.Resolver.Active );

		const char* resolverModeNames[] = { "Everyone", "Selected" };
		ImGui::Combo( "Resolver Target", &profile.Resolver.Mode, resolverModeNames, IM_ARRAYSIZE( resolverModeNames ) );

		const char* resolverTypeNames[] = { 
			"Off", 
			"Bruteforce 4-way", 
			"Bruteforce 8-way (miss)", 
			"Velocity", 
			"LBY / Last Moving", 
			"Smart v2 (vel+jitter+spin+bf)", 
			"Jitter", 
			"180 Backwards", 
			"90 Left/Right" 
		};
		ImGui::Combo( "Resolver Type", &profile.Resolver.Type, resolverTypeNames, IM_ARRAYSIZE( resolverTypeNames ) );

		ImGui::Checkbox( "Smart Resolver", &profile.Resolver.Smart );
		ImGui::TextWrapped( "Smart: uses yawDelta + velocity + spin/jitter detection + pitch resolver. Bruteforce advances on missed shots." );

		ImGui::Spacing( );
		ImGui::TextDisabled( "RESOLVER INFO" );
		ImGui::Separator( );
		if( ImGui::CollapsingHeader( "How it works" ) )
		{
			ImGui::BulletText( "Velocity: if moving, real yaw = velocity dir" );
			ImGui::BulletText( "LBY: uses last moving yaw when standing" );
			ImGui::BulletText( "Bruteforce 8-way cycles 0,90,180,-90,45,-45,135,-135 on miss" );
			ImGui::BulletText( "Smart v2: vel + jitter detection (flips side) + spin counter + LBY + bf" );
			ImGui::BulletText( "Jitter: alternates 90/-90 to catch jitter AA" );
			ImGui::BulletText( "Pitch resolver: 89/-89 -> 0 or last valid pitch" );
		}

		ImGui::Spacing( );
		ImGui::TextDisabled( "RAGE INFO" );
		ImGui::Separator( );
		ImGui::TextWrapped( "Rage Bot: AutoShoot/Silent/MultiSpot/HitScan/AutoWall/Resolver/PosAdjustment/MinDamage only. No legit features (FOV/Smooth/Triggerbot/SnapLimiter)." );
		ImGui::BulletText( "FOV is fixed 180, Smooth 1 for rage" );
		ImGui::BulletText( "Use Legit tab for humanized settings" );
		ImGui::BulletText( "Hitbox Groups + Priority: select groups and order" );
	}
	ImGui::EndChild( );

	if( g_CVars.AimbotProfile == 1 )
		g_Stuff.LoadAimbotSettings( profile );
}

void GUI::RenderAimbotCommon( AimbotSettings& profile, bool bIsLegit )
{
	// kept for compatibility, redirects to the proper tab
	if( bIsLegit ) RenderLegitTab( );
	else RenderRageTab( );
}

void GUI::RenderVisualsTab( void )
{
	float halfWidth = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x ) * 0.5f;

	ImGui::BeginChild( "Visuals_Left", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "ESP (SURFACE RENDER)" );
		ImGui::Separator( );
		ImGui::Checkbox( "Bounding Box", &g_CVars.Visuals.ESP.Box );
		ImGui::Checkbox( "Player Name", &g_CVars.Visuals.ESP.Name );
		ImGui::Checkbox( "Health Bar / Text", &g_CVars.Visuals.ESP.Health );
		ImGui::Checkbox( "Weapon Name", &g_CVars.Visuals.ESP.Weapon );
		ImGui::Checkbox( "Skeleton / Bone", &g_CVars.Visuals.ESP.Bone );
		ImGui::Checkbox( "Aim Spot", &g_CVars.Visuals.ESP.AimSpot );
		ImGui::Checkbox( "Hitmarker", &g_CVars.Visuals.ESP.Hit );
		ImGui::Checkbox( "Ground ESP", &g_CVars.Visuals.ESP.Ground );
		ImGui::Checkbox( "Enemy Only", &g_CVars.Visuals.ESP.EnemyOnly );
		ImGui::Checkbox( "Dormant ESP (gray)", &g_CVars.Visuals.ESP.Dormant );
		if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "Show dormant players gray, without dormant text - last known position, 10 sec timeout" );
		ImGui::Checkbox( "Out of FOV Arrows", &g_CVars.Visuals.ESP.OutOfFOV );
		if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "Show arrows at screen edge for enemies out of FOV" );

		ImGui::Spacing( );
		ImGui::TextDisabled( "CHAMS & MODELS" );
		ImGui::Separator( );
		ImGui::Checkbox( "Player Chams", &g_CVars.Visuals.Chams.Active );
		ImGui::Checkbox( "Weapon Chams", &g_CVars.Visuals.Chams.Weapons );
		ImGui::Checkbox( "Draw Shadows", &g_CVars.Visuals.Chams.Shadows );
		ImGui::Checkbox( "Model Outline", &g_CVars.Visuals.Chams.Outline );
		ImGui::Checkbox( "Hands Outline", &g_CVars.Visuals.Chams.HandsOutline );
		ImGui::Checkbox( "Chams Enemy Only", &g_CVars.Visuals.Chams.EnemyOnly );
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "Visuals_Right", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "WORLD & SCREEN" );
		ImGui::Separator( );
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
		ImGui::TextDisabled( "CUSTOM COLORS" );
		ImGui::Separator( );

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

	ImGui::BeginChild( "Misc_Left", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "ANTI-AIM (HVH)" );
		ImGui::Separator( );
		ImGui::Checkbox( "Anti-Aim Active", &g_CVars.Miscellaneous.AntiAim.Active );

		const char* pitchNames[] = { "Off", "Normal", "Inverse Normal", "Safe", "Fake Down", "Down", "Up", "Lag Down", "Lag Up" };
		ImGui::Combo( "Pitch", &g_CVars.Miscellaneous.AntiAim.Pitch, pitchNames, IM_ARRAYSIZE( pitchNames ) );

		const char* yawNames[] = { "Forwards", "Backwards", "Sideways", "Jitter", "Static", "Static Reversed", "Lisp", "Custom" };
		ImGui::Combo( "Yaw", &g_CVars.Miscellaneous.AntiAim.Yaw, yawNames, IM_ARRAYSIZE( yawNames ) );

		std::vector< const char* > yawVariations;
		if( g_CVars.Miscellaneous.AntiAim.Yaw == 0 )
		{
			yawVariations = { "Off", "Fake Side 1", "Fake Side 2", "Backwards" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 3 )
		{
			yawVariations = { "90 / -90", "0 / 180", "45 / -45", "135 / -135" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 6 )
		{
			yawVariations = { "Spin Slow 90/s", "Spin Fast 360/s", "90 / -90 Jitter", "Random Spin" };
		}
		else if( g_CVars.Miscellaneous.AntiAim.Yaw == 7 )
		{
			yawVariations = { "Add Real", "World Locked" };
		}
		else
		{
			yawVariations = { "Normal", "Fake Side 1", "Fake Side 2", "Fake Forwards" };
		}

		if( g_CVars.Miscellaneous.AntiAim.Variation >= ( int )yawVariations.size( ) )
			g_CVars.Miscellaneous.AntiAim.Variation = 0;

		ImGui::Combo( "Yaw Mode", &g_CVars.Miscellaneous.AntiAim.Variation, yawVariations.data( ), ( int )yawVariations.size( ) );

		ImGui::SliderFloat( "Custom Real Yaw", &g_CVars.Miscellaneous.AntiAim.RealValue, 0.0f, 360.0f, "%.1f deg" );
		ImGui::SliderFloat( "Custom Fake Yaw", &g_CVars.Miscellaneous.AntiAim.FakeValue, 0.0f, 360.0f, "%.1f deg" );

		ImGui::Checkbox( "Relative Yaw", &g_CVars.Miscellaneous.AntiAim.RelativeYaw );
		ImGui::Checkbox( "InAttack Pitch", &g_CVars.Miscellaneous.AntiAim.Static );
		ImGui::Checkbox( "Wall Detection", &g_CVars.Miscellaneous.AntiAim.WallDetection );

		const char* wallDtcModes[] = { "Normal", "Fake", "Fake Out", "Jitter" };
		ImGui::Combo( "Wall DTC Mode", &g_CVars.Miscellaneous.AntiAim.WallDetectionMode, wallDtcModes, IM_ARRAYSIZE( wallDtcModes ) );

		ImGui::Checkbox( "At Targets", &g_CVars.Miscellaneous.AntiAim.AtTargets );
		ImGui::Checkbox( "Duck In Air", &g_CVars.Miscellaneous.AntiAim.DuckInAir );
		ImGui::Checkbox( "Enemy Check", &g_CVars.Miscellaneous.AntiAim.TurnOff );

		ImGui::Spacing( );
		if( ImGui::CollapsingHeader( "How anti-aim works (improved old)" ) )
		{
			ImGui::TextWrapped( "Старые режимы улучшены максимально, без добавления новых. В CSS v34 нет LBY как в CS:GO (LBY = LowerBodyYaw, обновляется при движении). Поэтому LBY Breaker убран - в CSS он не нужен. Вместо lisp (697049) который не работает в CSS, теперь рабочие 89/-89 с десинком." );
			ImGui::BulletText( "Pitch: Off, Normal=89 down (прячет голову, было 180), Inverse=-89 up (было -180), Safe=70, FakeDown=real 0 fake 89 (десинк, было -179.99), Down=89 (было lisp 697049 нерабочий), Up=-89 (было lisp 696871), LagDown/LagUp=real 0 fake 89 и наоборот с чоком 14 тиков" );
			ImGui::BulletText( "Yaw: Forwards/Backwards/Sideways теперь с десинком real vs fake (bSendPacket), Jitter 90/-90 и 0/180 и 45/-45, Static world locked, Lisp заменен на Spin 90/s и 360/s (реально работает в CSS), Custom RealValue/FakeValue" );
			ImGui::BulletText( "Защита головы: Pitch 89 down прячет голову, Fake Pitch десинк, FakeLag чок 14 тиков max (враг видит фейк), Wall DTC прячет за стеной, AtTargets смотрит на врага" );
		}
	}
	ImGui::EndChild( );

	ImGui::SameLine( );

	ImGui::BeginChild( "Misc_Right", ImVec2( halfWidth, 0 ), true );
	{
		ImGui::TextDisabled( "FAKE LAG" );
		ImGui::Separator( );
		ImGui::Checkbox( "Fake Lag Active", &g_CVars.Miscellaneous.Fakelag.Active );
		ImGui::Checkbox( "Fake Lag In Attack", &g_CVars.Miscellaneous.Fakelag.InAttack );
		ImGui::Checkbox( "Fake Lag Air Only", &g_CVars.Miscellaneous.Fakelag.AirOnly );
		ImGui::SliderInt( "Choke Ticks", &g_CVars.Miscellaneous.Fakelag.Value, 0, 14 );

		const char* fakelagModes[] = { "Factor", "Switch", "Adaptive" };
		ImGui::Combo( "Fake Lag Mode", &g_CVars.Miscellaneous.Fakelag.Mode, fakelagModes, IM_ARRAYSIZE( fakelagModes ) );

		ImGui::Spacing( );
		ImGui::TextDisabled( "ANTI SMAC (GLOBAL)" );
		ImGui::Separator( );
		ImGui::Checkbox( "Anti SMAC Active", &g_CVars.Miscellaneous.AntiSMAC );
		const char* antiSMACModeNames[] = { "Clamp Only", "Hide AA", "Full (Clamp+Limit Snap)" };
		ImGui::Combo( "AntiSMAC Mode", &g_CVars.Miscellaneous.AntiSMACMode, antiSMACModeNames, IM_ARRAYSIZE( antiSMACModeNames ) );
		ImGui::TextWrapped( "Works for both Legit and Rage. Clamps pitch/yaw, hides AA, limits snap to 35 deg/tick." );

		ImGui::Spacing( );
		ImGui::TextDisabled( "MOVEMENT & EXPLOITS" );
		ImGui::Separator( );
		ImGui::Checkbox( "Bunny Hop", &g_CVars.Miscellaneous.BunnyHop );
		ImGui::Checkbox( "Auto Strafe", &g_CVars.Miscellaneous.AutoStrafe );
		ImGui::Checkbox( "Circle Strafe (hold V)", &g_CVars.Miscellaneous.CircleStrafe );
		ImGui::Checkbox( "Air Stuck (press F)", &g_CVars.Miscellaneous.AirStuck );
		ImGui::Checkbox( "Auto Knife", &g_CVars.Miscellaneous.AutoKnife );
		ImGui::Checkbox( "Speedhack (hold E)", &g_CVars.Miscellaneous.Speedhack );
		ImGui::SliderInt( "Speedhack Factor", &g_CVars.Miscellaneous.SpeedhackValue, 0, 13 );

		ImGui::Spacing( );
		ImGui::TextDisabled( "OTHER" );
		ImGui::Separator( );
		ImGui::Checkbox( "Round Say", &g_CVars.Miscellaneous.RoundSay );
		ImGui::Checkbox( "sv_cheats Bypass", &g_CVars.Miscellaneous.CheatsBypass );
		ImGui::Checkbox( "Third Person View", &g_CVars.Miscellaneous.ThirdPerson );
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

		for( int i = 1; i <= g_pGlobals->maxClients; i++ )
		{
			if( i == g_pEngineClient->GetLocalPlayer( ) )
				continue;

			BasePlayer* pEnt = ( BasePlayer* )g_pClientEntityList->GetClientEntity( i );
			if( !pEnt || !g_pEngineClient->GetPlayerInfo( i, &pInfo ) )
				continue;

			ImGui::TableNextRow( );
			ImGui::PushID( i );

			ImGui::TableSetColumnIndex( 0 );
			ImGui::Text( "%d", i );

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( "%s", pInfo.name );

			ImGui::TableSetColumnIndex( 2 );
			bool isFriend = g_CVars.PlayerList.Friend[ i ];
			if( ImGui::Checkbox( "##Friend", &isFriend ) )
			{
				g_CVars.PlayerList.Friend[ i ] = isFriend;
			}

			ImGui::TableSetColumnIndex( 3 );
			int curPitch = g_CVars.PlayerList.Pitch[ i ];
			if( curPitch < 0 || curPitch > 3 ) curPitch = 0;
			ImGui::SetNextItemWidth( -1 );
			if( ImGui::Combo( "##Pitch", &curPitch, pitchNames, IM_ARRAYSIZE( pitchNames ) ) )
			{
				g_CVars.PlayerList.Pitch[ i ] = curPitch;
			}

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
		ImGui::TextDisabled( "CONFIGURATION MANAGER" );
		ImGui::Separator( );

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
