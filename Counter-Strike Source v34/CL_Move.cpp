#include "Main.h"
#include "detours.h"
typedef void (*CL_Move_t)(float,bool);
CL_Move_t _CL_Move;
void Hooked_CL_Move(float accumulated_extra_samples,bool bFinalTick)
{
    _CL_Move(accumulated_extra_samples,bFinalTick);
    if(!bFinalTick) return;
    if(!(g_CVars.Miscellaneous.Speedhack && (GetAsyncKeyState(0x45) & 0x8000))) return;
    if(!g_pEngineClient || !g_pEngineClient->IsInGame()) return;
    BasePlayer* LocalPlayer=(BasePlayer*)g_pClientEntityList->GetClientEntity(g_pEngineClient->GetLocalPlayer());
    if(!LocalPlayer || LocalPlayer->m_lifeState()!=0) return;
    int shift = g_CVars.Miscellaneous.SpeedhackValue;
    if(shift < 6) shift = 8;
    if(shift > 14) shift = 14;
    static int recharge = 0;
    recharge++;
    if(recharge < shift) return;
    recharge = 0;
    CSWeapon* Weapon=(CSWeapon*)LocalPlayer->GetActiveBaseCombatWeapon();
    if(!Weapon || !Weapon->IsWeapon()) return;
    if(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;
    g_bCL_Move=true;
    for(int i=0;i<shift;i++) _CL_Move(accumulated_extra_samples,false);
    g_bCL_Move=false;
}
void CL_Move(void)
{
    _CL_Move=(CL_Move_t)DetourFunction((PBYTE)((DWORD)BASE_ENGINE+0x42510),(PBYTE)Hooked_CL_Move);
}
