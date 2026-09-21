#include "Hooked.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Weapon.hpp"
#include "Config.hpp"	
#include "ImGui.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "Debug.hpp"
#include "LagCompensation.hpp"
#include "Aimbot.hpp"
#include "HitMarker.hpp"
#include "Thirdperson.hpp"
#include "Resolver.hpp"

#define	CONTENTS_SOLID			0x1
bool bSendPacket = false;

// Hit-reactive AA direction: flipped every time we take damage while
// AntiAim->HitReactive is on. Mirrors spin direction / unbalanced jitter
// side to invalidate the enemy's locked resolver correction.
int g_iAAHurtDir = 1;
using namespace Direct3D9;

void RenderSkeleton(C_CSPlayer* player, matrix3x4_t* transform, const Color& color)
{
	auto model = player->GetModel();

	if (!model)
		return;

	auto studio = Source::m_pModelInfoClient->GetStudioModel(model);

	if (!studio)
		return;

	for (int i = 0; i < studio->numbones; i++)
	{
		auto data = studio->GetBone(i);

		if (!data)
			continue;

		if (!(data->flags & 0x0100))
			continue;

		if (data->parent == -1)
			continue;

		auto bone = Vector
		{
			transform[i][0][3],
			transform[i][1][3],
			transform[i][2][3],
		};

		auto parent = Vector
		{
			transform[data->parent][0][3],
			transform[data->parent][1][3],
			transform[data->parent][2][3],
		};

		auto bone_screen = Vector{};
		auto parent_screen = Vector{};

		if (Source::WorldToScreen(bone, bone_screen) && Source::WorldToScreen(parent, parent_screen))
			Source::m_pRenderer->DrawLine(bone_screen[0], bone_screen[1], parent_screen[0], parent_screen[1], color);
	}
}

// Optimal auto-strafer, ported from the epoximotion report (acos-optimal core §4+§6,
// WASD Direction Control §5: work.yaw -= DirectionOffset(buttons), formula version). The former Normal/Boost implementation was broken and is fully
// replaced: ApplyStrafe only wrote forwardmove (rotation incomplete — sidemove never rotated),
// abs() truncated the yaw delta to int, and the Boost math ran in Normal mode too (misplaced
// closing brace), so both modes did the same thing. New modes:
//   1 - Optimal: acos-optimal side strafe around the velocity yaw.
//   2 - Optimal + WASD: same core, work.yaw -= DirectionOffset(buttons) (§5 formula).
// Porting notes vs the report:
//   - The report's two flags (misc+0x02 enabled, misc+0xda directional) map onto our
//     0/1/2 combo, which covers the same meaningful states.
//   - No 90-tick ring buffer: CS:S v34 exposes live m_vecVelocity in CreateMove.
//   - No bhop gate: the report requires its own bhop enabled (config quirk); ours strafes
//     standalone (manual or auto bhop alike).
//   - No hardcoded m_flMaxspeed @ +0xF60 (build-specific, fragile — report §11.7):
//     the gain term uses the canonical live form instead, see below.
void AutoStrafe( CUserCmd* cmd, C_CSPlayer* player )
{
	const int iMode = Config::Misc->AutoStrafe;

	if( iMode != 1 && iMode != 2 )
		return;

	// Circle strafer owns the move vector while active — don't fight it.
	if( Config::Misc->Speed && Shared::m_bSpeed )
		return;

	if( player->m_MoveType() == MOVETYPE_LADDER || player->m_MoveType() == MOVETYPE_NOCLIP )
		return;

	if( player->m_lifeState() == LIFE_DEAD )
		return;

	if( Source::m_pDataManager->GetFlags() & FL_ONGROUND )
		return;

	static ConVar* pSideSpeed = nullptr;
	static ConVar* pAirAccelerate = nullptr;

	if( !pSideSpeed )
		pSideSpeed = Source::m_pCvar->FindVar( XorStr( "cl_sidespeed" ) );

	if( !pAirAccelerate )
		pAirAccelerate = Source::m_pCvar->FindVar( XorStr( "sv_airaccelerate" ) );

	if( !pSideSpeed || !pAirAccelerate )
		return;

	const float flSideSpeed = pSideSpeed->GetFloat();
	const float flAirAccelerate = pAirAccelerate->GetFloat();
	const float flDeltaTime = Source::m_pGlobalVars->interval_per_tick;

	if( flSideSpeed <= 0.0f || flAirAccelerate <= 0.0f || flDeltaTime <= 0.0f )
		return;

	// Report §4: pure side strafe; the writeback rotation aims it at targetYaw.
	cmd->forwardmove = 0.0f;

	// Report §5: WASD Direction Control (mode 2), exact branch emulation. The S+A
	// entry yields 225 deg (subtracted => effectively +135 deg) — verbatim quirk.
	float flWorkYaw = cmd->viewangles.y;

	if( iMode == 2 )
	{
		const int iButtons = cmd->buttons;
		float flDir = 0.0f;

		if( iButtons & IN_MOVELEFT )
		{
			flDir = -90.0f;

			if( iButtons & IN_MOVERIGHT )
				flDir += 90.0f;

			if( iButtons & IN_FORWARD )
				flDir *= 0.5f;
			else if( iButtons & IN_BACK )
				flDir = flDir * -0.5f + 180.0f;
		}
		else
		{
			if( iButtons & IN_MOVERIGHT )
				flDir += 90.0f;

			if( iButtons & IN_FORWARD )
				flDir *= 0.5f;
			else if( iButtons & IN_BACK )
				flDir = flDir * -0.5f + 180.0f;
		}

		flWorkYaw -= flDir;
	}

	const Vector3 vecVelocity = player->m_vecVelocity();
	const float flSpeed2D = vecVelocity.Length2D();

	float flTargetYaw = flWorkYaw;

	// Report §6.2: standing-still kick.
	if( flSpeed2D <= 1.0f )
	{
		flTargetYaw = flWorkYaw - 92.0f;
		cmd->sidemove = -flSideSpeed;
	}
	else
	{
		// Report §6.3, canonical live form: gain = airacc * 30 * dt. The pasted formula
		// (side * airacc * dt * maxspeed) saturates the acos argument to 0 at every sane
		// setting (e.g. 450*12*(1/64)*250 >> 30 cap), pinning the angle at a flat 90 deg;
		// the live form below yields the intended optimal curve (~85 deg @ 300 u/s … 0 @ stall).
		const float flCap = flSideSpeed < 30.0f ? flSideSpeed : 30.0f;
		const float flGain = flAirAccelerate * 30.0f * flDeltaTime;
		const float flClampedGain = flGain < flCap ? flGain : flCap;

		float flCos = ( flCap - flClampedGain ) / flSpeed2D;

		if( flCos < -1.0f )
			flCos = -1.0f;
		else if( flCos > 1.0f )
			flCos = 1.0f;

		const float flOptimal = ToDegrees( acosf( flCos ) );
		const float flVelocityYaw = ToDegrees( atan2f( vecVelocity.y, vecVelocity.x ) );
		const bool bPositive = AngleNormalize( flWorkYaw - flVelocityYaw ) > 0.0f;

		// Report §6.2/§9: the +/-90 deg compensates the side-axis offset + rotation
		// direction so the resulting wish direction equals velYaw +/- optimal.
		const float flAdd = bPositive ? flOptimal : -flOptimal;
		flTargetYaw = flVelocityYaw + flAdd + ( bPositive ? -90.0f : 90.0f );

		cmd->sidemove = bPositive ? -flSideSpeed : flSideSpeed;
	}

	// Report §6.4: rotate (forward, side) by (viewYaw - targetYaw); viewangles untouched.
	float flDelta = cmd->viewangles.y - flTargetYaw;
	flDelta = fmodf( flDelta, 360.0f );

	if( flDelta > 180.0f )
		flDelta -= 360.0f;
	else if( flDelta < -180.0f )
		flDelta += 360.0f;

	const float flRad = ToRadians( flDelta );
	const float flCosD = cosf( flRad );
	const float flSinD = sinf( flRad );
	const float flForward = cmd->forwardmove;
	const float flSide = cmd->sidemove;

	cmd->forwardmove = flForward * flCosD - flSide * flSinD;
	cmd->sidemove = flForward * flSinD + flSide * flCosD;
}

// Anti SMAC (Misc): последний штрих перед отправкой — чиним углы юзеркоманды,
// которые могли испортить аим/сайлент/анти-аим: NaN/Inf в нули, питч в ±89,
// yaw в ±180, roll строго 0. Именно такие углы проверяет smac_eyetest.
static bool IsBadFloat( float fl ) // NaN/Inf, побитово (не зависит от /fp)
{
	return ( ( *( unsigned int* )&fl ) & 0x7F800000 ) == 0x7F800000;
}

void AntiSMAC( CUserCmd* cmd )
{
	Vector3& vAngles = cmd->viewangles;

	if( IsBadFloat( vAngles.x ) )
		vAngles.x = 0.0f;

	if( IsBadFloat( vAngles.y ) )
		vAngles.y = 0.0f;

	if( IsBadFloat( vAngles.z ) )
		vAngles.z = 0.0f;

	ClampAngles( vAngles );

	vAngles.z = 0.0f;
}

void AutoJump( CUserCmd* cmd, C_CSPlayer* player )
{
	if( player->m_MoveType() == MOVETYPE_LADDER || player->m_MoveType() == MOVETYPE_NOCLIP )
		return;

	if (player->m_lifeState() == LIFE_DEAD)
		return;
		

	static bool bFirstJump = false;
	static bool bFakeJump = false;

	if( cmd->buttons & IN_JUMP )
	{
		if( !bFirstJump )
		{
			bFirstJump = bFakeJump = true;
		}
		else if( !( Source::m_pDataManager->GetFlags() & FL_ONGROUND ) )
		{
			if( bFakeJump && player->m_vecVelocity().z < 0.0f )
				bFakeJump = false;
			else
				cmd->buttons &= ~IN_JUMP;
		}
		else
		{
			bFakeJump = true;
		}
	}
	else
	{
		bFirstJump = false;
	}
}


void AutoPistol( CUserCmd* cmd, C_WeaponCSBaseGun* weapon )
{
	if( weapon->GetWeaponType() != WEAPONTYPE_PISTOL &&
		weapon->GetWeaponType() != WEAPONTYPE_SHOTGUN &&
		weapon->GetWeaponType() != WEAPONTYPE_SNIPER_RIFLE )
		return;

	if( !weapon->IsFireTime() )
		cmd->buttons &= ~IN_ATTACK;
}

void NoSpread( CUserCmd* cmd, C_WeaponCSBaseGun* weapon )
{
	if( weapon->IsMelee() )
		return;
	if( cmd->buttons & IN_ATTACK )
	{
		if( weapon->IsFireTime() )
		{
			if( Config::Current->Aimbot->NoSpread == 2 )
			{
				cmd->random_seed = 141;

				while( ( MD5_PseudoRandom( cmd->command_number ) & 255 ) != cmd->random_seed )
					cmd->command_number++;
			}
			Source::m_pAccuracy->ApplySpreadFix(weapon, cmd->random_seed, cmd->viewangles, cmd->viewangles, Config::Current->Aimbot->NoSpread);
		}
	}
}

void NoRecoil(CUserCmd* cmd, C_CSPlayer* player, C_WeaponCSBaseGun* weapon)
{
	if (cmd->buttons & IN_ATTACK)
	{
		if (weapon->IsFireTime())
		{
			bSendPacket = false;
			Source::m_pAccuracy->ApplyRecoilFix(player, cmd->viewangles);
		}
	}
}

