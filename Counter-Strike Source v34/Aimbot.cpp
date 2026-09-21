// ============================================================================
// Aimbot: захват цели, наведение, компенсация отдачи, хитскан.
//
// Конвейер одного тика (OnCreateMove):
//   1. Проверки состояния (жив, оружие, режим, кнопка).
//   2. ChangeTarget() — выбор лучшей цели по режиму TargetSelection.
//   3. IsTargetGood() — валидация + ComputeAimPoint() (хитбокс/лагокомпенсация)
//      + CanHitPoint() (видимость/прострел/хитскан) -> m_vTarget.
//   4. Доводка точки: рандомизация, оффсеты, предикт.
//   5. Расчёт углов: RCS -> сглаживание -> кламп -> запись в cmd.
// ============================================================================

#include "Aimbot.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Weapon.hpp"
#include "Config.hpp"
#include "LagCompensation.hpp"

#include <cmath>

namespace
{
	// Есть ли хоть одна живая подходящая цель (с учётом фильтра Target).
	bool IsEveryoneDead()
	{
		auto local = C_CSPlayer::GetLocalPlayer();

		if( !local )
			return true;

		const int iTargetMode = Config::Current->Aimbot->Target;
		const int iLocalTeam = local->m_iTeamNum();
		const int iMaxClients = Source::m_pEngine->GetMaxClients();

		for( int i = 1; i <= iMaxClients; i++ )
		{
			auto player = C_CSPlayer::GetPlayer( i );

			if( !player || player == local )
				continue;

			if( player->m_lifeState() != LIFE_ALIVE )
				continue;

			if( iTargetMode == 1 && player->m_iTeamNum() == iLocalTeam ) // только враги
				continue;

			if( iTargetMode == 2 && player->m_iTeamNum() != iLocalTeam ) // только свои
				continue;

			return false;
		}

		return true;
	}

	// Обновить состояние кнопок мыши 1/2/4/5/6. Посторонние сообщения
	// оставляют состояние как было.
	bool UpdateMouseKey( int iKey, UINT message, WPARAM wParam, bool bCurrent )
	{
		UINT uDown = 0, uDblClk = 0, uUp = 0;

		switch( iKey )
		{
		case 1: // Mouse 1
			uDown = WM_LBUTTONDOWN; uDblClk = WM_LBUTTONDBLCLK; uUp = WM_LBUTTONUP;
			break;
		case 2: // Mouse 2
			uDown = WM_RBUTTONDOWN; uDblClk = WM_RBUTTONDBLCLK; uUp = WM_RBUTTONUP;
			break;
		case 4: // Mouse 3
			uDown = WM_MBUTTONDOWN; uDblClk = WM_MBUTTONDBLCLK; uUp = WM_MBUTTONUP;
			break;
		case 5: // Mouse 4
		case 6: // Mouse 5
			if( HIWORD( wParam ) != ( iKey == 5 ? XBUTTON1 : XBUTTON2 ) )
				return bCurrent;
			uDown = WM_XBUTTONDOWN; uDblClk = WM_XBUTTONDBLCLK; uUp = WM_XBUTTONUP;
			break;
		default:
			return bCurrent;
		}

		if( message == uDown || message == uDblClk )
			return true;

		if( message == uUp )
			return false;

		return bCurrent;
	}
}

namespace Feature
{
	Aimbot::Aimbot()
		: m_bKeyPressed( false ),
		m_bChangeTarget( false ),
		m_bAutoScoped( false ),
		m_pCmd( nullptr ),
		m_pLocal( nullptr ),
		m_pWeapon( nullptr ),
		m_pData( nullptr ),
		m_pTarget( nullptr ),
		m_vOldPunch( 0.0f, 0.0f, 0.0f ),
		m_iLegitDelay( 0 )
	{
	}

