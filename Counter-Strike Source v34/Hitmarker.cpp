#include "HitMarker.hpp"
#include "Config.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Sounds.hpp"
#include <Windows.h>
#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")
float hitmarkerAlpha = 0.0f;
float hitmarkerAlpha1 = 0.0f;
int dmg0;
int health0;
int armor0;
bool headshot0;
namespace Feature
{
void HitMarker::FireGameEvent( IGameEvent* game_event )
{
	if( game_event )
	{
		auto name = game_event->GetName();

		if( !std::strcmp( name, "player_hurt" ) )
		{
			auto local = C_CSPlayer::GetLocalPlayer();

			if( local )
			{
				auto userid = game_event->GetInt("userid");
				auto attacker = game_event->GetInt("attacker");
				auto health = game_event->GetInt("health");
				auto armor = game_event->GetInt("armor");
				int dmg = game_event->GetInt("dmg_health");
				if (local->GetIndex() == Source::m_pEngine->GetPlayerForUserID(attacker))
				{
					m_render = true;
					m_timer.Reset();
					hitmarkerAlpha = 1.0f;
					hitmarkerAlpha1 = 1.0f;
					if (Config::Misc->Hitmarker == 1)
					{
						PlaySoundA(rawData, NULL, SND_ASYNC | SND_MEMORY);
					}
					else if (Config::Misc->Hitmarker == 2)
					{
						PlaySoundA(rawData1, NULL, SND_ASYNC | SND_MEMORY);
					}
					else if (Config::Misc->Hitmarker == 3)
					{
						PlaySoundA(rawData2, NULL, SND_ASYNC | SND_MEMORY);
					}
					health0 = health;
					dmg0 = dmg;
					armor0 = armor;
					if (Config::Misc->aaa == false)
						Config::Misc->aaa = true;
				}
			}
		}
	}
}
void HitMarker::Present()
{
	if( m_timer.Elapsed() >= 250 )
		m_render = false;

	if (m_render)
	{
		int W, H;
		Source::m_pEngine->GetScreenSize(W, H);

		int x = W / 2;
		int y = H / 2;
		auto white = Color(255, 255, 255, (hitmarkerAlpha * 255.f));
		if (hitmarkerAlpha < 0.f)
			hitmarkerAlpha = 0.f;
		else if (hitmarkerAlpha > 0.f)
			hitmarkerAlpha -= 0.01f;

		if (hitmarkerAlpha1 < 0.f)
			hitmarkerAlpha1 = 0.f;
		else if (hitmarkerAlpha1 > 0.f)
			hitmarkerAlpha1 -= 0.0005f;

		if (Config::Misc->HitmarkerEnabled)
		{
			Source::m_pRenderer->DrawLine(W / 2 - 10, H / 2 - 10, W / 2 - 5, H / 2 - 5, white);
			Source::m_pRenderer->DrawLine(W / 2 - 10, H / 2 + 10, W / 2 - 5, H / 2 + 5, white);
			Source::m_pRenderer->DrawLine(W / 2 + 10, H / 2 - 10, W / 2 + 5, H / 2 - 5, white);
			Source::m_pRenderer->DrawLine(W / 2 + 10, H / 2 + 10, W / 2 + 5, H / 2 + 5, white);
		}
		if (Config::Misc->HitmarkerHP)
		{
			Source::m_pRenderer->DrawText(Source::m_hFont, W / 2 + 2, H / 2 + 10, FONT_ALIGN_CENTER_H, Color(255, 255, 255, (hitmarkerAlpha1 * 255.f)), XorStr("%i"), dmg0);
		}
		if (health0 != 0)
		{
			Source::m_pRenderer->DrawText(Source::m_hFont, W / 2 + 2, H / 2 + 25, FONT_ALIGN_CENTER_H, Color(255, 255, 255, (hitmarkerAlpha1 * 255.f)), XorStr("%iHP left!"), health0);
			if (armor0 != 0)
			{
				Source::m_pRenderer->DrawText(Source::m_hFont, W / 2 + 2, H / 2 + 36, FONT_ALIGN_CENTER_H, Color(255, 255, 255, (hitmarkerAlpha1 * 255.f)), XorStr("%iAR left!"), armor0);
			}
		}
		if (health0 <= 0)
		{
				Source::m_pRenderer->DrawText(Source::m_hFont, W / 2 + 2, H / 2 + 25, FONT_ALIGN_CENTER_H, Color(255, 255, 255, (hitmarkerAlpha1 * 255.f)), XorStr("KILLED"));
		}
	}
}

}