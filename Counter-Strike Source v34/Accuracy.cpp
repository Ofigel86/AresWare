#include "Accuracy.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Weapon.hpp"

namespace Feature
{
	// Материал поверхности для расчёта прострела. GetSurfaceData возвращает
	// указатель из движка и может отдать nullptr (браш без props, отладочный
	// сервер) — обращение к ->game.material без проверки было падением при
	// включённом AutoWall. Для неизвестной поверхности берём бетон: у него
	// самые консервативные параметры пробития.
	static int MaterialOfSurface( int iSurfaceProps )
	{
		auto pData = Source::m_pPhysicsSurfaceProps->GetSurfaceData( iSurfaceProps );

		if( !pData )
			return CHAR_TEX_CONCRETE;

		return pData->game.material;
	}

	static void CompensateSpread( const Vector3& vBase, float flSpread, float flX, float flY, Vector3& vOut )
	{
		Vector3 vForward, vRight, vUp;

		AngleVectors( vBase, &vForward, &vRight, &vUp );

		Vector3 vDirection = vForward + ( -flSpread * flX * vRight ) + ( -flSpread * flY * vUp );

		VectorNormalize( vDirection );
		VectorAngles( vDirection, vOut );

		vOut.z = 0.0f;
	}

	void Accuracy::ApplySpreadFix( C_WeaponCSBaseGun* weapon, int random_seed, const Vector3& input, Vector3& va, int type, bool inverted )
	{
		if( !type )
			return;

		float flSpread = weapon->GetSpread();

		if( flSpread <= 0.0f )
			return;

		if( inverted )
			flSpread = -flSpread;

		Valve::RandomSeed( ( random_seed & 255 ) + 1 );

		// Движок: shared-random сид + два броска на ось.
		const float flX = Valve::RandomFloat( -0.5f, 0.5f ) + Valve::RandomFloat( -0.5f, 0.5f );
		const float flY = Valve::RandomFloat( -0.5f, 0.5f ) + Valve::RandomFloat( -0.5f, 0.5f );

		// Пуля летит forward + spread*x*right + spread*y*up в базисе
		// выстрела — целимся в (forward - spread*x*right - spread*y*up).
		// Предикт триггера (inverted) всегда однопроходный: предсказание
		// предсказания дало бы двойной доворот.
		if( type == 1 || inverted )
		{
			CompensateSpread( input, flSpread, flX, flY, va );
		}
		else if( type == 2 ) // Perfect: второй проход в уточнённом базисе.
		{
			Vector3 vOnce;

			CompensateSpread( input, flSpread, flX, flY, vOnce );
			CompensateSpread( vOnce, flSpread, flX, flY, va );
		}
	}

void Accuracy::ApplyRecoilFix(C_CSPlayer* player, Vector3& va, bool inverted)
	{
		if (inverted)
		{
			va += player->m_vecPunchAngle() * 2.0f;
		}
		else
		{
			va -= player->m_vecPunchAngle() * 2.0f;
		}
	}