	void Aimbot::OnCreateMove( CUserCmd* pCmd, C_WeaponCSBaseGun* pWeapon )
	{
		auto cfg = Config::Current->Aimbot;

		m_pCmd = pCmd;
		m_pLocal = C_CSPlayer::GetLocalPlayer();

		if( !m_pLocal || m_pLocal->m_lifeState() != LIFE_ALIVE )
			return;

		m_pWeapon = pWeapon ? pWeapon : m_pLocal->GetActiveWeapon();

		if( !m_pWeapon || m_pWeapon->IsMelee() )
			return;

		if( !cfg->Mode ) // Off
			return;

		if( IsEveryoneDead() )
			return;

		// Silent без AutoFire работает только в момент выстрела.
		const bool bShooting = ( pCmd->buttons & IN_ATTACK ) && m_pWeapon->IsFireTime();

		if( cfg->Silent && !cfg->AutoFire && !bShooting )
		{
			m_bChangeTarget = true;
			m_Timer.Reset();
			return;
		}

		if( cfg->Mode == 2 && !m_bKeyPressed ) // On Press
		{
			m_bChangeTarget = true;
			m_Timer.Reset();
			return;
		}

		if( !m_pWeapon->m_iClip1() )
		{
			if( cfg->AutoReload )
			{
				pCmd->buttons &= ~IN_ATTACK;
				pCmd->buttons |= IN_RELOAD;
			}
			return;
		}

		m_pData = m_pWeapon->GetCSWpnData();

		if( !m_pData )
			return;

		if( m_bChangeTarget )
			ChangeTarget();

		if( !IsTargetGood( m_pTarget ) )
		{
			Config::Misc->target = 0;

			// Автоскоп: цели нет — снимаем только СВОЙ зум.
			// Ручной зум игрока не трогаем (m_bAutoScoped выставлен лишь
			// нашим нажатием, сбрасывается при ручном раззуме).
			if( m_bAutoScoped )
			{
				if( !IsScoped() || !cfg->AutoScope )
					m_bAutoScoped = false;
				else if( IsScopedWeapon() )
				{
					pCmd->buttons |= IN_ATTACK2;
					m_bAutoScoped = false;
				}
			}

			// Легит: стендалон-RCS — держим спрей без захваченной цели.
			// Старый панч обновляем всегда, чтобы дельта не протухала.
			const Vector3& vPunchNow = m_pLocal->m_vecPunchAngle();

			if( Config::Main->AimbotStyle == 1 && cfg->RCS && cfg->RCSStandalone
			&& Config::Misc->Restriction != 1
			&& ( pCmd->buttons & IN_ATTACK )
			&& m_pLocal->m_iShotsFired() > cfg->RCSDelay
			&& !( cfg->FlashCheck && m_pLocal->m_flFlashMaxAlpha() > 40.0f ) )
			{
			Vector3 vComp = pCmd->viewangles;
			vComp.x -= ( vPunchNow.x - m_vOldPunch.x ) * ( cfg->RCSAmountX / 50.0f );
			vComp.y -= ( vPunchNow.y - m_vOldPunch.y ) * ( cfg->RCSAmountY / 50.0f );
			vComp.z = 0.0f;
			ClampAngles( vComp );
			VectorCopy( vComp, pCmd->viewangles );
			Source::m_pEngine->SetViewAngles( pCmd->viewangles );
			}

			m_vOldPunch = vPunchNow;

			if( cfg->NoSwitch )
				return;

			// Перед сменой цели выдерживаем SwitchDelay.
			if( cfg->SwitchDelay && m_Timer.Elapsed() < cfg->SwitchDelay )
				return;

			m_bChangeTarget = true;
			return;
		}

		Config::Misc->target = m_pTarget->GetIndex();

		// Автоскоп: есть цель, винтовка с зумом, зума нет — зумимся.
		// Выстрел в этом тике пропускаем: пуля уйдёт до зума с разбросом.
		bool bNeedScope = false;

		if( cfg->AutoScope && IsScopedWeapon() && !IsScoped() )
		{
			pCmd->buttons |= IN_ATTACK2;
			m_bAutoScoped = true;
			bNeedScope = true;
		}

		const int iAimDelay = ( Config::Main->AimbotStyle == 1 && cfg->HumanizeDelay ) ? m_iLegitDelay : cfg->Delay;

		if( iAimDelay && m_Timer.Elapsed() < iAimDelay )
			return;

		if( cfg->Duration && m_Timer.Elapsed() > cfg->Duration )
			return;

		// Дальше работаем с копией: m_vTarget остаётся чистым кэшем.
		Vector3 vPoint = m_vTarget;

		if( cfg->SpotRandomize )
		{
			Valve::RandomSeed( ( pCmd->random_seed & 255 ) + 1 );
			vPoint.x += Valve::RandomFloat( -1.5f, 1.5f );
			vPoint.y += Valve::RandomFloat( -1.5f, 1.5f );
			vPoint.z += Valve::RandomFloat( -1.5f, 1.5f );
		}

		if( cfg->Height )
		{
			vPoint.x += cfg->HeightScaleX;
			vPoint.y += cfg->HeightScaleY;
			vPoint.z += cfg->HeightScale;
		}

		ApplyPrediction( vPoint );

		const Vector3 vEye = m_pLocal->EyePosition();

		Vector3 vDirection = vPoint - vEye;
		VectorNormalize( vDirection );

		Vector3 vAim;
		VectorAngles( vDirection, vAim );

		ApplyRecoilCompensation( vAim );

		// Стендалон-RCS: трекинг панча каждый тик доводки (рейдж не читает).
		m_vOldPunch = m_pLocal->m_vecPunchAngle();

		if( cfg->Smooth == 1 ) // Step
			ApplyStepSmooth( vAim );
		else if( cfg->Smooth == 2 ) // Linear
			ApplyLinearSmooth( vAim );

		vAim.z = 0.0f;
		ClampAngles( vAim );

		if( Config::Misc->Restriction == 1 ) // SMAC-режим: двигаем курсор, а не углы
			ApplyMouseAim( vAim, vPoint );
		else
		{
			VectorCopy( vAim, pCmd->viewangles );

			if( !cfg->Silent )
				Source::m_pEngine->SetViewAngles( pCmd->viewangles );
		}

		if( cfg->AutoFire && !bNeedScope )
		{
			// Легит: огонь только когда смуз почти довёлся до точки — иначе
			// стреляем в стены раньше прицела. SMAC-режим мимо: там углы не
			// двигаем (доводка мышью), остаточный угол не показатель.
			bool bHoldFire = false;

			if( Config::Main->AimbotStyle == 1 && Config::Misc->Restriction != 1 )
			{
				const float flResidual = GetFOV( pCmd->viewangles + m_pLocal->m_vecPunchAngle() * 2.0f, m_pLocal->EyePosition(), vPoint );

				if( flResidual > 2.0f )
					bHoldFire = true;
			}

			if( !bHoldFire )
			{
				pCmd->buttons &= ~IN_RELOAD;
				pCmd->buttons |= IN_ATTACK;
			}
		}

		if( cfg->AutoStop )
		{
			pCmd->forwardmove = 0.0f;
			pCmd->sidemove = 0.0f;
			pCmd->upmove = 0.0f;
		}

		if( cfg->AutoCrouch )
			pCmd->buttons |= IN_DUCK;
	}

