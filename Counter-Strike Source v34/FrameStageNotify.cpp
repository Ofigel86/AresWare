#include "Main.h"

typedef void(__thiscall* FrameStageNotify_t)(void*,ClientFrameStage_t);
void __fastcall Hooked_FrameStageNotify(void* ecx,void* edx,ClientFrameStage_t curStage)
{
    BasePlayer* LocalPlayer=(BasePlayer*)g_pClientEntityList->GetClientEntity(g_pEngineClient->GetLocalPlayer());
    if(g_pEngineClient->IsInGame())
    {
        if(curStage==FRAME_UNDEFINED)
        {
            CreateMoveVMT->Function<FrameStageNotify_t>(32)(ecx,curStage);
            return;
        }
    }
    static bool once;
    if(LocalPlayer && LocalPlayer->m_lifeState()==0)
    {
        if(g_CVars.Miscellaneous.CheatsBypass && g_CVars.Miscellaneous.ThirdPerson)
        {
            *(float*)((DWORD)LocalPlayer+0xD14)=g_qThirdPerson.y;
            if(once)
            {
                g_pEngineClient->ExecuteClientCmd(XorStr<0x20,12,0x7A808928>("\x54\x49\x4B\x51\x40\x55\x43\x55\x5B\x46\x44"+0x7A808928).s);
                once=false;
            }
            else once=true;
        }
    }
    CreateMoveVMT->Function<FrameStageNotify_t>(32)(ecx,curStage);
    if(!g_pEngineClient->IsInGame()) return;
    static float tempYaw[64];
    if(curStage==FRAME_NET_UPDATE_POSTDATAUPDATE_START)
    {
        for(auto Index=g_pGlobals->maxClients;Index>=1;--Index)
        {
            BasePlayer* Entity=(BasePlayer*)g_pClientEntityList->GetClientEntity(Index);
            if(Entity==0) continue;
            if(Index==g_pEngineClient->GetLocalPlayer()) continue;
            if(Entity->m_lifeState()!=0) continue;
            if(Entity->m_iHealth()<=0 || Entity->m_iHealth()>=500) continue;
            if(!g_CVars.Aimbot.FriendlyFire && LocalPlayer)
            {
                if(Entity->m_iTeamNum()==LocalPlayer->m_iTeamNum()) continue;
            }
            if(Entity->IsDormant()) continue;
            if(!g_Whitelist.List(Index) && g_CVars.Aimbot.Resolver.Active)
            {
                tempYaw[Index]=g_CVars.PlayerList.ViewAngles[Index].y;
                bool ret=true;
                if(g_CVars.Aimbot.Resolver.Mode==1 && g_CVars.PlayerList.Yaw[Index]!=1) ret=false;
                if(ret)
                {
                    Vector resultLocal=EyePosition;
                    Vector resultentity=Entity->EyePosition();
                    Vector m_vTraceVector=resultLocal - resultentity;
                    QAngle m_vAimAngles;
                    static float yawDelta[64];
                    static int yawMode[64];
                    if(!resultLocal.IsValid() || !resultentity.IsValid() || !m_vTraceVector.IsValid()) continue;
                    VectorAngles(m_vTraceVector,m_vAimAngles);
                    m_vAimAngles.x*=-1;
                    if(!m_vAimAngles.IsValid()) continue;
                    yawDelta[Index]=m_vAimAngles.y - tempYaw[Index];
                    yawDelta[Index]=g_Stuff.GuwopNormalize(yawDelta[Index]);
                    if(yawDelta[Index]<0.f) yawDelta[Index]+=360.f;
                    if(yawDelta[Index]<=20.f || yawDelta[Index]>=340.f) yawMode[Index]=1;
                    else if((yawDelta[Index]>=70.f && yawDelta[Index]<=110.f) || (yawDelta[Index]>=250.f && yawDelta[Index]<=290.f)) yawMode[Index]=2;
                    else if(yawDelta[Index]>=160.f && yawDelta[Index]<=200.f) yawMode[Index]=3;
                    else yawMode[Index]=0;
                    if(yawMode[Index]==2) g_CVars.Aimbot.AutoHeightMode[Index]=1;
                    else g_CVars.Aimbot.AutoHeightMode[Index]=0;
                    float velLen2D=Entity->m_vecVelocity().Length2D();
                    bool isMoving=velLen2D>1.0f;
                    bool isFakePitch=(g_CVars.PlayerList.ViewAngles[Index].x==89.f || g_CVars.PlayerList.ViewAngles[Index].x==-89.f);
                    if(isMoving)
                    {
                        Entity->m_angEyeAngles().y=tempYaw[Index];
                        g_CVars.Aimbot.AutoHeightMode[Index]=0;
                    }
                    else if(isFakePitch && yawMode[Index]!=2)
                    {
                        float bodyYaw=0;
                        int pose=Entity->LookupPoseParameter("body_yaw");
                        if(pose>=0)
                        {
                            float poseVal=*(float*)((DWORD)Entity+0x518+pose*4);
                            bodyYaw=poseVal;
                            if(bodyYaw>90) bodyYaw=90;
                            if(bodyYaw<-90) bodyYaw=-90;
                        }
                        static int lastFired[64]={0};
                        static int tol[64]={0};
                        static int side[64]={0};
                        if(g_iBulletsFired[Index]!=lastFired[Index])
                        {
                            if(tol[Index]>0) tol[Index]--;
                            else
                            {
                                side[Index]=(side[Index]+1)%2;
                                tol[Index]=2;
                            }
                            lastFired[Index]=g_iBulletsFired[Index];
                        }
                        float feetYaw=tempYaw[Index]-bodyYaw;
                        float targetYaw=feetYaw;
                        if(g_CVars.Aimbot.Resolver.Type==0)
                        {
                            targetYaw=feetYaw + (side[Index]==0 ? 90.f : -90.f);
                        }
                        else if(g_CVars.Aimbot.Resolver.Type==1)
                        {
                            targetYaw=feetYaw + (side[Index]==0 ? 45.f : -45.f);
                            if(g_iGameTicks%4==3) side[Index]^=1;
                        }
                        else if(g_CVars.Aimbot.Resolver.Type==2)
                        {
                            targetYaw=feetYaw + (g_iGameTicks%2==0 ? 90.f : -90.f);
                            if(g_iGameTicks%4==0) targetYaw+=0.087936f;
                        }
                        Entity->m_angEyeAngles().y=g_Stuff.GuwopNormalize(targetYaw);
                    }
                    else
                    {
                        g_CVars.Aimbot.AutoHeightMode[Index]=0;
                        if(g_CVars.Aimbot.Resolver.Type==0)
                        {
                            int v=g_iGameTicks%4;
                            if(v==0) Entity->m_angEyeAngles().y=0.f;
                            else if(v==1) Entity->m_angEyeAngles().y=90.f;
                            else if(v==2) Entity->m_angEyeAngles().y=180.f;
                            else Entity->m_angEyeAngles().y=270.f;
                        }
                        else if(g_CVars.Aimbot.Resolver.Type==1)
                        {
                            static bool half[64];
                            int v=g_iGameTicks%4;
                            if(v==0) Entity->m_angEyeAngles().y=half[Index]?0.f:180.f;
                            else if(v==1) Entity->m_angEyeAngles().y=half[Index]?45.f:225.f;
                            else if(v==2) Entity->m_angEyeAngles().y=half[Index]?90.f:270.f;
                            else {Entity->m_angEyeAngles().y=half[Index]?135.f:315.f; half[Index]=!half[Index];}
                        }
                        else if(g_CVars.Aimbot.Resolver.Type==2)
                        {
                            Entity->m_angEyeAngles().y=(g_iGameTicks%2==0)?90.f:-90.f;
                            if(g_iGameTicks%4==0) Entity->m_angEyeAngles().y+=0.087936f;
                        }
                    }
                }
            }
            if(pPlayerHistory[Index][0].m_SimulationTime!=Entity->m_flSimulationTime())
            {
                for(int tick=31;tick>0;tick--) pPlayerHistory[Index][tick]=pPlayerHistory[Index][tick-1];
                g_Stuff.StoreTickRecord(Entity,&pPlayerHistory[Index][0]);
            }
        }
    }
}
