#pragma once

#include "Core.hpp"
#include "Framework.hpp"

#include "dt_common.hpp"
#include "dt_recv.hpp"

#include "checksum_md5.hpp"

#include <d3d9.h>

#include <array>
#include <deque>
#include <algorithm>

#include <cassert>
//
// defines
//
#ifdef GetProp
#undef GetProp
#endif
#ifdef DrawText
#undef DrawText
#endif

#undef min
#undef max

#define TIME_TO_TICKS( dt ) ( ( int )( 0.5f + ( float )( dt ) / Source::m_pGlobalVars->interval_per_tick ) )
#define TICKS_TO_TIME( t ) ( Source::m_pGlobalVars->interval_per_tick * ( t ) )

//
// enums
//
enum
{
	// Source Engine
	IBaseClientDLL_GetAllClasses = 5,
	IBaseClientDLL_CreateMove = 18,
	IBaseClientDLL_FrameStageNotify = 32,

	IClientEntityList_GetBaseEntity = 3,
	IClientEntityList_GetBaseEntityFromHandle = 4,
	IClientEntityList_GetHighestEntityIndex = 6,

	IGameMovement_ProcessMovement = 1,

	IPrediction_RunCommand = 19,
	IPrediction_SetupMove = 20,
	IPrediction_FinishMove = 21,
	IPrediction_Update = 23,

	IInput_GetUserCmd = 8,
	IInput_ResetMouse = 23,

	IVEngineClient_GetScreenSize = 5,
	IVEngineClient_GetPlayerInfo = 8,
	IVEngineClient_GetLocalPlayer = 12,
	IVEngineClient_GetViewAngles = 19,
	IVEngineClient_SetViewAngles = 20,
	IVEngineClient_GetMaxClients = 21,
	IVEngineClient_WorldToScreenMatrix = 38,

	IVModelInfoClient_GetModelName = 3,
	IVModelInfoClient_GetStudioModel = 27,

	IVRenderView_SetBlend = 4,
	IVRenderView_SetColorModulation = 6,

	IVModelRender_ForcedMaterialOverride = 1,
	IVModelRender_DrawModelEx = 19,

	IEngineTrace_GetPointContents = 0,
	IEngineTrace_TraceRay = 4,

	ICvar_FindVar = 7,

	IPhysicsSurfaceProps_GetSurfaceData = 5,

	IMaterialSystem_CreateMaterial = 123,

	// Direct3D
	IDirect3DDevice9_Reset = 16,
	IDirect3DDevice9_Present = 17,
};

enum
{
	MULTIPLAYER_BACKUP = 90,
};

enum
{
	FL_ONGROUND = (1 << 0),
	FL_DUCKING = (1 << 1),
};

enum
{
	IN_ATTACK = (1 << 0),
	IN_JUMP = (1 << 1),
	IN_DUCK = (1 << 2),
	IN_FORWARD = (1 << 3),
	IN_BACK = (1 << 4),
	IN_USE = (1 << 5),
	IN_CANCEL = (1 << 6),
	IN_LEFT = (1 << 7),
	IN_RIGHT = (1 << 8),
	IN_MOVELEFT = (1 << 9),
	IN_MOVERIGHT = (1 << 10),
	IN_ATTACK2 = (1 << 11),
	IN_RUN = (1 << 12),
	IN_RELOAD = (1 << 13),
	IN_ALT1 = (1 << 14),
	IN_ALT2 = (1 << 15),
	IN_SCORE = (1 << 16),
	IN_SPEED = (1 << 17),
	IN_WALK = (1 << 18),
	IN_ZOOM = (1 << 19),
	IN_WEAPON1 = (1 << 20),
	IN_WEAPON2 = (1 << 21),
	IN_BULLRUSH = (1 << 22),
	IN_GRENADE1 = (1 << 23),
	IN_GRENADE2 = (1 << 24),
};

enum ClientFrameStage_t
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END,
};

enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY,
	TRACE_ENTITIES_ONLY,
	TRACE_EVERYTHING_FILTER_PROPS,
};

enum CSWeaponType
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN,
};

enum CSWeaponID
{
	WEAPON_NONE = 0,
	WEAPON_P228,
	WEAPON_GLOCK,
	WEAPON_SCOUT,
	WEAPON_HEGRENADE,
	WEAPON_XM1014,
	WEAPON_C4,
	WEAPON_MAC10,
	WEAPON_AUG,
	WEAPON_SMOKEGRENADE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_UMP45,
	WEAPON_SG550,
	WEAPON_GALIL,
	WEAPON_FAMAS,
	WEAPON_USP,
	WEAPON_AWP,
	WEAPON_MP5NAVY,
	WEAPON_M249,
	WEAPON_M3,
	WEAPON_M4A1,
	WEAPON_TMP,
	WEAPON_G3SG1,
	WEAPON_FLASHBANG,
	WEAPON_DEAGLE,
	WEAPON_SG552,
	WEAPON_AK47,
	WEAPON_KNIFE,
	WEAPON_P90,
	WEAPON_SHIELD,
	WEAPON_KEVLAR,
	WEAPON_ASSAULTSUIT,
	WEAPON_NIGHTVISION,

	WEAPON_MAX,
};

enum MaterialType_t
{
	CHAR_TEX_ANTLION = 'A',
	CHAR_TEX_BLOODYFLESH = 'B',
	CHAR_TEX_CONCRETE = 'C',
	CHAR_TEX_DIRT = 'D',
	CHAR_TEX_EGGSHELL = 'E',
	CHAR_TEX_FLESH = 'F',
	CHAR_TEX_GRATE = 'G',
	CHAR_TEX_ALIENFLESH = 'H',
	CHAR_TEX_CLIP = 'I',
	CHAR_TEX_PLASTIC = 'L',
	CHAR_TEX_METAL = 'M',
	CHAR_TEX_SAND = 'N',
	CHAR_TEX_FOLIAGE = 'O',
	CHAR_TEX_COMPUTER = 'P',
	CHAR_TEX_SLOSH = 'S',
	CHAR_TEX_TILE = 'T',
	CHAR_TEX_VENT = 'V',
	CHAR_TEX_WOOD = 'W',
	CHAR_TEX_GLASS = 'Y',
	CHAR_TEX_WARPSHIELD = 'Z'
};

enum
{
	LIFE_ALIVE = 0,
	LIFE_DYING,
	LIFE_DEAD,
	LIFE_RESPAWNABLE,
	LIFE_DISCARDBODY
};

enum MapLoadType_t
{
	MapLoad_NewGame = 0,
	MapLoad_LoadGame,
	MapLoad_Transition,
	MapLoad_Background,
};

typedef enum
{
	PREDICTION_SIMULATION_RESULTS_ARRIVING_ON_SEND_FRAME = 0,
	PREDICTION_NORMAL,
} PREDICTION_REASON;

enum MaterialVarFlags_t
{
	MATERIAL_VAR_DEBUG = (1 << 0),
	MATERIAL_VAR_NO_DEBUG_OVERRIDE = (1 << 1),
	MATERIAL_VAR_NO_DRAW = (1 << 2),
	MATERIAL_VAR_USE_IN_FILLRATE_MODE = (1 << 3),
	MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA = (1 << 5),
	MATERIAL_VAR_SELFILLUM = (1 << 6),
	MATERIAL_VAR_ADDITIVE = (1 << 7),
	MATERIAL_VAR_ALPHATEST = (1 << 8),
	MATERIAL_VAR_MULTIPASS = (1 << 9),
	MATERIAL_VAR_ZNEARER = (1 << 10),
	MATERIAL_VAR_MODEL = (1 << 11),
	MATERIAL_VAR_FLAT = (1 << 12),
	MATERIAL_VAR_NOCULL = (1 << 13),
	MATERIAL_VAR_NOFOG = (1 << 14),
	MATERIAL_VAR_IGNOREZ = (1 << 15),
	MATERIAL_VAR_DECAL = (1 << 16),
	MATERIAL_VAR_ENVMAPSPHERE = (1 << 17),
	MATERIAL_VAR_NOALPHAMOD = (1 << 18),
	MATERIAL_VAR_ENVMAPCAMERASPACE = (1 << 19),
	MATERIAL_VAR_BASEALPHAENVMAPMASK = (1 << 20),
	MATERIAL_VAR_TRANSLUCENT = (1 << 21),
	MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
	MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = (1 << 23),
	MATERIAL_VAR_OPAQUETEXTURE = (1 << 24),
	MATERIAL_VAR_ENVMAPMODE = (1 << 25),
	MATERIAL_VAR_SUPPRESS_DECALS = (1 << 26),
	MATERIAL_VAR_HALFLAMBERT = (1 << 27),
	MATERIAL_VAR_WIREFRAME = (1 << 28),
	MATERIAL_VAR_ALLOWALPHATOCOVERAGE = (1 << 29),
};