void AntiKAC( CUserCmd* cmd )
{
// todo
}


void FakeLag(CUserCmd* cmd)
{
	static int fake_tick_count = 0;

	if (fake_tick_count == 0)
	{
		fake_tick_count = Config::Misc->ChokedPackets;

		bSendPacket = true;
	}
	else
	{
		fake_tick_count--;

		bSendPacket = false;
	}
}

void RotateMovement(CUserCmd* cmd, float rotation)
{
	rotation = ToRadians(rotation);
	float cosr, sinr;
	cosr = cos(rotation);
	sinr = sin(rotation);
	float forwardmove, sidemove;
	forwardmove = (cosr * cmd->forwardmove) - (sinr * cmd->sidemove);
	sidemove = (sinr * cmd->forwardmove) - (cosr * cmd->sidemove);
	cmd->forwardmove = forwardmove;
	cmd->sidemove = sidemove;
}

void Speed(C_CSPlayer* player, CUserCmd* cmd, Vector3& Originalview)
{

	static float yaw;
	if (!Shared::m_bSpeed)
	{
		yaw = 0;
		return;
	}

	if (Source::m_pDataManager->GetFlags() & FL_ONGROUND)
		return;
	yaw = AngleNormalize(cmd->viewangles.y - yaw);

	if (player->m_MoveType() == MOVETYPE_LADDER || player->m_MoveType() == MOVETYPE_NOCLIP)
		return;
	if (player->m_lifeState() == LIFE_DEAD)
		return;
	if (GetAsyncKeyState(VK_SPACE))
	{
		Vector3 View(cmd->viewangles);
		float blackcock = 0;
		cmd->forwardmove = 450.f;
		int random = rand() % 100;
		int random2 = rand() % 1000;
		static bool dir;
		static float current_y = View.y;
		if (player->m_vecVelocity().Length() > 50.f)
		{
			blackcock += 0.00007;
			current_y += (Config::Misc->SpeedMod) - blackcock;
		}
		else
		{
			blackcock = 0;
		}
		View.y = current_y;
		if (random == random2)
			View.y += random;
		// Clamp(View);
		else
		{
			float blackcock = 0;
		}
		RotateMovement(cmd, current_y);
	}
}

void FakeWalk(CUserCmd* cmd, C_CSPlayer* player)
{
	(void)player;

	if (!GetAsyncKeyState(Config::AntiAim->FakeWalkKey))
		return;

	// On-demand fakelag: move in choked commands, send stationary snapshots.
	// (Original did pointer arithmetic on cmd and corrupted frametime.)
	static int iChoked = 0;

	if (iChoked < 3)
	{
		bSendPacket = false;
		iChoked++;
	}
	else
	{
		bSendPacket = true;
		iChoked = 0;
		cmd->forwardmove = 0.0f;
		cmd->sidemove = 0.0f;
	}
}