	void Aimbot::OnKeyEvent( UINT message, WPARAM wParam, LPARAM lParam )
	{
		( void )lParam;

		if( Config::Current->Aimbot->Mode != 2 )
			return;

		const int iKey = Config::Current->Aimbot->Key;

		if( iKey == 1 || iKey == 2 || iKey == 4 || iKey == 5 || iKey == 6 )
		{
			m_bKeyPressed = UpdateMouseKey( iKey, message, wParam, m_bKeyPressed );
			return;
		}

		if( ( int )wParam == iKey )
		{
			if( message == WM_KEYDOWN || message == WM_SYSKEYDOWN )
				m_bKeyPressed = true;
			else if( message == WM_KEYUP || message == WM_SYSKEYUP )
				m_bKeyPressed = false;
		}
	}

	void Aimbot::ChangeTarget()
	{
		auto cfg = Config::Current->Aimbot;

		m_pTarget = nullptr;
		m_bChangeTarget = true; // останется true, если цель не найдётся

		if( IsEveryoneDead() )
			return;

		const Vector3 vEye = m_pLocal->EyePosition();
		const Vector3 vView = m_pCmd->viewangles + m_pLocal->m_vecPunchAngle() * 2.0f;
		const int iMaxClients = Source::m_pEngine->GetMaxClients();

		C_CSPlayer* pBest = nullptr;
		Vector3 vBestPoint;
		float flBest = 0.0f;
		bool bFirst = true;

		for( int i = 1; i <= iMaxClients; i++ )
		{
			auto pTarget = ToCSPlayer( Source::m_pEntList->GetBaseEntity( i ) );

			// IsTargetGood заодно кладёт точку прицеливания кандидата в m_vTarget.
			if( !IsTargetGood( pTarget ) )
				continue;

			if( cfg->TargetSelection == 0 ) // First valid
			{
				pBest = pTarget;
				vBestPoint = m_vTarget;
				break;
			}

			if( cfg->TargetSelection == 1 ) // Distance: ближайший
			{
				const float flDistance = m_vTarget.DistTo( vEye );

				if( bFirst || flDistance < flBest )
				{
					flBest = flDistance;
					pBest = pTarget;
					vBestPoint = m_vTarget;
				}
			}
			else if( cfg->TargetSelection == 2 ) // Crosshair: ближайший к прицелу
			{
				const float flFov = GetFOV( vView, vEye, m_vTarget );

				if( bFirst || flFov < flBest )
				{
					flBest = flFov;
					pBest = pTarget;
					vBestPoint = m_vTarget;
				}
			}
			else if( cfg->TargetSelection == 3 ) // Spawn time: дольше всех жив
			{
				auto player = Source::m_pPlayerList->GetPlayer( i );
				const float flTime = player ? ( float )player->m_spawn_time : 0.0f;

				if( bFirst || flTime > flBest )
				{
					flBest = flTime;
					pBest = pTarget;
					vBestPoint = m_vTarget;
				}
			}
			else // неизвестный режим — первый валидный
			{
				pBest = pTarget;
				vBestPoint = m_vTarget;
				break;
			}

			bFirst = false;
		}

		if( pBest )
		{
			m_pTarget = pBest;
			m_vTarget = vBestPoint;
			m_bChangeTarget = false;

			// Delay / Duration / SwitchDelay считаются от момента захвата.
			m_Timer.Reset();

			// Легит: гуманизация задержки — реакция плавает от Delay до 1.5x.
			if( Config::Main->AimbotStyle == 1 && cfg->HumanizeDelay && cfg->Delay > 0 )
			m_iLegitDelay = cfg->Delay + ( int )( GetTickCount() % ( ( unsigned )cfg->Delay / 2 + 1 ) );
			else
			m_iLegitDelay = cfg->Delay;
		}
	}

