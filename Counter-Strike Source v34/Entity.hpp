#pragma once

#include "Valve.hpp"

#define MakeNetVar( cast, table, name ) static auto var = Source::m_pNetVarManager->Get( table, name ); return *( cast* )( this + var )

class C_AnimationLayer
{
public:
	int m_nSequence = 0;
	float m_flPrevCycle = 0.f;
	float m_flWeight = 0.f;
	int m_nOrder = 0;
	float m_flPlaybackRate = 0.f;
	float m_flCycle = 0.f;
	float m_flLayerAnimtime = 0.f;
	float m_flLayerFadeOuttime = 0.f;
};

class C_BaseEntity
{
public:
	auto GetBaseEntity() -> C_BaseEntity*
	{
		return memory::vcall< 7u, C_BaseEntity* >(this);
	}
	// ICollideable
	const Vector3&	GetMins();
	const Vector3&	GetMaxs();

	// IClientNetworkable
	ClientClass*	GetClientClass();
	bool			IsDormant();
	int				GetIndex();

	// IClientRenderable
	const model_t*	GetModel();
	bool			SetupBones(Matrix3x4* pOut, int iMaxBones, int iBoneMask, float fCurrentTime);

	// IClientEntity
	const Vector& GetAbsOrigin();
	const QAngle& GetAbsAngles();

	// C_BaseEntity
	bool			IsPlayer();

	// DT_BaseEntity
	unsigned char&	m_MoveType();

	int&			m_iTeamNum();
	float&			m_flSimulationTime();
	Vector&			m_vecMins();
	Vector&			m_vecMaxs();
	Vector3&		m_vecOrigin();
	Matrix3x4&		m_rgflCoordinateFrame();

	// DT_BaseAnimating
	std::array< float, 24u >& m_flPoseParameter();

public:
	const Vector& OBBMins();
	const Vector& OBBMaxs();

	void InvalidateBoneCache();

	void SetAbsOrigin(const Vector& origin);
	void SetAbsAngles(const QAngle& angles);

	int GetNumAnimOverlays();
	C_AnimationLayer* GetAnimOverlays();
	C_AnimationLayer* GetAnimOverlay(std::size_t index);

public:
	// Source
	void*			GetCollideable();
	void*			GetNetworkable();
	void*			GetRenderable();
};