void AtTarget(CUserCmd* cmd, C_CSPlayer* player)
{
	auto best = -1;
	auto distance = 8192.0f;

	Vector3 end, aim;

	for (int i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
	{
		auto enemy = ToCSPlayer(Source::m_pEntList->GetBaseEntity(i));

		if (!enemy)
			continue;

		if (enemy == player)
			continue;

		if (enemy->m_lifeState() != LIFE_ALIVE)
			continue;

		if (enemy->IsDormant())
			continue;

		if (Config::AntiAim->AtTarget == 1) // Enemy
		{
			if (player->m_iTeamNum() == enemy->m_iTeamNum())
				continue;
		}
		else if (Config::AntiAim->AtTarget == 2) // Friendly
		{
			if (player->m_iTeamNum() != enemy->m_iTeamNum())
				continue;
		}

		if (!enemy->GetHitboxVector(12, end))
			continue;

		auto current = end.DistTo(player->EyePosition());

		if (current < distance)
		{
			distance = current;
			best = i;
		}
	}

	if (best != -1)
	{
		auto direction = end - player->EyePosition();

		VectorNormalize(direction);
		VectorAngles(direction, aim);

		cmd->viewangles.y = aim.y;
	}
}

auto IsEveryoneDead1()
{
		auto local = C_CSPlayer::GetLocalPlayer();

		for (auto i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
		{
			auto player = C_CSPlayer::GetPlayer(i);

			if (player && player != local && player->m_lifeState() == LIFE_ALIVE)
			{
				if (Config::AntiAim->NoEnemy == 1) // Enemy
				{
					if (player->m_iTeamNum() == local->m_iTeamNum())
						continue;
				}
				else if (Config::AntiAim->NoEnemy == 2) // Friendly
				{
					if (player->m_iTeamNum() != local->m_iTeamNum())
						continue;
				}
				return false;
		
			}
		}

		return true;
	
}

auto IsEveryoneDead2()
{
	auto local = C_CSPlayer::GetLocalPlayer();

	for (auto i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
	{
		auto player = C_CSPlayer::GetPlayer(i);

		if (player && player != local && player->m_lifeState() == LIFE_ALIVE)
		{
			if (Config::Current->Aimbot->Target == 1) // Enemy
			{
				if (player->m_iTeamNum() == local->m_iTeamNum())
					continue;
			}
			else if (Config::Current->Aimbot->Target == 2) // Friendly
			{
				if (player->m_iTeamNum() != local->m_iTeamNum())
					continue;
			}
			return false;

		}
	}

	return true;

}

auto IsEveryoneDead3()
{
	auto local = C_CSPlayer::GetLocalPlayer();

	for (auto i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
	{
		auto player = C_CSPlayer::GetPlayer(i);

		if (player && player != local && player->m_lifeState() == LIFE_ALIVE)
		{
			if (Config::Current->Triggerbot->Target == 1) // Enemy
			{
				if (player->m_iTeamNum() == local->m_iTeamNum())
					continue;
			}
			else if (Config::Current->Triggerbot->Target == 2) // Friendly
			{
				if (player->m_iTeamNum() != local->m_iTeamNum())
					continue;
			}
			return false;

		}
	}

	return true;

}

void BreakLagComp(CUserCmd* cmd, C_CSPlayer* player)
{
	(void)cmd;

	// Force a send once we moved 68u since the last sent snapshot: the server
	// lag compensator discards records that jumped > 64u (teleport), so every
	// send lands as a teleport - enemy backtrack has no valid record to aim at.
	static Vector3 vLastSent(0.0f, 0.0f, 0.0f);
	static int iChoked = 0;
	static bool bInit = false;

	if (!bInit)
	{
		vLastSent = player->m_vecOrigin();
		bInit = true;
	}

	if (bSendPacket)
	{
		vLastSent = player->m_vecOrigin();
		iChoked = 0;
		return;
	}

	int iMaxChoke = Config::Misc->ChokedPackets;

	if (iMaxChoke < 1)
		iMaxChoke = 1;

	if (iMaxChoke > 14)
		iMaxChoke = 14;

		iChoked++;

	if (iChoked >= iMaxChoke || vLastSent.DistTo(player->m_vecOrigin()) >= 68.0f)
	{
		bSendPacket = true;
		vLastSent = player->m_vecOrigin();
		iChoked = 0;
	}
}

void Hooked_GameEvent(IGameEvent* game_event)
{
	if (!Config::AntiAim->HitReactive)
		return;

	if (std::strcmp(game_event->GetName(), "player_hurt") != 0)
		return;

	auto local = C_CSPlayer::GetLocalPlayer();

	if (!local)
		return;

	if (Source::m_pEngine->GetPlayerForUserID(game_event->GetInt("userid")) != local->GetIndex())
		return;

	// We took damage: mirror spin direction / jitter side to invalidate the
	// enemy's locked resolver correction.
	g_iAAHurtDir = -g_iAAHurtDir;
}

void AntiAim(CUserCmd* cmd, C_CSPlayer* player, C_WeaponCSBaseGun* weapon)
{
	auto va = cmd->viewangles;

	if (cmd->buttons & IN_USE)
		return;

	if (weapon->GetWeaponType() == WEAPONTYPE_GRENADE)
		return;
	if (Config::AntiAim->NoEnemyEnabled)
	{
		if (IsEveryoneDead1())
			return;
	}
	if (Config::AntiAim->OnKnife == 0)
	{

		if (weapon->GetWeaponType() == WEAPONTYPE_KNIFE)
			return;
	}
	if (Config::AntiAim->OnKnife == 1)
	{
		if (cmd->buttons & IN_ATTACK2 && weapon->IsFireTime())
			return;
	}
	if (player->m_MoveType() == MOVETYPE_LADDER || player->m_MoveType() == MOVETYPE_NOCLIP)
		return;

	if ((cmd->buttons & IN_ATTACK) && weapon->IsFireTime())
		return;

	if (Config::AntiAim->AtTargetEnabled)
		AtTarget(cmd, player);
	
	auto velocity = player->m_vecVelocity();

	float speed = velocity.Length2D();
	int type = cmd->command_number % 3;
	if (speed > 100.01)
	{
		if (Config::AntiAim->PitchMove == 1) // Emotion
		{
			cmd->viewangles.x = 70.f;
		}
		else if (Config::AntiAim->PitchMove == 3) // Custom
		{
			static bool p5 = false;

			if (p5)
			{
				cmd->viewangles.x = (Config::AntiAim->MoveCustomAngleFakePitch);
			}
			else
			{
				cmd->viewangles.x = (Config::AntiAim->MoveCustomAnglePitch);
			}
			p5 = !p5;

		}
		else if (Config::AntiAim->PitchMove == 2) // Fake Down 2
		{
			cmd->viewangles.x = -180.f;

		}
		else if (Config::AntiAim->PitchMove == 4) // Flip
		{
			cmd->viewangles.y -= 180.f;
			cmd->viewangles.x -= 180.f;
		}
		else if (Config::AntiAim->PitchMove == 5) // Switch
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
				choked_tick_count = Config::AntiAim->MoveSwitchPitchDelay;

			if (choked_tick_count >= Config::AntiAim->MoveSwitchPitchDelay/2)
			{
				cmd->viewangles.x = Config::AntiAim->MoveCustomAnglePitch;
				choked_tick_count--;
			}

			if (choked_tick_count < Config::AntiAim->MoveSwitchPitchDelay/2)
			{
				cmd->viewangles.x = Config::AntiAim->MoveCustomAngleFakePitch;
				choked_tick_count--;
			}
		}
		else if (Config::AntiAim->PitchMove == 6) // Send Jitter
		{
			// Alternate the custom pitches per SEND tick: per-tick flips alias
			// to a static angle under choke, this guarantees the sent pitch
			// keeps changing. Pairs best with a non-choking yaw + Misc fakelag.
			static int iSendCount = 0;

			if (bSendPacket)
				iSendCount++;

			if (iSendCount % 2)
				cmd->viewangles.x = Config::AntiAim->MoveCustomAngleFakePitch;
			else
				cmd->viewangles.x = Config::AntiAim->MoveCustomAnglePitch;
		}
		if (Config::AntiAim->YawMove == 1) // Backward
		{
			cmd->viewangles.y += 180.f;
		}
		else if (Config::AntiAim->YawMove == 2) // Legit
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += 180.f;
			}
		}

		else if (Config::AntiAim->YawMove == 3) // Fake Sideways Left
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;

				cmd->viewangles.y += 90.f;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y -= 90.f;
			}
		}
		else if (Config::AntiAim->YawMove == 4) // Fake Sideways Right
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;

				cmd->viewangles.y -= 90.f;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += 90.f;
			}
		}
		else if (Config::AntiAim->YawMove == 5) // Spin
		{
			static float yaw = 0.f;

			yaw += (Config::AntiAim->MoveSpinSpeed * g_iAAHurtDir);

			cmd->viewangles.y += yaw;

			if (yaw > 360.f)
				yaw = 0.f;
		}

		else if (Config::AntiAim->YawMove == 6) // Jitter 
		{
			static int lelkek = 0;
			static int choked_tick_count = 0;
			if (lelkek == 0)
			{
				if (choked_tick_count == 0)
				{
					choked_tick_count = Config::AntiAim->MoveChokedPackets;

					bSendPacket = true;

					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleFakeYaw1;
				}
				else
				{
					choked_tick_count--;
					lelkek++;

					bSendPacket = false;

					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleYaw1;
				}
			}

			else if (lelkek == 1)
			{
				if (choked_tick_count == 0)
				{
					choked_tick_count = Config::AntiAim->MoveChokedPackets;

					bSendPacket = true;

					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleFakeYaw2;
				}
				else
				{
					choked_tick_count--;
					lelkek = 0;

					bSendPacket = false;

					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleYaw2;
				}
			}

		}
		else if (Config::AntiAim->YawMove == 7) // Custom Static Jitter
		{
			static bool customjitt = false;

			if (customjitt)
				cmd->viewangles.y = (Config::AntiAim->MoveCustomAngleYaw);
			else
				cmd->viewangles.y = (Config::AntiAim->MoveCustomAngleFakeYaw);

			customjitt = !customjitt;
		}
		else if (Config::AntiAim->YawMove == 8) // Custom Jitter
		{
			static bool customjitt1 = false;

			if (customjitt1)
				cmd->viewangles.y += (Config::AntiAim->MoveCustomAngleYaw);
			else
				cmd->viewangles.y += (Config::AntiAim->MoveCustomAngleFakeYaw);

			customjitt1 = !customjitt1;
		}

		else if (Config::AntiAim->YawMove == 9) // Static Custom

		{
			cmd->viewangles.y = (Config::AntiAim->MoveStaticModifer);
		}

		else if (Config::AntiAim->YawMove == 10) // Custom Fake
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;
				cmd->viewangles.y += (Config::AntiAim->MoveCustomAngleFakeYaw);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += (Config::AntiAim->MoveCustomAngleYaw);
			}
		}

		else if (Config::AntiAim->YawMove == 11) // Static FakeLag
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;
				cmd->viewangles.y = (Config::AntiAim->MoveCustomAngleFakeYaw);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y = (Config::AntiAim->MoveCustomAngleYaw);
			}
		}
		else if (Config::AntiAim->YawMove == 12) // Fake Spin
		{
			static int choked_tick_count = 0;
			static float yaw = 0.f;

			yaw += (Config::AntiAim->MoveFakeSpinSpeed);

			if (yaw > 360.f)
				yaw = 0.f;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;
				bSendPacket = true;
				cmd->viewangles.y = (Config::AntiAim->MoveFakeSpinAngle);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;
				cmd->viewangles.y += yaw;
				
			}
		}

		else if (Config::AntiAim->YawMove == 13) // Fake Spin 2
		{
			static int choked_tick_count = 0;
			static float yaw = 0.f;

			yaw += (Config::AntiAim->MoveFakeSpinSpeed * g_iAAHurtDir);

			if (yaw > 360.f)
				yaw = 0.f;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;
				bSendPacket = true;
				cmd->viewangles.y += yaw;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;
				cmd->viewangles.y += yaw + 180	;
			}
		}
		else if (Config::AntiAim->YawMove == 14) // Unbalanced Jitter
		{
			// 3:1 unbalanced pattern counted over SENDS: three sends of the
			// custom yaw, one of the fake yaw. Median of recent sends lands on
			// the majority side while the mean sits between - beats median /
			// average feedback resolvers that assume symmetric jitter.
			static int choked_tick_count = 0;
			static int iSendCount = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->MoveChokedPackets;

				bSendPacket = true;

				if (iSendCount % 4 == 3)
					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleFakeYaw * g_iAAHurtDir;
				else
					cmd->viewangles.y += Config::AntiAim->MoveCustomAngleYaw * g_iAAHurtDir;

				iSendCount++;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += Config::AntiAim->MoveCustomAngleYaw;
			}
		}
}
	if (speed < 100.01)
	{
		if (Config::AntiAim->PitchStand == 1) // Emotion
		{
			cmd->viewangles.x = 70.f;
		}
		else if (Config::AntiAim->PitchStand == 3) // Custom
		{
			static bool p5 = false;

			if (p5)
			{
				cmd->viewangles.x = (Config::AntiAim->StandCustomAngleFakePitch);
			}
			else
			{
				cmd->viewangles.x = (Config::AntiAim->StandCustomAnglePitch);
			}
			p5 = !p5;

		}
		else if (Config::AntiAim->PitchStand == 2) // Fake Down 2
		{
			cmd->viewangles.x = -180.f;

		}
		else if (Config::AntiAim->PitchStand == 4) // Flip
		{
			cmd->viewangles.y -= 180.f;
			cmd->viewangles.x -= 180.f;
		}
		else if (Config::AntiAim->PitchStand == 5) // Switch
		{
			static int hhh = 0;

			if (hhh == 0)
				hhh = Config::AntiAim->StandSwitchPitchDelay;

			if (hhh >= Config::AntiAim->StandSwitchPitchDelay / 2)
			{
				cmd->viewangles.x = Config::AntiAim->StandCustomAngleFakePitch;
				hhh--;
			}

			if (hhh < Config::AntiAim->StandSwitchPitchDelay / 2)
			{
				cmd->viewangles.x = Config::AntiAim->StandCustomAnglePitch;
				hhh--;
			}
		}
		else if (Config::AntiAim->PitchStand == 6) // Send Jitter
		{
			// Alternate the custom pitches per SEND tick: per-tick flips alias
			// to a static angle under choke, this guarantees the sent pitch
			// keeps changing. Pairs best with a non-choking yaw + Misc fakelag.
			static int iSendCount = 0;

			if (bSendPacket)
				iSendCount++;

			if (iSendCount % 2)
				cmd->viewangles.x = Config::AntiAim->StandCustomAngleFakePitch;
			else
				cmd->viewangles.x = Config::AntiAim->StandCustomAnglePitch;
		}
		if (Config::AntiAim->YawStand == 1) // Backward
		{
			cmd->viewangles.y += 180.f;
		}
		else if (Config::AntiAim->YawStand == 2) // Legit
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += 180.f;
			}
		}

		else if (Config::AntiAim->YawStand == 3) // Fake Sideways Left
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;

				cmd->viewangles.y += 90.f;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y -= 90.f;
			}
		}
		else if (Config::AntiAim->YawStand == 4) // Fake Sideways Right
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;

				cmd->viewangles.y -= 90.f;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += 90.f;
			}
		}
		else if (Config::AntiAim->YawStand == 5) // Spin
		{
			static float yaw = 0.f;

			yaw += (Config::AntiAim->StandSpinSpeed * g_iAAHurtDir);

			cmd->viewangles.y += yaw;

			if (yaw > 360.f)
				yaw = 0.f;
		}

		else if (Config::AntiAim->YawStand == 6) // Jitter 
		{
			static int lelkek = 0;
			static int choked_tick_count = 0;
			if (lelkek == 0)
			{
				if (choked_tick_count == 0)
				{
					choked_tick_count = Config::AntiAim->StandChokedPackets;

					bSendPacket = true;

					cmd->viewangles.y += Config::AntiAim->StandCustomAngleFakeYaw1;
				}
				else
				{
					choked_tick_count--;
					lelkek++;

					bSendPacket = false;

					cmd->viewangles.y += Config::AntiAim->StandCustomAngleYaw1;
				}
			}

			else if (lelkek == 1)
			{
				if (choked_tick_count == 0)
				{
					choked_tick_count = Config::AntiAim->StandChokedPackets;

					bSendPacket = true;

					cmd->viewangles.y += Config::AntiAim->StandCustomAngleFakeYaw2;
				}
				else
				{
					choked_tick_count--;
					lelkek = 0;

					bSendPacket = false;

					cmd->viewangles.y += Config::AntiAim->StandCustomAngleYaw2;
				}
			}
		}
		else if (Config::AntiAim->YawStand == 7) // Custom Static Jitter
		{
			static bool customjitt = false;

			if (customjitt)
				cmd->viewangles.y = (Config::AntiAim->StandCustomAngleYaw);
			else
				cmd->viewangles.y = (Config::AntiAim->StandCustomAngleFakeYaw);

			customjitt = !customjitt;
		}
		else if (Config::AntiAim->YawStand == 8) // Custom Jitter
		{
			static bool customjitt1 = false;

			if (customjitt1)
				cmd->viewangles.y += (Config::AntiAim->StandCustomAngleYaw);
			else
				cmd->viewangles.y += (Config::AntiAim->StandCustomAngleFakeYaw);

			customjitt1 = !customjitt1;
		}

		else if (Config::AntiAim->YawStand == 9) // Static Custom

		{
			cmd->viewangles.y = (Config::AntiAim->StandStaticModifer);
		}

		else if (Config::AntiAim->YawStand == 10) // Custom Fake
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;
				cmd->viewangles.y += (Config::AntiAim->StandCustomAngleFakeYaw);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += (Config::AntiAim->StandCustomAngleYaw);
			}
		}

		else if (Config::AntiAim->YawStand == 11) // Static FakeLag
		{
			static int choked_tick_count = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;
				cmd->viewangles.y = (Config::AntiAim->StandCustomAngleFakeYaw);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y = (Config::AntiAim->StandCustomAngleYaw);
			}
		}

		else if (Config::AntiAim->YawStand == 12) // Fake Spin
		{
			static int choked_tick_count = 0;
			static float yaw = 0.f;

			yaw += (Config::AntiAim->StandFakeSpinSpeed);

			if (yaw > 360.f)
				yaw = 0.f;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;
				cmd->viewangles.y = (Config::AntiAim->StandFakeSpinAngle);
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;
				cmd->viewangles.y += yaw;

			}
		}

		else if (Config::AntiAim->YawStand == 13) // Fake Spin 2
		{
			static int choked_tick_count = 0;
			static float yaw = 0.f;

			yaw += (Config::AntiAim->StandFakeSpinSpeed * g_iAAHurtDir);

			if (yaw > 360.f)
				yaw = 0.f;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;
				bSendPacket = true;
				cmd->viewangles.y += yaw;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;
				cmd->viewangles.y += yaw + 180;
			}
		}
		else if (Config::AntiAim->YawStand == 14) // Unbalanced Jitter
		{
			// 3:1 unbalanced pattern counted over SENDS: three sends of the
			// custom yaw, one of the fake yaw. Median of recent sends lands on
			// the majority side while the mean sits between - beats median /
			// average feedback resolvers that assume symmetric jitter.
			static int choked_tick_count = 0;
			static int iSendCount = 0;

			if (choked_tick_count == 0)
			{
				choked_tick_count = Config::AntiAim->StandChokedPackets;

				bSendPacket = true;

				if (iSendCount % 4 == 3)
					cmd->viewangles.y += Config::AntiAim->StandCustomAngleFakeYaw * g_iAAHurtDir;
				else
					cmd->viewangles.y += Config::AntiAim->StandCustomAngleYaw * g_iAAHurtDir;

				iSendCount++;
			}
			else
			{
				choked_tick_count--;

				bSendPacket = false;

				cmd->viewangles.y += Config::AntiAim->StandCustomAngleYaw;
			}
		}
	}

	// Flap the duck bit on SEND ticks (final bSendPacket): ducking choked
	// commands never reaches the server, so only sent ticks matter.
	// (Original ducked choked ticks - invisible, did nothing.)
	if (Config::AntiAim->FakeDuck && player->m_MoveType() != MOVETYPE_LADDER)
	{
		static bool bDuckFlip = false;

		if (bSendPacket)
			bDuckFlip = !bDuckFlip;

		if (bDuckFlip)
			cmd->buttons |= IN_DUCK;
	}
}

