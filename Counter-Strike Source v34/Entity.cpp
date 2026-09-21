#include "Entity.hpp"
#include "Source.hpp"

// ICollideable
const Vector3& C_BaseEntity::GetMins()
{
	auto pCol = GetCollideable();

	typedef const Vector3& (__thiscall* GetMinsFn)(void*);
	return Memory::VCall< GetMinsFn >(pCol, 1)(pCol);
}

const Vector3& C_BaseEntity::GetMaxs()
{
	auto pCol = GetCollideable();

	typedef const Vector3& (__thiscall* GetMaxsFn)(void*);
	return Memory::VCall< GetMaxsFn >(pCol, 2)(pCol);
}

// IClientNetworkable
ClientClass* C_BaseEntity::GetClientClass()
{
	auto pNet = GetNetworkable();

	typedef ClientClass* (__thiscall* GetClientClassFn)(void*);
	return Memory::VCall< GetClientClassFn >(pNet, 2)(pNet);
}

bool C_BaseEntity::IsDormant()
{
	auto pNet = GetNetworkable();

	typedef bool(__thiscall* IsDormantFn)(void*);
	return Memory::VCall< IsDormantFn >(pNet, 8)(pNet);
}

int C_BaseEntity::GetIndex()
{
	auto pNet = GetNetworkable();

	typedef int(__thiscall* GetIndexFn)(void*);
	return Memory::VCall< GetIndexFn >(pNet, 9)(pNet);
}

// IClientRenderable
const model_t* C_BaseEntity::GetModel()
{
	auto pRen = GetRenderable();

	typedef const model_t* (__thiscall* GetModelFn)(void*);
	return Memory::VCall< GetModelFn >(pRen, 8)(pRen);
}

bool C_BaseEntity::SetupBones(Matrix3x4* pOut, int iMaxBones, int iBoneMask, float fCurrentTime)
{
	auto pRen = GetRenderable();

	typedef bool(__thiscall* SetupBonesFn)(void*, Matrix3x4*, int, int, float);
	return Memory::VCall< SetupBonesFn >(pRen, 15)(pRen, pOut, iMaxBones, iBoneMask, fCurrentTime);
}

// IClientEntity
const Vector& C_BaseEntity::GetAbsOrigin()
{
	using Fn = const Vector&(__thiscall*)(void*);
	return Memory::VCall< Fn >(this, 9)(this);
}

const QAngle& C_BaseEntity::GetAbsAngles()
{
	using Fn = const QAngle&(__thiscall*)(void*);
	return Memory::VCall< Fn >(this, 10)(this);
}

// C_BaseEntity
bool C_BaseEntity::IsPlayer()
{
	typedef bool(__thiscall* IsPlayerFn)(void*);
	return Memory::VCall< IsPlayerFn >(this, 118)(this); // search for CCSGameMovement::CheckParameters
}

unsigned char& C_BaseEntity::m_MoveType()
{
	return *(unsigned char*)(this + 0x134); // search for CCSGameMovement::CheckParameters
}

// DT_BaseEntity
int& C_BaseEntity::m_iTeamNum()
{
	MakeNetVar(int, XorStr("DT_BaseEntity"), XorStr("m_iTeamNum"));
}

float& C_BaseEntity::m_flSimulationTime()
{
	MakeNetVar(float, XorStr("DT_BaseEntity"), XorStr("m_flSimulationTime"));
}

Vector& C_BaseEntity::m_vecMins()
{
	MakeNetVar(Vector, XorStr("DT_BaseEntity"), XorStr("m_vecMins"));
}

Vector& C_BaseEntity::m_vecMaxs()
{
	MakeNetVar(Vector, XorStr("DT_BaseEntity"), XorStr("m_vecMaxs"));
}

Vector3& C_BaseEntity::m_vecOrigin()
{
	MakeNetVar(Vector3, XorStr("DT_BaseEntity"), XorStr("m_vecOrigin"));
}

Matrix3x4& C_BaseEntity::m_rgflCoordinateFrame()
{
	static auto var = Source::m_pNetVarManager->Get(XorStr("DT_BasePlayer"), XorStr("m_fFlags")) - sizeof(Vector3) - sizeof(Vector3) - sizeof(Matrix3x4);

	return *(Matrix3x4*)(this + var);
}

std::array< float, 24u >& C_BaseEntity::m_flPoseParameter()
{
	static auto offset = Source::m_pNetVarManager->Get(XorStr("DT_BaseAnimating"), XorStr("m_flPoseParameter"));
	return *(std::array< float, 24u >*)(this + offset);
}

// 
// =========================================
// 
const Vector& C_BaseEntity::OBBMins()
{
	return GetMins();
}

const Vector& C_BaseEntity::OBBMaxs()
{
	return GetMaxs();
}

void C_BaseEntity::InvalidateBoneCache()
{
	using Fn = void(__thiscall*)(void*);

	static auto image = reinterpret_cast<std::uintptr_t>(ModuleD(XorStr("client.dll")));
	static auto procedure = reinterpret_cast<Fn>(image + 0x000317F0);

	return procedure(this);
}

void C_BaseEntity::SetAbsOrigin(const Vector& origin)
{
	using Fn = void(__thiscall*)(void*, const Vector&);

	static auto image = reinterpret_cast<std::uintptr_t>(ModuleD(XorStr("client.dll")));
	static auto procedure = reinterpret_cast<Fn>(image + 0x00045620);

	return procedure(this, origin);
}

void C_BaseEntity::SetAbsAngles(const QAngle& angles)
{
	using Fn = void(__thiscall*)(void*, const QAngle&);

	static auto image = reinterpret_cast<std::uintptr_t>(ModuleD(XorStr("client.dll")));
	static auto procedure = reinterpret_cast<Fn>(image + 0x00045720);

	return procedure(this, angles);
}

int C_BaseEntity::GetNumAnimOverlays()
{
	static auto offset = 0x06BC;
	return *(int*)(this + offset);
}

C_AnimationLayer* C_BaseEntity::GetAnimOverlays()
{
	static auto offset = 0x06B0;
	return *(C_AnimationLayer**)(this + offset);
}

C_AnimationLayer* C_BaseEntity::GetAnimOverlay(std::size_t index)
{
	auto layers = GetAnimOverlays();
	return &layers[index];
}

// private
void* C_BaseEntity::GetCollideable()
{
	typedef void* (__thiscall* GetCollideableFn)(void*);
	return Memory::VCall< GetCollideableFn >(this, 3)(this);
}

void* C_BaseEntity::GetNetworkable()
{
	typedef void* (__thiscall* GetNetworkableFn)(void*);
	return Memory::VCall< GetNetworkableFn >(this, 4)(this);
}

void* C_BaseEntity::GetRenderable()
{
	typedef void* (__thiscall* GetRenderableFn)(void*);
	return Memory::VCall< GetRenderableFn >(this, 5)(this);
}