enum MoveType_t
{
	MOVETYPE_NONE = 0,
	MOVETYPE_ISOMETRIC,
	MOVETYPE_WALK,
	MOVETYPE_STEP,
	MOVETYPE_FLY,
	MOVETYPE_FLYGRAVITY,
	MOVETYPE_VPHYSICS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_LADDER,
	MOVETYPE_OBSERVER,
	MOVETYPE_CUSTOM,
	MOVETYPE_LAST = MOVETYPE_CUSTOM,
	MOVETYPE_MAX_BITS = 4
};

//
// structs
//
struct model_t;

struct player_info_t;
struct mstudiobone_t;
struct mstudiobbox_t;
struct mstudiohitboxset_t;
struct studiohdr_t;
struct cplane_t;
struct csurface_t;
struct Ray_t;
struct surfacephysicsparams_t;
struct surfaceaudioparams_t;
struct surfacesoundnames_t;
struct surfacegameprops_t;
struct surfacedata_t;
struct string_t;
struct ModelRenderInfo_t;

//
// classes
//
class IConVar;
class IMoveHelper;
class IMaterial;

class IHandleEntity;
class IClientNetworkable;
class IClientRenderable;

class C_BaseEntity;
class C_CSPlayer;
class C_WeaponCSBaseGun;
class CCSWeaponInfo;

class ConVar;
class ClientClass;
class CGlobalVars;
class CUserCmd;
class CVerifiedUserCmd;
class CBaseTrace;
class CGameTrace;
class CMoveData;

class IBaseClientDLL;
class IClientEntityList;
class IGameMovement;
class IPrediction;
class IInput;
class IVEngineClient;
class IVModelInfoClient;
class IVRenderView;
class IVModelRender;
class IEngineTrace;
class ICvar;
class IPhysicsSurfaceProps;
class IMaterialSystem;

class KeyValues;
class D3DDeviceWrapper;

class ITraceFilter;
class CTraceFilter;
class CTraceFilterSimple;
class CTraceFilterSkipTwoEntities;

//
// typedefs
//
typedef void* (*CreateInterfaceFn)(const char*, int*);
typedef void(*FnChangeCallback_t)(IConVar*, const char*, float);

typedef unsigned long CBaseHandle;
typedef CGameTrace trace_t;
typedef unsigned short WEAPON_FILE_INFO_HANDLE;
typedef unsigned short ModelInstanceHandle_t;

//
// hooks
//
typedef void(__thiscall* CreateMoveFn)(void*, int, float, bool);
typedef void(__thiscall* FrameStageNotifyFn)(void*, ClientFrameStage_t);
typedef void(__thiscall* RunCommandFn)(void*, C_CSPlayer*, CUserCmd*, IMoveHelper*);
typedef void(__thiscall* FinishMoveFn)(void*, C_CSPlayer*, CUserCmd*, CMoveData*);
typedef void(__thiscall* UpdateFn)(void*, bool, bool, int, int);
typedef void(__thiscall* ResetMouseFn)(void*);
typedef int(__thiscall* DrawModelExFn)(void*, ModelRenderInfo_t*);

typedef HRESULT(D3DAPI* ResetFn)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef HRESULT(D3DAPI* PresentFn)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);

using matrix3x4_t = Matrix3x4;
using VMatrix = Matrix4x4;

using QAngle = Vector3;
using Vector = Vector3;
using Vector2D = Vector2;
using Vector4D = Vector4;

//
// implementation
//
struct player_info_t
{
	char			name[32];
	int				userID;
	char			guid[33];
	unsigned int	friendsID;
	char			friendsName[32];
	bool			fakeplayer;
	bool			ishltv;
	unsigned long	customFiles[4];
	unsigned char	filesDownloaded;
};

struct mstudiobone_t
{
	int			sznameindex;
	int			parent;
	int			bonecontroller[6];
	Vector3		pos;
	float		quat[4];
	Vector3		rot;
	Vector3		posscale;
	Vector3		rotscale;
	Matrix3x4	poseToBone;
	float		qAlignment[4];
	int			flags;
	int			proctype;
	int			procindex;
	mutable int	physicsbone;
	int			surfacepropidx;
	int			contents;
	int			unused[8];

	const char* GetName() const
	{
		return ((const char*)this) + sznameindex;
	}
};

struct mstudiobbox_t
{
	int		bone;
	int		group;
	Vector3	bbmin;
	Vector3	bbmax;
	int		szhitboxnameindex;
	int		unused[3];
	float	radius;
	int		unused2[4];

	const char* GetName() const
	{
		if (szhitboxnameindex == 0)
			return "";

		return ((const char*)this) + szhitboxnameindex;
	}
};

struct mstudiohitboxset_t
{
	int sznameindex;
	int numhitboxes;
	int hitboxindex;

	const char* GetName() const
	{
		return ((const char*)this) + sznameindex;
	}

	mstudiobbox_t* GetHitbox(int i) const
	{
		if (i < 0 || i >= numhitboxes)
			return nullptr;

		return (mstudiobbox_t*)(((byte*)this) + hitboxindex) + i;
	}

	int GetHitboxCount() const
	{
		return numhitboxes;
	}
};

struct studiohdr_t
{
	int			id;
	int			version;
	long		checksum;
	char		name[64];
	int			length;
	Vector3		eyeposition;
	Vector3		illumposition;
	Vector3		hull_min;
	Vector3		hull_max;
	Vector3		view_bbmin;
	Vector3		view_bbmax;
	int			flags;
	int			numbones;
	int			boneindex;
	int			numbonecontrollers;
	int			bonecontrollerindex;
	int			numhitboxsets;
	int			hitboxsetindex;
	int			numlocalanim;
	int			localanimindex;
	mutable int	activitylistversion;
	mutable int	eventsindexed;
	int			numtextures;
	int			textureindex;

	const char* GetName() const
	{
		return name;
	}

	mstudiobone_t* GetBone(int i) const
	{
		return (mstudiobone_t*)(((byte*)this) + boneindex) + i;
	}

	int GetBoneCount() const
	{
		return numbones;
	}

	mstudiohitboxset_t* GetHitboxSet(int i) const
	{
		if (i < 0 || i >= numhitboxsets)
			return nullptr;

		return (mstudiohitboxset_t*)(((byte*)this) + hitboxsetindex) + i;
	}
};

struct cplane_t
{
	Vector3	normal;
	float	dist;
	byte	type;
	byte	signbits;
	byte	pad[2];
};

struct csurface_t
{
	const char*		name;
	short			surfaceProps;
	unsigned short	flags;
};

struct Ray_t
{
	Vector3				m_Start;
	Vector3				m_Delta;
	Vector3				m_StartOffset;
	Vector3				m_Extents;

	bool				m_IsRay;
	bool				m_IsSwept;

	void Set(const Vector3& vStart, const Vector3& vEnd)
	{
		m_Delta.x = vEnd.x - vStart.x;
		m_Delta.y = vEnd.y - vStart.y;
		m_Delta.z = vEnd.z - vStart.z;
		m_IsSwept = (m_Delta.LengthSqr() != 0);
		m_Extents.Set();
		m_IsRay = true;
		m_StartOffset.Set();
		m_Start.x = vStart.x;
		m_Start.y = vStart.y;
		m_Start.z = vStart.z;
	}

	void Init(const Vector& start, const Vector& end)
	{
		Set(start, end);
	}

	void Init1(Vector const& start, Vector const& end, Vector const& mins, Vector const& maxs)
	{
		VectorSubtract(end, start, m_Delta);

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		VectorSubtract(maxs, mins, m_Extents);
		m_Extents *= 0.5f;
		m_IsRay = (m_Extents.LengthSqr() < 1e-6);

		// Offset m_Start to be in the center of the box...
		VectorAdd(mins, maxs, m_StartOffset);
		m_StartOffset *= 0.5f;
		VectorAdd(start, m_StartOffset, m_Start);
		m_StartOffset *= -1.0f;
	}
};

struct surfacephysicsparams_t
{
	float	friction;
	float	elasticity;
	float	density;
	float	thickness;
	float	dampening;
};

struct surfaceaudioparams_t
{
	float	reflectivity;
	float	hardnessFactor;
	float	roughnessFactor;

	float	roughThreshold;
	float	hardThreshold;
	float	hardVelocityThreshold;
};

struct surfacesoundnames_t
{
	unsigned short	stepleft;
	unsigned short	stepright;

	unsigned short	impactSoft;
	unsigned short	impactHard;

	unsigned short	scrapeSmooth;
	unsigned short	scrapeRough;

	unsigned short	bulletImpact;
	unsigned short	rolling;

	unsigned short	breakSound;
	unsigned short	strainSound;
};

struct surfacesoundhandles_t
{
	short	stepleft;
	short	stepright;

	short	impactSoft;
	short	impactHard;

	short	scrapeSmooth;
	short	scrapeRough;

