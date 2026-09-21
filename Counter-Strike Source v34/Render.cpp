#include "Render.hpp"
#include "LagCompensation.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Config.hpp"

namespace Feature
{
	Render::Render()
	{
		m_pVertexIn = CreateMaterial(true, true);
		m_pVertexOut = CreateMaterial(true, false);
		m_pMatIn = CreateMaterial(false, true);
		m_pMatOut = CreateMaterial(false, false);
		
		m_pOut = CreateMaterial(false, true, false);
		m_pWireIn = CreateMaterial(false, true, true);
		m_pWireOut = CreateMaterial(false, false, true);
	}

	void Render::OnDrawModel(void* ecx, ModelRenderInfo_t* info)
	{
		auto player = C_CSPlayer::GetLocalPlayer();

		if (!player)
			return;

		auto enemy = ToCSPlayer(Source::m_pEntList->GetBaseEntity(info->ent_index));

		if (!enemy)
			return;

		if (enemy->m_lifeState() != LIFE_ALIVE)
			return;

		if (Config::Render->ChamsTarget == 1) // Enemy
		{
			if (player->m_iTeamNum() == enemy->m_iTeamNum())
				return;
		}
		else if (Config::Render->ChamsTarget == 2) // Friendly
		{
			if (player->m_iTeamNum() != enemy->m_iTeamNum())
				return;
		}

		IMaterial* pIn = nullptr;
		IMaterial* pOut = nullptr;

		if (Config::Render->ChamsMode == 1) // Flat
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pMatIn;
			}
			pOut = m_pMatOut;
		}
		else if (Config::Render->ChamsMode == 2) // Shadow
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pVertexIn;
			}
			pOut = m_pVertexOut;
		}
		else if (Config::Render->ChamsMode == 3)  //Shadow && Flat
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pMatIn;
			}

			pOut = m_pVertexOut;
		}
		else if (Config::Render->ChamsMode == 4) // Chipolino
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pMatIn;
			}
			pOut = m_pMatOut;
		}
		
		else if (Config::Render->ChamsMode == 5) // Wireframe
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pWireIn;
			}
			pOut = m_pWireOut;
		}
		Color color = Config::Colors->ChamsOutlinedC;

		if (Config::Render->ChamsOutlined)
		{
			m_pOut->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
		}
		else
		{
			m_pOut->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, false);
		}
		ForceMaterial(color, m_pOut);
		Source::m_pModelRenderSwap->VCall< DrawModelExFn >(IVModelRender_DrawModelEx)(ecx, info);

		if (enemy->m_iTeamNum() == 2) // T
			color = Config::Colors->T_Chams_Normal;
		else if (enemy->m_iTeamNum() == 3) // CT
			color = Config::Colors->CT_Chams_Normal;

		ForceMaterial(color, pIn);
		Source::m_pModelRenderSwap->VCall< DrawModelExFn >(IVModelRender_DrawModelEx)(ecx, info);

		if (Config::Render->ChamsColored)
		{
			if (enemy->m_iTeamNum() == 2) // T
				color = Config::Colors->T_Chams_Colored;
			else if (enemy->m_iTeamNum() == 3) // CT
				color = Config::Colors->CT_Chams_Colored;
		}

		ForceMaterial(color, pOut);
	}

	IMaterial* Render::CreateMaterial(bool bVertexLit, bool bIgnoreZ, bool bWireframe /*= false*/)
	{
		static int iCreated = 0;
		static const char szMaterialStruct[] =
		{
			"\"%s\"\
			\n{\
			\n\t\"$basetexture\" \"vgui/white_additive\"\
			\n\t\"$envmap\" \"\"\
			\n\t\"$model\" \"1\"\
			\n\t\"$receiveflashlight\" \"1\"\
			\n\t\"$singlepassflashlight\" \"1\"\
			\n\t\"$flat\" \"1\"\
			\n\t\"$nocull\" \"0\"\
			\n\t\"$selfillum\" \"1\"\
			\n\t\"$halflambert\" \"1\"\
			\n\t\"$nofog\" \"0\"\
			\n\t\"$ignorez\" \"%i\"\
			\n\t\"$znearer\" \"0\"\
			\n\t\"$wireframe\" \"%i\"\
			\n}\n"
		};

		const char* szBaseType = bVertexLit ? "VertexLitGeneric" : "UnlitGeneric";

		char szMaterial[512];
		sprintf_s(szMaterial, sizeof(szMaterial), szMaterialStruct, szBaseType, bIgnoreZ ? 1 : 0, bWireframe ? 1 : 0);

		char szName[512];
		sprintf_s(szName, sizeof(szName), "custom_material_%i.vmt", iCreated);

		iCreated++;

		KeyValues* pKey = new KeyValues(szBaseType);

		pKey->LoadFromBuffer(szName, szMaterial);

		IMaterial* pMat = Source::m_pMatSystem->CreateMaterial(szName, pKey);

		return pMat;
	}

	void Render::ForceMaterial(const Color& color, IMaterial* mat, bool mod)
	{
		if (mod)
		{
			float col[3] =
			{
				color.R / 255.0f,
				color.G / 255.0f,
				color.B / 255.0f
			};

			float alpha = color.A / 255.0f;

			Source::m_pRenderView->SetBlend(alpha);
			Source::m_pRenderView->SetColorModulation(col);
		}

		Source::m_pModelRender->ForcedMaterialOverride(mat);
	}
}
/*
#include "Render.hpp"
#include "Source.hpp"
#include "Player.hpp"
#include "Config.hpp"

namespace Feature
{
	Render::Render()
	{
		m_pVertexIn = CreateMaterial(true, true);
		m_pVertexOut = CreateMaterial(true, false);
		m_pMatIn = CreateMaterial(false, true);
		m_pMatOut = CreateMaterial(false, false);

		m_pOut = CreateMaterial(false, true, false);
	}

	void Render::OnDrawModel(void* ecx, ModelRenderInfo_t* info)
	{
	auto player = C_CSPlayer::GetLocalPlayer();

	if (!player)
		return;

		auto enemy = ToCSPlayer(Source::m_pEntList->GetBaseEntity(info->ent_index));
		

	if (enemy->m_lifeState() != LIFE_ALIVE)
			return;

		if (Config::Render->ChamsTarget == 1) // Enemy
		{
			if (player->m_iTeamNum() == enemy->m_iTeamNum())
				return;
		}
		else if (Config::Render->ChamsTarget == 2) // Friendly
		{
			if (player->m_iTeamNum() != enemy->m_iTeamNum())
				return;
		}
		
		Color color;

		if (enemy->m_iTeamNum() == 2) // T
			color = Config::Colors->T_Chams_Normal;
		else if (enemy->m_iTeamNum() == 3) // CT
			color = Config::Colors->CT_Chams_Normal;

		IMaterial* pIn = nullptr;
		IMaterial* pOut = nullptr;

		if (Config::Render->ChamsMode == 1) // Flat
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pMatIn;
			}
			pOut = m_pMatOut;
		}
		else if (Config::Render->ChamsMode == 2) // Shadow
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pVertexIn;
			}
			pOut = m_pVertexOut;
		}
		else if (Config::Render->ChamsMode == 3)  //Shadow && Flat
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
				pIn = m_pMatIn;
			}

			pOut = m_pVertexOut;
		}
		else if (Config::Render->ChamsMode == 4) // Chipolino
		{
			if (Config::Render->ChamsVisOnly == 0)
			{
			pIn = m_pMatIn;
			}
			pOut = m_pMatOut;
		}
		ÿ
		ForceMaterial(color, pIn);
		Source::m_pModelRenderSwap->VCall< DrawModelExFn >(IVModelRender_DrawModelEx)(ecx, info);

		if (Config::Render->ChamsColored)
		{
		if (enemy->m_iTeamNum() == 2) // T
				color = Config::Colors->T_Chams_Colored;
			else if (enemy->m_iTeamNum() == 3) // CT
				color = Config::Colors->CT_Chams_Colored;
		}

		ForceMaterial(color, pOut);
	}

	IMaterial* Render::CreateMaterial(bool bVertexLit, bool bIgnoreZ, bool bWireframe)
	{
		static int iCreated = 0;
		static const char szMaterialStruct[] =
		{
			"\"%s\"\
			\n{\
			\n\t\"$basetexture\" \"vgui/white_additive\"\
			\n\t\"$envmap\" \"\"\
			\n\t\"$model\" \"1\"\
			\n\t\"$receiveflashlight\" \"1\"\
			\n\t\"$singlepassflashlight\" \"1\"\
			\n\t\"$flat\" \"1\"\
			\n\t\"$nocull\" \"0\"\
			\n\t\"$selfillum\" \"1\"\
			\n\t\"$halflambert\" \"1\"\
			\n\t\"$nofog\" \"0\"\
			\n\t\"$ignorez\" \"%i\"\
			\n\t\"$znearer\" \"0\"\
			\n\t\"$wireframe\" \"%i\"\
			\n}\n"
		};

		const char* szBaseType = bVertexLit ? "VertexLitGeneric" : "UnlitGeneric";

		char szMaterial[512];
		sprintf_s(szMaterial, sizeof(szMaterial), szMaterialStruct, szBaseType, bIgnoreZ ? 1 : 0, bWireframe ? 1 : 0);

		char szName[512];
		sprintf_s(szName, sizeof(szName), "custom_material_%i.vmt", iCreated);

		iCreated++;

		KeyValues* pKey = new KeyValues(szBaseType);

		pKey->LoadFromBuffer(szName, szMaterial);

		IMaterial* pMat = Source::m_pMatSystem->CreateMaterial(szName, pKey);

		return pMat;
	}

	void Render::ForceMaterial(const Color& color, IMaterial* mat, bool mod)
	{
		if (mod)
		{
			float col[3] =
			{
				color.R / 255.0f,
				color.G / 255.0f,
				color.B / 255.0f
			};

			float alpha = 100;

			Source::m_pRenderView->SetBlend(alpha);
			Source::m_pRenderView->SetColorModulation(col);
		}

		Source::m_pModelRender->ForcedMaterialOverride(mat);
	}
}*/