	bool Aimbot::IsTargetGood( C_CSPlayer* pTarget )
	{
		auto cfg = Config::Current->Aimbot;

		if( !m_pLocal || !m_pCmd || !m_pData )
			return false;

		if( !pTarget || pTarget == m_pLocal )
			return false;

		if( pTarget->IsDormant() )
			return false;

		if( pTarget->m_lifeState() != LIFE_ALIVE )
			return false;

		if( cfg->AntiSpawnProtection && pTarget->m_iHealth() > 110 )
			return false;

		const int iLocalTeam = m_pLocal->m_iTeamNum();
		const int iTargetTeam = pTarget->m_iTeamNum();

		if( cfg->Target == 1 && iTargetTeam == iLocalTeam ) // только враги
			return false;

		if( cfg->Target == 2 && iTargetTeam != iLocalTeam ) // только свои
			return false;

		// Легит: во флешке не целимся (по видимой белизне — с NoFlash работает).
		if( Config::Main->AimbotStyle == 1 && cfg->FlashCheck && m_pLocal->m_flFlashMaxAlpha() > 40.0f )
			return false;

		// Точка прицеливания: хитбокс с учётом лаг-компенсации.
		if( !ComputeAimPoint( pTarget, m_vTarget ) )
			return false;

		const Vector3 vEye = m_pLocal->EyePosition();

		if( m_vTarget.DistTo( vEye ) > m_pData->m_flRange )
			return false;

		// Легит-стиль: FOV-лимит действует при любом TargetSelection — иначе
		// доводка идёт на цели по всему экрану (нелегитно и палится SMAC).
		if( cfg->TargetSelection == 2 || Config::Main->AimbotStyle == 1 ) // Crosshair: режем по FOV
		{
			const float flFov = GetFOV( m_pCmd->viewangles + m_pLocal->m_vecPunchAngle() * 2.0f, vEye, m_vTarget );

			if( flFov > cfg->FieldOfView )
				return false;
		}

		// Видимость / прострел / хитскан. Может подвинуть m_vTarget.
		return CanHitPoint( pTarget, m_vTarget );
	}