	short	bulletImpact;
	short	rolling;

	short	breakSound;
	short	strainSound;
};

struct surfacegameprops_t
{
	float			maxSpeedFactor;
	float			jumpFactor;

	unsigned short	material;

	unsigned char	climbable;
	unsigned char	pad;
};

struct surfacedata_t
{
	surfacephysicsparams_t	physics;
	surfaceaudioparams_t	audio;
	surfacesoundnames_t		sounds;
	surfacegameprops_t		game;

	surfacesoundhandles_t	soundhandles;
};

struct string_t
{
public:
	bool operator!() const
	{
		return (pszValue == nullptr);
	}

	bool operator==(const string_t& rhs) const
	{
		return (pszValue == rhs.pszValue);
	}

	bool operator!=(const string_t& rhs) const
	{
		return (pszValue != rhs.pszValue);
	}

	bool operator<(const string_t& rhs) const
	{
		return ((void*)pszValue < (void*)rhs.pszValue);
	}

	const char* ToCStr() const
	{
		return (pszValue) ? pszValue : "";
	}

protected:
	const char* pszValue;
};

struct ModelRenderInfo_t
{
	int						flags;
	IClientRenderable*		pRenderable;
	ModelInstanceHandle_t	instance;
	int						ent_index;
	const model_t*			pModel;
	Vector3					origin;
	Vector3					angles;
	int						skin;
	int						body;
	int						hitboxset;
	const Matrix3x4*		pModelToWorld;
	const Matrix3x4*		pLightingOffset;
	const Vector3*			pLightingOrigin;

	ModelRenderInfo_t(void)
		: pModelToWorld(nullptr),
		pLightingOffset(nullptr),
		pLightingOrigin(nullptr)
	{

	}
};

class IMaterial
{
public:
	void SetMaterialVarFlag(MaterialVarFlags_t flag, bool state)
	{
		typedef void(__thiscall* SetMaterialVarFlagFn)(void*, MaterialVarFlags_t, bool);
		return Memory::VCall< SetMaterialVarFlagFn >(this, 31)(this, flag, state);
	}
};

class ConVar
{
public:
	float GetFloat()
	{
		return m_fValue;
	}

	int GetInt()
	{
		return m_nValue;
	}

	bool GetBool()
	{
		return !!GetInt();
	}

public:
	void*		m_pVmt;
	ConVar*		m_pNext;
	BOOL		m_bRegistered;
	const char*	m_pszName;
	const char*	m_pszHelpString;
	int			m_nFlags;
	ConVar*		m_pParent;
	const char*	m_pszDefaultValue;
	char*		m_pszString;
	int			m_StringLength;
	float		m_fValue;
	int			m_nValue;
	bool		m_bHasMin;
	float		m_fMinVal;
	bool		m_bHasMax;
	float		m_fMaxVal;
	void*		m_fnChangeCallback;
};

class ClientClass
{
public:
	void*			m_pCreateFn;
	void*			m_pCreateEventFn;
	const char*		m_pNetworkName;
	RecvTable*		m_pRecvTable;
	ClientClass*	m_pNext;
	int				m_ClassID;
};

class CGlobalVarsBase
{
public:
	float	realtime;
	int		framecount;
	float	absoluteframetime;
	float	curtime;
	float	frametime;
	int		maxClients;
	int		tickcount;
	float	interval_per_tick;
	float	interpolation_amount;
	int		simTicksThisFrame;
	int		network_protocol;
	void*	pSaveData;
	bool	m_bClient;
	int		nTimestampNetworkingBase;
	int		nTimestampRandomizeWindow;
};

class CGlobalVars : public CGlobalVarsBase
{
public:
	string_t		mapname;
	string_t		mapGroupName;
	int				mapversion;
	string_t		startspot;
	MapLoadType_t	eLoadType;
	bool			bMapLoadFailed;
	bool			deathmatch;
	bool			coop;
	bool			teamplay;
	int				maxEntities;
	int				serverCount;
	void*			pEdicts;
};
class CNetChan
{
public:
	MakePad(0x0008);
	int m_nOutSequenceNr;
};
class CUserCmd
{
public:
	CUserCmd()
	{
		Reset();
	}

	virtual ~CUserCmd()
	{
	}

	void Reset()
	{
		command_number = 0;
		tick_count = 0;
		viewangles.Set();
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		buttons = 0;
		impulse = 0;
		weaponselect = 0;
		weaponsubtype = 0;
		random_seed = 0;
		mousedx = 0;
		mousedy = 0;
		hasbeenpredicted = false;
	}

	CUserCmd& operator=(const CUserCmd& src)
	{
		if (this == &src)
			return *this;

		command_number = src.command_number;
		tick_count = src.tick_count;
		viewangles = src.viewangles;
		forwardmove = src.forwardmove;
		sidemove = src.sidemove;
		upmove = src.upmove;
		buttons = src.buttons;
		impulse = src.impulse;
		weaponselect = src.weaponselect;
		weaponsubtype = src.weaponsubtype;
		random_seed = src.random_seed;
		mousedx = src.mousedx;
		mousedy = src.mousedy;
		hasbeenpredicted = src.hasbeenpredicted;

		return *this;
	}

	CUserCmd(const CUserCmd& src)
	{
		*this = src;
	}

public:
	int		command_number;
	int		tick_count;
	Vector3	viewangles;
	float	forwardmove;
	float	sidemove;
	float	upmove;
	int		buttons;
	byte	impulse;
	int		weaponselect;
	int		weaponsubtype;
	int		random_seed;
	short	mousedx;
	short	mousedy;
	bool	hasbeenpredicted;
};

class CBaseTrace
{
public:
	Vector3			startpos;
	Vector3			endpos;
	cplane_t		plane;
	float			fraction;
	int				contents;
	unsigned short	dispFlags;
	bool			allsolid;
	bool			startsolid;
};

class CGameTrace : public CBaseTrace
{
public:
	float			fractionleftsolid;
	csurface_t		surface;
	int				hitgroup;
	short			physicsbone;
	C_BaseEntity*	m_pEnt;
	int				hitbox;

public:
	bool			DidHitWorld(void) const;
	bool			DidHitNonWorldEntity(void) const;
};

class CMoveData
{
private:
	MakePad(1024);
};

class IBaseClientDLL
{
public:
	ClientClass* GetAllClasses()
	{
		typedef ClientClass* (__thiscall* GetAllClassesFn)(void*);
		return Memory::VCall< GetAllClassesFn >(this, IBaseClientDLL_GetAllClasses)(this);
	}
};

class IClientEntityList
{
public:

	C_BaseEntity* GetBaseEntity(int index)
	{
		typedef C_BaseEntity* (__thiscall* GetBaseEntityFn)(void*, int);
		return Memory::VCall< GetBaseEntityFn >(this, IClientEntityList_GetBaseEntity)(this, index);
	}

	C_BaseEntity* GetBaseEntityFromHandle(CBaseHandle hEntity)
	{
		typedef C_BaseEntity* (__thiscall* GetBaseEntityFromHandleFn)(void*, CBaseHandle);
		return Memory::VCall< GetBaseEntityFromHandleFn >(this, IClientEntityList_GetBaseEntityFromHandle)(this, hEntity);
	}

	int GetHighestEntityIndex()
	{
		typedef int(__thiscall* GetHighestEntityIndexFn)(void*);
		return Memory::VCall< GetHighestEntityIndexFn >(this, IClientEntityList_GetHighestEntityIndex)(this);
	}
};

class IGameMovement
{
public:
	void ProcessMovement(C_CSPlayer* pPlayer, CMoveData* pMove)
	{
		typedef void(__thiscall* ProcessMovementFn)(void*, C_CSPlayer*, CMoveData*);
		Memory::VCall< ProcessMovementFn >(this, IGameMovement_ProcessMovement)(this, pPlayer, pMove);
	}
};

class IPrediction
{
public:
	void Update(int startframe, bool validframe, int incoming_acknowledged, int outgoing_command)
	{
		typedef void(__thiscall* UpdateFn)(void*, int, bool, int, int);
		Memory::VCall< UpdateFn >(this, 3)(this, startframe, validframe, incoming_acknowledged, outgoing_command);
	}

	void SetupMove(C_CSPlayer* pPlayer, CUserCmd* pCmd, IMoveHelper* pHelper, CMoveData* pMove)
	{
		typedef void(__thiscall* SetupMoveFn)(void*, C_CSPlayer*, CUserCmd*, IMoveHelper*, CMoveData*);
		Memory::VCall< SetupMoveFn >(this, IPrediction_SetupMove)(this, pPlayer, pCmd, pHelper, pMove);
	}

	void FinishMove(C_CSPlayer* pPlayer, CUserCmd* pCmd, CMoveData* pMove)
	{
		typedef void(__thiscall* FinishMoveFn)(void*, C_CSPlayer*, CUserCmd*, CMoveData*);
		Memory::VCall< FinishMoveFn >(this, IPrediction_FinishMove)(this, pPlayer, pCmd, pMove);
	}
};

