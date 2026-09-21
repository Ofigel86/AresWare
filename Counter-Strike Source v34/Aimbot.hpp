#pragma once

#include "Valve.hpp"

namespace Feature
{
	class Aimbot
	{
	public:
		Aimbot();

		void				OnCreateMove( CUserCmd* pCmd, C_WeaponCSBaseGun* pWeapon );
		void				OnKeyEvent( UINT message, WPARAM wParam, LPARAM lParam );

	private:
		// Выбор лучшей цели по режиму TargetSelection. Кладёт точку
		// прицеливания выбранной цели в m_vTarget.
		void				ChangeTarget();

		// Полная проверка цели: фильтры + точка + видимость/прострел.
		// Обновляет m_vTarget (с учётом хитскана).
		bool				IsTargetGood( C_CSPlayer* pTarget );

		// Точка прицеливания: хитбокс Spot с учётом лаг-компенсации.
		bool				ComputeAimPoint( C_CSPlayer* pTarget, Vector3& vPoint );

		// Видимость/прострел точки. Если основная точка не бьётся,
		// запускает хитскан (может подвинуть vPoint).
		bool				CanHitPoint( C_CSPlayer* pTarget, Vector3& vPoint );

		// Одна проверка: прострел (AutoWall) или прямая видимость.
		// В pDamage (если не nullptr) кладёт урон прострела.
		bool				IsPointHittable( C_CSPlayer* pTarget, const Vector3& vPoint, int* pDamage );

		// Хитскан по центрам хитбоксов / по углам боксов / мультипойнты.
		bool				HitScanCenter( C_CSPlayer* pTarget, Vector3& vPoint );
		bool				HitScanCorners( C_CSPlayer* pTarget, Vector3& vPoint );
		bool				HitScanMultipoint( C_CSPlayer* pTarget, Vector3& vPoint );

		// Оценка одного кандидата хитскана: бьётся ли и побил ли рекорд.
		// С прострелом рекорд = макс. урон, без — мин. FOV до прицела.
		bool				ConsiderHitScanPoint( C_CSPlayer* pTarget, const Vector3& vCandidate,
								Vector3& vPoint, int& iBestDamage, float& flBestFov );

		// Доводка углов.
		void				ApplyPrediction( Vector3& vPoint );
		void				ApplyRecoilCompensation( Vector3& vAim );
		void				ApplyStepSmooth( Vector3& vAim );
		void				ApplyLinearSmooth( Vector3& vAim );

		// SMAC-режим (Restriction == 1): двигаем курсор вместо углов.
		void				ApplyMouseAim( const Vector3& vAim, const Vector3& vPoint );

		Vector3				MakeVector( const Vector3& angles );
		float				GetFOV( const Vector3& va, const Vector3& src, const Vector3& dest );

		// Автоскоп: оружие со вторичным зумом / факт зума по FOV.
		bool				IsScopedWeapon();
		bool				IsScoped();

		// Мин. урон с учётом оверрайда на клавише.
		int					EffectiveMinDamage();

	private:
		bool				m_bKeyPressed;
		bool				m_bChangeTarget;
		bool				m_bAutoScoped;

		CUserCmd*			m_pCmd;
		C_CSPlayer*			m_pLocal;
		C_WeaponCSBaseGun*	m_pWeapon;
		CCSWeaponInfo*		m_pData;

		C_CSPlayer*			m_pTarget;
		Vector3				m_vTarget;

		// Легит: трекинг панча для стендалон-RCS + гуманизированная задержка.
		Vector3				m_vOldPunch;
		int					m_iLegitDelay;

		Shared::Timer		m_Timer;
	};
}