	bool Aimbot::ComputeAimPoint( C_CSPlayer* pTarget, Vector3& vPoint )
	{
		auto cfg = Config::Current->Aimbot;

		if( cfg->SetAbs )
		{
			pTarget->SetAbsAngles( pTarget->m_angEyeAngles() );
			pTarget->SetAbsOrigin( pTarget->m_vecOrigin() );
		}

		switch( cfg->LagCompensation )
		{
		case 1: // Backtrack: старые записи
		{
			auto& lag = LagCompensation::Instance();

			if( lag.BacktrackPlayer( pTarget, m_pCmd, vPoint ) )
				return true;

			break; // фолбэк на текущий хитбокс
		}
		case 2: // Backtrack: свежие записи
		{
			auto& lag = LagCompensation1::Instance();

			// Без фолбэка (как в оригинале): нет записи — нет выстрела.
			return lag.BacktrackPlayer1( pTarget, m_pCmd, vPoint );
		}
		case 3: // Both: сначала свежие, потом старые
		{
			auto& lagFront = LagCompensation1::Instance();
			auto& lagBack = LagCompensation::Instance();

			if( lagFront.BacktrackPlayer1( pTarget, m_pCmd, vPoint ) )
				return true;

			if( lagBack.BacktrackPlayer( pTarget, m_pCmd, vPoint ) )
				return true;

			break; // фолбэк на текущий хитбокс
		}
		default: // Off
			break;
		}

		return pTarget->GetHitboxVector( cfg->Spot, vPoint );
	}

	bool Aimbot::CanHitPoint( C_CSPlayer* pTarget, Vector3& vPoint )
	{
		auto cfg = Config::Current->Aimbot;

		int iDamage = 0;

		// Основная точка бьётся — хитскан не нужен.
		if( IsPointHittable( pTarget, vPoint, &iDamage ) )
			return true;

		if( cfg->HitScan == 1 )
			return HitScanCenter( pTarget, vPoint );

		if( cfg->HitScan == 2 )
			return HitScanCorners( pTarget, vPoint );

		if( cfg->HitScan == 3 )
			return HitScanMultipoint( pTarget, vPoint );

		return false;
	}

	bool Aimbot::IsPointHittable( C_CSPlayer* pTarget, const Vector3& vPoint, int* pDamage )
	{
		auto cfg = Config::Current->Aimbot;

		if( cfg->AutoWall )
		{
			int iDamage = -1;

			if( !Source::m_pAccuracy->CanPenetrate( m_pLocal->EyePosition(), vPoint, EffectiveMinDamage(), cfg->Target, &iDamage ) )
				return false;

			if( pDamage )
				*pDamage = iDamage;

			return true;
		}

		// Без прострела достаточно прямой видимости до игрока.
		return Source::TraceLine( vPoint, pTarget );
	}