#pragma pack( push, 1 )

class IInput
{
public:
	CUserCmd* GetUserCmd(int sequence_number)
	{
		return &m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
	}

public:
	MakePad(0x008D);
	bool m_fCameraInThirdPerson = false;
	MakePad(0x0002);
	Vector m_vecCameraOffset = {};
	MakePad(0x0028);
	CUserCmd* m_pCommands = nullptr;

};
#pragma pack( pop )

#define FLOW_OUTGOING 0
#define FLOW_INCOMING 1
#define MAX_FLOWS 2

class INetChannelInfo
{
public:
	enum
	{
		GENERIC = 0,	// must be first and is default group
		LOCALPLAYER,	// bytes for local player entity update
		OTHERPLAYERS,	// bytes for other players update
		ENTITIES,		// all other entity bytes
		SOUNDS,			// game sounds
		EVENTS,			// event messages
		USERMESSAGES,	// user messages
		ENTMESSAGES,	// entity messages
		VOICE,			// voice data
		STRINGTABLE,	// a stringtable update
		MOVE,			// client move cmds
		STRINGCMD,		// string command
		SIGNON,			// various signondata
		TOTAL,			// must be last and is not a real group
	};

	virtual const char* GetName() const = 0;	// get channel name
	virtual const char* GetAddress() const = 0; // get channel IP address as string
	virtual float GetTime() const = 0;	// current net time
	virtual float GetTimeConnected() const = 0;	// get connection time in seconds
	virtual int GetBufferSize() const = 0;	// netchannel packet history size
	virtual int GetDataRate() const = 0; // send data rate in byte/sec

	virtual bool IsLoopback() const = 0;	// true if loopback channel
	virtual bool IsTimingOut() const = 0;	// true if timing out
	virtual bool IsPlayback() const = 0;	// true if demo playback

	virtual float GetLatency(int flow) const = 0;	 // current latency (RTT), more accurate but jittering
	virtual float GetAvgLatency(int flow) const = 0; // average packet latency in seconds
	virtual float GetAvgLoss(int flow) const = 0;	 // avg packet loss[0..1]
	virtual float GetAvgChoke(int flow) const = 0;	 // avg packet choke[0..1]
	virtual float GetAvgData(int flow) const = 0;	 // data flow in bytes/sec
	virtual float GetAvgPackets(int flow) const = 0; // avg packets/sec
	virtual int GetTotalData(int flow) const = 0;	 // total flow in/out in bytes
	virtual int GetSequenceNr(int flow) const = 0;	// last send seq number
	virtual bool IsValidPacket(int flow, int frame_number) const = 0; // true if packet was not lost/dropped/chocked/flushed
	virtual float GetPacketTime(int flow, int frame_number) const = 0; // time when packet was send
	virtual int GetPacketBytes(int flow, int frame_number, int group) const = 0; // group size of this packet
	virtual bool GetStreamProgress(int flow, int *received, int *total) const = 0;  // TCP progress if transmitting
	virtual float GetTimeSinceLastReceived(void) const = 0;	// get time since last recieved packet in seconds
	virtual	float GetCommandInterpolationAmount(int flow, int frame_number) const = 0;
	virtual void GetPacketResponseLatency(int flow, int frame_number, int *pnLatencyMsecs, int *pnChoke) const = 0;
	virtual void GetRemoteFramerate(float *pflFrameTime, float *pflFrameTimeStdDeviation) const = 0;

	virtual float GetTimeoutSeconds() const = 0;

	int32_t m_nOutSequenceNr;    //0x0018 last send outgoing sequence number
};

class	IDemoRecorder;
class	INetMessage;
class	INetChannelHandler;
class	INetChannelInfo;
typedef struct netpacket_s netpacket_t;
typedef struct netadr_s	netadr_t;

class bf_write;
class CViewSetup
{
public:
	CViewSetup()
	{
		m_flAspectRatio = 0.0f;
		// These match mat_dof convars
		m_flNearBlurDepth = 20.0;
		m_flNearFocusDepth = 100.0;
		m_flFarFocusDepth = 250.0;
		m_flFarBlurDepth = 1000.0;
		m_flNearBlurRadius = 10.0;
		m_flFarBlurRadius = 5.0;
		m_nDoFQuality = 0;
		m_bCustomViewMatrix = false;

		//m_bRenderToSubrectOfLargerScreen = false;
		//m_bDoBloomAndToneMapping = true;
		//m_nMotionBlurMode = MOTION_BLUR_GAME;
		//m_bDoDepthOfField = false;
		//m_bHDRTarget = false;
		//m_bOffCenter = false;
		//m_bCacheFullSceneState = false;
		//m_bDrawWorldNormal = false;
		//m_bCullFrontFaces = false;
		//		//m_bUseExplicitViewVector = false;
		//m_bRenderFlashlightDepthTranslucents = false;
	}

	// left side of view window
	int			x, x_old;
	// top side of view window
	int			y, y_old;
	// width of view window
	int			width, width_old;
	// height of view window
	int			height, height_old;
	// the rest are only used by 3D views
	// Orthographic projection?
	bool		m_bOrtho;
	// View-space rectangle for ortho projection.
	float		m_OrthoLeft;
	float		m_OrthoTop;
	float		m_OrthoRight;
	float		m_OrthoBottom;
	bool		m_bCustomViewMatrix; // 0x34
	Matrix3x4	m_matCustomViewMatrix; // 0x38
	char		pad_0x68[0x48]; // 0x68
								// horizontal FOV in degrees
	float		fov; //	0xB0
					 // horizontal FOV in degrees for in-view model
	float		fovViewmodel;
	// 3D origin of camera
	Vector		origin;
	// heading of camera (pitch, yaw, roll)
	Vector		angles;
	// local Z coordinate of near plane of camera
	float		zNear;
	// local Z coordinate of far plane of camera
	float		zFar;
	// local Z coordinate of near plane of camera ( when rendering view model )
	float		zNearViewmodel;
	// local Z coordinate of far plane of camera ( when rendering view model )
	float		zFarViewmodel;
	// The aspect ratio to use for computing the perspective projection matrix
	// (0.0f means use the viewport)
	float		m_flAspectRatio;
	// Camera settings to control depth of field
	float		m_flNearBlurDepth;
	float		m_flNearFocusDepth;
	float		m_flFarFocusDepth;
	float		m_flFarBlurDepth;
	float		m_flNearBlurRadius;
	float		m_flFarBlurRadius;
	int			m_nDoFQuality;
	// Camera settings to control motion blur
	//MotionBlurMode_t	m_nMotionBlurMode;
	float		m_flShutterTime;				// In seconds
	Vector		m_vShutterOpenPosition;			// Start of frame or "shutter open"
	Vector		m_shutterOpenAngles;			//
	Vector		m_vShutterClosePosition;		// End of frame or "shutter close"
	Vector		m_shutterCloseAngles;			// 
												// Controls for off-center projection (needed for poster rendering)
	float		m_flOffCenterTop;
	float		m_flOffCenterBottom;
	float		m_flOffCenterLeft;
	float		m_flOffCenterRight;
	int			m_EdgeBlur; // 0x148
							//end
							//bool		m_bOffCenter:1;
							//// set to true if this is to draw into a subrect of the larger screen
							//// this really is a hack, but no more than the rest of the way this class is used
							//bool		m_bRenderToSubrectOfLargerScreen:1;
							//// Controls that the SFM needs to tell the engine when to do certain post-processing steps
							//bool		m_bDoBloomAndToneMapping:1;
							//bool		m_bDoDepthOfField:1;
							//bool		m_bHDRTarget:1;
							//bool		m_bDrawWorldNormal:1;
							//bool		m_bCullFrontFaces:1;
							//// Cached mode for certain full-scene per-frame varying state such as sun entity coverage
							//bool		m_bCacheFullSceneState:1;
							//bool		m_bRenderFlashlightDepthTranslucents:1;

}; // size = 0x14C = 332

class INetChannel : public INetChannelInfo
{
public:
	virtual	~INetChannel(void)
	{ };

	virtual void	SetDataRate(float rate) = 0;
	virtual bool	RegisterMessage(INetMessage *msg) = 0;
	virtual bool	StartStreaming(unsigned int challengeNr) = 0;
	virtual void	ResetStreaming(void) = 0;
	virtual void	SetTimeout(float seconds) = 0;
	virtual void	SetDemoRecorder(IDemoRecorder *recorder) = 0;
	virtual void	SetChallengeNr(unsigned int chnr) = 0;

	virtual void	Reset(void) = 0;
	virtual void	Clear(void) = 0;
	virtual void	Shutdown(const char *reason) = 0;

