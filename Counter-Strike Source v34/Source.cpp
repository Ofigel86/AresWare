#include "Source.hpp"
#include "Hooked.hpp"
#include "Player.hpp"
#include "HitMarker.hpp"
#include "Resolver.hpp"
#include "Debug.hpp"
#include <cstdio>
#include <cstring>
class GameEventListener : public IGameEventListener2
{
public:
	void FireGameEvent(IGameEvent* game_event)
	{
		auto& hitmarker = Feature::HitMarker::Instance();
		hitmarker.FireGameEvent(game_event);
		Feature::Resolver::OnGameEvent(game_event);
		Hooked_GameEvent(game_event);
	}
};

GameEventListener g_GameEventListener = {};

// Безопасно достаём D3D9-устройство.
//
// Смещение внутри shaderapidx9.dll (Valve::Dx9Device) жёстко зашито под один
// билд движка. На другом билде вызов уйдёт по мусорному адресу, поэтому
// оборачиваем его в SEH: вместо падения игры старт просто провалится и
// запишет причину в лог.
static IDirect3DDevice9* GetDx9Device()
{
	__try
	{
		auto pWrapper = Valve::Dx9Device();

		if( !pWrapper )
			return nullptr;

		return pWrapper->m_pD3DDevice;
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return nullptr;
	}
}

namespace Source
{
	IBaseClientDLL*								m_pClient = nullptr;
	IClientEntityList*							m_pEntList = nullptr;
	IGameMovement*								m_pGameMovement = nullptr;
	IPrediction*								m_pPrediction = nullptr;
	IVEngineClient*								m_pEngine = nullptr;
	IVModelInfoClient*							m_pModelInfoClient = nullptr;
	IVRenderView*								m_pRenderView = nullptr;
	IVModelRender*								m_pModelRender = nullptr;
	IEngineTrace*								m_pEngineTrace = nullptr;
	ICvar*										m_pCvar = nullptr;
	IPhysicsSurfaceProps*						m_pPhysicsSurfaceProps = nullptr;
	IMaterialSystem*							m_pMatSystem = nullptr;
	IGameEventManager2*							m_pGameEventManager = nullptr;

	CGlobalVars*								m_pGlobalVars = nullptr;
	IInput*										m_pInput = nullptr;

	IDirect3DDevice9*							m_pDevice = nullptr;

	std::shared_ptr< Input::Win32 >				m_pTargetInput = nullptr;

	std::shared_ptr< Source::NetVarManager >	m_pNetVarManager = nullptr;
	std::shared_ptr< Direct3D9::Renderer >		m_pRenderer = nullptr;

	Direct3D9::HFont							m_hFont = INVALID_FONT_HANDLE;

	std::shared_ptr< Feature::Accuracy >		m_pAccuracy = nullptr;
	std::shared_ptr< Feature::Aimbot >			m_pAimbot = nullptr;
	std::shared_ptr< Feature::Triggerbot >		m_pTriggerbot = nullptr;
	std::shared_ptr< Feature::DataManager >		m_pDataManager = nullptr;
	std::shared_ptr< Feature::PlayerList >		m_pPlayerList = nullptr;
	std::shared_ptr< Feature::Menu >			m_pMenu = nullptr;
	std::shared_ptr< Feature::Render >			m_pRender = nullptr;

	std::shared_ptr< Memory::VmtSwap >			m_pClientSwap = nullptr;
	std::shared_ptr< Memory::VmtSwap >			m_pPredictionSwap = nullptr;
	std::shared_ptr< Memory::VmtSwap >			m_pInputSwap = nullptr;
	std::shared_ptr< Memory::VmtSwap >			m_pModelRenderSwap = nullptr;
	std::shared_ptr< Memory::VmtSwap >			m_pDeviceSwap = nullptr;

	std::shared_ptr< Memory::Detour >			m_pPresentSwap = nullptr;
	std::shared_ptr< Memory::Detour >			m_pRunPredictionSwap = nullptr;

	RecvVarProxyFn								m_nTickBase = nullptr;
	RecvVarProxyFn								m_vecPunchAngle = nullptr;
	RecvVarProxyFn								m_flSpawnTime = nullptr;
	RecvVarProxyFn								m_angEyeAnglesX = nullptr;
	RecvVarProxyFn								m_angEyeAnglesY = nullptr;
	RecvVarProxyFn								m_flPoseParameter[ 24 ] = {};