	bool Aimbot::HitScanCenter( C_CSPlayer* pTarget, Vector3& vPoint )
	{
		auto studio = Source::m_pModelInfoClient->GetStudioModel( pTarget->GetModel() );

		if( !studio )
			return false;

		const int iSet = pTarget->m_nHitboxSet();

		if( iSet < 0 || iSet >= studio->numhitboxsets )
			return false;

		auto hitboxSet = studio->GetHitboxSet( iSet );

		if( !hitboxSet )
			return false;

		auto cfg = Config::Current->Aimbot;

		bool bFound = false;
		int iBestDamage = EffectiveMinDamage();
		float flBestFov = 180.0f;

		for( int i = 0; i < hitboxSet->numhitboxes; i++ )
		{
			Vector3 vCurrent;

			if( !pTarget->GetHitboxVector( i, vCurrent ) )
				continue;

			if( ConsiderHitScanPoint( pTarget, vCurrent, vPoint, iBestDamage, flBestFov ) )
				bFound = true;
		}

		return bFound;
	}

	bool Aimbot::HitScanCorners( C_CSPlayer* pTarget, Vector3& vPoint )
	{
		auto studio = Source::m_pModelInfoClient->GetStudioModel( pTarget->GetModel() );

		if( !studio )
			return false;

		const int iSet = pTarget->m_nHitboxSet();

		if( iSet < 0 || iSet >= studio->numhitboxsets )
			return false;

		auto hitboxSet = studio->GetHitboxSet( iSet );

		if( !hitboxSet )
			return false;

		auto cfg = Config::Current->Aimbot;

		bool bFound = false;
		int iBestDamage = EffectiveMinDamage();
		float flBestFov = 180.0f;

		for( int i = 0; i < hitboxSet->numhitboxes; i++ )
		{
			Vector3 vMin, vMax;

			if( !pTarget->GetHitboxBounds( i, vMin, vMax ) )
				continue;

			// Масштабируем бокс относительно его центра.
			// (Оригинал ошибочно масштабировал мировые координаты
			// относительно начала карты.)
			const Vector3 vCenter = ( vMin + vMax ) * 0.5f;
			vMin = vCenter + ( vMin - vCenter ) * cfg->HitScanScale;
			vMax = vCenter + ( vMax - vCenter ) * cfg->HitScanScale;

			const Vector3 vCandidates[ 2 ] = { vMin, vMax };

			// Обе точки проверяем независимо: оригинал смотрел vMax
			// только если vMin не пробился (else if).
			for( int j = 0; j < 2; j++ )
			{
				if( ConsiderHitScanPoint( pTarget, vCandidates[ j ], vPoint, iBestDamage, flBestFov ) )
					bFound = true;
			}
		}

		return bFound;
	}