	bool Accuracy::CanPenetrate(const Vector3& vStart, const Vector3& vEnd, int iMinDamage, int iTeam, int* pDamageOut, int* pHitbox, int* pHitgroup, C_BaseEntity** ppEnt)
	{
		auto pLocal = C_CSPlayer::GetLocalPlayer();

		if (!pLocal)
			return false;

		auto pWeapon = pLocal->GetActiveWeapon();

		if (!pWeapon)
			return false;

		auto pData = pWeapon->GetCSWpnData();

		if (!pData)
			return false;

		trace_t tr;
		Ray_t ray;

		float flDistance = pData->m_flRange;
		int iPenetration = pData->m_iPenetration;
		int iBulletType = pWeapon->m_iPrimaryAmmoType();
		int iDamage = pData->m_iDamage;
		float flRangeModifier = pData->m_flRangeModifier;

		float flCurrentDamage = (float)iDamage;
		float flCurrentDistance = 0.0f;

		float flPenetrationPower = 0.0f;
		float flPenetrationDistance = 0.0f;
		float flDamageModifier = 0.5f;
		float flPenetrationModifier = 1.0f;

		Valve::GetBulletTypeParameters(iBulletType, flPenetrationPower, flPenetrationDistance);

		Vector3 vSource = vStart;

		Vector3 vDirection = vEnd - vStart;

		VectorNormalize(vDirection);

		C_CSPlayer* pLastPlayerHit = nullptr;

		int iModDamage = 0;

		while (flCurrentDamage > 0.0f)
		{
			Vector3 vDesired = vSource + vDirection * flDistance;

			CTraceFilterSkipTwoEntities trace(pLocal, pLastPlayerHit);

			ray.Set(vSource, vDesired);

			Source::m_pEngineTrace->TraceRay(ray, 0x4600400B, &trace, &tr);

			Valve::ClipTraceToPlayers(vSource, vDesired + vDirection * 40.0f, 0x4600400B, &trace, &tr);

			pLastPlayerHit = ToCSPlayer(tr.m_pEnt);

			if (tr.fraction == 1.0f)
				break;

			const int iEnterMaterial = MaterialOfSurface( tr.surface.surfaceProps );

			Valve::GetMaterialParameters(iEnterMaterial, flPenetrationModifier, flDamageModifier);

			bool bHitGrate = (tr.contents & 0x8);

			if (bHitGrate)
			{
				flPenetrationModifier = 1.0f;
				flDamageModifier = 0.99f;
			}

			// Фоллоф считаем по отрезку, а не по накопленной дистанции:
			// оригинал пересчитывал весь путь на каждой итерации и урон
			// таял в разы быстрее честного.
			const float flLegDistance = tr.fraction * flDistance;

			flCurrentDistance += flLegDistance;
			flCurrentDamage *= pow(flRangeModifier, flLegDistance / 500.0f);

			if (flCurrentDistance > pData->m_flRange)
				break;

			if (flCurrentDistance > flPenetrationDistance && iPenetration > 0)
				iPenetration = 0;

			if (pLastPlayerHit)
			{
				if ((iTeam == 0)
					|| (iTeam == 1 && pLocal->m_iTeamNum() != pLastPlayerHit->m_iTeamNum())
					|| (iTeam == 2 && pLocal->m_iTeamNum() == pLastPlayerHit->m_iTeamNum()))
				{
					float flModDamage = Valve::GetHitgroupModDamage(flCurrentDamage, tr.hitgroup);

					int iHitDamage = Valve::GetPlayerModDamage(flModDamage, pLastPlayerHit->m_ArmorValue(), pData->m_flArmorRatio, tr.hitgroup, pLastPlayerHit->m_iTeamNum() == pLocal->m_iTeamNum(), pLastPlayerHit->m_bHasHelmet());

					// Максимум, а не сумма: вдоль луча может быть несколько
					// тел — мин. урон и выходные хитбокс/сущность берём
					// от самого уронного попадания.
					if (iHitDamage >= iModDamage)
					{
						iModDamage = iHitDamage;

						if (pHitbox)
							*pHitbox = tr.hitbox;

						if (pHitgroup)
							*pHitgroup = tr.hitgroup;

						if (ppEnt)
							*ppEnt = pLastPlayerHit;
					}
				}
			}

			if (iPenetration == 0 && !bHitGrate)
				break;

			if (iPenetration < 0)
				break;

			Vector3 vPenetrationEnd;

			if (!Valve::TraceToExit(tr.endpos, vDirection, vPenetrationEnd, 24, 128))
				break;

			trace_t exit;

			ray.Set(vPenetrationEnd, tr.endpos);

			Source::m_pEngineTrace->TraceRay(ray, 0x4600400B, nullptr, &exit);

			if (exit.m_pEnt != tr.m_pEnt && exit.m_pEnt != nullptr)
			{
				CTraceFilterSimple trace(exit.m_pEnt);

				ray.Set(vPenetrationEnd, tr.endpos);

				Source::m_pEngineTrace->TraceRay(ray, 0x4600400B, &trace, &exit);
			}

			const int iExitMaterial = MaterialOfSurface( exit.surface.surfaceProps );

			bHitGrate = bHitGrate && (exit.contents & 0x8);

			if (iEnterMaterial == iExitMaterial)
			{
				if (iExitMaterial == CHAR_TEX_WOOD ||
					iExitMaterial == CHAR_TEX_METAL)
				{
					flPenetrationModifier *= 2.0f;
				}
			}

			float flTraceDistance = exit.endpos.DistTo(tr.endpos);

			if (flTraceDistance > (flPenetrationPower * flPenetrationModifier))
				break;

			flPenetrationPower -= flTraceDistance / flPenetrationModifier;
			flCurrentDistance += flTraceDistance;

			vSource = exit.endpos;

			flDistance = (flDistance - flCurrentDistance) * 0.5f;

			flCurrentDamage *= flDamageModifier;

			iPenetration--;
		}

		if (iModDamage == 0)
			iModDamage = -1;

		if (pDamageOut)
			*pDamageOut = iModDamage;

		if (iModDamage >= iMinDamage)
			return true;

		return false;
	}
}