	bool Startup()
	{
		m_pClient = ( IBaseClientDLL* )QueryInterface( XorStr( "client.dll" ), XorStr( "VClient" ) );

		if( !m_pClient )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VClient' interface!" ) );
			return false;
		}

		m_pEntList = ( IClientEntityList* )QueryInterface( XorStr( "client.dll" ), XorStr( "VClientEntityList" ) );

		if( !m_pEntList )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VClientEntityList' interface!" ) );
			return false;
		}

		m_pGameMovement = ( IGameMovement* )QueryInterface( XorStr( "client.dll" ), XorStr( "GameMovement" ) );

		if( !m_pGameMovement )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'GameMovement' interface!" ) );
			return false;
		}

		m_pPrediction = ( IPrediction* )QueryInterface( XorStr( "client.dll" ), XorStr( "VClientPrediction" ) );

		if( !m_pPrediction )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VClientPrediction' interface!" ) );
			return false;
		}

		m_pEngine = ( IVEngineClient* )QueryInterface( XorStr( "engine.dll" ), XorStr( "VEngineClient" ) );

		if( !m_pEngine )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VEngineClient' interface!" ) );
			return false;
		}

		m_pModelInfoClient = ( IVModelInfoClient* )QueryInterface( XorStr( "engine.dll" ), XorStr( "VModelInfoClient" ) );

		if( !m_pModelInfoClient )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VModelInfoClient' interface!" ) );
			return false;
		}

		m_pRenderView = ( IVRenderView* )QueryInterface( XorStr( "engine.dll" ), XorStr( "VEngineRenderView" ) );

		if( !m_pRenderView )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VEngineRenderView' interface!" ) );
			return false;
		}

		m_pModelRender = ( IVModelRender* )QueryInterface( XorStr( "engine.dll" ), XorStr( "VEngineModel" ) );

		if( !m_pModelRender )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VEngineModel' interface!" ) );
			return false;
		}

		m_pEngineTrace = ( IEngineTrace* )QueryInterface( XorStr( "engine.dll" ), XorStr( "EngineTraceClient" ) );

		if( !m_pEngineTrace )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'EngineTraceClient' interface!" ) );
			return false;
		}

		m_pCvar = ( ICvar* )QueryInterface( XorStr( "engine.dll" ), XorStr( "VEngineCvar003" ), true );

		if( !m_pCvar )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VEngineCvar003' interface!" ) );
			return false;
		}

		m_pPhysicsSurfaceProps = ( IPhysicsSurfaceProps* )QueryInterface( XorStr( "vphysics.dll" ), XorStr( "VPhysicsSurfaceProps" ) );

		if( !m_pPhysicsSurfaceProps )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VPhysicsSurfaceProps' interface!" ) );
			return false;
		}

		m_pMatSystem = ( IMaterialSystem* )QueryInterface( XorStr( "materialsystem.dll" ), XorStr( "VMaterialSystem" ) );

		if( !m_pMatSystem )
		{
			LOG( XorStr( "[Source::Startup] Can't query 'VMaterialSystem' interface!" ) );
			return false;
		}

		m_pGameEventManager = (IGameEventManager2*)QueryInterface(XorStr("engine.dll"), XorStr("GAMEEVENTSMANAGER002"), true);

		if (!m_pGameEventManager)
		{
			LOG(XorStr("[Source::Startup] Can't query 'GAMEEVENTSMANAGER' interface!"));
			return false;
		}

		// Сигнатуры: сначала получаем адрес, проверяем его и только потом
		// разыменовываем. Раньше при несовпадении паттерна (другой билд игры)
		// читалась память по адресу 0x1 — игра падала при инжекте.
		const auto uGlobals = Memory::PatternScan( XorStr( "client.dll" ), XorStr( "A3 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8D 54 24 34" ) );

		if( !uGlobals )
		{
			LOG( XorStr( "[Source::Startup] Can't find 'global vars' pattern in client.dll!" ) );
			return false;
		}

		m_pGlobalVars = **( CGlobalVars*** )( uGlobals + 1 );

		if( !m_pGlobalVars )
		{
			LOG( XorStr( "[Source::Startup] Can't find 'global vars'!" ) );
			return false;
		}

		const auto uInput = Memory::PatternScan( XorStr( "client.dll" ), XorStr( "8B 0D ?? ?? ?? ?? 8B 11 50 8B 44 24 10" ) );

		if( !uInput )
		{
			LOG( XorStr( "[Source::Startup] Can't find 'input' pattern in client.dll!" ) );
			return false;
		}

		m_pInput = **( IInput*** )( uInput + 2 );

		if( !m_pInput )
		{
			LOG( XorStr( "[Source::Startup] Can't find 'input'!" ) );
			return false;
		}

		m_pDevice = GetDx9Device();

		if( !m_pDevice )
		{
			LOG( XorStr( "[Source::Startup] Can't get 'device'!" ) );
			return false;
		}

		m_pTargetInput = std::make_shared< Input::Win32 >();

		if( !m_pTargetInput->Capture() )
		{
			LOG( XorStr( "[Source::Startup] Can't capture input!" ) );
			return false;
		}

		m_pNetVarManager = std::make_shared< Source::NetVarManager >();

		if( !m_pNetVarManager->Create( m_pClient ) )
		{
			LOG( XorStr( "[Source::Startup] Can't create netvar manager!" ) );
			return false;
		}

		m_pRenderer = std::make_shared< Direct3D9::Renderer >();

		if( !m_pRenderer->Create( m_pDevice ) )
		{
			LOG( XorStr( "[Source::Startup] Can't create renderer!" ) );
			return false;
		}

		m_hFont = m_pRenderer->CreateFont( XorStr( "Tahoma" ), 8, FONT_CREATE_BOLD | FONT_CREATE_DROPSHADOW );

		if( m_hFont == INVALID_FONT_HANDLE )
		{
			LOG( XorStr( "[Source::Startup] Can't create default font!" ) );
			return false;
		}

		m_pAccuracy = std::make_shared< Feature::Accuracy >();
		m_pAimbot = std::make_shared< Feature::Aimbot >();
		m_pTriggerbot = std::make_shared< Feature::Triggerbot >();
		m_pDataManager = std::make_shared< Feature::DataManager >();
		m_pPlayerList = std::make_shared< Feature::PlayerList >();
		m_pMenu = std::make_shared< Feature::Menu >();
		m_pRender = std::make_shared< Feature::Render >();

		if( !m_pMenu->Create( m_pTargetInput->GetTarget(), m_pDevice ) )
		{
			LOG( XorStr( "[Source::Startup] Can't create menu!" ) );
			return false;
		}

		m_pGameEventManager->AddListener(&g_GameEventListener, "player_hurt", false);
		m_pGameEventManager->AddListener(&g_GameEventListener, "weapon_fire", false);
		m_pGameEventManager->AddListener(&g_GameEventListener, "player_death", false);
		m_pGameEventManager->AddListener(&g_GameEventListener, "round_start", false);

		m_pClientSwap = std::make_shared< Memory::VmtSwap >();
		m_pPredictionSwap = std::make_shared< Memory::VmtSwap >();
		m_pInputSwap = std::make_shared< Memory::VmtSwap >();
		m_pModelRenderSwap = std::make_shared< Memory::VmtSwap >();
		m_pDeviceSwap = std::make_shared< Memory::VmtSwap >();

	//	m_pPresentSwap = std::make_shared< Memory::Detour >();
		m_pRunPredictionSwap = std::make_shared< Memory::Detour >();

		if( !m_pClientSwap->Apply( m_pClient ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'client' vmt swap!" ) );
			return false;
		}

		if( !m_pPredictionSwap->Apply( m_pPrediction ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'prediction' vmt swap!" ) );
			return false;
		}

		if( !m_pInputSwap->Apply( m_pInput ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'input' vmt swap!" ) );
			return false;
		}

		if( !m_pModelRenderSwap->Apply( m_pModelRender ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'model render' vmt swap!" ) );
			return false;
		}

		if( !m_pDeviceSwap->Apply( m_pDevice ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'device' vmt swap!" ) );
			return false;
		}

	/*	if( !m_pPresentSwap->Apply( m_pDeviceSwap->VCall< std::uintptr_t >( IDirect3DDevice9_Present ) + 5, ( std::uintptr_t )&Hooked_Present, 6 ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'present' code swap!" ) );
			return false;
		}*/

		auto CL_RunPrediction = Memory::PatternScan( XorStr( "engine.dll" ), XorStr( "A1 ?? ?? ?? ?? 39 05 ?? ?? ?? ?? 74 55" ) );

		if( !CL_RunPrediction )
		{
			LOG( XorStr( "[Source::Startup] Can't find 'CL_RunPrediction' pattern in engine.dll!" ) );
			return false;
		}

		if( !m_pRunPredictionSwap->Apply( CL_RunPrediction, ( std::uintptr_t )Hooked_CL_RunPrediction, 5 ) )
		{
			LOG( XorStr( "[Source::Startup] Can't apply 'runprediction' code swap!" ) );
			return false;
		}

		m_pClientSwap->Hook(&Hooked_CreateMove, IBaseClientDLL_CreateMove);
		m_pClientSwap->Hook( &Hooked_FrameStageNotify, IBaseClientDLL_FrameStageNotify );

		m_pPredictionSwap->Hook( &Hooked_RunCommand, IPrediction_RunCommand );
		m_pPredictionSwap->Hook( &Hooked_FinishMove, IPrediction_FinishMove );
		m_pPredictionSwap->Hook( &Hooked_Update, IPrediction_Update );

		m_pInputSwap->Hook( &Hooked_GetUserCmd, IInput_GetUserCmd );
		m_pInputSwap->Hook( &Hooked_ResetMouse, IInput_ResetMouse );

		m_pModelRenderSwap->Hook( &Hooked_DrawModelEx, IVModelRender_DrawModelEx );

		m_pDeviceSwap->Hook( &Hooked_Reset, IDirect3DDevice9_Reset );
		m_pDeviceSwap->Hook( &Hooked_Present, IDirect3DDevice9_Present );

		// Если прокси не поставился (свойство не найдено в таблице),
		// соответствующая фича молча не работает — пишем в лог.
		m_nTickBase = m_pNetVarManager->HookProp( XorStr( "DT_BasePlayer" ), XorStr( "m_nTickBase" ), DT_BasePlayer_m_nTickBase );

		if( !m_nTickBase )
			LOG( XorStr( "[Source::Startup] Can't hook proxy 'DT_BasePlayer::m_nTickBase'." ) );

		m_vecPunchAngle = m_pNetVarManager->HookProp( XorStr( "DT_BasePlayer" ), XorStr( "m_vecPunchAngle" ), DT_BasePlayer_m_vecPunchAngle );

		if( !m_vecPunchAngle )
			LOG( XorStr( "[Source::Startup] Can't hook proxy 'DT_BasePlayer::m_vecPunchAngle'." ) );

		m_flSpawnTime = m_pNetVarManager->HookProp( XorStr( "DT_ParticleSmokeGrenade" ), XorStr( "m_flSpawnTime" ), DT_ParticleSmokeGrenade_m_flSpawnTime );

		if( !m_flSpawnTime )
			LOG( XorStr( "[Source::Startup] Can't hook proxy 'DT_ParticleSmokeGrenade::m_flSpawnTime'." ) );

		m_angEyeAnglesX = m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), XorStr( "m_angEyeAngles[0]" ), DT_CSPlayer_m_angEyeAnglesX );

		if( !m_angEyeAnglesX )
			LOG( XorStr( "[Source::Startup] Can't hook proxy 'DT_CSPlayer::m_angEyeAngles[0]'." ) );

		m_angEyeAnglesY = m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), XorStr( "m_angEyeAngles[1]" ), DT_CSPlayer_m_angEyeAnglesY );

		if( !m_angEyeAnglesY )
			LOG( XorStr( "[Source::Startup] Can't hook proxy 'DT_CSPlayer::m_angEyeAngles[1]'." ) );

		for( int i = 0; i < 24; i++ )
		{
			char poseProp[ 32 ] = {};
			sprintf_s( poseProp, "m_flPoseParameter[%d]", i );
			m_flPoseParameter[ i ] = m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), poseProp, Feature::Resolver::DT_CSPlayer_m_flPoseParameter );
		}

		return true;
	}

	bool Release()
	{
		// Раньше здесь был return: если не удалось восстановить оконную
		// процедуру, все VMT/Detour-хуки оставались висеть и игра падала
		// при выгрузке. Теперь просто пишем в лог и идём дальше.
		if( m_pTargetInput )
		{
			if( !m_pTargetInput->Release() )
				LOG( XorStr( "[Source::Release] Can't release input!" ) );
		}

		if( m_pClientSwap )
			m_pClientSwap->Release();

		if( m_pPredictionSwap )
			m_pPredictionSwap->Release();

		if( m_pInputSwap )
			m_pInputSwap->Release();

		if( m_pModelRenderSwap )
			m_pModelRenderSwap->Release();

		if( m_pDeviceSwap )
			m_pDeviceSwap->Release();

		if( m_pPresentSwap )
			m_pPresentSwap->Release();

		if( m_pRunPredictionSwap )
			m_pRunPredictionSwap->Release();

		// Слушателя могли зарегистрировать несколько раз, если Startup
		// вызывался повторно (см. цикл повторов в Main.cpp): дубликаты
		// обрабатывали бы игровые события по два раза.
		if( m_pGameEventManager )
			m_pGameEventManager->RemoveListener( &g_GameEventListener );

		// Снимаем прокси netvar'ов. Если менеджер не создан (провал старта) —
		// просто обнуляем указатели: иначе HostProp дёрнет nullptr.
		if( m_pNetVarManager )
		{
			if( m_nTickBase )
			{
				m_pNetVarManager->HookProp( XorStr( "DT_BasePlayer" ), XorStr( "m_nTickBase" ), m_nTickBase );
				m_nTickBase = nullptr;
			}

			if( m_vecPunchAngle )
			{
				m_pNetVarManager->HookProp( XorStr( "DT_BasePlayer" ), XorStr( "m_vecPunchAngle" ), m_vecPunchAngle );
				m_vecPunchAngle = nullptr;
			}

			if( m_flSpawnTime )
			{
				m_pNetVarManager->HookProp( XorStr( "DT_ParticleSmokeGrenade" ), XorStr( "m_flSpawnTime" ), m_flSpawnTime );
				m_flSpawnTime = nullptr;
			}

			if( m_angEyeAnglesX )
			{
				m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), XorStr( "m_angEyeAngles[0]" ), m_angEyeAnglesX );
				m_angEyeAnglesX = nullptr;
			}

			if( m_angEyeAnglesY )
			{
				m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), XorStr( "m_angEyeAngles[1]" ), m_angEyeAnglesY );
				m_angEyeAnglesY = nullptr;
			}

			for( int i = 0; i < 24; i++ )
			{
				if( !m_flPoseParameter[ i ] )
					continue;

				char poseProp[ 32 ] = {};
				sprintf_s( poseProp, "m_flPoseParameter[%d]", i );
				m_pNetVarManager->HookProp( XorStr( "DT_CSPlayer" ), poseProp, m_flPoseParameter[ i ] );
				m_flPoseParameter[ i ] = nullptr;
			}
		}
		else
		{
			m_nTickBase = nullptr;
			m_vecPunchAngle = nullptr;
			m_flSpawnTime = nullptr;
			m_angEyeAnglesX = nullptr;
			m_angEyeAnglesY = nullptr;

			for( int i = 0; i < 24; i++ )
				m_flPoseParameter[ i ] = nullptr;
		}

		return true;
	}

	void Free()
	{
		// hooks
		Memory::SafeReset( m_pClientSwap );
		Memory::SafeReset( m_pPredictionSwap );
		Memory::SafeReset( m_pInputSwap );
		Memory::SafeReset( m_pModelRenderSwap );
		Memory::SafeReset( m_pDeviceSwap );
		Memory::SafeReset( m_pPresentSwap );
		Memory::SafeReset( m_pRunPredictionSwap );

		// framework
		Memory::SafeReset( m_pTargetInput );
		Memory::SafeReset( m_pNetVarManager );
		Memory::SafeReset( m_pRenderer );

		// features
		Memory::SafeReset( m_pAccuracy );
		Memory::SafeReset( m_pAimbot );
		Memory::SafeReset( m_pTriggerbot );
		Memory::SafeReset( m_pDataManager );
		Memory::SafeReset( m_pPlayerList );
		Memory::SafeReset( m_pMenu );
		Memory::SafeReset( m_pRender );
	}

	void* QueryInterface( const char* szMod, const char* szName, bool bCustom )
	{
		auto hMod = ModuleD( szMod );

		if( !hMod )
		{
			LOG( XorStr( "[Source::QueryInterface] Module '%s' not found! Timeout!" ), szMod );
			return nullptr;
		}

		auto pCreateInterface = ( CreateInterfaceFn )GetProcedure( hMod, XorStr( "CreateInterface" ) );

		if( !pCreateInterface )
		{
			LOG( XorStr( "[Source::QueryInterface] Can't get 'CreateInterface' address!" ) );
			return nullptr;
		}

		if( bCustom )
			return pCreateInterface( szName, nullptr );

		char szFormat[ 1024 ];

		for( int i = 0; i < 1000; i++ )
		{
			Crypt::sprintf_s( szFormat, sizeof( szFormat ), XorStr( "%s%03i" ), szName, i );

			auto pRet = pCreateInterface( szFormat, nullptr );

			if( pRet )
				return pRet;
		}

		LOG( XorStr( "[Source::QueryInterface] Interface '%s' not found!" ), szName );

		return nullptr;
	}
	void MovementFix( CUserCmd* cmd, const Vector3& va, bool aa )
	{
		float yaw, speed;

		Vector3& move = *(Vector3*)&cmd->forwardmove;

		speed = move.Length2D();

		yaw = ToDegrees(atan2(move.y, move.x));
		yaw = ToRadians(cmd->viewangles.y - va.y + yaw);

		if (cmd->viewangles.x > 90.00 || cmd->viewangles.x < -90.00 )
			move.x = -cos(yaw) * speed;
		else
			move.x = cos(yaw) * speed;

		move.y = sin(yaw) * speed;
		
	}

	bool TraceLine( const Vector3& vEnd, C_BaseEntity* pEnt )
	{
		auto player = C_CSPlayer::GetLocalPlayer();

		if( !player )
			return false;

		Ray_t ray;
		trace_t tr;

		CTraceFilterSimple trace( player );

		ray.Set( player->EyePosition(), vEnd );

		m_pEngineTrace->TraceRay( ray, 0x46004003, &trace, &tr );

		if( pEnt )
			return ( tr.fraction == 1.0f || tr.m_pEnt == pEnt );

		return ( tr.fraction == 1.0f );
	}

	// Легит ThroughSmoke: движок v34 LineGoesThroughSmoke не отдаёт — считаем сами:
	// сегмент глаз→точка против сфер активного дыма (CParticleSmokeGrenade, ~130u).
	bool LineThroughSmoke( const Vector3& vFrom, const Vector3& vTo )
	{
		Vector3 vDir = vTo - vFrom;

		const float flLenSq = vDir.Dot( vDir );

		if( flLenSq < 1.0f )
			return false;

		const int iMax = m_pEntList->GetHighestEntityIndex();

		for( int i = 0; i <= iMax; i++ )
		{
			auto pEnt = m_pEntList->GetBaseEntity( i );

			if( !pEnt || pEnt->IsDormant() )
				continue;

			auto pClass = pEnt->GetClientClass();

			if( !pClass || !pClass->m_pNetworkName )
				continue;

			if( !strstr( pClass->m_pNetworkName, "ParticleSmoke" ) )
				continue;

			const Vector3 vSmoke = pEnt->m_vecOrigin();
			const Vector3 vRel = vSmoke - vFrom;

			float flT = vRel.Dot( vDir ) / flLenSq;

			if( flT < 0.0f )
				flT = 0.0f;
			else if( flT > 1.0f )
				flT = 1.0f;

			const Vector3 vClosest = vFrom + vDir * flT;
			const Vector3 vDiff = vSmoke - vClosest;

			if( vDiff.Dot( vDiff ) < 130.0f * 130.0f )
				return true;
		}

		return false;
	}

	bool WorldToScreen( const Vector3& vPoint, Vector3& vOut )
	{
		auto vMatrix = m_pEngine->WorldToScreenMatrix();

		vOut.x = vMatrix[ 0 ][ 0 ] * vPoint.x + vMatrix[ 0 ][ 1 ] * vPoint.y + vMatrix[ 0 ][ 2 ] * vPoint.z + vMatrix[ 0 ][ 3 ];
		vOut.y = vMatrix[ 1 ][ 0 ] * vPoint.x + vMatrix[ 1 ][ 1 ] * vPoint.y + vMatrix[ 1 ][ 2 ] * vPoint.z + vMatrix[ 1 ][ 3 ];

		auto w = vMatrix[ 3 ][ 0 ] * vPoint.x + vMatrix[ 3 ][ 1 ] * vPoint.y + vMatrix[ 3 ][ 2 ] * vPoint.z + vMatrix[ 3 ][ 3 ];

		if( w < 0.01f )
			return false;

		auto invw = 1.0f / w;

		vOut.x *= invw;
		vOut.y *= invw;

		int width, height;

		m_pEngine->GetScreenSize( width, height );

		float x = ( float )( width / 2 );
		float y = ( float )( height / 2 );

		x += 0.5f * vOut.x * width + 0.5f;
		y -= 0.5f * vOut.y * height + 0.5f;

		vOut.x = x;
		vOut.y = y;

		return true;
	}
}