	bool Aimbot::HitScanMultipoint( C_CSPlayer* pTarget, Vector3& vPoint )
	{
		auto studio = Source::m_pModelInfoClient->GetStudioModel( pTarget->GetModel() );

		if( !studio )
			return false;

		const int iSet = pTarget->m_nHitboxSet();

		if( iSet < 0 || iSet >= studio->numhitboxsets )
			return false;

		auto hitboxSet = studio->GetHitboxSet( iSet );

		if( !hitboxSet )
			return false;

		auto cfg = Config::Current->Aimbot;

		bool bFound = false;
		int iBestDamage = EffectiveMinDamage();
		float flBestFov = 180.0f;

		const Vector3 vEye = m_pLocal->EyePosition();

		for( int i = 0; i < hitboxSet->numhitboxes; i++ )
		{
			auto box = hitboxSet->GetHitbox( i );

			if( !box )
				continue;

			Vector3 vCenter;

			if( !pTarget->GetHitboxVector( i, vCenter ) )
				continue;

			// Центр — всегда первый кандидат.
			if( ConsiderHitScanPoint( pTarget, vCenter, vPoint, iBestDamage, flBestFov ) )
				bFound = true;

			// Радиус кольца из локального размера бокса.
			const Vector3 vSize = box->bbmax - box->bbmin;

			float flMinSize = vSize.x;

			if( vSize.y < flMinSize )
				flMinSize = vSize.y;

			if( vSize.z < flMinSize )
				flMinSize = vSize.z;

			const float flRadius = 0.5f * flMinSize * cfg->HitScanScale;

			if( flRadius <= 0.0f )
				continue;

			// Screen-space базис: точки ложатся в плоскость, перпендикулярную
			// линии выстрела — боковые покрывают именно yaw-ошибку резолвера.
			Vector3 vForward = vCenter - vEye;

			if( vForward.LengthSqr() < 0.001f )
				continue;

			VectorNormalize( vForward );

			Vector3 vRefUp( 0.0f, 0.0f, 1.0f );

			if( vForward.z > 0.99f || vForward.z < -0.99f )
				vRefUp = Vector3( 0.0f, 1.0f, 0.0f );

			Vector3 vRight, vUp;

			CrossProduct( vForward, vRefUp, vRight );
			VectorNormalize( vRight );
			CrossProduct( vRight, vForward, vUp );

			if( box->group == 1 ) // Голова: центр + кольцо из 4 точек.
			{
				const Vector3 vRing[ 4 ] =
				{
					vCenter + vRight * flRadius,
					vCenter - vRight * flRadius,
					vCenter + vUp * flRadius,
					vCenter - vUp * flRadius
				};

				for( int j = 0; j < 4; j++ )
				{
					if( ConsiderHitScanPoint( pTarget, vRing[ j ], vPoint, iBestDamage, flBestFov ) )
						bFound = true;
				}
			}
			else if( box->group == 2 || box->group == 3 ) // Грудь/живот: центр + бока.
			{
				const Vector3 vSides[ 2 ] =
				{
					vCenter + vRight * flRadius,
					vCenter - vRight * flRadius
				};

				for( int j = 0; j < 2; j++ )
				{
					if( ConsiderHitScanPoint( pTarget, vSides[ j ], vPoint, iBestDamage, flBestFov ) )
						bFound = true;
				}
			}
			// Руки/ноги: только центр (бюджет ~30 точек на цель).
		}

		return bFound;
	}

	bool Aimbot::ConsiderHitScanPoint( C_CSPlayer* pTarget, const Vector3& vCandidate,
		Vector3& vPoint, int& iBestDamage, float& flBestFov )
	{
		auto cfg = Config::Current->Aimbot;

		int iDamage = 0;

		if( !IsPointHittable( pTarget, vCandidate, &iDamage ) )
			return false;

		if( cfg->AutoWall )
		{
			// С прострелом берём самый уронный хитбокс.
			if( iDamage < iBestDamage )
				return false;

			iBestDamage = iDamage;
		}
		else
		{
			// Без прострела — видимый хитбокс ближе к прицелу.
			const float flFov = GetFOV( m_pCmd->viewangles + m_pLocal->m_vecPunchAngle() * 2.0f,
				m_pLocal->EyePosition(), vCandidate );

			if( flFov > flBestFov )
				return false;

			flBestFov = flFov;
		}

		vPoint = vCandidate;
		return true;
	}

	bool Aimbot::IsScopedWeapon()
	{
		const CSWeaponID id = m_pWeapon->GetWeaponID();

		return id == WEAPON_SCOUT || id == WEAPON_AUG || id == WEAPON_SG550 ||
			id == WEAPON_SG552 || id == WEAPON_AWP || id == WEAPON_G3SG1;
	}

	bool Aimbot::IsScoped()
	{
		// В зуме FOV падает ниже дефолтных 90; 0 = дефолт (не зум).
		const int fov = m_pLocal->m_iFOV();

		return fov != 0 && fov < 90;
	}

	int Aimbot::EffectiveMinDamage()
	{
		auto cfg = Config::Current->Aimbot;

		if( cfg->MinDamageOverrideKey && GetAsyncKeyState( cfg->MinDamageOverrideKey ) )
			return cfg->MinDamageOverride;

		return cfg->MinDamage;
	}

	void Aimbot::ApplyPrediction( Vector3& vPoint )
	{
		// Пули в CS:S — хитскан (летят мгновенно), поэтому дальний предикт
		// не нужен. Компенсируем только движение цели за 1 тик —
		// задержку между расчётом углов и выстрелом.
		// (Оригинал делил скорость на дистанцию — физически бессмысленно
		// и вносило ошибку.)
		vPoint += m_pTarget->m_vecVelocity() * Source::m_pGlobalVars->interval_per_tick;
	}