	virtual void	ProcessPlayback(void) = 0;
	virtual bool	ProcessStream(void) = 0;
	virtual void	ProcessPacket(struct netpacket_s* packet, bool bHasHeader) = 0;

	virtual bool	SendNetMsg(INetMessage &msg, bool bForceReliable = false, bool bVoice = false) = 0;

	virtual bool	SendData(bf_write &msg, bool bReliable = true) = 0;
	virtual bool	SendFile(const char *filename, unsigned int transferID) = 0;
	virtual void	DenyFile(const char *filename, unsigned int transferID) = 0;
	virtual void	RequestFile_OLD(const char *filename, unsigned int transferID) = 0;	// get rid of this function when we version the 
	virtual void	SetChoked(void) = 0;
	virtual int		SendDatagram(bf_write *data) = 0;
	virtual bool	Transmit(bool onlyReliable = false) = 0;

	virtual const netadr_t	&GetRemoteAddress(void) const = 0;
	virtual INetChannelHandler *GetMsgHandler(void) const = 0;
	virtual int				GetDropNumber(void) const = 0;
	virtual int				GetSocket(void) const = 0;
	virtual unsigned int	GetChallengeNr(void) const = 0;
	virtual void			GetSequenceData(int &nOutSequenceNr, int &nInSequenceNr, int &nOutSequenceNrAck) = 0;
	virtual void			SetSequenceData(int nOutSequenceNr, int nInSequenceNr, int nOutSequenceNrAck) = 0;

	virtual void	UpdateMessageStats(int msggroup, int bits) = 0;
	virtual bool	CanPacket(void) const = 0;
	virtual bool	IsOverflowed(void) const = 0;
	virtual bool	IsTimedOut(void) const = 0;
	virtual bool	HasPendingReliableData(void) = 0;

	virtual void	SetFileTransmissionMode(bool bBackgroundMode) = 0;
	virtual void	SetCompressionMode(bool bUseCompression) = 0;
	virtual unsigned int RequestFile(const char *filename) = 0;
	virtual float	GetTimeSinceLastReceived(void) const = 0;	// get time since last received packet in seconds

	virtual void	SetMaxBufferSize(bool bReliable, int nBytes, bool bVoice = false) = 0;

	virtual bool	IsNull() const = 0;
	virtual int		GetNumBitsWritten(bool bReliable) = 0;
	virtual void	SetInterpolationAmount(float flInterpolationAmount) = 0;
	virtual void	SetRemoteFramerate(float flFrameTime, float flFrameTimeStdDeviation) = 0;

	// Max # of payload bytes before we must split/fragment the packet
	virtual void	SetMaxRoutablePayloadSize(int nSplitSize) = 0;
	virtual int		GetMaxRoutablePayloadSize() = 0;

	virtual int		GetProtocolVersion() = 0;
};

class old_bf_write
{
public:
	old_bf_write();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	old_bf_write(void *pData, int nBytes, int nMaxBits = -1);
	old_bf_write(const char *pDebugName, void *pData, int nBytes, int nMaxBits = -1);

	// Start writing to the specified buffer.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartWriting(void *pData, int nBytes, int iStartBit = 0, int nMaxBits = -1);

	// Restart buffer writing.
	void			Reset();

	// Get the base pointer.
	unsigned char*	GetBasePointer()
	{
		return m_pData;
	}

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow(bool bAssert);

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName();
	void			SetDebugName(const char *pDebugName);


	// Seek to a specific position.
public:

	void			SeekToBit(int bitPos);


	// Bit functions.
public:

	void			WriteOneBit(int nValue);
	void			WriteOneBitNoCheck(int nValue);
	void			WriteOneBitAt(int iBit, int nValue);

	// Write signed or unsigned. Range is only checked in debug.
	void			WriteUBitLong(unsigned int data, int numbits, bool bCheckRange = true);
	void			WriteSBitLong(int data, int numbits);

	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void			WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool			WriteBits(const void *pIn, int nBits);

	// writes an unsigned integer with variable bit length
	void			WriteUBitVar(unsigned int data);

	// Copy the bits straight out of pIn. This seeks pIn forward by nBits.
	// Returns an error if this buffer or the read buffer overflows.
	bool			WriteBitsFromBuffer(class bf_read *pIn, int nBits);

	void			WriteBitAngle(float fAngle, int numbits);
	void			WriteBitCoord(const float f);
	void			WriteBitCoordMP(const float f, bool bIntegral, bool bLowPrecision);
	void			WriteBitFloat(float val);
	void			WriteBitVec3Coord(const Vector& fa);
	void			WriteBitNormal(float f);
	void			WriteBitVec3Normal(const Vector& fa);
	void			WriteBitAngles(const QAngle& fa);


	// Byte functions.
public:

	void			WriteChar(int val);
	void			WriteByte(int val);
	void			WriteShort(int val);
	void			WriteWord(int val);
	void			WriteLong(long val);
	void			WriteLongLong(__int64 val);
	void			WriteFloat(float val);
	bool			WriteBytes(const void *pBuf, int nBytes);

	// Returns false if it overflows the buffer.
	bool			WriteString(const char *pStr);


	// Status.
public:

	// How many bytes are filled in?
	int				GetNumBytesWritten();
	int				GetNumBitsWritten();
	int				GetMaxNumBits();
	int				GetNumBitsLeft();
	int				GetNumBytesLeft();
	unsigned char*	GetData();

	// Has the buffer overflowed?
	bool			CheckForOverflow(int nBits);
	inline bool		IsOverflowed() const
	{
		return m_bOverflow;
	}

	inline void		SetOverflowFlag();


public:
	// The current buffer.
	unsigned char*	m_pData;
	int				m_nDataBytes;
	int				m_nDataBits;

	// Where we are in the buffer.
	int				m_iCurBit;

private:

	// Errors?
	bool			m_bOverflow;

	bool			m_bAssertOnOverflow;
	const char		*m_pDebugName;
};


//-----------------------------------------------------------------------------
// Inlined methods
//-----------------------------------------------------------------------------

// How many bytes are filled in?


inline int old_bf_write::GetNumBitsWritten()
{
	return m_iCurBit;
}

inline int old_bf_write::GetMaxNumBits()
{
	return m_nDataBits;
}

inline int old_bf_write::GetNumBitsLeft()
{
	return m_nDataBits - m_iCurBit;
}

inline int old_bf_write::GetNumBytesLeft()
{
	return GetNumBitsLeft() >> 3;
}

inline unsigned char* old_bf_write::GetData()
{
	return m_pData;
}

inline bool old_bf_write::CheckForOverflow(int nBits)
{
	if (m_iCurBit + nBits > m_nDataBits)
	{
		SetOverflowFlag();
	}

	return m_bOverflow;
}

inline void old_bf_write::SetOverflowFlag()
{
	if (m_bAssertOnOverflow)
	{
		assert(false);
	}

	m_bOverflow = true;
}

inline void old_bf_write::WriteOneBitNoCheck(int nValue)
{
	if (nValue)
		m_pData[m_iCurBit >> 3] |= (1 << (m_iCurBit & 7));
	else
		m_pData[m_iCurBit >> 3] &= ~(1 << (m_iCurBit & 7));

	++m_iCurBit;
}

inline void old_bf_write::WriteOneBit(int nValue)
{
	if (!CheckForOverflow(1))
		WriteOneBitNoCheck(nValue);
}


inline void	old_bf_write::WriteOneBitAt(int iBit, int nValue)
{
	if (iBit + 1 > m_nDataBits)
	{
		SetOverflowFlag();
		return;
	}

	if (nValue)
		m_pData[iBit >> 3] |= (1 << (iBit & 7));
	else
		m_pData[iBit >> 3] &= ~(1 << (iBit & 7));
}

#define WordSwap  WordSwapAsm
#define DWordSwap DWordSwapAsm

#pragma warning(push)
#pragma warning (disable:4035) // no return value

template <typename T>
inline T WordSwapAsm(T w)
{
	__asm
	{
		mov ax, w
		xchg al, ah
	}
}

template <typename T>
inline T DWordSwapAsm(T dw)
{
	__asm
	{
		mov eax, dw
		bswap eax
	}
}

#pragma warning(pop)

#if defined(LITTLE_ENDIAN)

#define BigShort( val )				WordSwap( val )
#define BigWord( val )				WordSwap( val )
#define BigLong( val )				DWordSwap( val )
#define BigDWord( val )				DWordSwap( val )
#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define SwapShort( val )			BigShort( val )
#define SwapWord( val )				BigWord( val )
#define SwapLong( val )				BigLong( val )
#define SwapDWord( val )			BigDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )

#elif defined(BIG_ENDIAN)

#define BigShort( val )				( val )
#define BigWord( val )				( val )
#define BigLong( val )				( val )
#define BigDWord( val )				( val )
#define LittleShort( val )			WordSwap( val )
#define LittleWord( val )			WordSwap( val )
#define LittleLong( val )			DWordSwap( val )
#define LittleDWord( val )			DWordSwap( val )
#define SwapShort( val )			LittleShort( val )
#define SwapWord( val )				LittleWord( val )
#define SwapLong( val )				LittleLong( val )
#define SwapDWord( val )			LittleDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else