void PerfectSilent(CUserCmd* cmd, C_WeaponCSBaseGun* weapon)
{
	if (weapon->GetWeaponType() == WEAPONTYPE_GRENADE)
		return;
	if (weapon->GetWeaponType() == WEAPONTYPE_C4)
		return;

	// Force-send firing commands: choking a shot only delays our own damage,
	// the aim snap is animated server-side either way.
	if (cmd->buttons & IN_ATTACK && weapon->IsFireTime())
		bSendPacket = true;

	if (Config::AntiAim->OnKnife && weapon->GetWeaponType() == WEAPON_KNIFE)
	{
			if (cmd->buttons & IN_ATTACK2 && weapon->IsFireTime())
				bSendPacket = true;
	}

}
void AirStuck(CUserCmd* cmd)
{

	auto player = C_CSPlayer::GetLocalPlayer();
	auto weapon = player->GetActiveWeapon();
	if( !Shared::m_bStuck )
		return;
	if ((cmd->buttons & IN_ATTACK) && weapon->IsFireTime())
		return;

	cmd->tick_count = 0xFFFFFF;
}

auto g_recording = false;
auto g_playing = false;

std::vector< CUserCmd > g_usercmd_array = {};

auto g_play_index = 0u;

auto MovementRecord(CUserCmd* usercmd) -> void
{
	if (GetAsyncKeyState(Config::Misc->RecorderRecKey) & 1 & !Shared::m_bMenu)
	{
		if (!g_playing)
		{
			if (!g_recording)
			{
				g_usercmd_array.clear();
				g_play_index = 0u;
			}

			g_recording = !g_recording;
		}
	}

	if (GetAsyncKeyState(Config::Misc->RecorderPlayKey) & 1 )
	{
		if (!g_recording)
		{
			g_playing = !g_playing;
		}
	}

	if (g_recording)
	{
		g_usercmd_array.emplace_back(*usercmd);
	}
	else if (g_playing)
	{
		if (!g_usercmd_array.empty())
		{
			if (g_play_index >= g_usercmd_array.size())
			{
				g_playing = false;
				g_play_index = 0u;
			}
			else
			{
				auto& record = g_usercmd_array[g_play_index];

				usercmd->buttons = record.buttons;
				usercmd->forwardmove = record.forwardmove;
				usercmd->sidemove = record.sidemove;
				usercmd->upmove = record.upmove;
				usercmd->viewangles = record.viewangles;
				if (!Config::Misc->RecorderSilent)
				{
					Source::m_pEngine->SetViewAngles(record.viewangles);
				}
				g_play_index++;
			}
		}
		else
		{
			g_playing = false;
			g_play_index = 0u;
		}
	}
}
void RemoveInterpolation()
{
	// 0x390028 client.dll
	static std::uintptr_t interpolation_list = ( std::uintptr_t )ModuleD( XorStr( "client.dll" ) ) + 0x390028;

	*( std::uint16_t* )( interpolation_list + 12 ) = 0xFFFF;
	*(std::uint16_t*)(interpolation_list + 18) = 0;
}

#include "inetmsghandler.hpp"
auto Lag(CUserCmd* usercmd) -> void
{
	auto netchan = (CNetChan*)Source::m_pEngine->GetNetChannelInfo();
	C_CSPlayer* player;
	auto weapon = player->GetActiveWeapon();
	if (netchan)
	{
		auto command = 0;
		if (Config::Misc->LagExploit == 1)
		{
			if (GetAsyncKeyState(Config::Misc->LagExploitKey))
			{
				bSendPacket = ((usercmd->command_number % 5) == 1);
				command = (MULTIPLAYER_BACKUP * 5);
			}
		}
		if (Config::Misc->LagExploit == 2)
		{
			if (GetAsyncKeyState(Config::Misc->LagExploitKey))
			{
				bSendPacket = ((usercmd->command_number % Config::Misc->test0) == 1);
				command = (MULTIPLAYER_BACKUP * Config::Misc->test);
			}
		}
		if (Config::Misc->LagExploit == 3)
		{
			if (GetAsyncKeyState(Config::Misc->LagExploitKey))
			{
				command = (MULTIPLAYER_BACKUP / Config::Misc->test);
			}
		}
		if (Config::Misc->LagExploit == 4)
		{
			if (GetAsyncKeyState(Config::Misc->LagExploitKey))
			{
				bSendPacket = ((usercmd->command_number % 5) == 1);
				command = (MULTIPLAYER_BACKUP / Config::Misc->test);
			}
		}
		if (Config::Misc->LagExploitSpeed)
		{
			if (GetAsyncKeyState(Config::Misc->LagExploitSpeedKey))
			{
				if ((usercmd->buttons & IN_ATTACK) && weapon->IsFireTime())
					return;
				command = (MULTIPLAYER_BACKUP / 6);
			}
		}
		if (Config::Misc->LagExploitSwitch)
		{
			if (usercmd->weaponselect != 0)
				command = (MULTIPLAYER_BACKUP);
		}
		if (command)
		{
			netchan->m_nOutSequenceNr += command;
		}
	}
}
QAngle angl = {};
QAngle RealAngles = {};
QAngle FakeAngles = {};

void __fastcall CreateMove( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	__try
	{
	//	QAngle qLastTickAngles = QAngle(0.0f, 0.0f, 0.0f);
		Source::m_pClientSwap->VCall< CreateMoveFn >(IBaseClientDLL_CreateMove)(ecx, sequence_number, input_sample_frametime, active);

		auto cmd = Source::m_pInput->GetUserCmd(sequence_number);
		if (Config::Misc->Crash == 1)
		{
			if (GetAsyncKeyState(Config::Misc->CrashKey))
			{
				auto chan = (INetChannel*)Source::m_pEngine->GetNetChannelInfo();
				auto channel = (INetChannelHandler*)Source::m_pEngine->GetNetChannelInfo();

				unsigned short bits = 0;

				Memory::VCall< void(__thiscall*)(void*, bool, int) >(chan, 59)(chan, false, 96000);
				for (int i = 0; i < 200; i++)
				{
					bf_write* buf = (bf_write*)((int)chan + 68);
					if (Config::Misc->CrashRestricion)
					{
						if (buf->GetNumBitsWritten() > Config::Misc->CrashRestricion)
							return;
					}

					buf->WriteUBitLong(10, 5);
					buf->WriteWord(bits);
				}
			}
		}

		if (Config::Misc->Crash == 2)
		{
			if (GetAsyncKeyState(Config::Misc->CrashKey))
			{
				auto chan = (INetChannel*)Source::m_pEngine->GetNetChannelInfo();

				unsigned short bits = -3;

				Memory::VCall< void(__thiscall*)(void*, bool, int) >(chan, 59)(chan, false, 96000);

				for (int i = 0; i < 200; i++)
				{

					bf_write* buf = (bf_write*)((int)chan + 68);
					if (Config::Misc->CrashRestricion)
					{
						if (buf->GetNumBitsWritten() > Config::Misc->CrashRestricion)
							return;
					}
					buf->WriteUBitLong(10, 5);
					buf->WriteWord(bits);

				}
			}
		}

		if (Config::Misc->Crash == 3)
		{
			if (GetAsyncKeyState(Config::Misc->CrashKey))
			{
				auto chan = (INetChannel*)Source::m_pEngine->GetNetChannelInfo();

				unsigned short bits = Config::Misc->Val0;

				Memory::VCall< void(__thiscall*)(void*, bool, int) >(chan, 59)(chan, false, 96000);

				for (int i = 0; i < 200; i++)
				{

					bf_write* buf = (bf_write*)((int)chan + 68);
					if (Config::Misc->CrashRestricion)
					{
						if (buf->GetNumBitsWritten() > Config::Misc->CrashRestricion)
							return;
					}
					buf->WriteUBitLong(Config::Misc->Val1, Config::Misc->Val2);
					buf->WriteWord(bits);

				}
			}
		}
		if (cmd)
		{
			Config::OnCreateMove();
			auto player = C_CSPlayer::GetLocalPlayer();

			if (Source::m_pCvar->FindVar(XorStr("cl_minmodels"))->m_nValue != 1337)
				Source::m_pCvar->FindVar(XorStr("cl_minmodels"))->m_nValue = 1337;
	
			if (player)
			{
				Source::m_pDataManager->PreCreateMove(cmd, player);

				Source::m_pPlayerList->OnCreateMove();

				auto va = cmd->viewangles;
				auto usercmd = cmd;
				if (player->m_lifeState() == LIFE_ALIVE)
				{
					if (Config::Misc->Speed)
						Speed(player,cmd,Vector3());

					if (Config::Misc->Restriction != 1)
					{
						if (Config::Misc->AutoJump)
							AutoJump(cmd, player);

						if (Config::Misc->AutoStrafe)
							AutoStrafe(cmd, player);
					
						if (Config::Misc->AirStuck)
							AirStuck(cmd);


					}

					if (Config::Misc->Recorder)
						MovementRecord(cmd);

				
					auto weapon = player->GetActiveWeapon();
					if (weapon)
					{
					
						auto va = cmd->viewangles;

					
							if (Config::Current->Aimbot->Mode)
								Source::m_pAimbot->OnCreateMove(cmd, weapon);
						
							if (Config::Current->Triggerbot->Mode)
								Source::m_pTriggerbot->OnCreateMove(cmd);
						
						if (Config::Misc->FakePing)
							Source::m_pCvar->FindVar(XorStr("net_fakelag"))->m_nValue = Config::Misc->FakePing;
						else
							Source::m_pCvar->FindVar(XorStr("net_fakelag"))->m_nValue = 0;
						
						if (Config::Misc->FakeLag)
							FakeLag(cmd);

						if (Config::AntiAim->FakeWalk)
							FakeWalk(cmd, player);

						if (Config::Current->Aimbot->NoSpreadActive)
						{
							if (Config::Current->Aimbot->NoSpread)
								NoSpread(cmd, weapon);
						}

						if (Config::Removals->NoRecoil)							
							NoRecoil(cmd, player, weapon);

						if (Config::Misc->Restriction != 1)
						{
							if (Config::Current->Aimbot->Silent)
								PerfectSilent(cmd, weapon);
						}

						if (cmd->buttons & IN_ATTACK && !weapon->IsFireTime())
							cmd->viewangles = va;

						if (Config::Misc->Restriction != 1)
						{
							if (Config::Misc->AutoPistol)
								AutoPistol(cmd, weapon);

						

							if (Config::AntiAim->AtTarget || Config::AntiAim->PitchMove || Config::AntiAim->YawMove || Config::AntiAim->PitchStand || Config::AntiAim->YawStand)
								AntiAim(cmd, player, weapon);
							if (Config::AntiAim->BreakLC)
								BreakLagComp(cmd, player);
							if (Config::Misc->LagExploit)
								Lag(cmd);

							if (bSendPacket)
								RealAngles = QAngle(cmd->viewangles.z, cmd->viewangles.x, cmd->viewangles.y);
							else
								FakeAngles = QAngle(cmd->viewangles.z, cmd->viewangles.x, cmd->viewangles.y);
							
						angl = QAngle(cmd->viewangles.z, cmd->viewangles.x, cmd->viewangles.y);
						}
						if( Config::Misc->AntiSMAC )
							AntiSMAC( cmd );

						Source::MovementFix(cmd, va, false);
					}
				}
				Source::m_pDataManager->PostCreateMove(player);
			}
		}
	}
	
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		Source::m_pClientSwap->VCall< CreateMoveFn >( IBaseClientDLL_CreateMove )( ecx, sequence_number, input_sample_frametime, active );
	}
}