	void Aimbot::ApplyRecoilCompensation( Vector3& vAim )
	{
		auto cfg = Config::Current->Aimbot;

		if( !cfg->RCS )
			return;

		if( m_pLocal->m_iShotsFired() < cfg->RCSDelay )
			return;

		// В CS:S прицел уводит на punch * 2 — отсюда деление процентов на 50.
		const Vector3& punch = m_pLocal->m_vecPunchAngle();

		vAim.x -= punch.x * ( cfg->RCSAmountX / 50.0f );
		vAim.y -= punch.y * ( cfg->RCSAmountY / 50.0f );
	}

	void Aimbot::ApplyStepSmooth( Vector3& vAim )
	{
		auto cfg = Config::Current->Aimbot;

		Vector3 vDelta = vAim - m_pCmd->viewangles;
		AnglesNormalize( vDelta );

		const float flStep[ 2 ] = { cfg->StepX / 100.0f, cfg->StepY / 100.0f };

		for( int i = 0; i < 2; i++ )
		{
			if( flStep[ i ] <= 0.0f )
				continue;

			float flMove = flStep[ i ];

			if( fabsf( vDelta[ i ] ) < flMove )
				flMove = fabsf( vDelta[ i ] );

			vAim[ i ] = m_pCmd->viewangles[ i ] + ( vDelta[ i ] < 0.0f ? -flMove : flMove );
		}
	}

	void Aimbot::ApplyLinearSmooth( Vector3& vAim )
	{
		auto cfg = Config::Current->Aimbot;

		Vector3 vDelta = m_pCmd->viewangles - vAim;
		AnglesNormalize( vDelta );

		float flFactorX = cfg->SmoothX;
		float flFactorY = cfg->SmoothY;

		// Фактор < 1 даёт перелёт за цель и осцилляцию — клампим до мгновенной доводки.
		if( flFactorX > 0.0f )
		{
			if( flFactorX < 1.0f )
				flFactorX = 1.0f;

			vAim.x = m_pCmd->viewangles.x - vDelta.x / flFactorX;
		}

		if( flFactorY > 0.0f )
		{
			if( flFactorY < 1.0f )
				flFactorY = 1.0f;

			vAim.y = m_pCmd->viewangles.y - vDelta.y / flFactorY;
		}
	}

	void Aimbot::ApplyMouseAim( const Vector3& vAim, const Vector3& vPoint )
	{
		const Vector3 vWithPunch = vAim + m_pLocal->m_vecPunchAngle();

		POINT ptClient = { 0, 0 };
		ScreenToClient( Source::m_pTargetInput->GetTarget(), &ptClient );

		Vector3 vForward;
		AngleVectors( vWithPunch, &vForward );

		const Vector3 vEye = m_pLocal->EyePosition();
		const Vector3 vEnd = vEye + vForward * vPoint.DistTo( vEye );

		Vector3 vScreen;

		if( Source::WorldToScreen( vEnd, vScreen ) )
			SetCursorPos( ( int )vScreen.x - ptClient.x, ( int )vScreen.y - ptClient.y );
	}

	Vector3 Aimbot::MakeVector( const Vector3& angles )
	{
		const float pitch = ToRadians( angles.x );
		const float yaw = ToRadians( angles.y );
		const float temp = cosf( pitch );

		return Vector3( temp * cosf( yaw ), sinf( yaw ) * temp, -sinf( pitch ) );
	}

	float Aimbot::GetFOV( const Vector3& va, const Vector3& src, const Vector3& dest )
	{
		Vector3 vDirection = dest - src;

		if( vDirection.IsZero() )
			return 0.0f;

		VectorNormalize( vDirection );

		float flDot = MakeVector( va ).Dot( vDirection );

		// acos вне [-1, 1] даёт NaN из-за погрешностей float.
		if( flDot > 1.0f )
			flDot = 1.0f;
		else if( flDot < -1.0f )
			flDot = -1.0f;

		return ToDegrees( acosf( flDot ) );
	}
}