// @Note (toml 05-02-02): this technique expects the compiler to
// optimize the expression and eliminate the other path. On any new
// platform/compiler this should be tested.
inline short BigShort(short val)
{
	int test = 1; return (*(char *)&test == 1) ? WordSwap(val) : val;
}
inline unsigned short BigWord(unsigned short val)
{
	int test = 1; return (*(char *)&test == 1) ? WordSwap(val) : val;
}
inline long BigLong(long val)
{
	int test = 1; return (*(char *)&test == 1) ? DWordSwap(val) : val;
}
inline unsigned int BigDWord(unsigned int val)
{
	int test = 1; return (*(char *)&test == 1) ? DWordSwap(val) : val;
}
inline short LittleShort(short val)
{
	int test = 1; return (*(char *)&test == 1) ? val : WordSwap(val);
}
inline unsigned short LittleWord(unsigned short val)
{
	int test = 1; return (*(char *)&test == 1) ? val : WordSwap(val);
}
inline long LittleLong(long val)
{
	int test = 1; return (*(char *)&test == 1) ? val : DWordSwap(val);
}
inline unsigned int LittleDWord(unsigned int val)
{
	int test = 1; return (*(char *)&test == 1) ? val : DWordSwap(val);
}
inline short SwapShort(short val)
{
	return WordSwap(val);
}
inline unsigned short SwapWord(unsigned short val)
{
	return WordSwap(val);
}
inline long SwapLong(long val)
{
	return DWordSwap(val);
}
inline unsigned int SwapDWord(unsigned int val)
{
	return DWordSwap(val);
}

#endif

inline unsigned long LoadLittleDWord(unsigned long *base, unsigned int dwordIndex)
{
	return LittleDWord(base[dwordIndex]);
}

inline void StoreLittleDWord(unsigned long *base, unsigned int dwordIndex, unsigned long dword)
{
	base[dwordIndex] = LittleDWord(dword);
}

inline void old_bf_write::WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange)
{
#ifdef _DEBUG
	// Make sure it doesn't overflow.
	if (bCheckRange && numbits < 32)
	{
		if (curData >= (unsigned long)(1 << numbits))
		{

		}
	}
	assert(numbits >= 0 && numbits <= 32);
#endif

	extern unsigned long g_BitWriteMasks[32][33];

	// Bounds checking..
	if ((m_iCurBit + numbits) > m_nDataBits)
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		return;
	}

	int nBitsLeft = numbits;
	int iCurBit = m_iCurBit;

	// Mask in a dword.
	unsigned int iDWord = iCurBit >> 5;
	assert((iDWord * 4 + sizeof(long)) <= (unsigned int)m_nDataBytes);

	unsigned long iCurBitMasked = iCurBit & 31;

	unsigned long dword = LoadLittleDWord((unsigned long*)m_pData, iDWord);

	dword &= g_BitWriteMasks[iCurBitMasked][nBitsLeft];
	dword |= curData << iCurBitMasked;

	// write to stream (lsb to msb ) properly
	StoreLittleDWord((unsigned long*)m_pData, iDWord, dword);

	// Did it span a dword?
	int nBitsWritten = 32 - iCurBitMasked;
	if (nBitsWritten < nBitsLeft)
	{
		nBitsLeft -= nBitsWritten;
		curData >>= nBitsWritten;

		// read from stream (lsb to msb) properly 
		dword = LoadLittleDWord((unsigned long*)m_pData, iDWord + 1);

		dword &= g_BitWriteMasks[0][nBitsLeft];
		dword |= curData;

		// write to stream (lsb to msb) properly 
		StoreLittleDWord((unsigned long*)m_pData, iDWord + 1, dword);
	}

	m_iCurBit += numbits;
}

// using bf_write = old_bf_write;
class bf_write : public old_bf_write
{
public:

};
class IGameEvent
{
public:
	virtual ~IGameEvent() { }
	virtual const char* GetName() const = 0;
	virtual bool IsReliable() const = 0;
	virtual bool IsLocal() const = 0;
	virtual bool IsEmpty(const char *keyName = NULL) = 0;
	virtual bool GetBool(const char *keyName = NULL, bool defaultValue = false) = 0;
	virtual int GetInt(const char *keyName = NULL, int defaultValue = 0) = 0;
	virtual float GetFloat(const char *keyName = NULL, float defaultValue = 0.0f) = 0;
	virtual const char* GetString(const char *keyName = NULL, const char *defaultValue = "") = 0;
	virtual void SetBool(const char *keyName, bool value) = 0;
	virtual void SetInt(const char *keyName, int value) = 0;
	virtual void SetFloat(const char *keyName, float value) = 0;
	virtual void SetString(const char *keyName, const char *value) = 0;
};

class IGameEventListener2
{
public:
	virtual	~IGameEventListener2() { }
	virtual void FireGameEvent(IGameEvent *event) = 0;
};

class IGameEventManager2
{
public:
	virtual	~IGameEventManager2() { };

	// load game event descriptions from a file eg "resource\gameevents.res"
	virtual int LoadEventsFromFile(const char *filename) = 0;

	// removes all and anything
	virtual void  Reset() = 0;

	// adds a listener for a particular event
	virtual bool AddListener(IGameEventListener2 *listener, const char *name, bool bServerSide) = 0;

	// returns true if this listener is listens to given event
	virtual bool FindListener(IGameEventListener2 *listener, const char *name) = 0;

	// removes a listener 
	virtual void RemoveListener(IGameEventListener2 *listener) = 0;
};

class IVEngineClient
{
public:
	int GetPlayerForUserID(int userID)
	{
		return memory::vcall< 9u, int >(this, userID);
	}
	void GetScreenSize(int& width, int& height)
	{
		typedef void(__thiscall* GetScreenSizeFn)(void*, int&, int&);
		Memory::VCall< GetScreenSizeFn>(this, IVEngineClient_GetScreenSize)(this, width, height);
	}

	bool GetPlayerInfo(int ent_num, player_info_t* pinfo)
	{
		typedef bool(__thiscall* GetPlayerInfoFn)(void*, int, player_info_t*);
		return Memory::VCall< GetPlayerInfoFn >(this, IVEngineClient_GetPlayerInfo)(this, ent_num, pinfo);
	}

	int GetLocalPlayer(void)
	{
		typedef int(__thiscall* GetLocalPlayerFn)(void*);
		return Memory::VCall< GetLocalPlayerFn >(this, IVEngineClient_GetLocalPlayer)(this);
	}

	float GetLastTimeStamp()
	{
		using Fn = float(__thiscall*)(void*);
		return Memory::VCall< Fn >(this, 14)(this);
	}

	void GetViewAngles(Vector3& va)
	{
		typedef void(__thiscall* GetViewAnglesFn)(void*, Vector3&);
		Memory::VCall< GetViewAnglesFn >(this, IVEngineClient_GetViewAngles)(this, va);
	}
	void GetViewAngles0(QAngle* va)
	{
		typedef void(__thiscall* GetViewAnglesFn)(void*, QAngle*);
		Memory::VCall< GetViewAnglesFn >(this, IVEngineClient_GetViewAngles)(this, va);
	}
	void SetViewAngles(Vector3& va)
	{
		typedef void(__thiscall* SetViewAnglesFn)(void*, Vector3&);
		Memory::VCall< SetViewAnglesFn >(this, IVEngineClient_SetViewAngles)(this, va);
	}

	int GetMaxClients(void)
	{
		typedef int(__thiscall* GetMaxClientsFn)(void*);
		return Memory::VCall< GetMaxClientsFn >(this, IVEngineClient_GetMaxClients)(this);
	}

	const Matrix4x4& WorldToScreenMatrix(void)
	{
		typedef const Matrix4x4& (__thiscall* WorldToScreenMatrixFn)(void*);
		return Memory::VCall< WorldToScreenMatrixFn >(this, IVEngineClient_WorldToScreenMatrix)(this);
	}

	INetChannelInfo* GetNetChannelInfo()
	{
		using Fn = INetChannelInfo*(__thiscall*)(void*);
		return Memory::VCall< Fn >(this, 72)(this);
	}
};

class IVModelInfoClient
{
public:
	const char* GetModelName(const model_t* model)
	{
		typedef const char* (__thiscall* GetModelNameFn)(void*, const model_t*);
		return Memory::VCall< GetModelNameFn >(this, IVModelInfoClient_GetModelName)(this, model);
	}

	studiohdr_t* GetStudioModel(const model_t* mod)
	{
		typedef studiohdr_t* (__thiscall* GetStudioModelFn)(void*, const model_t*);
		return Memory::VCall< GetStudioModelFn >(this, IVModelInfoClient_GetStudioModel)(this, mod);
	}
};