void __declspec( naked ) __fastcall Hooked_CreateMove( void* ecx, void* edx, int sequence_number, float input_sample_frametime, bool active )
{
	__asm
	{
		push    ebp
		mov     ebp, esp

		mov     bSendPacket, bl

		movzx	eax, active
		push	eax
		mov		eax, input_sample_frametime
		push	eax
		mov		eax, sequence_number
		push	eax
		call    CreateMove

		mov     bl, bSendPacket

		mov     esp, ebp
		pop     ebp

		retn    0xC
	}
}

void __fastcall Hooked_FrameStageNotify(void* ecx, void* edx, ClientFrameStage_t curStage)
{
	__try
	{
		Vector3 saved;
		CViewSetup* view;
		auto local = C_CSPlayer::GetLocalPlayer();
		QAngle vecAngles = { };
		if (curStage == FRAME_RENDER_START)
		{
			if (local && local->m_lifeState() == LIFE_ALIVE)	
			{
		
				Source::m_pEngine->GetViewAngles(vecAngles);
				saved = local->m_vecPunchAngle();

				if (Config::Misc->Lag)
				{
					bool boole = false;
					if (GetAsyncKeyState(Config::Misc->LagKey) & !boole)
					{
						boole = true;
					}
					else if (!GetAsyncKeyState(Config::Misc->LagKey) & boole)
					{
						boole = false;
					}
					if (boole)
					{
						if (Source::m_pInput->m_fCameraInThirdPerson == true)
							Source::m_pInput->m_fCameraInThirdPerson = false;
						else
							Source::m_pInput->m_fCameraInThirdPerson = true;
					}

					if (Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue != 7991801)
						Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue = 7991801;
					if (Source::m_pInput->m_fCameraInThirdPerson)
					{
						Source::m_pInput->m_vecCameraOffset = Vector(vecAngles[0], vecAngles[1], 128.0f);
						local->v_angle() = RealAngles;
					}
					else
						Source::m_pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, 0);
				
					
				}
				else
				{
					if (Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue == 7991801)
						Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue = 0;
					Source::m_pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, 0);
					Source::m_pInput->m_fCameraInThirdPerson = false;

				}
				if (Config::Removals->NoVisualRecoil)
					local->m_vecPunchAngle().Set();

				local->m_flFlashMaxAlpha() = (float)Config::Removals->FlashAmount * 2.55f;
			}
			else
			{
				Source::m_pInput->m_fCameraInThirdPerson = false;
			}
		}
		auto& lc = Feature::LagCompensation::Instance();
		auto& lc1 = Feature::LagCompensation1::Instance();
		if (Config::Current->Aimbot->UpdateAnim)
		{
			if (Config::Current->Aimbot->LagCompensation == 1)
			{
				lc.UpdateAnimationData(curStage);
			}
			if (Config::Current->Aimbot->LagCompensation == 2)
			{
				lc1.UpdateAnimationData1(curStage);
			}
			if (Config::Current->Aimbot->LagCompensation == 3)
			{
				lc.UpdateAnimationData(curStage);
				lc1.UpdateAnimationData1(curStage);
			}
		}
		if (curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		{
			if (local)
			{
				if (Config::Misc->FakePing)
				{
					bool tr = true;
					if (tr)
					{
						Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue = 7991802;
						Source::m_pCvar->FindVar(XorStr("net_fakelag"))->m_nValue = Config::Misc->FakePing;
					}
				}
				else
				{
					if (Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue == 7991802)
						Source::m_pCvar->FindVar(XorStr("sv_cheats"))->m_nValue = 0;
					Source::m_pCvar->FindVar(XorStr("net_fakelag"))->m_nValue = 0;
				}
				for (int i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
				{
					auto player = C_CSPlayer::GetPlayer(i);

					if (!player)
						continue;

					if (player == local)
						continue;

					if (player->IsDormant())
						continue;

					if (Config::Current->Aimbot->LagCompensation == 1)
						lc.UpdateLagRecord(player);

					if (Config::Current->Aimbot->LagCompensation == 2)
						lc1.UpdateLagRecord1(player);

					if (Config::Current->Aimbot->LagCompensation == 3)
					{
						lc1.UpdateLagRecord1(player);
						lc.UpdateLagRecord(player);
					}
					if (player->m_lifeState() == LIFE_ALIVE && Config::Current->Aimbot->SetAbs)
					{
						player->SetAbsAngles(player->m_angEyeAngles());
						player->SetAbsOrigin(player->m_vecOrigin());
					}
				
				}
			}
		}

		Source::m_pClientSwap->VCall< FrameStageNotifyFn >(IBaseClientDLL_FrameStageNotify)(ecx, curStage);

		if (curStage == FRAME_RENDER_START)
		{
			if (local)
				local->m_vecPunchAngle() = saved;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Source::m_pClientSwap->VCall< FrameStageNotifyFn >(IBaseClientDLL_FrameStageNotify)(ecx, curStage);
	}
}


void __fastcall Hooked_RunCommand( void* ecx, void* edx, C_CSPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper )
{
	__try
	{
		Source::m_pPredictionSwap->VCall< RunCommandFn >( IPrediction_RunCommand )( ecx, player, ucmd, moveHelper );

		if( player == C_CSPlayer::GetLocalPlayer() )
			Source::m_pDataManager->OnDataUpdate();
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		Source::m_pPredictionSwap->VCall< RunCommandFn >( IPrediction_RunCommand )( ecx, player, ucmd, moveHelper );
	}
}

void __fastcall Hooked_FinishMove( void* ecx, void* edx, C_CSPlayer* player, CUserCmd* ucmd, CMoveData* move )
{
	__try
	{
		Source::m_pPredictionSwap->VCall< FinishMoveFn >( IPrediction_FinishMove )( ecx, player, ucmd, move );

		*( Vector3* )( player + 0xEC ) = player->m_vecVelocity() + player->m_vecBaseVelocity();
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		Source::m_pPredictionSwap->VCall< FinishMoveFn >( IPrediction_FinishMove )( ecx, player, ucmd, move );
	}
}

void __fastcall Hooked_Update( void* ecx, void* edx, bool recieved_new_world_update, bool validframe, int incoming_acknowledged, int outgoing_command )
{
	__try
	{
		Source::m_pPredictionSwap->VCall< UpdateFn >( IPrediction_Update )( ecx, true, validframe, incoming_acknowledged, outgoing_command );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		Source::m_pPredictionSwap->VCall< UpdateFn >( IPrediction_Update )( ecx, recieved_new_world_update, validframe, incoming_acknowledged, outgoing_command );
	}
}

CUserCmd* __fastcall Hooked_GetUserCmd( void* ecx, void* edx, int sequence_number )
{
	__try
	{
		return Source::m_pInput->GetUserCmd( sequence_number );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return nullptr;
	}
}

void __fastcall Hooked_ResetMouse( void* ecx, void* edx )
{
	__try
	{
		if( !Shared::m_bPanic )
		{
			if( Shared::m_bMenu )
				return;
		}

		Source::m_pInputSwap->VCall< ResetMouseFn >( IInput_ResetMouse )( ecx );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		Source::m_pInputSwap->VCall< ResetMouseFn >( IInput_ResetMouse )( ecx );
	}
}

int __fastcall Hooked_DrawModelEx( void* ecx, void* edx, ModelRenderInfo_t* info )
{
	__try
	{
		if( !Shared::m_bPanic )
		{
			if( Config::Render->ChamsMode )
				Source::m_pRender->OnDrawModel( ecx, info );
		
			int iRet = Source::m_pModelRenderSwap->VCall< DrawModelExFn >( IVModelRender_DrawModelEx )( ecx, info );

			Source::m_pRender->ForceMaterial( Direct3D9::Color::Empty, nullptr, false );

			return iRet;
		}

		return Source::m_pModelRenderSwap->VCall< DrawModelExFn >( IVModelRender_DrawModelEx )( ecx, info );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return Source::m_pModelRenderSwap->VCall< DrawModelExFn >( IVModelRender_DrawModelEx )( ecx, info );
	}
}


HRESULT D3DAPI Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	__try
	{
		Source::m_pRenderer->OnLostDevice();
		Source::m_pMenu->OnLostDevice();

		Source::m_pDeviceSwap->Restore();
		auto hRet = Source::m_pDeviceSwap->VCall< ResetFn >(IDirect3DDevice9_Reset)(pDevice, pPresentationParameters);
		Source::m_pDeviceSwap->Replace();

		Source::m_pRenderer->OnResetDevice();
		Source::m_pMenu->OnResetDevice();
		return hRet;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return Source::m_pDeviceSwap->VCall< ResetFn >(IDirect3DDevice9_Reset)(pDevice, pPresentationParameters);
	}
}

		void PaintEntity(C_CSPlayer* player)
		{
			using Direct3D9::Color;

			if (player->IsDormant())
				return;

			if (player->m_lifeState() != LIFE_ALIVE)
				return;

			if (Config::ESP->Target == 1) // Enemy
			{
				if (player->m_iTeamNum() == C_CSPlayer::GetLocalPlayer()->m_iTeamNum())
					return;
			}
			else if (Config::ESP->Target == 2) // Friendly
			{
				if (player->m_iTeamNum() != C_CSPlayer::GetLocalPlayer()->m_iTeamNum())
					return;
			}

			Vector3 head;

			if (!player->GetHitboxVector(12, head))
				return;

			Color color;

			if (player->m_iTeamNum() == 2) // T
			{
				color = Config::Colors->T_ESP_Normal;

				if (Config::ESP->Colored && Source::TraceLine(head, player))
					color = Config::Colors->T_ESP_Colored;
			}
			else if (player->m_iTeamNum() == 3) // CT
			{
				color = Config::Colors->CT_ESP_Normal;

				if (Config::ESP->Colored && Source::TraceLine(head, player))
					color = Config::Colors->CT_ESP_Colored;
			}

			const auto& transform = player->m_rgflCoordinateFrame();

			Vector3 position(transform[0][3], transform[1][3], transform[2][3]);

			Vector3 mins = player->GetMins();
			Vector3 maxs = player->GetMaxs() + Vector3(0.0f, 0.0f, 10.0f);

			Vector3 points[] =
			{
				{ mins.x, mins.y, mins.z },
				{ mins.x, maxs.y, mins.z },
				{ maxs.x, maxs.y, mins.z },
				{ maxs.x, mins.y, mins.z },
				{ maxs.x, maxs.y, maxs.z },
				{ mins.x, maxs.y, maxs.z },
				{ mins.x, mins.y, maxs.z },
				{ maxs.x, mins.y, maxs.z },
			};

			Vector3 transformed[8];

			for (int i = 0; i < 8; i++)
				VectorTransform(points[i], transform, transformed[i]);

			Vector3 flb, brt, blb, frt, frb, brb, blt, flt;

			if (!Source::WorldToScreen(transformed[3], flb) ||
				!Source::WorldToScreen(transformed[0], blb) ||
				!Source::WorldToScreen(transformed[2], frb) ||
				!Source::WorldToScreen(transformed[6], blt) ||
				!Source::WorldToScreen(transformed[5], brt) ||
				!Source::WorldToScreen(transformed[4], frt) ||
				!Source::WorldToScreen(transformed[1], brb) ||
				!Source::WorldToScreen(transformed[7], flt))
				return;

			Vector3 screen[] = { flb, brt, blb, frt, frb, brb, blt, flt };

			float left = flb.x;
			float top = flb.y;
			float right = flb.x;
			float bottom = flb.y;

			for (int i = 0; i < 8; i++)
			{
				if (left > screen[i].x)
					left = screen[i].x;
				if (top < screen[i].y)
					top = screen[i].y;
				if (right < screen[i].x)
					right = screen[i].x;
				if (bottom > screen[i].y)
					bottom = screen[i].y;
			}

			int x = (int)std::round(left);
			int y = (int)std::round(bottom);

			int w = (int)std::round(right - left);
			int h = (int)std::round(top - bottom);

			if (Config::ESP->Box == 1)
			{
				int line_w = (int)std::round(w / 4);
				int line_h = (int)std::round(h / 4);
				if (Config::ESP->Filled)
				{
					Source::m_pRenderer->DrawRect(x, y, line_w * 4 + 2, line_h * 4 + 2, Color(color[0], color[1], color[2], 90));
				}
				if (Config::ESP->Outlined)
					Source::m_pRenderer->DrawBorderBoxOut(x, y, w, h, 1, color, Color(0, 0, 0, color.A));
				else
					Source::m_pRenderer->DrawBorderBox(x, y, w, h, 1, color);
			}
		
			else if (Config::ESP->Box == 2)
			{
				int line_w = (int)std::round(w / 4);
				int line_h = (int)std::round(h / 4);
				if (Config::ESP->Filled)
				{
					Source::m_pRenderer->DrawRect(x, y, line_w * 4 + 2, line_h * 4 + 2, Color(color[0], color[1], color[2], 90));
				}
				if (Config::ESP->Outlined)
				{
					Source::m_pRenderer->DrawRect(x - 1, y - 1, line_w + 2, 3, Color(0, 0, 0, color.A)); // top left -> right
					Source::m_pRenderer->DrawRect(x - 1, y - 1, 3, line_h + 2, Color(0, 0, 0, color.A)); // top left -> bottom

					Source::m_pRenderer->DrawRect(x - 1, y + h - line_h, 3, line_h + 2, Color(0, 0, 0, color.A)); // bottom left -> top
					Source::m_pRenderer->DrawRect(x - 1, y + h - 1, line_w + 2, 3, Color(0, 0, 0, color.A)); // bottom left -> right

					Source::m_pRenderer->DrawRect(x + w - line_w, y - 1, line_w + 2, 3, Color(0, 0, 0, color.A)); // top right -> left
					Source::m_pRenderer->DrawRect(x + w - 1, y - 1, 3, line_h + 2, Color(0, 0, 0, color.A)); // top right -> bottom

					Source::m_pRenderer->DrawRect(x + w - line_w, y + h - 1, line_w + 2, 3, Color(0, 0, 0, color.A)); // bottom right -> left
					Source::m_pRenderer->DrawRect(x + w - 1, y + h - line_h, 3, line_h + 2, Color(0, 0, 0, color.A)); // bottom right -> top
				}

				Source::m_pRenderer->DrawRect(x, y, line_w, 1, color); // top left -> right
				Source::m_pRenderer->DrawRect(x, y, 1, line_h, color); // top left -> bottom

				Source::m_pRenderer->DrawRect(x, y + h - line_h + 1, 1, line_h, color); // bottom left -> top
				Source::m_pRenderer->DrawRect(x, y + h, line_w, 1, color); // bottom left -> right

				Source::m_pRenderer->DrawRect(x + w - line_w + 1, y, line_w, 1, color); // top right -> left
				Source::m_pRenderer->DrawRect(x + w, y, 1, line_h, color); // top right -> bottom

				Source::m_pRenderer->DrawRect(x + w - line_w + 1, y + h, line_w, 1, color); // bottom right -> left
				Source::m_pRenderer->DrawRect(x + w, y + h - line_h + 1, 1, line_h, color); // bottom right -> top
			}

			int pad_h = 0;

			if (Config::ESP->Name)
			{
				player_info_t data;

				if (Source::m_pEngine->GetPlayerInfo(player->GetIndex(), &data))
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w / 2, y - 16, FONT_ALIGN_CENTER_H, Color::White, data.name);
			}

			if (Config::ESP->Weapon)
			{
				auto weapon = player->GetActiveWeapon();

				if (weapon)
				{
					auto name = weapon->GetName();

					if (name)
						Source::m_pRenderer->DrawText(Source::m_hFont, x + w / 2, y + h + 2, FONT_ALIGN_CENTER_H, Color::White, "%s", (name + 7));
				}
			}

			if (Config::ESP->AimSpot)
			{
				Vector3 spot;

				if (player->GetHitboxVector(Config::Current->Aimbot->Spot, spot))
				{
					Vector3 screen;
					if (Source::WorldToScreen(spot, screen))
						Source::m_pRenderer->DrawRectOut(screen.x - 1, screen.y - 1, 3, 3, Color::White);
				}
			}

			if (Config::ESP->Health == 1) // Text
			{
				int pad_w = 0;

				if (Config::ESP->Armor == 2)
					pad_w += 5;

				Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y, FONT_ALIGN_LEFT, Color::White, "H: %i", player->m_iHealth());

				pad_h += 12;
			}
			else if (Config::ESP->Health == 2) // Bar
			{
				int health = player->m_iHealth();

				if (health > 100)
					health = 100;

				int size_h = (int)std::round(((h + 1) * health) / 100);
				int real_h = (h + 1) - size_h;

				Source::m_pRenderer->DrawRect(x - 6, y - 1, 4, h + 3, Color::Black);
				Source::m_pRenderer->DrawRect(x - 5, y + real_h, 2, size_h, Color(255 - (player->m_iHealth() * 2.55f), player->m_iHealth() * 2.55f, 0));
			}

			if (Config::ESP->Armor == 1) // Text
			{
				Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4, y + pad_h, FONT_ALIGN_LEFT, Color::White, "A: %i", player->m_ArmorValue());

				pad_h += 13;
			}
			else if (Config::ESP->Armor == 2) // Bar
			{
				int armor = player->m_ArmorValue();

				if (armor > 100)
					armor = 100;

				int size_h = (int)std::round(((h + 1) * armor) / 100);
				int real_h = (h + 1) - size_h;

				Source::m_pRenderer->DrawRect(x + w + 3, y - 1, 4, h + 3, Color::Black);
				Source::m_pRenderer->DrawRect(x + w + 4, y + real_h, 2, size_h, Color(180, 180, 180));
			}

			auto resolver_text = Feature::Resolver::GetText( player->GetIndex() );

			if( resolver_text && resolver_text[ 0 ] )
			{
				Source::m_pRenderer->DrawText( Source::m_hFont, x + w + 4, y + pad_h, FONT_ALIGN_LEFT, Color( 0, 255, 255 ), "R: %s", resolver_text );

				pad_h += 13;
			}

			if (Config::ESP->Skeleton == 1)
			{
				auto pModel = player->GetModel();

				if (pModel)
				{
					auto pStudioHdr = Source::m_pModelInfoClient->GetStudioModel(pModel);

					if (pStudioHdr)
					{
						for (auto i = 0; i < pStudioHdr->numbones; i++)
						{
							auto pBone = pStudioHdr->GetBone(i);

							if (!pBone)
								continue;

							if (!(pBone->flags & 0x100))
								continue;

							if (pBone->parent == -1)
								continue;

							Vector3 vBone, vParent;
							Vector3 vBoneS, vParentS;

							player->GetBoneVector(i, vBone);
							player->GetBoneVector(pBone->parent, vParent);

							if (Source::WorldToScreen(vBone, vBoneS) && Source::WorldToScreen(vParent, vParentS))
								Source::m_pRenderer->DrawLine(vBoneS.x, vBoneS.y, vParentS.x, vParentS.y, color);
						}
					}
				}
			}

			if (Config::ESP->Skeleton == 2)
			{
				matrix3x4_t transform[128] = {};

				if (player->SetupBones(transform, 128, 0x0100, Source::m_pEngine->GetLastTimeStamp()))
				{
					RenderSkeleton(player, transform, color);

					if (Config::Current->Aimbot->LagCompensation)
					{
						auto& lc = Feature::LagCompensation::Instance();
						auto& lc1 = Feature::LagCompensation1::Instance();

						auto index = (player->GetIndex() - 1);
						auto& record_data = lc.m_LagRecord[index];
						auto& record_data1 = lc1.m_LagRecord[index];
						if (Config::Current->Aimbot->LagCompensation == 1)
						{
							for (auto& record : record_data)
								RenderSkeleton(player, record.m_BoneTransform.data(), color);
						}
						if (Config::Current->Aimbot->LagCompensation == 2)
						{
							for (auto& record1 : record_data1)
								RenderSkeleton(player, record1.m_BoneTransform.data(), color);
						}
					}
				}
			}

			if (Config::ESP->Defusing)
			{
				if (Config::ESP->Name)
					y -= 16;

				if (player->m_bIsDefusing())
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w / 2, y - 16, FONT_ALIGN_CENTER_H, Color::White, XorStr("Defusing!"));
			}

		}


