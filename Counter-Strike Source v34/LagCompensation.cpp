#include "LagCompensation.hpp"
#include "Source.hpp"

#include "Player.hpp"
#include "Weapon.hpp"

#include "Config.hpp"

namespace Feature
{
	
	auto LagCompensation::UpdateLagRecord(C_CSPlayer* player) -> void
	{
		auto index = (player->GetIndex() - 1);

		auto record = LagRecord{};
		auto& record_data = m_LagRecord[index];

		player->InvalidateBoneCache();

		record.m_fFlags = player->m_fFlags();
		record.m_flSimulationTime = player->m_flSimulationTime();
		record.m_vecMins = player->OBBMins();
		record.m_vecMaxs = player->OBBMaxs();
		record.m_vecVelocity = player->m_vecVelocity();
		record.m_vecOrigin = player->m_vecOrigin();
		record.m_angEyeAngles = player->m_angEyeAngles();
		record.m_flPoseParameter = player->m_flPoseParameter();

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				record.m_LayerRecord[i].m_nOrder = layer->m_nOrder;
				record.m_LayerRecord[i].m_nSequence = layer->m_nSequence;
				record.m_LayerRecord[i].m_flWeight = layer->m_flWeight;
				record.m_LayerRecord[i].m_flCycle = layer->m_flCycle;
			}
		}

		player->SetupBones(record.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp());

		if (!record_data.empty())
		{
			for (auto i = 0u; i < record_data.size(); i++)
			{
				if (!IsRecordGood(record_data[i]))
					record_data.erase(record_data.begin() + i);
			}

			for (const auto& record_current : record_data)
			{
				if (record.m_flSimulationTime <= record_current.m_flSimulationTime)
					record.m_flSimulationTime = 0.f;
			}
		}

		if (IsRecordGood(record))
			record_data.emplace_back(record);
	}

	auto LagCompensation::UpdateAnimationData(ClientFrameStage_t stage) -> void
	{
		auto local = C_CSPlayer::GetLocalPlayer();

		if (!local)
			return;

		if (local->m_lifeState() != LIFE_ALIVE)
			return;

		static int userID[64] = {};
		static C_AnimationLayer backup_layers_update[64][15] = {};
		static C_AnimationLayer backup_layers_interp[64][15] = {};

		for (int i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
		{
			auto player = C_CSPlayer::GetPlayer(i);

			if (!player)
				continue;

			if (player == local)
				continue;

			if (player->IsDormant())
				continue;

			player_info_t info = {};

			if (!Source::m_pEngine->GetPlayerInfo(i, &info))
				continue;

			switch (stage)
			{
			case FRAME_NET_UPDATE_START:
			{
				userID[i] = info.userID;
				std::memcpy(&backup_layers_update[i], player->GetAnimOverlays(), sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			case FRAME_RENDER_START:
			{
				if (info.userID == userID[i])
					continue;

				std::memcpy(&backup_layers_interp[i], player->GetAnimOverlays(), sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				std::memcpy(player->GetAnimOverlays(), &backup_layers_update[i], sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			case FRAME_RENDER_END:
			{
				if (info.userID != userID[i])
					continue;

				std::memcpy(player->GetAnimOverlays(), &backup_layers_interp[i], sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			}
		}
	}
	auto LagCompensation::StartLagCompensation(C_CSPlayer* player) -> bool
	{
		auto index = (player->GetIndex() - 1);

		auto& record_data = m_LagRecord[index];
		auto& record_restore = m_RestoreRecord[index];

		player->InvalidateBoneCache();

		record_restore.m_fFlags = player->m_fFlags();
		record_restore.m_flSimulationTime = player->m_flSimulationTime();
		record_restore.m_vecMins = player->OBBMins();
		record_restore.m_vecMaxs = player->OBBMaxs();
		record_restore.m_vecOrigin = player->GetAbsOrigin();
		record_restore.m_vecVelocity = player->m_vecVelocity();
		record_restore.m_angEyeAngles = player->m_angEyeAngles();
		record_restore.m_flPoseParameter = player->m_flPoseParameter();

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				record_restore.m_LayerRecord[i].m_nOrder = layer->m_nOrder;
				record_restore.m_LayerRecord[i].m_nSequence = layer->m_nSequence;
				record_restore.m_LayerRecord[i].m_flWeight = layer->m_flWeight;
				record_restore.m_LayerRecord[i].m_flCycle = layer->m_flCycle;
			}
		}

		return player->SetupBones(record_restore.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp());
	}

	auto LagCompensation::GetBestRecord(C_CSPlayer* player, LagRecord* record) -> bool
	{
		auto index = (player->GetIndex() - 1);

		auto& record_data = m_LagRecord[index];
		auto& record_restore = m_RestoreRecord[index];

		if (record_data.empty())
			return false;

		auto record_recent = record_data.front();
		auto record_previous = LagRecord{};

		auto record_current = std::find(record_data.begin(), record_data.end(), record_recent);
		auto record_index = std::distance(record_data.begin(), record_current);

		if (record_index != 0u)
			record_previous = *std::prev(record_current);

		if (!IsRecordGood(record_recent))
		{
			record_data.pop_front();
			return false;
		}

		if ((record_index != 0u) && (record_recent.m_vecOrigin - record_previous.m_vecOrigin).LengthSqr() > 4096.f)
		{
			record_data.pop_front();
			return false;
		}

		player->InvalidateBoneCache();

		player->m_vecMins() = record_recent.m_vecMins;
		player->m_vecMaxs() = record_recent.m_vecMaxs;
		player->SetAbsOrigin(record_recent.m_vecOrigin);
		player->SetAbsAngles(record_recent.m_angEyeAngles);
		player->m_flPoseParameter() = record_recent.m_flPoseParameter;

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				layer->m_nOrder = record_recent.m_LayerRecord[i].m_nOrder;
				layer->m_nSequence = record_recent.m_LayerRecord[i].m_nSequence;
				layer->m_flWeight = record_recent.m_LayerRecord[i].m_flWeight;
				layer->m_flCycle = record_recent.m_LayerRecord[i].m_flCycle;
			}
		}

		*record = record_recent;
		return true;
	}

	auto LagCompensation::FinishLagCompensation(C_CSPlayer* player) -> void
	{
		auto index = (player->GetIndex() - 1);

		auto& record_restore = m_RestoreRecord[index];

		player->InvalidateBoneCache();

		player->m_vecMins() = record_restore.m_vecMins;
		player->m_vecMaxs() = record_restore.m_vecMaxs;
		player->SetAbsOrigin(record_restore.m_vecOrigin);
		player->SetAbsAngles(record_restore.m_angEyeAngles);
		player->m_flPoseParameter() = record_restore.m_flPoseParameter;

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				layer->m_nOrder = record_restore.m_LayerRecord[i].m_nOrder;
				layer->m_nSequence = record_restore.m_LayerRecord[i].m_nSequence;
				layer->m_flWeight = record_restore.m_LayerRecord[i].m_flWeight;
				layer->m_flCycle = record_restore.m_LayerRecord[i].m_flCycle;
			}
		}
	}

	auto GetSpot(C_CSPlayer* player, matrix3x4_t* transform, Vector& output) -> bool
	{
		auto model = player->GetModel();

		if (!model)
			return false;

		auto studio = Source::m_pModelInfoClient->GetStudioModel(model);

		if (!studio)
			return false;

		auto set = studio->GetHitboxSet(player->m_nHitboxSet());

		if (!set)
			return false;

		auto box = set->GetHitbox(Config::Current->Aimbot->Spot);

		if (!box)
			return false;

		auto min = Vector{};
		auto max = Vector{};

		VectorTransform(box->bbmin, transform[box->bone], min);
		VectorTransform(box->bbmax, transform[box->bone], max);

		output = ((min + max) * 0.5f);

		if (output.IsZero())
			return false;

		return Source::TraceLine(output, player);
	}

	auto LagCompensation::BacktrackPlayer(C_CSPlayer* player, CUserCmd* usercmd, Vector& spot) -> bool
	{
		auto index = (player->GetIndex() - 1);

		if (!StartLagCompensation(player))
			return false;

		auto record = LagRecord{};

		if (GetBestRecord(player, &record))
		{
			if (player->SetupBones(record.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp()))
			{
				if (GetSpot(player, record.m_BoneTransform.data(), spot))
				{
					FinishLagCompensation(player);
					UpdateCommand(player, usercmd, record);
					return true;
				}
			}

			FinishLagCompensation(player);
		}

		return false;
	}

	auto LagCompensation::UpdateCommand(C_CSPlayer* player, CUserCmd* usercmd, const LagRecord& record) -> void
	{
		auto cmd_lerp = TIME_TO_TICKS(GetLerpTime());
		auto cmd_correct = TIME_TO_TICKS(IsRecordGood(record) ? record.m_flSimulationTime : player->m_flSimulationTime());

		usercmd->tick_count = (cmd_correct + cmd_lerp);
	}

	auto LagCompensation::IsRecordGood(const LagRecord& record) -> bool
	{
		static auto sv_maxunlag = Source::m_pCvar->FindVar(XorStr("sv_maxunlag"));

		if (record.m_flSimulationTime == 0.f)
			return false;

		auto net_channel = Source::m_pEngine->GetNetChannelInfo();

		if (!net_channel)
			return false;

		auto lerp = GetLerpTime();

		auto in = net_channel->GetLatency(FLOW_INCOMING);
		auto out = net_channel->GetLatency(FLOW_OUTGOING);

		auto correct = (out + TICKS_TO_TIME(TIME_TO_TICKS(lerp)));

		correct = std::clamp(correct, 0.f, sv_maxunlag->GetFloat());

		auto command = ((Source::m_pGlobalVars->tickcount + 1) + TIME_TO_TICKS(in + out));
		auto dt = (correct - (TICKS_TO_TIME(command) - record.m_flSimulationTime));

		return (std::abs(dt) < 0.2f); // should be '<=' but idc
	}

	auto LagCompensation::GetLerpTime() -> float
	{
		static auto cl_updaterate = Source::m_pCvar->FindVar(XorStr("cl_updaterate"));
		static auto cl_interp = Source::m_pCvar->FindVar(XorStr("cl_interp"));
		static auto cl_interp_ratio = Source::m_pCvar->FindVar(XorStr("cl_interp_ratio"));
		static auto sv_minupdaterate = Source::m_pCvar->FindVar(XorStr("sv_minupdaterate"));
		static auto sv_maxupdaterate = Source::m_pCvar->FindVar(XorStr("sv_maxupdaterate"));
		static auto sv_client_min_interp_ratio = Source::m_pCvar->FindVar(XorStr("sv_client_min_interp_ratio"));
		static auto sv_client_max_interp_ratio = Source::m_pCvar->FindVar(XorStr("sv_client_max_interp_ratio"));

		auto updaterate = std::clamp(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());

		auto interp = cl_interp->GetFloat();
		auto interp_ratio = cl_interp_ratio->GetFloat();

		if (interp_ratio == 0.f)
			interp_ratio = 1.f;

		interp_ratio = std::clamp(interp_ratio, sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());

		return std::max(interp, (interp_ratio / updaterate));
	}

	auto LagCompensation1::UpdateAnimationData1(ClientFrameStage_t stage) -> void
	{
		auto local = C_CSPlayer::GetLocalPlayer();

		if (!local)
			return;

		if (local->m_lifeState() != LIFE_ALIVE)
			return;

		static int userID[64] = {};
		static C_AnimationLayer backup_layers_update[64][15] = {};
		static C_AnimationLayer backup_layers_interp[64][15] = {};

		for (int i = 1; i <= Source::m_pEngine->GetMaxClients(); i++)
		{
			auto player = C_CSPlayer::GetPlayer(i);

			if (!player)
				continue;

			if (player == local)
				continue;

			if (player->IsDormant())
				continue;

			player_info_t info = {};

			if (!Source::m_pEngine->GetPlayerInfo(i, &info))
				continue;

			switch (stage)
			{
			case FRAME_NET_UPDATE_START:
			{
				userID[i] = info.userID;
				std::memcpy(&backup_layers_update[i], player->GetAnimOverlays(), sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			case FRAME_RENDER_START:
			{
				if (info.userID == userID[i])
					continue;

				std::memcpy(&backup_layers_interp[i], player->GetAnimOverlays(), sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				std::memcpy(player->GetAnimOverlays(), &backup_layers_update[i], sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			case FRAME_RENDER_END:
			{
				if (info.userID != userID[i])
					continue;

				std::memcpy(player->GetAnimOverlays(), &backup_layers_interp[i], sizeof(C_AnimationLayer) * player->GetNumAnimOverlays());
				break;
			}
			}
		}
	}
	auto LagCompensation1::UpdateLagRecord1(C_CSPlayer* player) -> void
	{
		auto index = (player->GetIndex() - 1);

		auto record1 = LagRecord1{ };
		auto& record_data1 = m_LagRecord[index];

		player->InvalidateBoneCache();

		record1.m_fFlags = player->m_fFlags();
		record1.m_flSimulationTime = player->m_flSimulationTime();
		record1.m_vecMins = player->OBBMins();
		record1.m_vecMaxs = player->OBBMaxs();
		record1.m_vecVelocity = player->m_vecVelocity();
		record1.m_vecOrigin = player->m_vecOrigin();
		record1.m_angEyeAngles = player->m_angEyeAngles();
		record1.m_flPoseParameter = player->m_flPoseParameter();

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				record1.m_LayerRecord[i].m_nOrder = layer->m_nOrder;
				record1.m_LayerRecord[i].m_nSequence = layer->m_nSequence;
				record1.m_LayerRecord[i].m_flWeight = layer->m_flWeight;
				record1.m_LayerRecord[i].m_flCycle = layer->m_flCycle;
			}
		}

		player->SetupBones(record1.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp());

		if (!record_data1.empty())
		{
			for (auto i = 0u; i < record_data1.size(); i++)
			{
				if (!IsRecordGood1(record_data1[i]))
					record_data1.erase(record_data1.begin() + i);
			}

			for (const auto& record_current1 : record_data1)
			{
				if (record1.m_flSimulationTime <= record_current1.m_flSimulationTime)
					record1.m_flSimulationTime = 0.f;
			}
		}

		if (IsRecordGood1(record1))
			record_data1.emplace_back(record1);
	}

	auto LagCompensation1::StartLagCompensation1(C_CSPlayer* player) -> bool
	{
		auto index = (player->GetIndex() - 1);

		auto& record_data = m_LagRecord[index];
		auto& record_restore = m_RestoreRecord[index];

		player->InvalidateBoneCache();

		record_restore.m_fFlags = player->m_fFlags();
		record_restore.m_flSimulationTime = player->m_flSimulationTime();
		record_restore.m_vecMins = player->OBBMins();
		record_restore.m_vecMaxs = player->OBBMaxs();
		record_restore.m_vecOrigin = player->GetAbsOrigin();
		record_restore.m_vecVelocity = player->m_vecVelocity();
		record_restore.m_angEyeAngles = player->m_angEyeAngles();
		record_restore.m_flPoseParameter = player->m_flPoseParameter();

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				record_restore.m_LayerRecord[i].m_nOrder = layer->m_nOrder;
				record_restore.m_LayerRecord[i].m_nSequence = layer->m_nSequence;
				record_restore.m_LayerRecord[i].m_flWeight = layer->m_flWeight;
				record_restore.m_LayerRecord[i].m_flCycle = layer->m_flCycle;
			}
		}

		return player->SetupBones(record_restore.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp() + Source::m_pEngine->GetLastTimeStamp());
	}

	auto LagCompensation1::GetBestRecord1(C_CSPlayer* player, LagRecord1* record1) -> bool
	{
		auto index = (player->GetIndex() - 1);

		auto& record_data = m_LagRecord[index];
		auto& record_restore = m_RestoreRecord[index];

		if (record_data.empty())
			return false;
		
		auto record_recent = record_data.back(); // HEREE 2222222222222 
		auto record_previous = LagRecord1{};

		auto record_current = std::find(record_data.begin(), record_data.end(), record_recent);
		auto record_index = std::distance(record_data.begin(), record_current);

		if (record_index != 0u)
			record_previous = *std::prev(record_current);

		if (!IsRecordGood1(record_recent))
		{
			record_data.pop_front();
			return false;
		}

		if ((record_index != 0u) && (record_recent.m_vecOrigin - record_previous.m_vecOrigin).LengthSqr() > 4096.f)
		{
			record_data.pop_front();
			return false;
		}

		player->InvalidateBoneCache();

		player->m_vecMins() = record_recent.m_vecMins;
		player->m_vecMaxs() = record_recent.m_vecMaxs;
		player->SetAbsOrigin(record_recent.m_vecOrigin);
		player->SetAbsAngles(record_recent.m_angEyeAngles);
		player->m_flPoseParameter() = record_recent.m_flPoseParameter;

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				layer->m_nOrder = record_recent.m_LayerRecord[i].m_nOrder;
				layer->m_nSequence = record_recent.m_LayerRecord[i].m_nSequence;
				layer->m_flWeight = record_recent.m_LayerRecord[i].m_flWeight;
				layer->m_flCycle = record_recent.m_LayerRecord[i].m_flCycle;
			}
		}

		*record1 = record_recent;
		return true;
	}

	auto LagCompensation1::FinishLagCompensation1(C_CSPlayer* player) -> void
	{
		auto index = (player->GetIndex() - 1);

		auto& record_restore = m_RestoreRecord[index];

		player->InvalidateBoneCache();

		player->m_vecMins() = record_restore.m_vecMins;
		player->m_vecMaxs() = record_restore.m_vecMaxs;
		player->SetAbsOrigin(record_restore.m_vecOrigin);
		player->SetAbsAngles(record_restore.m_angEyeAngles);
		player->m_flPoseParameter() = record_restore.m_flPoseParameter;

		for (auto i = 0; i < player->GetNumAnimOverlays(); i++)
		{
			auto layer = player->GetAnimOverlay(i);

			if (layer)
			{
				layer->m_nOrder = record_restore.m_LayerRecord[i].m_nOrder;
				layer->m_nSequence = record_restore.m_LayerRecord[i].m_nSequence;
				layer->m_flWeight = record_restore.m_LayerRecord[i].m_flWeight;
				layer->m_flCycle = record_restore.m_LayerRecord[i].m_flCycle;
			}
		}
	}

	auto GetSpot1(C_CSPlayer* player, matrix3x4_t* transform, Vector& output) -> bool
	{
		auto model = player->GetModel();

		if (!model)
			return false;

		auto studio = Source::m_pModelInfoClient->GetStudioModel(model);

		if (!studio)
			return false;

		auto set = studio->GetHitboxSet(player->m_nHitboxSet());

		if (!set)
			return false;

		auto box = set->GetHitbox(Config::Current->Aimbot->Spot);

		if (!box)
			return false;

		auto min = Vector{};
		auto max = Vector{};

		VectorTransform(box->bbmin, transform[box->bone], min);
		VectorTransform(box->bbmax, transform[box->bone], max);

		output = ((min + max) * 0.5f);

		if (output.IsZero())
			return false;

		return Source::TraceLine(output, player);
	}

	auto LagCompensation1::BacktrackPlayer1(C_CSPlayer* player, CUserCmd* usercmd, Vector& spot) -> bool
	{
		auto index = (player->GetIndex() - 1);

		if (!StartLagCompensation1(player))
			return false;

		auto record = LagRecord1{};

		if (GetBestRecord1(player, &record))
		{
			if (player->SetupBones(record.m_BoneTransform.data(), 128, 0x0100, Source::m_pEngine->GetLastTimeStamp()))
			{
				if (GetSpot1(player, record.m_BoneTransform.data(), spot))
				{
					FinishLagCompensation1(player);
					UpdateCommand1(player, usercmd, record);
					return true;
				}
			}

			FinishLagCompensation1(player);
		}

		return false;
	}

	auto LagCompensation1::UpdateCommand1(C_CSPlayer* player, CUserCmd* usercmd, const LagRecord1& record1) -> void
	{
		auto cmd_lerp = TIME_TO_TICKS(GetLerpTime1());
		auto cmd_correct = TIME_TO_TICKS(IsRecordGood1(record1) ? record1.m_flSimulationTime : player->m_flSimulationTime());

		usercmd->tick_count = (cmd_correct + cmd_lerp);
	}

	auto LagCompensation1::IsRecordGood1(const LagRecord1& record1) -> bool
	{
		static auto sv_maxunlag = Source::m_pCvar->FindVar(XorStr("sv_maxunlag"));

		if (record1.m_flSimulationTime == 0.f)
			return false;

		auto net_channel = Source::m_pEngine->GetNetChannelInfo();

		if (!net_channel)
			return false;

		auto lerp = GetLerpTime1();

		auto in = net_channel->GetLatency(FLOW_INCOMING);
		auto out = net_channel->GetLatency(FLOW_OUTGOING);

		auto correct = (out + TICKS_TO_TIME(TIME_TO_TICKS(lerp)));

		correct = std::clamp(correct, 0.f, sv_maxunlag->GetFloat());

		auto command = ((Source::m_pGlobalVars->tickcount + 1) + TIME_TO_TICKS(in + out));
		auto dt = (correct - (TICKS_TO_TIME(command) - record1.m_flSimulationTime));

		return (std::abs(dt) < 0.2f); // should be '<=' but idc
	}

	auto LagCompensation1::GetLerpTime1() -> float
	{
		static auto cl_updaterate = Source::m_pCvar->FindVar(XorStr("cl_updaterate"));
		static auto cl_interp = Source::m_pCvar->FindVar(XorStr("cl_interp"));
		static auto cl_interp_ratio = Source::m_pCvar->FindVar(XorStr("cl_interp_ratio"));
		static auto sv_minupdaterate = Source::m_pCvar->FindVar(XorStr("sv_minupdaterate"));
		static auto sv_maxupdaterate = Source::m_pCvar->FindVar(XorStr("sv_maxupdaterate"));
		static auto sv_client_min_interp_ratio = Source::m_pCvar->FindVar(XorStr("sv_client_min_interp_ratio"));
		static auto sv_client_max_interp_ratio = Source::m_pCvar->FindVar(XorStr("sv_client_max_interp_ratio"));

		auto updaterate = std::clamp(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());

		auto interp = cl_interp->GetFloat();
		auto interp_ratio = cl_interp_ratio->GetFloat();

		if (interp_ratio == 0.f)
			interp_ratio = 1.f;

		interp_ratio = std::clamp(interp_ratio, sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());

		return std::max(interp, (interp_ratio / updaterate));
	}
}