class IVRenderView
{
public:
	void SetBlend(float blend)
	{
		typedef void(__thiscall* SetBlendFn)(void*, float);
		Memory::VCall< SetBlendFn >(this, IVRenderView_SetBlend)(this, blend);
	}

	void SetColorModulation(const float* color)
	{
		typedef void(__thiscall* SetColorModulationFn)(void*, const float*);
		Memory::VCall< SetColorModulationFn >(this, IVRenderView_SetColorModulation)(this, color);
	}
};

class IVModelRender
{
public:
	void ForcedMaterialOverride(IMaterial* mat)
	{
		typedef void(__thiscall* ForcedMaterialOverrideFn)(void*, IMaterial*);
		Memory::VCall< ForcedMaterialOverrideFn >(this, IVModelRender_ForcedMaterialOverride)(this, mat);
	}
};

class IEngineTrace
{
public:
	int GetPointContents(const Vector3& vecAbsPosition, IHandleEntity** ppEntity = nullptr)
	{
		typedef int(__thiscall* GetPointContentsFn)(void*, const Vector3&, IHandleEntity**);
		return Memory::VCall< GetPointContentsFn >(this, IEngineTrace_GetPointContents)(this, vecAbsPosition, ppEntity);
	}

	void TraceRay(const Ray_t& ray, unsigned int fMask, ITraceFilter* pTraceFilter, trace_t* pTrace)
	{
		typedef void(__thiscall* TraceRayFn)(void*, const Ray_t&, unsigned int, ITraceFilter*, trace_t*);
		Memory::VCall< TraceRayFn >(this, IEngineTrace_TraceRay)(this, ray, fMask, pTraceFilter, pTrace);
	}
};

class ICvar
{
public:
	ConVar* FindVar(const char* var_name)
	{
		typedef ConVar* (__thiscall* FindVarFn)(void*, const char*);
		return Memory::VCall< FindVarFn >(this, ICvar_FindVar)(this, var_name);
	}
};

class IPhysicsSurfaceProps
{
public:
	surfacedata_t* GetSurfaceData(int dataIndex)
	{
		typedef surfacedata_t* (__thiscall* GetSurfaceDataFn)(void*, int);
		return Memory::VCall< GetSurfaceDataFn >(this, IPhysicsSurfaceProps_GetSurfaceData)(this, dataIndex);
	}
};

class IMaterialSystem
{
public:
	IMaterial* CreateMaterial(const char* name, KeyValues* key)
	{
		typedef IMaterial* (__thiscall* CreateMaterialFn)(void*, const char*, KeyValues*);
		return Memory::VCall< CreateMaterialFn >(this, IMaterialSystem_CreateMaterial)(this, name, key);
	}

};

class KeyValues
{
public:
	KeyValues(const char* setName);

	bool LoadFromBuffer(const char* resourceName, const char* pBuffer, void* pFileSystem = nullptr, const char* pPathID = nullptr);

private:
	MakePad(0x25);
};

class D3DDeviceWrapper
{
public:
	IDirect3DDevice9* m_pD3DDevice;
};

class ITraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask) = 0;
	virtual TraceType_t GetTraceType(void) const = 0;
};

class CTraceFilter : public ITraceFilter
{
public:
	virtual TraceType_t GetTraceType(void) const;
};

class CTraceFilterSimple : public CTraceFilter
{
public:
	CTraceFilterSimple(void);
	CTraceFilterSimple(const C_BaseEntity* pPassEnt);

	virtual bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask);

private:
	const C_BaseEntity* m_pPassEnt;
};

class CTraceFilterSkipTwoEntities : public CTraceFilter
{
public:
	CTraceFilterSkipTwoEntities(const C_BaseEntity* pPassEnt, const C_BaseEntity* pPassEnt2);

	virtual bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask);

private:
	const C_BaseEntity* m_pPassEnt;
	const C_BaseEntity* m_pPassEnt2;
};

class IHandleEntity
{
public:
	auto SetRefEHandle(const CBaseHandle& handle) -> void
	{
		return memory::vcall< 1u, void >(this, handle);
	}

	auto GetRefEHandle() -> const CBaseHandle&
	{
		return memory::vcall< 2u, const CBaseHandle& >(this);
	}
};
class CTraceFilterWorldOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask);
	TraceType_t GetTraceType() const;
};
namespace Valve
{
	extern CCSWeaponInfo*		GetCSWpnData(C_WeaponCSBaseGun* pWeapon);
	extern D3DDeviceWrapper*	Dx9Device();

	extern void					SetPredictionSeed(CUserCmd* pCmd);
	extern void					SetPredictionPlayer(C_CSPlayer* pPlayer);

	extern void					RandomSeed(unsigned int nSeed);
	extern float				RandomFloat(float flMin, float flMax);
	extern int					RandomInt(int iMin, int iMax);

	extern void					GetBulletTypeParameters(int iBulletType, float& flPenetrationPower, float& flPenetrationDistance);
	extern void					ClipTraceToPlayers(const Vector3& vStart, const Vector3& vEnd, unsigned int iMask, ITraceFilter* pTraceFilter, trace_t* pTrace);
	extern void					GetMaterialParameters(int iMaterial, float& flPenetrationModifier, float& flDamageModifier);
	extern bool					TraceToExit(Vector3& vStart, Vector3& vDirection, Vector3& vEnd, float flStepSize, float flMaxDistance);
	extern int					GetPlayerModDamage(float flDamage, int iArmorValue, float flArmorRatio, int iHitgroup, bool bIsFriendly, bool bHasHelmet);
	extern float				GetHitgroupModDamage(float flDamage, int iHitgroup);
	class CClientState //: public CBaseClientState, public CClientFrameManager
	{
		typedef struct CustomFile_s
		{
		//	CRC32_t			crc;	//file CRC
			unsigned int	reqID;	// download request ID
	} CustomFile_t;

	public:
	//	CClientState();
//		~CClientState();

	public: // IConnectionlessPacketHandler interface:

		bool ProcessConnectionlessPacket(struct netpacket_s *packet);

	public: // CBaseClientState overrides:
		void Disconnect(bool bShowMainMenu);
	//	void FullConnect(netadr_t &adr);
		bool SetSignonState(int state, int count);
		void PacketStart(int incoming_sequence, int outgoing_acknowledged);
		void PacketEnd(void);
		void FileReceived(const char *fileName, unsigned int transferID);
		void FileRequested(const char *fileName, unsigned int transferID);
		void FileDenied(const char *fileName, unsigned int transferID);
		void ConnectionCrashed(const char * reason);
		void ConnectionClosing(const char * reason);
		const char *GetCDKeyHash(void);
	//	void SetFriendsID(uint friendsID, const char *friendsName);
		void SendClientInfo(void);
		void InstallStringTableCallback(char const *tableName);
		bool HookClientStringTable(char const *tableName);
		bool InstallEngineStringTableCallback(char const *tableName);

		void StartUpdatingSteamResources();
		void CheckUpdatingSteamResources();
		void CheckFileCRCsWithServer();
		void FinishSignonState_New();
		void ConsistencyCheck(bool bForce);
		void RunFrame();

		//void ReadEnterPVS( CEntityReadInfo &u );
		//void ReadLeavePVS( CEntityReadInfo &u );
		//void ReadDeltaEnt( CEntityReadInfo &u );
		//void ReadPreserveEnt( CEntityReadInfo &u );
		//void ReadDeletions( CEntityReadInfo &u );

		// In case the client DLL is using the old interface to set area bits,
		// copy what they've passed to us into the m_chAreaBits array (and 0xFF-out the m_chAreaPortalBits array).
		void UpdateAreaBits_BackwardsCompatible();

		// Used to be pAreaBits.
		unsigned char** GetAreaBits_BackwardCompatibility();

	public: // IServerMessageHandlers

			//PROCESS_NET_MESSAGE(Tick);
			//PROCESS_NET_MESSAGE(StringCmd);
			//PROCESS_SVC_MESSAGE(ServerInfo);
			//PROCESS_SVC_MESSAGE(ClassInfo);
			//PROCESS_SVC_MESSAGE(SetPause);
			//PROCESS_SVC_MESSAGE(VoiceInit);
			//PROCESS_SVC_MESSAGE(VoiceData);
			//PROCESS_SVC_MESSAGE(Sounds);
			//PROCESS_SVC_MESSAGE(FixAngle);
			//PROCESS_SVC_MESSAGE(CrosshairAngle);
			//PROCESS_SVC_MESSAGE(BSPDecal);
			//PROCESS_SVC_MESSAGE(GameEvent);
			//PROCESS_SVC_MESSAGE(UserMessage);
			//PROCESS_SVC_MESSAGE(EntityMessage);
			//PROCESS_SVC_MESSAGE(PacketEntities);
			//PROCESS_SVC_MESSAGE(TempEntities);
			//PROCESS_SVC_MESSAGE(Prefetch);