float ScaleDamage( float damage, int armor )
{
	float ratio = 0.5f;
	float bonus = 0.5f;

	if( armor > 0 )
	{
		float new_damage = damage * ratio;
		float new_armor = ( damage - new_damage ) * bonus;

		if( new_armor > static_cast< float >( armor ) )
		{
			new_armor = static_cast< float >( armor ) * ( 1.0f / bonus );
			new_damage = damage - new_armor;
		}

		damage = new_damage;
	}

	return damage;
}

void PaintGround(C_BaseEntity* ent)
{
	using Direct3D9::Color;

	if (ent->IsDormant())
		return;

	auto name = ent->GetClientClass()->m_pNetworkName;

	auto world = ent->m_vecOrigin();

	if (world.IsZero())
		return;

	if (std::strcmp(name, XorStr("CPlantedC4")) == 0)
	{
		if (Config::ESP->Bomb)
		{
			Vector3 screen;

			if (Source::WorldToScreen(world, screen))
				Source::m_pRenderer->DrawText(Source::m_hFont, screen.x, screen.y, FONT_ALIGN_LEFT, Color::Red, XorStr("[ C4 ]"));
		}

		if (Config::Misc->BombWarning)
		{
			auto player = C_CSPlayer::GetLocalPlayer();

			Vector3 bomb_origin = ent->m_vecOrigin();
			Vector3 spot_top(bomb_origin.x, bomb_origin.y, bomb_origin.z + 8.0f);
			Vector3 spot_bottom(spot_top.x, spot_top.y, spot_top.z - 40.0f);

			CTraceFilterSimple trace(ent);

			Ray_t ray;
			ray.Set(spot_top, spot_bottom);

			trace_t tr;
			Source::m_pEngineTrace->TraceRay(ray, 0x200400B, &trace, &tr);

			float bomb_radius = 500.0f;

			if (tr.fraction != 1.0f)
				bomb_origin = tr.endpos + tr.plane.normal * 0.6f;

			float damage = bomb_radius;
			float radius = bomb_radius * 3.5f;

			float adjusted_damage, falloff, damage_percentage;

			Vector3 src = bomb_origin;
			src.z += 1.0f;

			Vector3 end = player->EyePosition();

			damage_percentage = 1.0f;
			falloff = (radius != 0.0f) ? damage / radius : 1.0f;

			Vector3 target = end - src;

			adjusted_damage = (damage - target.Length() * falloff) * damage_percentage;
			adjusted_damage = ScaleDamage(adjusted_damage, player->m_ArmorValue());

			if (adjusted_damage > 0.0f)
			{
				int w, h;
				Source::m_pEngine->GetScreenSize(w, h);

				w /= 2;

				Color color(Color::Red);

				if (adjusted_damage < 80.0f)
					color = Color::Yellow;
				else if (adjusted_damage < 20.0f)
					color = Color::Green;

				Source::m_pRenderer->DrawText(Source::m_hFont, w, 5.0f, FONT_ALIGN_CENTER_H, color, XorStr("Explosion Warning! [%.2f]"), adjusted_damage);
			}
		}
	}
}	

