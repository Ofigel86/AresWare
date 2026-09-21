#pragma once

#include "Valve.hpp"

template< class T >
class Singleton
{
public:
	static auto Instance() -> T&
	{
		static T instance;
		return instance;
	}

protected:
	Singleton() = default;
	Singleton(const Singleton&) = delete;
	auto operator = (const Singleton&)->Singleton& = delete;
};

namespace Feature
{

	struct LayerRecord
	{
		LayerRecord() = default;

		LayerRecord(const LayerRecord& record)
		{
			m_nOrder = record.m_nOrder;
			m_nSequence = record.m_nSequence;
			m_flWeight = record.m_flWeight;
			m_flCycle = record.m_flCycle;
		}

		int m_nOrder = 0;
		int m_nSequence = 0;
		float m_flWeight = 0.f;
		float m_flCycle = 0.f;
	};

	struct LagRecord
	{
		LagRecord() = default;

		LagRecord(const LagRecord& record)
		{
			m_fFlags = record.m_fFlags;
			m_flSimulationTime = record.m_flSimulationTime;
			m_vecMins = record.m_vecMins;
			m_vecMaxs = record.m_vecMaxs;
			m_vecOrigin = record.m_vecOrigin;
			m_vecVelocity = record.m_vecVelocity;
			m_angEyeAngles = record.m_angEyeAngles;
			m_flPoseParameter = record.m_flPoseParameter;
			m_LayerRecord = record.m_LayerRecord;
			m_BoneTransform = record.m_BoneTransform;
		}

		auto operator == (const LagRecord& record) -> bool
		{
			return (m_flSimulationTime == record.m_flSimulationTime);
		}

		int m_fFlags = 0;

		float m_flSimulationTime = 0.f;

		Vector m_vecMins = {};
		Vector m_vecMaxs = {};

		Vector m_vecOrigin = {};
		Vector m_vecVelocity = {};
		QAngle m_angEyeAngles = {};

		std::array< float, 24u > m_flPoseParameter = {};
		std::array< LayerRecord, 15u > m_LayerRecord = {};

		std::array< matrix3x4_t, 128u > m_BoneTransform = {};
	};

	class LagCompensation : public Singleton< LagCompensation >
	{
	public:
		auto UpdateLagRecord(C_CSPlayer* player) -> void;

		auto UpdateAnimationData(ClientFrameStage_t stage) -> void;

		auto StartLagCompensation(C_CSPlayer* player) -> bool;
		auto GetBestRecord(C_CSPlayer* player, LagRecord* record) -> bool;
		auto FinishLagCompensation(C_CSPlayer* player) -> void;

		auto BacktrackPlayer(C_CSPlayer* player, CUserCmd* usercmd, Vector& spot) -> bool;

		auto UpdateCommand(C_CSPlayer* player, CUserCmd* usercmd, const LagRecord& record) -> void;

		auto IsRecordGood(const LagRecord& record) -> bool;

		auto GetLerpTime() -> float;

	public:
		std::array< std::deque< LagRecord >, 64u > m_LagRecord = {};
		std::array< LagRecord, 64u > m_RestoreRecord = {};
	};
	
	struct LayerRecord1
	{
		LayerRecord1() = default;

		LayerRecord1(const LayerRecord1& record1)
		{
			m_nOrder = record1.m_nOrder;
			m_nSequence = record1.m_nSequence;
			m_flWeight = record1.m_flWeight;
			m_flCycle = record1.m_flCycle;
		}

		int m_nOrder = 0;
		int m_nSequence = 0;
		float m_flWeight = 0.f;
		float m_flCycle = 0.f;
	};

	struct LagRecord1
	{
		LagRecord1() = default;

		LagRecord1(const LagRecord1& record1)
		{
			m_fFlags = record1.m_fFlags;
			m_flSimulationTime = record1.m_flSimulationTime;
			m_vecMins = record1.m_vecMins;
			m_vecMaxs = record1.m_vecMaxs;
			m_vecOrigin = record1.m_vecOrigin;
			m_vecVelocity = record1.m_vecVelocity;
			m_angEyeAngles = record1.m_angEyeAngles;
			m_flPoseParameter = record1.m_flPoseParameter;
			m_LayerRecord = record1.m_LayerRecord;
			m_BoneTransform = record1.m_BoneTransform;
		}

		auto operator == (const LagRecord1& record1) -> bool
		{
			return (m_flSimulationTime == record1.m_flSimulationTime);
		}

		int m_fFlags = 0;

		float m_flSimulationTime = 0.f;

		Vector m_vecMins = {};
		Vector m_vecMaxs = {};

		Vector m_vecOrigin = {};
		Vector m_vecVelocity = {};
		QAngle m_angEyeAngles = {};

		std::array< float, 24u > m_flPoseParameter = {};
		std::array< LayerRecord1, 15u > m_LayerRecord = {};

		std::array< matrix3x4_t, 128u > m_BoneTransform = {};
	};

	class LagCompensation1 : public Singleton< LagCompensation1 >
	{
	public:
		auto UpdateLagRecord1(C_CSPlayer* player) -> void;

		auto UpdateAnimationData1(ClientFrameStage_t stage) -> void;

		auto StartLagCompensation1(C_CSPlayer* player) -> bool;
		auto GetBestRecord1(C_CSPlayer* player, LagRecord1* record1) -> bool;
		auto FinishLagCompensation1(C_CSPlayer* player) -> void;

		auto BacktrackPlayer1(C_CSPlayer* player, CUserCmd* usercmd, Vector& spot) -> bool;

		auto UpdateCommand1(C_CSPlayer* player, CUserCmd* usercmd, const LagRecord1& record1) -> void;

		auto IsRecordGood1(const LagRecord1 & record1) -> bool;

	
		auto GetLerpTime1() -> float;

	public:
		std::array< std::deque< LagRecord1 >, 64u > m_LagRecord = {};
		std::array< LagRecord1, 64u > m_RestoreRecord = {};
	};
}