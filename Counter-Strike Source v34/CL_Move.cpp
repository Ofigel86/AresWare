#include "Main.h"
#include "detours.h"

typedef void (*CL_Move_t)(float,bool);
CL_Move_t _CL_Move;

void Hooked_CL_Move(float accumulated_extra_samples,bool bFinalTick)
{
    _CL_Move(accumulated_extra_samples,bFinalTick);
    if(bFinalTick && g_CVars.Miscellaneous.Speedhack && (GetAsyncKeyState(0x45) & 0x8000))
    {
        if(g_pEngineClient && g_pEngineClient->IsInGame())
        {
            BasePlayer* LocalPlayer=(BasePlayer*)g_pClientEntityList->GetClientEntity(g_pEngineClient->GetLocalPlayer());
            if(LocalPlayer && LocalPlayer->m_lifeState()==0)
            {
                g_bCL_Move=true;
                for(int i=0;i<g_CVars.Miscellaneous.SpeedhackValue;i++) _CL_Move(accumulated_extra_samples,bFinalTick);
                g_bCL_Move=false;
                return;
            }
        }
        g_bCL_Move=true;
        for(int i=0;i<g_CVars.Miscellaneous.SpeedhackValue;i++) _CL_Move(accumulated_extra_samples,bFinalTick);
        g_bCL_Move=false;
    }
    else
    {
        g_bCL_Move=false;
    }
}

void CL_Move(void)
{
    _CL_Move=(CL_Move_t)DetourFunction((PBYTE)((DWORD)BASE_ENGINE+0x42510),(PBYTE)Hooked_CL_Move);
}