void ESP()
{
	auto player = C_CSPlayer::GetLocalPlayer();

	if (!player)
		return;

	int size = Source::m_pEntList->GetHighestEntityIndex();

	for (int i = 0; i <= size; i++)
	{
		auto ent = Source::m_pEntList->GetBaseEntity(i);

		if (!ent)
			continue;

		PaintGround(ent);

		auto enemy = ToCSPlayer(ent);

		if (!enemy)
			continue;

		if (enemy == player)
		{
			if (Config::Misc->Lag && player->m_lifeState() == LIFE_ALIVE && Source::m_pInput->m_fCameraInThirdPerson == true)
			{
				const auto& transform = player->m_rgflCoordinateFrame();

				Vector3 position(transform[0][3], transform[1][3], transform[2][3]);

				Vector3 mins = player->GetMins();
				Vector3 maxs = player->GetMaxs() + Vector3(0.0f, 0.0f, 10.0f);

				Vector3 points[] =
				{
					{ mins.x, mins.y, mins.z },
					{ mins.x, maxs.y, mins.z },
					{ maxs.x, maxs.y, mins.z },
					{ maxs.x, mins.y, mins.z },
					{ maxs.x, maxs.y, maxs.z },
					{ mins.x, maxs.y, maxs.z },
					{ mins.x, mins.y, maxs.z },
					{ maxs.x, mins.y, maxs.z },
				};

				Vector3 transformed[8];

				for (int i = 0; i < 8; i++)
					VectorTransform(points[i], transform, transformed[i]);

				Vector3 flb, brt, blb, frt, frb, brb, blt, flt;

				if (!Source::WorldToScreen(transformed[3], flb) ||
					!Source::WorldToScreen(transformed[0], blb) ||
					!Source::WorldToScreen(transformed[2], frb) ||
					!Source::WorldToScreen(transformed[6], blt) ||
					!Source::WorldToScreen(transformed[5], brt) ||
					!Source::WorldToScreen(transformed[4], frt) ||
					!Source::WorldToScreen(transformed[1], brb) ||
					!Source::WorldToScreen(transformed[7], flt))
					return;

				Vector3 screen[] = { flb, brt, blb, frt, frb, brb, blt, flt };

				float left = flb.x;
				float top = flb.y;
				float right = flb.x;
				float bottom = flb.y;

				for (int i = 0; i < 8; i++)
				{
					if (left > screen[i].x)
						left = screen[i].x;
					if (top < screen[i].y)
						top = screen[i].y;
					if (right < screen[i].x)
						right = screen[i].x;
					if (bottom > screen[i].y)
						bottom = screen[i].y;
				}

				int x = (int)std::round(left);
				int y = (int)std::round(bottom);

				int w = (int)std::round(right - left);
				int h = (int)std::round(top - bottom);
				using Direct3D9::Color;
				if (player->IsDormant())
					return;

				if (player->m_lifeState() != LIFE_ALIVE)
					return;

				if (!bSendPacket)
				{
					int pad_w = 0;

					if (Config::ESP->Armor == 2)
						pad_w += 5;
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y, FONT_ALIGN_LEFT, Color::Red, "X:%4.2f", angl[1]);
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y + 12, FONT_ALIGN_LEFT, Color::Red, "Y:%4.2f", angl[2]);
				}
				else
				{
					int pad_w = 0;

					if (Config::ESP->Armor == 2)
						pad_w += 5;
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y, FONT_ALIGN_LEFT, Color::Green, "X:%4.2f", angl[1]);
					Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y + 12, FONT_ALIGN_LEFT, Color::Green, "Y:%4.2f", angl[2]);
				}
			}
			else
			{
				continue;
			}
		}

		PaintEntity(enemy);
	}
}
/*
int size = Source::m_pEntList->GetHighestEntityIndex();

for (int i = 0; i <= size; i++)
{
auto ent = Source::m_pEntList->GetBaseEntity(i);

if (!ent)
continue;

PaintGround(ent);

auto enemy = ToCSPlayer(ent);

if (!enemy)
continue;
const auto& transform = player->m_rgflCoordinateFrame();

Vector3 position(transform[0][3], transform[1][3], transform[2][3]);

Vector3 mins = player->GetMins();
Vector3 maxs = player->GetMaxs() + Vector3(0.0f, 0.0f, 10.0f);

Vector3 points[] =
{
{ mins.x, mins.y, mins.z },
{ mins.x, maxs.y, mins.z },
{ maxs.x, maxs.y, mins.z },
{ maxs.x, mins.y, mins.z },
{ maxs.x, maxs.y, maxs.z },
{ mins.x, maxs.y, maxs.z },
{ mins.x, mins.y, maxs.z },
{ maxs.x, mins.y, maxs.z },
};

Vector3 transformed[8];

for (int i = 0; i < 8; i++)
VectorTransform(points[i], transform, transformed[i]);

Vector3 flb, brt, blb, frt, frb, brb, blt, flt;

if (!Source::WorldToScreen(transformed[3], flb) ||
!Source::WorldToScreen(transformed[0], blb) ||
!Source::WorldToScreen(transformed[2], frb) ||
!Source::WorldToScreen(transformed[6], blt) ||
!Source::WorldToScreen(transformed[5], brt) ||
!Source::WorldToScreen(transformed[4], frt) ||
!Source::WorldToScreen(transformed[1], brb) ||
!Source::WorldToScreen(transformed[7], flt))
return;

Vector3 screen[] = { flb, brt, blb, frt, frb, brb, blt, flt };

float left = flb.x;
float top = flb.y;
float right = flb.x;
float bottom = flb.y;

for (int i = 0; i < 8; i++)
{
if (left > screen[i].x)
left = screen[i].x;
if (top < screen[i].y)
top = screen[i].y;
if (right < screen[i].x)
right = screen[i].x;
if (bottom > screen[i].y)
bottom = screen[i].y;
}

int x = (int)std::round(left);
int y = (int)std::round(bottom);

int w = (int)std::round(right - left);
int h = (int)std::round(top - bottom);
using Direct3D9::Color;
if (enemy == player)
{
if (Config::Misc->Lag)
{
if (player->IsDormant())
return;

if (player->m_lifeState() != LIFE_ALIVE)
return;

if (!bSendPacket)
{
int pad_w = 0;

if (Config::ESP->Armor == 2)
pad_w += 5;
Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y, FONT_ALIGN_LEFT, Color::Red, "X:%0.f", angl[1], angl[2]);
Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y + 12, FONT_ALIGN_LEFT, Color::Red, "Y:%0.f", angl[1], angl[2]);
}
else
{
int pad_w = 0;

if (Config::ESP->Armor == 2)
pad_w += 5;
Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y, FONT_ALIGN_LEFT, Color::Green, "X:%0.f", RealAngles[1]);
Source::m_pRenderer->DrawText(Source::m_hFont, x + w + 4 + pad_w, y + 12, FONT_ALIGN_LEFT, Color::Green, "Y:%0.f", RealAngles[2]);
}
}
else
continue;
}
PaintEntity(enemy);

}
*/
void Crosshair()
{
	using Direct3D9::Color;

	auto player = C_CSPlayer::GetLocalPlayer();

	if (!player)
		return;

	
	int w, h;
	Source::m_pEngine->GetScreenSize(w, h);

	int x = w / 2;
	int y = h / 2;
	auto& hitmarker = Feature::HitMarker::Instance();
	hitmarker.Present();
		if (Config::Misc->ShowRecoil)
	{
		int dx, dy;
		if (Config::Removals->NoVisualRecoil)
		{
			dx = w / 74;
			dy = h / 47;

			x -= (dx * player->m_vecPunchAngle().y);
			y += (dy * player->m_vecPunchAngle().x);
		}
		if (Config::Removals->NoVisualRecoil == 0)
		{
			dx = w / 129;
			dy = h / 97;

			x -= (dx * player->m_vecPunchAngle().y);
			y += (dy * player->m_vecPunchAngle().x);
		}

	}
	if (Config::Misc->Crosshair == 1) // Dot
	{
		if (Config::Misc->Outlined)
			Source::m_pRenderer->DrawRect(x - 2, y - 2, 4, 4, Color::Black);

		Source::m_pRenderer->DrawRect(x - 1, y - 1, 2, 2, Config::Colors->Crosshair);
	}
	else if (Config::Misc->Crosshair == 2) // Cross
	{
		if (Config::Misc->Outlined)
		{
			Source::m_pRenderer->DrawRect(x - 11, y - 1, 23, 3, Color::Black);
			Source::m_pRenderer->DrawRect(x - 1, y - 11, 3, 23, Color::Black);
		}

		Source::m_pRenderer->DrawRect(x - 10, y, 21, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y - 10, 1, 21, Config::Colors->Crosshair);
	}
	else if (Config::Misc->Crosshair == 3) // Swastika
	{
		if (Config::Misc->Outlined)
		{
			Source::m_pRenderer->DrawRect(x - 11, y - 1, 23, 3, Color::Black);
			Source::m_pRenderer->DrawRect(x - 1, y - 11, 3, 23, Color::Black);

			Source::m_pRenderer->DrawRect(x - 1, y - 11, 13, 3, Color::Black); // top -> left
			Source::m_pRenderer->DrawRect(x - 11, y - 11, 3, 11, Color::Black); // top -> bottom
			Source::m_pRenderer->DrawRect(x + 9, y, 3, 12, Color::Black); // right -> bottom
			Source::m_pRenderer->DrawRect(x - 11, y + 9, 11, 3, Color::Black); // bottom -> left
		}

		Source::m_pRenderer->DrawRect(x - 10, y, 21, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y - 10, 1, 21, Config::Colors->Crosshair);

		Source::m_pRenderer->DrawRect(x, y - 10, 11, 1, Config::Colors->Crosshair); // top -> left
		Source::m_pRenderer->DrawRect(x - 10, y - 10, 1, 10, Config::Colors->Crosshair); // top -> bottom
		Source::m_pRenderer->DrawRect(x + 10, y, 1, 11, Config::Colors->Crosshair); // right -> bottom
		Source::m_pRenderer->DrawRect(x - 10, y + 10, 10, 1, Config::Colors->Crosshair); // bottom -> left
	}
	else if (Config::Misc->Crosshair == 4) // Ikaros
	{
		if (Config::Misc->Outlined)
		{
			Source::m_pRenderer->DrawRect(x - 5, y - 1, 11, 3, Color::Black);
			Source::m_pRenderer->DrawRect(x - 1, y - 5, 3, 11, Color::Black);
		}

		Source::m_pRenderer->DrawRect(x - 4, y, 9, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y - 4, 1, 9, Config::Colors->Crosshair);
	}
	else if (Config::Misc->Crosshair == 5) // Default
	{
		if (Config::Misc->Outlined)
		{
			Source::m_pRenderer->DrawRectOut(x - 10, y, 7, 1, Config::Colors->Crosshair, Color::Black);
			Source::m_pRenderer->DrawRectOut(x, y - 10, 1, 7, Config::Colors->Crosshair, Color::Black);


			Source::m_pRenderer->DrawRectOut(x + 5, y, 7, 1, Config::Colors->Crosshair, Color::Black);
			Source::m_pRenderer->DrawRectOut(x, y + 5, 1, 7, Config::Colors->Crosshair, Color::Black);
		}
		Source::m_pRenderer->DrawRect(x - 10, y, 7, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y - 10, 1, 7, Config::Colors->Crosshair);


		Source::m_pRenderer->DrawRect(x + 5, y, 7, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y + 5, 1, 7, Config::Colors->Crosshair);

	}
	else if (Config::Misc->Crosshair == 6) // Aimware
	{
		Source::m_pRenderer->DrawRect(x - 15, y, 7, 1, Color::Red);
		Source::m_pRenderer->DrawRect(x, y - 15, 1, 7, Color::Red);

		Source::m_pRenderer->DrawRect(x - 10, y, 11, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y - 10, 1, 11, Config::Colors->Crosshair);

		Source::m_pRenderer->DrawRect(x + 10, y, 7, 1, Color::Red);
		Source::m_pRenderer->DrawRect(x, y + 10, 1, 7, Color::Red);

		Source::m_pRenderer->DrawRect(x, y, 11, 1, Config::Colors->Crosshair);
		Source::m_pRenderer->DrawRect(x, y, 1, 11, Config::Colors->Crosshair);
	}
	if (Config::ESP->Fov)
	{
		// В легите FOV-лимит действует при любом TargetSelection — круг тоже.
		if (Config::Current->Aimbot->TargetSelection != 2 && Config::Main->AimbotStyle != 1)
			return;
		if (Config::Current->Aimbot->FieldOfView < 0.005)
			return;
		int xs;
		int ys;
		float FoV;
		std::vector<int> HitBoxesToScan;

		auto weapon = player->GetActiveWeapon();

		if (!weapon)
			return;

		FoV = (Config::Current->Aimbot->FieldOfView);

		Source::m_pEngine->GetScreenSize(xs, ys);
		xs /= 2; ys /= 2;

		Source::m_pRenderer->DrawCircle1(xs, ys, FoV * 8.5, FoV * 8.5, Config::Colors->Crosshair);
	}
	if (Config::ESP->Spread)
	{
		if (!player)
			return;

		auto weapon = player->GetActiveWeapon();

		if (!weapon)
			return;

		int xs;
		int ys;
		Source::m_pEngine->GetScreenSize(xs, ys);
		xs /= 2; ys /= 2;
		if (weapon->GetSpread() > 0)
		{
			if (Config::Current->Aimbot->NoSpread == 0)
			{
				Source::m_pRenderer->DrawCircle1(xs, ys, weapon->GetSpread() * 1500, weapon->GetSpread() * 1500, Config::Colors->Crosshair);
			}
			else
			{
				Source::m_pRenderer->DrawCircle1(xs, ys, 5, 5, Config::Colors->Crosshair);
			}
		}
	}

}


