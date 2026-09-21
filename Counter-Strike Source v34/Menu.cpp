// <- error here
//
//
#include "Menu.hpp"
#include "Source.hpp"
#include "Config.hpp"
#include "Player.hpp"
#include "ImGui.hpp"
#include "ImGuiDX9.hpp"
#include <ctime>
int iTab;
float menusize = 50.0f;
bool open;
float mainmenu1 = 650.0f;
float mainmenu2 = 560.0f;
float backsize1 = mainmenu1+10;
float backsize2 = mainmenu2+10;
IDirect3DTexture9 *tImage = nullptr;

ImFont*  bad = nullptr; //глобально
ImFont*  def = nullptr; //глобально
ImFont*  def1 = nullptr; //глобально
ImFont*	 und = nullptr;
#include "picture.hpp"
extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
namespace Feature
{
	Menu::Menu()
		:	m_bMouse( false ),
			m_iWeaponAimbot( 0 ),
			m_iWeaponTriggerbot( 0 ),
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
		bad = io.Fonts->AddFontFromFileTTF(XorStr("C:\\Windows\\Fonts\\badcache.ttf"), 22.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
		und = io.Fonts->AddFontFromFileTTF(XorStr("C:\\Windows\\Fonts\\toma.ttf"), 12.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
		def = io.Fonts->AddFontFromFileTTF(XorStr(u8"C:\\Windows\\Fonts\\tahoma.ttf"), 14.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
		def1 = io.Fonts->AddFontFromFileTTF(XorStr(u8"C:\\Windows\\Fonts\\tahoma.ttf"), 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
		if (tImage == nullptr)
		{
			D3DXCreateTextureFromFileInMemoryEx(pDevice, &pic, sizeof(pic), 96, 96, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tImage);
		}
		ImGuiStyle& Style = ImGui::GetStyle();

		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
		style.Colors[ImGuiCol_WindowBg] = ImColor(15, 15, 15, 255);
		style.Colors[ImGuiCol_ChildWindowBg] = ImColor(10, 10, 10, 255);
		style.Colors[ImGuiCol_Border] = ImColor(15, 15, 15, 255);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.09f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 0.88f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15f, 0.60f, 0.78f, 0.78f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.70f, 0.02f, 0.60f, 0.22f);
		style.Colors[ImGuiCol_CloseButton] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
		style.WindowRounding = 0.f;
		style.FramePadding = ImVec2(4, 0);
		style.WindowPadding = ImVec2(0, 0);
		style.ItemSpacing = ImVec2(0, 0);
		style.ScrollbarSize = 10.f;
		style.ScrollbarRounding = 0.f;
		style.GrabMinSize = 5.f;

		SetColors();

		return true;
	}
	void ButtonColor(int r, int g, int b)
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_Button] = ImColor(r, g, b);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(r, g, b);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(r, g, b);
}
	const char* ModeList[ ] =
	{
		"Off",
		"Auto",
		"On Press",
	};
	const char* colorlist[] =
	{
		"ESP T Not-Visible",
		"ESP T Visible",
		"ESP CT Visible",
		"ESP CT Not-Visible",
		"Chams T Visible",
		"Chams T Not-Visible",
		"Chams CT Visible",
		"Chams CT Not-Visible",
		"Crosshiar",
		"Chams Outline",
	};
	const char* ggg[] =
	{
		"Off",
		"call of duty",
		"skeet",
		"pew",
	};
	const char* Crashlist[] =
	{
		"Off",
		"Mass Disconnect",
		"Crash Server",
		"Custom Packets",
	};
	const char* Laglist[] =
	{
		"Off",
		"Classic",
		"Custom Classic",
		"Custom Reversed",
		"Custom Reversed + Classic",
	};
	const char* NoEnemyList[] =
	{
		"Everyone",
		"Enemy",
		"Friendly",
	};
	const char* AtTargetList[] =
	{
		"Everyone",
		"Enemy",
		"Friendly",
	};
	const char* backtracklist[] =
	{
		"Off",
		"BackTrack",
		"LagFix",
		"Both(LOW FPS)",
	};
	const char* SpotList[] =
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
		"Right Hand",
	};

	const char* SkeletonList[] =
	{
		"Off",
		"Normal",
		"BackTrack",
	};

	const char* TargetSelectionList[ ] =
	{
		"Fast",
		"Distance",
		"FOV",
	};

	const char* SmoothList[ ] =
	{
		"Off",
		"Step",
		"Linear",
	};

	const char* HitScanList[ ] =
	{
		"Off",
		"Normal",
		"Extra",
	};

	const char* SilentList[ ] =
	{
		"Off",
		"Normal",
		"Perfect",
	};

	const char* AccuracyList[ ] =
	{
		"Normal",
		"Perfect",
		"Seed",
	};

	const char* EspTargetList[ ] =
	{
		"Everyone",
		"Enemy",
		"Friendly",
	};
	const char* ChamsTargetList[] =
	{
		"Everyone",
		"Enemy",
		"Friendly",
	};
	const char* AimTargetList[] =
	{
		"Everyone",
		"Enemy",
		"Friendly",
	};
	const char* BoxList[ ] =
	{
		"Off",
		"Normal",
		"Corners",
	};

	const char* InfoTypeList[ ] =
	{
		"Off",
		"Text",
		"Bar",
	};

	const char* InfoAlignList[ ] =
	{
		"Left",
		"Right",
		"Top",
		"Bottom",
	};

	const char* ChamsModeList[ ] =
	{
		"Off",
		"Flat",
		"Shadow",
		"Shadow Flat",
		"Chipolino",
	};
	const char* PitchMoveList[] =
	{
		"Off",
		"Emotion",
		"FakeUp",
		"Custom",
		"Flip",
		"Switch",
	};
	
	const char* YawMoveList[] =
	{
		"Off",
		"Backward",//1
		"Legit",//2
		"Fake Sideway Left",//3
		"Fake Sideway Right",//4
		"Spin",//5
		"Custom Double Fake",//6
		"Custom Static Jitter",//7
		"Custom Jitter",//8
		"Custom Static",//9
		"Custom Fake",//10
		"Custom Static Fake",//11
		"Fake Spin",
		"Fake Spin 2",
		};
	const char* PitchStandList[ ] =
	{
		"Off",
		"Emotion",
		"FakeUp",
		"Custom",
		"Flip",
		"Switch",
	};
	const char* YawStandList[ ] =
	{
		"Off",
		"Backward",//1
		"Legit",//2
		"Fake Sideway Left",//3
		"Fake Sideway Right",//4
		"Spin",//5
		"Custom Double Fake",//6
		"Custom Static Jitter",//7
		"Custom Jitter",//8
		"Custom Static",//9
		"Custom Fake",//10
		"Custom Static Fake",//11
		"Fake Spin",
		"Fake Spin 2",
	};
	
	const char* NoSpreadList[ ] =
	{
		"Off",
		"Pitch/Yaw",
		"Perfect",
	};

	const char* AutoStrafeList[ ] =
	{
		"Off",
		"Mouse",
		"Auto",
	};

	const char* CrosshairList[ ] =
	{
		"Off",
		"Dot",
		"Cross",
		"Swastika",
		"Small Cross",
		"Default",
		"Aimware",
	};

	const char* CrosshairSniperList[ ] =
	{
		"Off",
		"Dot",
		"Cross",
		"Swastika",
		"Small Cross",
		"Default",
		"Aimware",
	};
	const char* RestrictionList[ ] =
	{
		"Off",
		"SMAC ULTR@",
		"KAC(disabled)",
	};

	std::string GetExtension( const std::string& target )
	{
		auto found = target.find_last_of( "." );

		std::string extension( "" );

		if( found != std::string::npos )
			extension = target.substr( found + 1, target.length() );

		return extension;
	}
	void pushhor()
	{
		ImGui::Spacing();
		ImGui::SameLine();
	}
	void pushver()
	{
		ImGui::Spacing();
	}
	void styled()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.f;
		style.FramePadding = ImVec2(4, 0);
		style.WindowPadding = ImVec2(0, 0);
		style.ItemSpacing = ImVec2(0, 0);
		style.ScrollbarSize = 10.f;
		style.ScrollbarRounding = 0.f;
		style.GrabMinSize = 5.f;
	}

	void Menu::OnPresentDevice()
	{

		if (m_bMouse != Shared::m_bMenu)
		{
			m_bMouse = Shared::m_bMenu;

			Source::m_pCvar->FindVar(XorStr("cl_mouseenable"))->m_nValue = (int)!m_bMouse;
		}

		if (!Shared::m_bMenu)
			return;

			ImGui_ImplDX9_NewFrame();
		ImGui::GetIO().MouseDrawCursor = true;
		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 pos;
		ImGuiWindowFlags Flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;


		ImGui::SetNextWindowSize(ImVec2(mainmenu1, mainmenu2));
		ImGui::Begin(XorStr("mainmenu"), NULL, ImVec2(mainmenu1, mainmenu2), 1.f, Flags);
		{
			pos = ImGui::GetWindowPos();

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnOffset(1, 135);

			ButtonColor(15, 15, 15);
			styled();
			ImGui::Button("##Fir521st", ImVec2(118, 20));
			ImGui::SameLine();
			ButtonColor(50, 50, 50);
			ImGui::Button("##Se512cond", ImVec2(1, 20));
			/*aim*/
			{
				if (Config::Misc->icons)
				{
					if (iTab == 0) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ra521geupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##f521gfgfg", ImVec2(1, 1));

					ButtonColor(15, 15, 15);
					ImGui::PushFont(bad);
					if (iTab == 0) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button("A", ImVec2(118, menusize))) iTab = 0;


					ImGui::SameLine();

					if (iTab != 0)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 0) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ragedownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 0) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ra521geupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##f521gfgfg", ImVec2(1, 1));

					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 0) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("AIMBOT"), ImVec2(118, menusize))) iTab = 0;


					ImGui::SameLine();

					if (iTab != 0)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 0) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ragedownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
			}

			/*trigger*/
			{
				if (Config::Misc->icons)
				{
					if (iTab == 1) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##lupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));

					ImGui::PopFont();
					ImGui::PushFont(und);
					ButtonColor(15, 15, 15);
					if (iTab == 1) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("^"), ImVec2(118, menusize))) iTab = 1;

					ImGui::SameLine();

					if (iTab != 1)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 1) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ldownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 1) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##lupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
					ImGui::PopFont();
					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 1) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("TRIGGERBOT"), ImVec2(118, menusize))) iTab = 1;

					ImGui::SameLine();

					if (iTab != 1)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 1) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##ldownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
			}

			/*visuals*/
			{
				if (Config::Misc->icons)
				{
					if (iTab == 2) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##vu152pline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fg121fgfg", ImVec2(1, 1));

					ImGui::PopFont();
					ImGui::PushFont(bad);
					ButtonColor(15, 15, 15);
					if (iTab == 2) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("D"), ImVec2(118, menusize))) iTab = 2;
					ImGui::SameLine();

					if (iTab != 2)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gf11gfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 2) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##vdo521wnline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##f215gfgfg", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 2) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##vu152pline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fg121fgfg", ImVec2(1, 1));
					ImGui::PopFont();
					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 2) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("VISUALS"), ImVec2(118, menusize))) iTab = 2;
					ImGui::SameLine();

					if (iTab != 2)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gf11gfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 2) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##vdo521wnline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##f215gfgfg", ImVec2(1, 1));
				}
			}

			/*misc*/
			{
				if (Config::Misc->icons)
				{
					if (iTab == 3) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##mupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));

					ButtonColor(15, 15, 15);
					if (iTab == 3) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("I"), ImVec2(118, menusize))) iTab = 3;
					ImGui::SameLine();

					if (iTab != 3)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 3) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##mdownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 3) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##mupline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
					ImGui::PopFont();
					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 3) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("MISC"), ImVec2(118, menusize))) iTab = 3;
					ImGui::SameLine();

					if (iTab != 3)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgfgfgfgf", ImVec2(1, menusize));

					if (iTab == 3) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##mdownline", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg", ImVec2(1, 1));
				}
			}

			/*hvh*/
			{
				if (Config::Misc->icons)
				{
					if (iTab == 4) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##sup12line", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgf512gfg", ImVec2(1, 1));

					ImGui::PopFont();
					ImGui::PushFont(und);
					ButtonColor(15, 15, 15);
					if (iTab == 4) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("@"), ImVec2(118, menusize))) iTab = 4;
					ImGui::SameLine();

					if (iTab != 4)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgf512gfgfgfgf421", ImVec2(1, menusize));

					if (iTab == 4) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##sdownline14", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg421", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 4) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##sup12line", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgf512gfg", ImVec2(1, 1));
					ImGui::PopFont();
					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 4) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("HVH"), ImVec2(118, menusize))) iTab = 4;
					ImGui::SameLine();

					if (iTab != 4)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgf512gfgfgfgf421", ImVec2(1, menusize));

					if (iTab == 4) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##sdownline14", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfg421", ImVec2(1, 1));
				}
			}
			{
				if (Config::Misc->icons)
				{
					if (iTab == 5) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##cupgas5125l1ne", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfgafgf1112g12212", ImVec2(1, 1));

					ImGui::PopFont();
					ImGui::PushFont(bad);
					ButtonColor(15, 15, 15);
					if (iTab == 5) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("G"), ImVec2(118, menusize))) iTab = 5;
					ImGui::SameLine();

					if (iTab != 5)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgf5521gfgfgfgfgfgfgfg55121f", ImVec2(1, menusize));

					if (iTab == 5) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##c12down12125linge", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfgfg12ac1fg61212fgfgfg", ImVec2(1, 1));
				}
				else
				{
					if (iTab == 5) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##cupgas5125l1ne", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfgafgf1112g12212", ImVec2(1, 1));
					ImGui::PopFont();
					ImGui::PushFont(def);
					ButtonColor(15, 15, 15);
					if (iTab == 5) style.Colors[ImGuiCol_Text] = ImColor(200, 200, 200); else style.Colors[ImGuiCol_Text] = ImColor(80, 80, 80);
					if (ImGui::Button(XorStr("SETTINGS"), ImVec2(118, menusize))) iTab = 5;
					ImGui::SameLine();

					if (iTab != 5)ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##gfgfgf5521gfgfgfgfgfgfgfg55121f", ImVec2(1, menusize));

					if (iTab == 5) ButtonColor(50, 50, 50); else ButtonColor(15, 15, 15);
					ImGui::Button("##c12down12125linge", ImVec2(118, 1));

					ImGui::SameLine();

					ButtonColor(50, 50, 50);
					ImGui::Button("##fgfgfgfg12ac1fg61212fgfgfg", ImVec2(1, 1));
				}
			}
			{
			}
			ImGui::PopFont();
			ImGui::PushFont(def1); ImGui::PushFont(def);
			ButtonColor(15, 15, 15);
			ImGui::Button("##upprtabs", ImVec2(118, 20));

			ImGui::SameLine();

			ButtonColor(50, 50, 50);
			ImGui::Button("##rageupline", ImVec2(1, 20));
			ImGui::TextDisabled(XorStr("WEED HACK PRIVATE"));
			ImGui::TextDisabled(XorStr("     REBORN	"));
			ImGui::TextDisabled(XorStr(""));
			ImGui::Spacing();
			ImGui::TextDisabled(XorStr("Hello, rraggerr"));
			ImGui::Spacing();
			ImGui::Text("", Color(0, 0, 0, 0));
			ImGui::TextDisabled(XorStr("BUILD: %s"), __DATE__);
			ImGui::Text("", Color(0, 0, 0, 0));
			ImGui::Image(tImage, ImVec2(96, 96));
			ImGui::NextColumn();
			style.WindowPadding = ImVec2(8, 8);
			style.ItemSpacing = ImVec2(4, 4);
			style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
			style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
			style.Colors[ImGuiCol_WindowBg] = ImColor(15, 15, 15, 255);
			style.Colors[ImGuiCol_ChildWindowBg] = ImColor(15, 15, 15, 255);
			style.Colors[ImGuiCol_Border] = ImColor(15, 15, 15, 255);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.09f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 0.88f);
			style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
			style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15f, 0.60f, 0.78f, 0.78f);
			style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.15f, 0.60f, 0.78f, 1.00f);
			style.Colors[ImGuiCol_Button] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.70f, 0.02f, 0.60f, 0.22f);
			style.Colors[ImGuiCol_CloseButton] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
			style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
			ImGui::BeginChild("main", ImVec2(backsize1,backsize2), Flags);
			{
				//do tabs here
				if (iTab == 0) // aimbot
				{
					//pushver();
					ImGui::BeginChild(XorStr(""), ImVec2(493.25, 533.25), true);
					{

					ImGui::Checkbox(XorStr("Weapon Config"), &Config::Main->AimbotWeaponConfig);
					if (Config::Main->AimbotWeaponConfig)
					{
						CSWeaponID i = Config::GetWeaponID(Config::WeaponList[m_iWeaponAimbot]);
							ImGui::PushItemWidth(150.0f);
							ImGui::Combo("", &m_iWeaponAimbot, Config::WeaponList, ARRAYSIZE(Config::WeaponList));
							ImGui::PopItemWidth();
						ImGui::Combo(XorStr("Mode"), &Config::Weapon[i]->Aimbot->Mode, ModeList, ARRAYSIZE(ModeList));
					//	ImGui::PopItemWidth();
						if (Config::Weapon[i]->Aimbot->Mode == 2) // On Press
						{
							
							ImGui::KeyButton(XorStr("Key"), &Config::Weapon[i]->Aimbot->Key);
							ImGui::PopItemWidth();
						}

						ImGui::Checkbox(XorStr("Auto Fire"), &Config::Weapon[i]->Aimbot->AutoFire);
						ImGui::SameLine();

						ImGui::Checkbox(XorStr("Auto Stop"), &Config::Weapon[i]->Aimbot->AutoStop);

						ImGui::Checkbox(XorStr("Auto Crouch"), &Config::Weapon[i]->Aimbot->AutoCrouch);
						ImGui::SameLine();

						ImGui::Checkbox(XorStr("Auto Reload"), &Config::Weapon[i]->Aimbot->AutoReload);

						ImGui::Checkbox(XorStr("Anti Spawn-Protection"), &Config::Weapon[i]->Aimbot->AntiSpawnProtection);

						ImGui::Checkbox(XorStr("No Switch"), &Config::Weapon[i]->Aimbot->NoSwitch);

						ImGui::Checkbox(XorStr("Resolver"), &Config::Weapon[i]->Aimbot->Resolver);
						if (Config::Main->Aimbot->Resolver)
						{
							ImGui::SliderInt(XorStr("Resolver Bullet"), &Config::Weapon[i]->Aimbot->ResvolerBullets, 1, 7);
							ImGui::SliderInt(XorStr("Resolver Change Delay"), &Config::Weapon[i]->Aimbot->ResvolerBulletsDelay, 2, 8);
							ImGui::SliderFloat(XorStr("Resolver Add Y"), &Config::Misc->ResolverAng, 0.0f, 180.0f);
						}

						ImGui::Separator();


						ImGui::Combo(XorStr("Spot"), &Config::Weapon[i]->Aimbot->Spot, SpotList, ARRAYSIZE(SpotList));
				//		ImGui::PopItemWidth();

						ImGui::Checkbox(XorStr("Randomize"), &Config::Weapon[i]->Aimbot->SpotRandomize);


						ImGui::Checkbox(XorStr("Height "), &Config::Weapon[i]->Aimbot->Height);
						if (Config::Weapon[i]->Aimbot->Height)
						{

							ImGui::SliderFloat(XorStr("Hegiht Scale"), &Config::Weapon[i]->Aimbot->HeightScale, -5.0f, 5.0f);
						}

						ImGui::Separator();

						
						ImGui::Combo(XorStr("Target Selection"), &Config::Weapon[i]->Aimbot->TargetSelection, TargetSelectionList, ARRAYSIZE(TargetSelectionList));
						if (Config::Weapon[i]->Aimbot->TargetSelection == 2)
						{
							ImGui::SliderFloat(XorStr("Filed Of View"), &Config::Weapon[i]->Aimbot->FieldOfView, 0.0f, 180.0f);
						}
						ImGui::Separator();

						ImGui::Combo(XorStr("Smooth"), &Config::Weapon[i]->Aimbot->Smooth, SmoothList, ARRAYSIZE(SmoothList));
						
						if (Config::Weapon[i]->Aimbot->Smooth == 1) // Step
						{

							ImGui::SliderFloat(XorStr("Vertical"), &Config::Weapon[i]->Aimbot->StepX, 0.0f, 100.0f);

							ImGui::SliderFloat(XorStr("Horizontal"), &Config::Weapon[i]->Aimbot->StepY, 0.0f, 100.0f);
						}
						else if (Config::Weapon[i]->Aimbot->Smooth == 2) // Linear
						{

							ImGui::SliderFloat(XorStr("Vertical"), &Config::Weapon[i]->Aimbot->SmoothX, 0.0f, 100.0f);

							ImGui::SliderFloat(XorStr("Horizontal"), &Config::Weapon[i]->Aimbot->SmoothY, 0.0f, 100.0f);
						}

						ImGui::Separator();

						ImGui::SliderInt(XorStr("Duration"), &Config::Weapon[i]->Aimbot->Duration, 0, 5000);

						ImGui::SliderInt(XorStr("Delay"), &Config::Weapon[i]->Aimbot->Delay, 0, 5000);

						ImGui::SliderInt(XorStr("Switch Delay"), &Config::Weapon[i]->Aimbot->SwitchDelay, 0, 5000);

						ImGui::Separator();


						ImGui::Checkbox(XorStr("RCS Active"), &Config::Weapon[i]->Aimbot->RCS);
						ImGui::SameLine();

						ImGui::Checkbox(XorStr("NoSpread Active"), &Config::Weapon[i]->Aimbot->NoSpreadActive);

						if (Config::Weapon[i]->Aimbot->RCS)
						{
							ImGui::SliderInt(XorStr("RCS Delay"), &Config::Weapon[i]->Aimbot->RCSDelay, 0, 10);

							ImGui::SliderInt(XorStr("RCS Amount X"), &Config::Weapon[i]->Aimbot->RCSAmountX, 0, 100, "%.0f%%");

							ImGui::SliderInt(XorStr("RCS Amount Y"), &Config::Weapon[i]->Aimbot->RCSAmountY, 0, 100, "%.0f%%");

						}

						if (Config::Weapon[i]->Aimbot->NoSpreadActive)
						{
							ImGui::Combo(XorStr("No Spread"), &Config::Weapon[i]->Aimbot->NoSpread, NoSpreadList, ARRAYSIZE(NoSpreadList));
						}

						ImGui::Separator();


						ImGui::Checkbox(XorStr("Auto Wall"), &Config::Weapon[i]->Aimbot->AutoWall);
						if (Config::Weapon[i]->Aimbot->AutoWall)
						{

							ImGui::SliderInt(XorStr("Min Damage"), &Config::Weapon[i]->Aimbot->MinDamage, 0, 100);
						}
						ImGui::Separator();



						ImGui::Combo(XorStr("Hit Scan"), &Config::Weapon[i]->Aimbot->HitScan, HitScanList, ARRAYSIZE(HitScanList));
						if (Config::Weapon[i]->Aimbot->HitScan == 2) // Extra
						{

							ImGui::SliderFloat(XorStr("Scale"), &Config::Weapon[i]->Aimbot->HitScanScale, 0.0f, 1.0f);
						}


						ImGui::Separator();



						ImGui::Combo(XorStr("Target"), &Config::Weapon[i]->Aimbot->Target, AimTargetList, ARRAYSIZE(AimTargetList));
						if (Config::Misc->Restriction != 1)
						{

							ImGui::Checkbox(XorStr("Perfect Silent"), &Config::Weapon[i]->Aimbot->Silent);
						}

						Config::Weapon[i]->Aimbot->Clamp();
					}
					else
					{

						ImGui::Separator();

						ImGui::Combo(XorStr("Mode"), &Config::Main->Aimbot->Mode, ModeList, ARRAYSIZE(ModeList));
						if (Config::Main->Aimbot->Mode == 2) // On Press
						{

							ImGui::KeyButton(XorStr("Key"), &Config::Main->Aimbot->Key);
						}

						ImGui::Checkbox(XorStr("Auto Fire"), &Config::Main->Aimbot->AutoFire);

						ImGui::Checkbox(XorStr("Auto Stop"), &Config::Main->Aimbot->AutoStop);

						ImGui::Checkbox(XorStr("Auto Crouch"), &Config::Main->Aimbot->AutoCrouch);

						ImGui::SameLine();
						ImGui::Checkbox(XorStr("Auto Reload"), &Config::Main->Aimbot->AutoReload);

						ImGui::Checkbox(XorStr("Anti Spawn-Protection"), &Config::Main->Aimbot->AntiSpawnProtection);

						ImGui::Checkbox(XorStr("No Switch"), &Config::Main->Aimbot->NoSwitch);
						ImGui::Checkbox(XorStr("Resolver"), &Config::Main->Aimbot->Resolver);
						if (Config::Main->Aimbot->Resolver)
						{
							ImGui::SliderInt(XorStr("Resolver Bullet"), &Config::Main->Aimbot->ResvolerBullets, 1, 7);
							ImGui::SliderInt(XorStr("Resolver Change Delay"), &Config::Main->Aimbot->ResvolerBulletsDelay, 2, 8);
							ImGui::SliderFloat(XorStr("Resolver Add Y"), &Config::Misc->ResolverAng, 0.0f, 180.0f);
						}

						ImGui::Separator();

						ImGui::Combo(XorStr("Spot"), &Config::Main->Aimbot->Spot, SpotList, ARRAYSIZE(SpotList));

						ImGui::Checkbox(XorStr("Randomize"), &Config::Main->Aimbot->SpotRandomize);

						ImGui::Checkbox(XorStr("Height "), &Config::Main->Aimbot->Height);

						if (Config::Main->Aimbot->Height)
						{

							ImGui::SliderFloat(XorStr("Hegiht Scale"), &Config::Main->Aimbot->HeightScale, -5.0f, 5.0f);

							ImGui::SliderFloat(XorStr("X Scale"), &Config::Main->Aimbot->HeightScaleX, -15.0f, 15.0f);
							ImGui::SliderFloat(XorStr("Y Scale"), &Config::Main->Aimbot->HeightScaleY, -15.0f, 15.0f);
						}


						ImGui::Separator();


						ImGui::Combo(XorStr("Target Selection"), &Config::Main->Aimbot->TargetSelection, TargetSelectionList, ARRAYSIZE(TargetSelectionList));
						if (Config::Main->Aimbot->TargetSelection == 2)
						{


							ImGui::SliderFloat(XorStr("Filed Of View"), &Config::Main->Aimbot->FieldOfView, 0.0f, 180.0f);
						}

						ImGui::Separator();


						ImGui::Combo(XorStr("Smooth"), &Config::Main->Aimbot->Smooth, SmoothList, ARRAYSIZE(SmoothList));
						if (Config::Main->Aimbot->Smooth == 1) // Step
						{


							ImGui::SliderFloat(XorStr("Vertical"), &Config::Main->Aimbot->StepX, 0.0f, 100.0f);

							ImGui::SliderFloat(XorStr("Horizontal"), &Config::Main->Aimbot->StepY, 0.0f, 100.0f);
						}
						else if (Config::Main->Aimbot->Smooth == 2) // Linear
						{


							ImGui::SliderFloat(XorStr("Vertical"), &Config::Main->Aimbot->SmoothX, 0.0f, 100.0f);

							ImGui::SliderFloat(XorStr("Horizontal"), &Config::Main->Aimbot->SmoothY, 0.0f, 100.0f);
						}


						ImGui::Separator();


						ImGui::SliderInt(XorStr("Duration"), &Config::Main->Aimbot->Duration, 0, 5000);

						ImGui::SliderInt(XorStr("Delay"), &Config::Main->Aimbot->Delay, 0, 5000);

						ImGui::SliderInt(XorStr("Switch Delay"), &Config::Main->Aimbot->SwitchDelay, 0, 5000);



						ImGui::Separator();


						ImGui::Checkbox(XorStr("RCS Active"), &Config::Main->Aimbot->RCS);

						ImGui::SameLine();
						ImGui::Checkbox(XorStr("NoSpread Active"), &Config::Main->Aimbot->NoSpreadActive);

						if (Config::Main->Aimbot->RCS)
						{


							ImGui::SliderInt(XorStr("RCS Delay"), &Config::Main->Aimbot->RCSDelay, 0, 10);

							ImGui::SliderInt(XorStr("RCS Amount X"), &Config::Main->Aimbot->RCSAmountX, 0, 100, "%.0f%%");

							ImGui::SliderInt(XorStr("RCS Amount Y"), &Config::Main->Aimbot->RCSAmountY, 0, 100, "%.0f%%");


						}


						if (Config::Main->Aimbot->NoSpreadActive)
						{

							ImGui::Combo(XorStr("No Spread"), &Config::Main->Aimbot->NoSpread, NoSpreadList, ARRAYSIZE(NoSpreadList));
			
						}

						ImGui::Separator();


						ImGui::Checkbox(XorStr("Auto Wall"), &Config::Main->Aimbot->AutoWall);
						if (Config::Main->Aimbot->AutoWall)
						{


							ImGui::SliderInt(XorStr("Min Damage"), &Config::Main->Aimbot->MinDamage, 0, 100);

							ImGui::Separator();
						}



						ImGui::Combo(XorStr("Hit Scan"), &Config::Main->Aimbot->HitScan, HitScanList, ARRAYSIZE(HitScanList));
						if (Config::Main->Aimbot->HitScan == 2) // Extra
						{



							ImGui::SliderFloat(XorStr("Scale"), &Config::Main->Aimbot->HitScanScale, 0.0f, 1.0f);
						}


						ImGui::Separator();



						ImGui::Combo(XorStr("Target"), &Config::Main->Aimbot->Target, AimTargetList, ARRAYSIZE(AimTargetList));


						if (Config::Misc->Restriction != 1)
						{
							ImGui::Checkbox(XorStr("Perfect Silent"), &Config::Main->Aimbot->Silent);
							ImGui::Combo(XorStr("Adjustment"), &Config::Main->Aimbot->LagCompensation, backtracklist, ARRAYSIZE(backtracklist));
							ImGui::Checkbox(XorStr("Adjustment Only Last Tick"), &Config::Main->Aimbot->LastTick);
							ImGui::Checkbox(XorStr("Update Anim"), &Config::Main->Aimbot->UpdateAnim);
							ImGui::Checkbox(XorStr("Update Abs"), &Config::Main->Aimbot->SetAbs);
						}
						/*	ImGui::Checkbox( XorStr( "	" ), &Config::Main->Aimbot->RCS );
						if( Config::Main->Aimbot->RCS )
						{
						ImGui::SliderInt( XorStr( "RCS Delay" ), &Config::Main->Aimbot->RCSDelay, 0, 10 );
						ImGui::SliderInt( XorStr( "RCS Amount X" ), &Config::Main->Aimbot->RCSAmountX, 0, 100, "%.0f%%" );
						ImGui::SliderInt( XorStr( "RCS Amount Y" ), &Config::Main->Aimbot->RCSAmountY, 0, 100, "%.0f%%" );
						}*/
						Config::Main->Aimbot->Clamp();
					}ImGui::EndChild();
					}
				}
				if (iTab == 1) // triggerbot
				{
					//pushver();
					ImGui::BeginChild(XorStr(""), ImVec2(493.25, 533.25), true);
					{
						ImGui::Checkbox(XorStr("Weapon Config"), &Config::Main->TriggerbotWeaponConfig);

						if (Config::Main->TriggerbotWeaponConfig)
						{
							CSWeaponID i = Config::GetWeaponID(Config::WeaponList[m_iWeaponTriggerbot]);
							ImGui::ListBox("", &m_iWeaponTriggerbot, Config::WeaponList, ARRAYSIZE(Config::WeaponList));

							ImGui::Separator();

							ImGui::Combo(XorStr("Mode"), &Config::Weapon[i]->Triggerbot->Mode, ModeList, ARRAYSIZE(ModeList));
							if (Config::Weapon[i]->Triggerbot->Mode == 2)
								ImGui::KeyButton(XorStr("Key"), &Config::Weapon[i]->Triggerbot->Key);

							ImGui::Separator();

							ImGui::Combo(XorStr("Accuracy"), &Config::Weapon[i]->Triggerbot->Accuracy, AccuracyList, ARRAYSIZE(AccuracyList));
							ImGui::SliderInt(XorStr("Delay"), &Config::Weapon[i]->Triggerbot->Delay, 0, 5000);
							ImGui::SliderInt(XorStr("Burst"), &Config::Weapon[i]->Triggerbot->Burst, 0, 10);

							ImGui::Separator();

							ImGui::ListBoxHeader("Spot");

							ImGui::Selectable("Head", &Config::Weapon[i]->Triggerbot->Head);
							ImGui::Selectable("Chest", &Config::Weapon[i]->Triggerbot->Chest);
							ImGui::Selectable("Stomach", &Config::Weapon[i]->Triggerbot->Stomach);
							ImGui::Selectable("Arms", &Config::Weapon[i]->Triggerbot->Arms);
							ImGui::Selectable("Legs", &Config::Weapon[i]->Triggerbot->Legs);

							ImGui::ListBoxFooter();

							ImGui::Separator();

							ImGui::Checkbox("Auto Wall", &Config::Weapon[i]->Triggerbot->AutoWall);
							if (Config::Weapon[i]->Triggerbot->AutoWall)
								ImGui::SliderInt("Min Damage", &Config::Weapon[i]->Triggerbot->MinDamage, 0, 100);

							ImGui::Separator();

							ImGui::Combo("Target", &Config::Weapon[i]->Triggerbot->Target, AimTargetList, ARRAYSIZE(AimTargetList));

							Config::Weapon[i]->Triggerbot->Clamp();
						}
						else
						{
							ImGui::Separator();

							ImGui::Combo(XorStr("Mode"), &Config::Main->Triggerbot->Mode, ModeList, ARRAYSIZE(ModeList));
							if (Config::Main->Triggerbot->Mode == 2)
								ImGui::KeyButton(XorStr("Key"), &Config::Main->Triggerbot->Key);

							ImGui::Separator();

							ImGui::Combo(XorStr("Accuracy"), &Config::Main->Triggerbot->Accuracy, AccuracyList, ARRAYSIZE(AccuracyList));
							ImGui::SliderInt(XorStr("Delay"), &Config::Main->Triggerbot->Delay, 0, 5000);
							ImGui::SliderInt(XorStr("Burst"), &Config::Main->Triggerbot->Burst, 0, 10);

							ImGui::Separator();

							ImGui::ListBoxHeader("Spot");

							ImGui::Selectable("Head", &Config::Main->Triggerbot->Head);
							ImGui::Selectable("Chest", &Config::Main->Triggerbot->Chest);
							ImGui::Selectable("Stomach", &Config::Main->Triggerbot->Stomach);
							ImGui::Selectable("Arms", &Config::Main->Triggerbot->Arms);
							ImGui::Selectable("Legs", &Config::Main->Triggerbot->Legs);

							ImGui::ListBoxFooter();

							ImGui::Separator();

							ImGui::Checkbox("Auto Wall", &Config::Main->Triggerbot->AutoWall);
							if (Config::Main->Triggerbot->AutoWall)
								ImGui::SliderInt("Min Damage", &Config::Main->Triggerbot->MinDamage, 0, 100);

							ImGui::Separator();

							ImGui::Combo("Target", &Config::Main->Triggerbot->Target, AimTargetList, ARRAYSIZE(AimTargetList));

							Config::Main->Triggerbot->Clamp();
						}
						} ImGui::EndChild();
				}
				if (iTab == 2)
				{
					ImGui::BeginChild(XorStr(""), ImVec2(493.25, 533.25), true);
					{
						ImGui::Combo(XorStr("Box"), &Config::ESP->Box, BoxList, ARRAYSIZE(BoxList));
						ImGui::Checkbox(XorStr("Outlined"), &Config::ESP->Outlined);
						ImGui::Checkbox(XorStr("Filled"), &Config::ESP->Filled);

						ImGui::Separator();

						ImGui::Checkbox(XorStr("Name"), &Config::ESP->Name);
						ImGui::SameLine();
						ImGui::Checkbox(XorStr("Weapon"), &Config::ESP->Weapon);
						ImGui::SameLine();
						ImGui::Checkbox(XorStr("Aim Spot"), &Config::ESP->AimSpot);
						ImGui::Checkbox(XorStr("Draw Fov"), &Config::ESP->Fov);
						ImGui::Checkbox(XorStr("Draw Spread"), &Config::ESP->Spread);


						ImGui::Separator();

						ImGui::Combo(XorStr("Health"), &Config::ESP->Health, InfoTypeList, ARRAYSIZE(InfoTypeList));
						ImGui::Combo(XorStr("Armor"), &Config::ESP->Armor, InfoTypeList, ARRAYSIZE(InfoTypeList));
						ImGui::Combo(XorStr("Skeleton"), &Config::ESP->Skeleton, SkeletonList, ARRAYSIZE(SkeletonList));
						ImGui::Separator();

						ImGui::SameLine();
						ImGui::Checkbox(XorStr("Defusing"), &Config::ESP->Defusing);
						ImGui::SameLine();
						ImGui::Checkbox(XorStr("Bomb"), &Config::ESP->Bomb);

						ImGui::Checkbox(XorStr("Esp V. color"), &Config::ESP->Colored);
						ImGui::Separator();

						ImGui::Combo(XorStr("Esp Target"), &Config::ESP->Target, EspTargetList, ARRAYSIZE(EspTargetList));
						ImGui::Separator();

						ImGui::Text(XorStr("Chams"));
						ImGui::Combo(XorStr("Mode"), &Config::Render->ChamsMode, ChamsModeList, ARRAYSIZE(ChamsModeList));
						if (Config::Render->ChamsMode != 0)
						{
							ImGui::Checkbox(XorStr("Chams V. color"), &Config::Render->ChamsColored);
							ImGui::Checkbox(XorStr("Chams outlined"), &Config::Render->ChamsOutlined);
							ImGui::Checkbox(XorStr("Chams V. only"), &Config::Render->ChamsVisOnly);
							ImGui::Combo(XorStr("Chams Target"), &Config::Render->ChamsTarget, ChamsTargetList, ARRAYSIZE(ChamsTargetList));
						}
					}ImGui::EndChild();
				}
				if (iTab == 3)
				{
					ImGui::BeginChild(XorStr(""), ImVec2(493.25, 533.25), true);
					{
						if (Config::Misc->Restriction != 1)
						{
							ImGui::Checkbox(XorStr("ThirdPerson"), &Config::Misc->Lag);
							if (Config::Misc->Lag)
							ImGui::KeyButton(XorStr("Thirdperson Key"), &Config::Misc->LagKey);
							ImGui::Checkbox(XorStr("Hitmarker"), &Config::Misc->HitmarkerEnabled);
							ImGui::Checkbox(XorStr("Hitmarker Damage"), &Config::Misc->HitmarkerHP);
							ImGui::Combo(XorStr("Hitsound"), &Config::Misc->Hitmarker, ggg, ARRAYSIZE(ggg));
							ImGui::Checkbox(XorStr("BunnyHop"), &Config::Misc->AutoJump);
							ImGui::SameLine();
							ImGui::Checkbox(XorStr("Auto Pistol"), &Config::Misc->AutoPistol);
							ImGui::Combo(XorStr("Auto Strafe"), &Config::Misc->AutoStrafe, AutoStrafeList, ARRAYSIZE(AutoStrafeList));
						}
						ImGui::Checkbox(XorStr("Bomb Warning"), &Config::Misc->BombWarning);

						ImGui::Separator();

						ImGui::Combo(XorStr("Crosshair"), &Config::Misc->Crosshair, CrosshairList, ARRAYSIZE(CrosshairList));
						if (Config::Misc->Crosshair)
						{
							if (Config::Misc->Crosshair != 6)
							{
								ImGui::Checkbox(XorStr("Outlined"), &Config::Misc->Outlined);
							}
							ImGui::Checkbox(XorStr("Show Recoil"), &Config::Misc->ShowRecoil);
						}
							
						ImGui::Separator();

						ImGui::Checkbox(XorStr("Fake Lag"), &Config::Misc->FakeLag);
						if (Config::Misc->FakeLag)
							ImGui::SliderInt(XorStr("Amount"), &Config::Misc->ChokedPackets, 1, 32);
						;
						if (Config::Misc->Restriction != 1)
						{
							ImGui::Separator();
							ImGui::Checkbox(XorStr("Air Stuck"), &Config::Misc->AirStuck);
							if (Config::Misc->AirStuck)
								ImGui::KeyButton(XorStr("Stuck Key"), &Config::Misc->StuckKey);
						}
						ImGui::Separator();

						ImGui::Checkbox(XorStr("Circle Strafer"), &Config::Misc->Speed);
						if (Config::Misc->Speed)
						{
							ImGui::KeyButton(XorStr("Circle Key"), &Config::Misc->SpeedKey);
							ImGui::SliderFloat(XorStr("Cirlce Modifer"), &Config::Misc->SpeedMod, 1.0f, 7.0f);
						}
						ImGui::Checkbox(XorStr("No Recoil"), &Config::Removals->NoRecoil);
						ImGui::SameLine();
						if (Config::Misc->Restriction != 1)
						{
							ImGui::Checkbox(XorStr("No Visual Recoil"), &Config::Removals->NoVisualRecoil);
							ImGui::Separator();
						}

						ImGui::SliderInt(XorStr("Fake Ping"), &Config::Misc->FakePing, 0, 260);
						ImGui::Checkbox(XorStr("No Smoke"), &Config::Removals->NoSmoke);
						ImGui::SliderInt(XorStr("Flash Amount"), &Config::Removals->FlashAmount, 0, 100, "%.0f%%");
						ImGui::Separator();

						ImGui::Combo(XorStr("Restriction"), &Config::Misc->Restriction, RestrictionList, ARRAYSIZE(RestrictionList));
						ImGui::Checkbox(XorStr("Movement Recorder"), &Config::Misc->Recorder);
						if (Config::Misc->Recorder)
						{
							ImGui::Checkbox(XorStr("Movement Recorder Silent"), &Config::Misc->RecorderSilent);
							ImGui::KeyButton(XorStr("Movement Recorder Record Key"), &Config::Misc->RecorderRecKey);
							ImGui::KeyButton(XorStr("Movement Recorder Play Key"), &Config::Misc->RecorderPlayKey);
						}

						ImGui::Combo(XorStr("Exploit"), &Config::Misc->Crash, Crashlist, ARRAYSIZE(Crashlist));
						if (Config::Misc->Crash != 0)
						{
							ImGui::KeyButton(XorStr("Exploits Key"), &Config::Misc->CrashKey);
							ImGui::SliderInt(XorStr("Exploits Restriction"), &Config::Misc->CrashRestricion, 0, 10000);
						}

					if (Config::Misc->Crash == 3)
					{
						ImGui::SliderInt(XorStr("bytes"), &Config::Misc->Val0, -1, 1);
						ImGui::SliderInt(XorStr("Packet0"), &Config::Misc->Val1, 0, 100);
						ImGui::SliderInt(XorStr("Packet1"), &Config::Misc->Val2, 0, 100);
					}
					ImGui::Combo(XorStr("Lag Exploit"), &Config::Misc->LagExploit, Laglist, ARRAYSIZE(Laglist));
					if (Config::Misc->LagExploit != 0)
					{
						ImGui::KeyButton(XorStr("Lag Exploit Key"), &Config::Misc->LagExploitKey);
						ImGui::Checkbox(XorStr("Lag Exploit Speedhack"), &Config::Misc->LagExploitSpeed);
						if (Config::Misc->LagExploitSpeed)
						{
							ImGui::KeyButton(XorStr("Lag Exploit Speedhack##j0105j2hirojhpra"), &Config::Misc->LagExploitSpeedKey);
						}
						ImGui::Checkbox(XorStr("Instant Switch"), &Config::Misc->LagExploitSwitch);
						if (Config::Misc->LagExploit == 1 || Config::Misc->LagExploit == 2 || Config::Misc->LagExploit == 3)
						{
							ImGui::SliderInt(XorStr("Custom value"), &Config::Misc->test, 0, 4000);
							ImGui::SliderInt(XorStr("Custom value2"), &Config::Misc->test0, 0, 4000);
						}
						
					}

						Config::Misc->Clamp();

					}ImGui::EndChild();
				}
				if (iTab == 4)
				{
					ImGui::BeginChild(XorStr(""), ImVec2(493.25, 533.25), true);
					{
						if (Config::Misc->Restriction != 1)
						{

							ImGui::Checkbox(XorStr("At Target Enabled"), &Config::AntiAim->AtTargetEnabled);
							if (Config::AntiAim->AtTargetEnabled)
							{
								ImGui::Combo(XorStr("At Target"), &Config::AntiAim->AtTarget, AtTargetList, ARRAYSIZE(AtTargetList));
							}
							ImGui::Separator();
							ImGui::Checkbox(XorStr("No Enemy Enabled"), &Config::AntiAim->NoEnemyEnabled);
							if (Config::AntiAim->NoEnemyEnabled)
							{
								ImGui::Combo(XorStr("No Enemy"), &Config::AntiAim->NoEnemy, NoEnemyList, ARRAYSIZE(NoEnemyList));
							}
							ImGui::Separator();
							ImGui::Checkbox(XorStr("On Knife"), &Config::AntiAim->OnKnife);
							ImGui::SameLine();
							ImGui::Checkbox(XorStr("FakeDuck"), &Config::AntiAim->FakeDuck);
							ImGui::Separator();
							ImGui::Checkbox(XorStr("FakeWalk"), &Config::AntiAim->FakeWalk);
							if (Config::AntiAim->FakeWalk)
							{
								ImGui::KeyButton(XorStr("FakeWalk Key"), &Config::AntiAim->FakeWalkKey);
							}
							ImGui::Combo(XorStr("Stand Pitch"), &Config::AntiAim->PitchStand, PitchStandList, ARRAYSIZE(PitchStandList));
							ImGui::Combo(XorStr("Move Pitch"), &Config::AntiAim->PitchMove, PitchMoveList, ARRAYSIZE(PitchMoveList));
							ImGui::Combo(XorStr("Stand Yaw"), &Config::AntiAim->YawStand, YawStandList, ARRAYSIZE(YawStandList));
							ImGui::Combo(XorStr("Move Yaw"), &Config::AntiAim->YawMove, YawMoveList, ARRAYSIZE(YawMoveList));

							if (Config::AntiAim->YawStand == 2 || Config::AntiAim->YawStand == 3 || Config::AntiAim->YawStand == 4 || Config::AntiAim->YawStand == 6 || Config::AntiAim->YawStand == 10 || Config::AntiAim->YawStand == 11 || Config::AntiAim->YawMove == 12 || Config::AntiAim->YawMove == 13 || Config::AntiAim->YawStand == 14)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Stand Choked Packets"), &Config::AntiAim->StandChokedPackets, 0, 15);
							}
							if (Config::AntiAim->YawStand == 9)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Stand Static Modifer "), &Config::AntiAim->StandStaticModifer, -180.f, 180.f);

							}
							if (Config::AntiAim->YawStand == 5)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Stand Spin Speed"), &Config::AntiAim->StandSpinSpeed, -100, 100);

							}
							if (Config::AntiAim->YawStand == 12 || Config::AntiAim->YawStand == 14)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Stand Custom Fake Spin Speed"), &Config::AntiAim->StandFakeSpinSpeed, -100, 100);
								ImGui::SliderFloat(XorStr("Stand Custom Fake Spin"), &Config::AntiAim->StandFakeSpinAngle, -180.f, 180.f);
							}
							if (Config::AntiAim->PitchStand == 3 || Config::AntiAim->PitchStand == 5)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Stand Custom Angle Pitch"), &Config::AntiAim->StandCustomAnglePitch, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Stand Custom Angle FakePitch"), &Config::AntiAim->StandCustomAngleFakePitch, -180.f, 180.f);
							}
							if (Config::AntiAim->PitchStand == 5)
							{
									ImGui::SliderInt(XorStr("Stand Switch Delay"), &Config::AntiAim->StandSwitchPitchDelay, 20, 620);

							}
							
							if (Config::AntiAim->YawStand == 7 || Config::AntiAim->YawStand == 8 || Config::AntiAim->YawStand == 10 || Config::AntiAim->YawStand == 11)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Stand Custom Angle Yaw"), &Config::AntiAim->StandCustomAngleYaw, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Stand Custom Angle FakeYaw"), &Config::AntiAim->StandCustomAngleFakeYaw, -180.f, 180.f);
							}
							if (Config::AntiAim->YawMove == 2 || Config::AntiAim->YawMove == 3 || Config::AntiAim->YawMove == 4 || Config::AntiAim->YawMove == 6 || Config::AntiAim->YawMove == 10 || Config::AntiAim->YawMove == 11 || Config::AntiAim->YawMove == 12 || Config::AntiAim->YawMove == 13 || Config::AntiAim->YawMove == 14)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Move Choked Packets"), &Config::AntiAim->MoveChokedPackets, 0, 15);
							}
							if (Config::AntiAim->YawStand == 6)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Stand First Fake"), &Config::AntiAim->StandCustomAngleFakeYaw1, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Stand Second Fake"), &Config::AntiAim->StandCustomAngleFakeYaw2, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Stand First Real"), &Config::AntiAim->StandCustomAngleYaw1, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Stand Second Real"), &Config::AntiAim->StandCustomAngleYaw2, -180.f, 180.f);

							}
							if (Config::AntiAim->YawMove == 6)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Move First Fake"), &Config::AntiAim->MoveCustomAngleFakeYaw1, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Move Second Fake"), &Config::AntiAim->MoveCustomAngleFakeYaw2, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Move First Real"), &Config::AntiAim->MoveCustomAngleYaw1, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Move Second Real"), &Config::AntiAim->MoveCustomAngleYaw2, -180.f, 180.f);

							}

							if (Config::AntiAim->YawMove == 9)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Move Static Modifer "), &Config::AntiAim->MoveStaticModifer, -180.f, 180.f);

							}
							if (Config::AntiAim->YawMove == 5)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Move Spin Speed"), &Config::AntiAim->MoveSpinSpeed, -100, 100);

							}

							if (Config::AntiAim->PitchMove == 3 || Config::AntiAim->PitchMove == 5)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Move Custom Angle Pitch"), &Config::AntiAim->MoveCustomAnglePitch, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Move Custom Angle FakePitch"), &Config::AntiAim->MoveCustomAngleFakePitch, -180.f, 180.f);
							}
							if (Config::AntiAim->PitchMove == 5)
							{
								ImGui::SliderInt(XorStr("Move Switch Delay"), &Config::AntiAim->MoveSwitchPitchDelay, 20, 620);
								
							}
							if (Config::AntiAim->YawMove == 7 || Config::AntiAim->YawMove == 8 || Config::AntiAim->YawMove == 10 || Config::AntiAim->YawMove == 11)
							{
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("Move Custom Angle Yaw"), &Config::AntiAim->MoveCustomAngleYaw, -180.f, 180.f);
								ImGui::SliderFloat(XorStr("Move Custom Angle FakeYaw"), &Config::AntiAim->MoveCustomAngleFakeYaw, -180.f, 180.f);
							}
							if (Config::AntiAim->YawMove == 12 || Config::AntiAim->YawMove == 14)
							{
								ImGui::Separator();
								ImGui::SliderInt(XorStr("Move Custom Fake Spin Speed"), &Config::AntiAim->MoveFakeSpinSpeed, -100, 100);
								ImGui::SliderFloat(XorStr("Move Custom Fake Spin"), &Config::AntiAim->MoveFakeSpinAngle, -180.f, 180.f);
							}

							RenderPlayers();
							Config::AntiAim->Clamp();
						}
					}ImGui::EndChild();
				}
				if (iTab == 5)
				{
					ImGui::BeginChild(XorStr("##192yhowgsdvkno"), ImVec2(493.25, 533.25), true);
					{
						int currentcolor;
						float color[3];
							pushver();
							pushver();
							pushhor();
							
							auto curlist = Config::Colors->curr;
							if (curlist == 0) if (ImGui::ColorPicker(m_flColors[COL_T_ESP_NORMAL])) ApplyColors();
							if (curlist == 1) if (ImGui::ColorPicker(m_flColors[COL_T_ESP_COLORED])) ApplyColors();
							if (curlist == 2) if (ImGui::ColorPicker(m_flColors[COL_CT_ESP_NORMAL])) ApplyColors();
							if (curlist == 3) if (ImGui::ColorPicker(m_flColors[COL_CT_ESP_COLORED])) ApplyColors();
							if (curlist == 4) if (ImGui::ColorPicker(m_flColors[COL_T_CHAMS_NORMAL])) ApplyColors();
							if (curlist == 5) if (ImGui::ColorPicker(m_flColors[COL_T_CHAMS_COLORED])) ApplyColors();
							if (curlist == 6) if (ImGui::ColorPicker(m_flColors[COL_CT_CHAMS_NORMAL])) ApplyColors();
							if (curlist == 7) if (ImGui::ColorPicker(m_flColors[COL_CT_CHAMS_COLORED])) ApplyColors();
							if (curlist == 8) if (ImGui::ColorPicker(m_flColors[COL_CROSSHAIR])) ApplyColors();
							if (curlist == 9) if (ImGui::ColorPicker(m_flColors[COL_CHAMSOUTLINEDC])) ApplyColors();
							
							ImGui::SameLine();
							
							ImGui::ListBox("##19horsnvcgh129fjg", &Config::Colors->curr, colorlist, ARRAYSIZE(colorlist), 10);
						
							pushver();
							pushver();

							pushhor();
						
							ImGui::ListBoxHeader("##91hl1h1jofo");
						

							WIN32_FIND_DATAA ffd;
							LARGE_INTEGER filesize;
							CHAR szDir[MAX_PATH];
							HANDLE hFind = INVALID_HANDLE_VALUE;

							std::string strPath = Config::GetPath();

							strncpy_s(szDir, strPath.c_str(), MAX_PATH);
							strncat_s(szDir, "*", MAX_PATH);

							hFind = FindFirstFileA(szDir, &ffd);

							std::vector<std::string> files;

							int index = 0;
							ImGui::SameLine();

							if (hFind != INVALID_HANDLE_VALUE)
							{
								do
								{
									auto ext = GetExtension(ffd.cFileName);

									if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && ext.compare("cfg") == 0)
									{
										const bool item_selected = (index == m_iConfig);
										filesize.LowPart = ffd.nFileSizeLow;
										filesize.HighPart = ffd.nFileSizeHigh;
										char szText[MAX_PATH];
										sprintf_s(szText, sizeof(szText), "%s [%lld bytes]", ffd.cFileName, filesize.QuadPart);
										ImGui::PushID(index);
										if (ImGui::Selectable(szText, item_selected))
										{
											m_iConfig = index;
											strncpy_s(m_szConfigName, ffd.cFileName, sizeof(m_szConfigName));
										}
										ImGui::PopID();
										index++;
										files.push_back(ffd.cFileName);
									}
								} while (FindNextFileA(hFind, &ffd) != 0);

								FindClose(hFind);
							}
							ImGui::ListBoxFooter();
							//		ImGui::SameLine();


							ImGui::InputText("##21yhwrjrej4hrv", m_szConfigName, sizeof(m_szConfigName));

							if (ImGui::Button(XorStr("Load##asdj12whj93901jgj"), ImVec2(70, 15)) && m_iConfig != -1)
							{
								Shared::m_strConfig = files[m_iConfig];
								Shared::m_bLoad = true;
							}

							ImGui::SameLine();

							if (ImGui::Button(XorStr("Save##120hroesvpnr3"), ImVec2(70, 15)) && !std::string(m_szConfigName).empty())
							{
								Shared::m_strConfig = m_szConfigName;
								Shared::m_bSave = true;
							}

							ImGui::SameLine();
							if (ImGui::Button(XorStr("Delete##21ehwrhwrjj31"), ImVec2(70, 15)))
								ImGui::OpenPopup(XorStr("Delete?##21hwejrjhbvvvvs"));

							if (ImGui::BeginPopupModal(XorStr("Delete?"), NULL, ImGuiWindowFlags_AlwaysAutoResize))
							{
								ImGui::Text("You are going to delete \"%s\"!\nAre you sure?\n\n", m_szConfigName);
								ImGui::Separator();

								if (ImGui::Button(XorStr("Yes"), ImVec2(125, 0)))
								{
									Config::Delete(m_szConfigName);
									ImGui::CloseCurrentPopup();
								}

								ImGui::SameLine();

								if (ImGui::Button(XorStr("No"), ImVec2(125, 0)))
									ImGui::CloseCurrentPopup();

								ImGui::EndPopup();
							}

							ImGui::KeyButton(XorStr("Menu##13yh0rjscvpc"), &Config::Binds->Menu);
							ImGui::KeyButton(XorStr("Eject##018yhewidshgoj3mm1m12"), &Config::Binds->Eject);
							ImGui::KeyButton(XorStr("Panic##zxg0gg0jjg1j002"), &Config::Binds->Panic);
							ImGui::Checkbox(XorStr("Icons"), &Config::Misc->icons);
							//	ImGui::Checkbox(XorStr("Menu Animation"), &Config::Binds->MenuSpeed);
							
						ImGui::EndChild();
					}
				}
			//	if (iTab == 6)
			//	{ }
				ImGui::EndChild();

				ImGui::End();

				style.Colors[ImGuiCol_WindowBg] = ImColor(30, 30, 30, 255);
				ImGui::SetNextWindowPos(ImVec2(pos.x - 4, pos.y - 4));
				ImGui::SetNextWindowSize(ImVec2(backsize1, backsize2));
				ImGui::Begin("##99noo1akspd", NULL, ImVec2(backsize1, backsize2), 1.f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
				{

				}ImGui::End();
				ImGui::Render();

			}
		}
	}

	bool Menu::OnKeyEvent( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		if (!Shared::m_bMenu)
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

	const char* TeamList[ ] =
	{
		"None",
		"Spectator",
		"T",
		"CT",
	};

	const char* PitchModList[ ] =
	{
		"Normal",
		"Zero",
		"Up",
		"Down",
		"Brutforce",
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
		"Brutforce",
		"Resolver",
	};

	void Menu::RenderPlayers()
	{
		ImGui::Columns( 5, "players_columns" );
		ImGui::Separator();
		ImGui::Text( XorStr( "Name" ) ); ImGui::NextColumn();
		ImGui::Text( XorStr( "Team" ) ); ImGui::NextColumn();
		ImGui::Text( XorStr( "SteamID" ) ); ImGui::NextColumn();
		ImGui::Text( XorStr( "Pitch" ) ); ImGui::NextColumn();
		ImGui::Text(XorStr("Yaw")); ImGui::NextColumn();
		ImGui::Separator();

		auto local = C_CSPlayer::GetLocalPlayer();

		if( local )
		{
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

				
				ImGui::Text( data.name ); ImGui::NextColumn();
				ImGui::Text( TeamList[ team ] ); ImGui::NextColumn();

				char guid[ 32 ];
				sprintf_s( guid, sizeof( guid ), "##guid_%i", i );

				ImGui::InputText( guid, data.guid, sizeof( data.guid ), ImGuiInputTextFlags_ReadOnly ); ImGui::NextColumn();
			//	ImGui::Text( std::to_string( data.userID ).c_str() ); ImGui::NextColumn();
				
				char pitch[ 32 ];
				sprintf_s(pitch, sizeof(pitch), "##pitch_%i", i); 
				char yaw[ 32 ];
				sprintf_s( yaw, sizeof( yaw ), "##yaw_%i", i );

				ImGui::Combo( pitch, &player_from_list->m_pitch, PitchModList, ARRAYSIZE( PitchModList ) ); ImGui::NextColumn();
				ImGui::Combo( yaw, &player_from_list->m_yaw, YawModList, ARRAYSIZE( YawModList ) ); ImGui::NextColumn();
			}
		}

		ImGui::Columns( 1 );
		ImGui::Separator();
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

			m_flColors[COL_CROSSHAIR][i] = Config::Colors->Crosshair[i] / 255.0f;
			m_flColors[COL_CHAMSOUTLINEDC][i] = Config::Colors->ChamsOutlinedC[i] / 255.0f;
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

			Config::Colors->Crosshair[i] = (int)(m_flColors[COL_CROSSHAIR][i] * 255.0f);
			Config::Colors->ChamsOutlinedC[i] = (int)(m_flColors[COL_CHAMSOUTLINEDC][i] * 255.0f);
		}

		for( int i = 0; i < 3; i++ )
		{
			ImGuiStyle& Style = ImGui::GetStyle();

		}
	}

}