	public:
		//char pad_0x0000[0x8]; //0x0000 vtable is at ptr + 0x8
		//N00000285* ClientStateVtable; //0x0008 
		char pad_0x000C[0x4]; //0x000C
		void* N00000017; //0x0010 
		void* N00000018; //0x0014 
		void* N00000019; //0x0018 
		void* N0000001A; //0x001C 
		void* N0000001B; //0x0020 
		void* N0000001C; //0x0024 
		void* N0000001D; //0x0028 
		void* N0000001E; //0x002C 
		void* N0000001F; //0x0030 
		void* N00000020; //0x0034 
		void* N00000021; //0x0038 
		void* N00000022; //0x003C 
		void* N00000023; //0x0040 
		void* N00000024; //0x0044 
		void* N00000025; //0x0048 
		void* N00000026; //0x004C 
		void* N00000027; //0x0050 
		void* N00000028; //0x0054 
		void* N00000029; //0x0058 
		void* N0000002A; //0x005C 
		void* N0000002B; //0x0060 
		void* N0000002C; //0x0064 
		void* N0000002D; //0x0068 
		void* N0000002E; //0x006C 
		void* N0000002F; //0x0070 
		void* N00000030; //0x0074 
		void* N0000039C; //0x0078 
		void* N00000032; //0x007C 
		void* N00000033; //0x0080 
		void* N00000034; //0x0084 
		void* N00000035; //0x0088 
		void* N00000036; //0x008C 
		__int32 m_Socket; //0x0090 
		INetChannelInfo* m_NetChannel; //0x0094 
		__int32 m_nChallengeNr; //0x0098 
		char pad_0x009C[0x4C]; //0x009C
		__int32 m_nSignonState; //0x00E8 
		char pad_0x00EC[0x8]; //0x00EC
		float m_flNextCmdTime; //0x00F4 
		__int32 m_nServerCount; //0x00F8 
		__int32 m_nCurrentSequence; //0x00FC 
		char pad_0x0100[0x8]; //0x0100
		float m_ClockOffsets[16]; //0x0108 
		__int32 m_iCurClockOffset; //0x0148 
		__int32 m_nServerTick; //0x014C 
		__int32 m_nClientTick; //0x0150 
		__int32 m_nDeltaTick; //0x0154 
		unsigned char m_bPaused; //0x0158 
		char pad_0x0159[0x3]; //0x0159
		__int32 m_nViewEntity; //0x015C 
		char pad_0x0160[0x8]; //0x0160
		char m_szLevelName[260]; //0x0168 
		char m_szLevelNameShort[40]; //0x026C 
		char pad_0x0294[0x5EC]; //0x0294
		void* N00000233; //0x0880 
		void* N00000426; //0x0884 
		void* N00000427; //0x0888 
		void* N00000428; //0x088C 
		void* N00000429; //0x0890 
		char pad_0x0894[0x3130]; //0x0894


								 //float		m_flLastServerTickTime;		// the timestamp of last message
								 //bool		insimulation;
								 //
								 //int			oldtickcount;		// previous tick
								 //float		m_tickRemainder;	// client copy of tick remainder
								 //float		m_frameTime;		// dt of the current frame
								 //
								 //int			lastoutgoingcommand;// Sequence number of last outgoing command
								 //int			chokedcommands;		// number of choked commands
		int			last_command_ack;	// last command sequence number acknowledged by server
	//	int32_t m_nOutSequenceNr;    //0x0018 last send outgoing sequence number
								 //int			command_ack;		// current command sequence acknowledged by server
								 //int			m_nSoundSequence;	// current processed reliable sound sequence number
								 //
								 ////
								 //// information that is static for the entire time connected to a server
								 ////
								 //bool		ishltv;			// true if HLTV server/demo
								 //
								 //CRC32_t		serverCRC;              // To determine if client is playing hacked .map. (entities lump is skipped)
								 //CRC32_t		serverClientSideDllCRC; // To determine if client is playing on a hacked client dll.
								 //
								 //unsigned char	m_chAreaBits[MAX_AREA_STATE_BYTES];
								 //unsigned char	m_chAreaPortalBits[MAX_AREA_PORTAL_STATE_BYTES];
								 //bool			m_bAreaBitsValid; // Have the area bits been set for this level yet?
								 //
								 //// refresh related state
								 //QAngle		viewangles;
								 //CUtlVector< AddAngle >	addangle;
								 //float		addangletotal;
								 //float		prevaddangletotal;
								 //int			cdtrack;			// cd audio
								 //
								 //CustomFile_t	m_nCustomFiles[MAX_CUSTOM_FILES]; // own custom files CRCs
								 //
								 //uint		m_nFriendsID;
								 //char		m_FriendsName[MAX_PLAYER_NAME_LENGTH];
								 //
								 //
								 //CUtlFixedLinkedList< CEventInfo > events;	// list of received events
								 //
								 //// demo loop control
								 //int			demonum;		                  // -1 = don't play demos
								 //char		demos[MAX_DEMOS][MAX_DEMONAME];	  // when not playing
								 //
								 //public:
								 //
								 //// If 'insimulation', returns the time (in seconds) at the client's current tick.
								 //// Otherwise, returns the exact client clock.
								 //float				GetTime() const;
								 //
								 //
								 //bool				IsPaused() const;
								 //
								 //float				GetFrameTime( void ) const;
								 //void				SetFrameTime( float dt ) { m_frameTime = dt; }
								 //
								 //float				GetClientInterpAmount();		// Formerly cl_interp, now based on cl_interp_ratio and cl_updaterate.
								 //	
								 //void				Clear( void );
								 //
								 //void				DumpPrecacheStats(  const char * name );
								 //
								 //// Public API to models
								 //model_t				*GetModel( int index );
								 //void				SetModel( int tableIndex );
								 //int					LookupModelIndex( char const *name );
								 //
								 //// Public API to generic
								 //char const			*GetGeneric( int index );
								 //void				SetGeneric( int tableIndex );
								 //int					LookupGenericIndex( char const *name );
								 //
								 //// Public API to sounds
								 //CSfxTable			*GetSound( int index );
								 //char const			*GetSoundName( int index );
								 //void				SetSound( int tableIndex );
								 //int					LookupSoundIndex( char const *name );
								 //void				ClearSounds();
								 //
								 //// Public API to decals
								 //char const			*GetDecalName( int index );
								 //void				SetDecal( int tableIndex );
								 //
								 //// customization files code
								 //void				CheckOwnCustomFiles(); // load own custom file
								 //void				CheckOthersCustomFile(CRC32_t crc); // check if we have to download custom files from server
								 //void				AddCustomFile( int slot, const char *resourceFile);
								 //
								 //public:
								 //INetworkStringTable *m_pModelPrecacheTable;	
								 //INetworkStringTable *m_pGenericPrecacheTable;	
								 //INetworkStringTable *m_pSoundPrecacheTable;
								 //INetworkStringTable *m_pDecalPrecacheTable;
								 //INetworkStringTable *m_pInstanceBaselineTable;
								 //INetworkStringTable *m_pLightStyleTable;
								 //INetworkStringTable *m_pUserInfoTable;
								 //INetworkStringTable *m_pServerStartupTable;
								 //INetworkStringTable *m_pDownloadableFileTable;
								 //
								 //CPrecacheItem		model_precache[ MAX_MODELS ];
								 //CPrecacheItem		generic_precache[ MAX_GENERIC ];
								 //CPrecacheItem		sound_precache[ MAX_SOUNDS ];
								 //CPrecacheItem		decal_precache[ MAX_BASE_DECALS ];
								 //
								 //WaitForResourcesHandle_t m_hWaitForResourcesHandle;
								 //bool m_bUpdateSteamResources;
								 //bool m_bShownSteamResourceUpdateProgress;
								 //bool m_bDownloadResources;
								 //bool m_bCheckCRCsWithServer;
								 //float m_flLastCRCBatchTime;
								 //
								 //// This is only kept around to print out the whitelist info if sv_pure is used.	
								 //CPureServerWhitelist *m_pPureServerWhitelist;
								 //
								 //private:
								 //
								 //// Note: This is only here for backwards compatibility. If it is set to something other than NULL,
								 //// then we'll copy its contents into m_chAreaBits in UpdateAreaBits_BackwardsCompatible.
								 //byte		*m_pAreaBits;
								 //
								 //// Set to false when we first connect to a server and true later on before we
								 //// respond to a new whitelist.
								 //bool		m_bMarkedCRCsUnverified;
};  //CClientState

	extern	CClientState	cl;
}

#ifdef ModuleD
#undef ModuleD
#endif // ModuleD

extern HMODULE ModuleD(const char* name);
extern FARPROC GetProcedure(HMODULE hMod, const char* name);