void Menu()
{
	Source::m_pMenu->OnPresentDevice();
}

int get_fps()
{
	using namespace std::chrono;
	static int count = 0;
	static auto last = high_resolution_clock::now();
	auto now = high_resolution_clock::now();
	static int fps = 0;

	count++;

	if (duration_cast<milliseconds>(now - last).count() > 1000) {
		fps = count;
		count = 0;
		last = now;
	}

	return fps;
}

void Watermark()
{
	static float rainbow; rainbow += 0.00025f; if
		(rainbow > 1.f) rainbow = 0.f;
	int w, h;

		Source::m_pEngine->GetScreenSize(w, h);
		auto net_channel = Source::m_pEngine->GetNetChannelInfo();
		auto player = C_CSPlayer::GetLocalPlayer();
		auto ping = net_channel->GetLatency(FLOW_OUTGOING) * 1000;
		Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 2.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("$$$ WEED $$$"));
		Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 14.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("PING: %0.f"), ping);
		Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 26.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("FPS: %3d"), get_fps());
		if (Config::Misc->Recorder)
		{
			if (g_recording)
			{
				Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 38.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("MR: RECORDING"));
			}
			if (g_playing)
			{
				Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 38.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("MR: PLAYING"));
			}
			if (!g_playing & !g_recording)
			{
				Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 38.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("MR: IDLING"));
			}

			Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 50.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("P: %f"), angl[1]);
			Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 62.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("Y: %f"), angl[2]);
		}
		else
		{
	//		Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 38.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("P: %0.f"), angl[1]);
	//		Source::m_pRenderer->DrawText(Source::m_hFont, (float)w - 5.0f, 50.0f, FONT_ALIGN_RIGHT, Direct3D9::Color::FromHSB(rainbow, 1.f, 1.f), XorStr("Y: %0.f"), angl[2]);
		}
}

void PresentProxy()
{
	__try
	{
		Source::m_pRenderer->Begin();

		if (!Shared::m_bPanic)
		{
			ESP();
			Crosshair();
			Menu();
			Watermark();
			
		}

		Source::m_pRenderer->End();
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
{
		
		}
}

HRESULT D3DAPI Hooked_Present( IDirect3DDevice9* device, const RECT* source_rect, const RECT* dest_rect, HWND dest_window_override, const RGNDATA* dirty_region )
{
	PresentProxy();
	return Source::m_pDeviceSwap->VCall< PresentFn >( IDirect3DDevice9_Present )( device, source_rect, dest_rect, dest_window_override, dirty_region );
}
/*
void __declspec( naked ) Hooked_Present()
{
	static std::uintptr_t uJumpBack = 0;

	uJumpBack = Source::m_pPresentSwap->GetReturnLocation();

	__asm
	{
		call PresentProxy

		and esp, 0xFFFFFFF8
		sub esp, 0x0C

		jmp uJumpBack
	}
}
*/
void Hooked_CL_RunPrediction( PREDICTION_REASON reason )
{
	static auto s_nSignonState = *( std::uintptr_t* )( Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "83 3D ?? ?? ?? ?? ?? A3 ?? ?? ?? ?? 75 47" ) ) + 2 );
	static auto s_nDeltaTick = *( std::uintptr_t* )( Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "83 3D ?? ?? ?? ?? ?? 7C 3E 8B 0D" ) ) + 2 );
	static auto s_last_command_ack = *( std::uintptr_t* )( Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "A1 ?? ?? ?? ?? 56 50 A1" ) ) + 1 );
	static auto s_lastoutgoingcommand = *( std::uintptr_t* )( Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 11 53 56" ) ) + 1 );
	static auto s_chokedcommands = *( std::uintptr_t* )( Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "8B 35 ?? ?? ?? ?? 03 F0 A1 ?? ?? ?? ??" ) ) + 2 );

	int nSignonState = *( int* )s_nSignonState;
	int nDeltaTick = *( int* )s_nDeltaTick;
	int last_command_ack = *( int* )s_last_command_ack;
	int lastoutgoingcommand = *( int* )s_lastoutgoingcommand;
	int chokedcommands = *( int* )s_chokedcommands;

	if( !( nSignonState == 6 ) )
		return;

	if( nDeltaTick < 0 )
		return;

	bool valid = nDeltaTick > 0;

	Source::m_pPrediction->Update( nDeltaTick, valid, last_command_ack, lastoutgoingcommand + chokedcommands );
}

void DT_BasePlayer_m_nTickBase( const CRecvProxyData* pData, void* pStruct, void* pOut )
{
	*( int* )( pOut ) = pData->m_Value.m_Int;

	if( pStruct == C_CSPlayer::GetLocalPlayer() )
		Source::m_pDataManager->OnDataRecieved();
}

void DT_BasePlayer_m_vecPunchAngle( const CRecvProxyData* pData, void* pStruct, void* pOut )
{
	*( Vector3* )( pOut ) = pData->m_Value.m_Vector;
}

void DT_ParticleSmokeGrenade_m_flSpawnTime( const CRecvProxyData* pData, void* pStruct, void* pOut )
{
	float Value = pData->m_Value.m_Float;

	if( Config::Removals->NoSmoke )
		Value = 0.0f;

	*( float* )( pOut ) = Value;
}
void DT_CSPlayer_m_angEyeAnglesX( const CRecvProxyData* pData, void* pStruct, void* pOut )
{
	float angle = pData->m_Value.m_Float;

	auto player = ( C_CSPlayer* )pStruct;
	auto player_from_list = Source::m_pPlayerList->GetPlayer( player->GetIndex() );

	if( player && player_from_list )
	{
		if( player_from_list->m_pitch == 1 ) // Zero
			angle = 0.0f;
		else if( player_from_list->m_pitch == 2 ) // Up
			angle = -89.0f;
		else if( player_from_list->m_pitch == 3 ) // Down
			angle = 89.0f;

		else if( player_from_list->m_pitch == 4 ) // Auto
			angle = Feature::Resolver::ResolvePitch( player, angle );
	}

	*( float* )( pOut ) = angle;
}
void DT_CSPlayer_m_angEyeAnglesY(const CRecvProxyData* pData, void* pStruct, void* pOut)
{
	float angle = pData->m_Value.m_Float;

	auto local = C_CSPlayer::GetLocalPlayer();
	auto player = (C_CSPlayer*)pStruct;
	auto player_from_list = Source::m_pPlayerList->GetPlayer(player->GetIndex());
	if (local && player && player_from_list)
	{
		if (player_from_list->m_yaw == 1) // Zero
		{
			angle = 0.0f;
		}
		else if (player_from_list->m_yaw == 2) // Reversed
		{
			angle += 180.0f;
		}
		else if (player_from_list->m_yaw == 3) // Forward ( At Local )
		{
			Vector3 start;

			if (player->GetHitboxVector(12, start))
			{
				Vector3 direction = local->EyePosition() - start;
				VectorNormalize(direction);

				Vector3 aim;
				VectorAngles(direction, aim);

				angle = aim.y;
			}
		}
		else if (player_from_list->m_yaw == 4) // Backward ( At Local )
		{
			Vector3 start;

			if (player->GetHitboxVector(12, start))
			{
				Vector3 direction = local->EyePosition() - start;
				VectorNormalize(direction);

				Vector3 aim;
				VectorAngles(direction, aim);

				angle = aim.y + 180.0f;
			}
		}
		else if (player_from_list->m_yaw == 5) // Sideway Left
		{
			Vector3 start;

			if (player->GetHitboxVector(12, start))
			{
				Vector3 direction = local->EyePosition() - start;
				VectorNormalize(direction);

				Vector3 aim;
				VectorAngles(direction, aim);

				angle = aim.y + 90.0f;
			}
		}
		else if (player_from_list->m_yaw == 6) // Sideway Right
		{
			Vector3 start;

			if (player->GetHitboxVector(12, start))
			{
				Vector3 direction = local->EyePosition() - start;
				VectorNormalize(direction);

				Vector3 aim;
				VectorAngles(direction, aim);

				angle = aim.y - 90.0f;
			}
		}
		else if( player_from_list->m_yaw == 7 ) // Auto
			angle = Feature::Resolver::ResolveYaw( player, angle );
		else if( player_from_list->m_yaw == 8 ) // Resolver
			angle = Feature::Resolver::ResolveYaw( player, angle );

	}

	*(float*)(pOut) = angle;
}
	