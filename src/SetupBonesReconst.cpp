#include "common.hpp"
#include "bone_setup.h"
#include "animationlayer.h"
#include <boost/algorithm/string.hpp>

namespace setupbones_reconst
{
#define MAX_OVERLAYS 15



void GetSkeleton(IClientEntity *ent, CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask)
{
    if (!pStudioHdr)
        return;

    if (!pStudioHdr->SequencesAvailable())
    {
        return;
    }

    static uintptr_t m_pIK_offset = 0x568;
    CIKContext **m_pIk            = (reinterpret_cast<CIKContext **>(reinterpret_cast<uint64_t>(ent) + (m_pIK_offset)));

    static uintptr_t m_AnimOverlay_offset       = 0x894;
    CUtlVector<C_AnimationLayer> &m_AnimOverlay = NET_VAR(ent, m_AnimOverlay_offset, CUtlVector<C_AnimationLayer>);

    IBoneSetup boneSetup(pStudioHdr, boneMask, &NET_FLOAT(ent, netvar.m_flPoseParameter));
    boneSetup.InitPose(pos, q);

    boneSetup.AccumulatePose(pos, q, NET_INT(ent, netvar.m_nSequence), NET_FLOAT(ent, netvar.m_flCycle), 1.0, g_GlobalVars->curtime, *m_pIk);

    // sort the layers
    int layer[MAX_OVERLAYS] = {};
    int i;
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        layer[i] = MAX_OVERLAYS;
    }
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        CAnimationLayer &pLayer = m_AnimOverlay[i];
        if ((pLayer.m_flWeight > 0) && pLayer.IsActive() && pLayer.m_nOrder >= 0 && pLayer.m_nOrder < m_AnimOverlay.Count())
        {
            layer[pLayer.m_nOrder] = i;
        }
    }
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        if (layer[i] >= 0 && layer[i] < m_AnimOverlay.Count())
        {
            CAnimationLayer &pLayer = m_AnimOverlay[layer[i]];
        }
    }

    if (m_pIk)
    {
        CIKContext auto_ik;
        QAngle angles = VectorToQAngle(re::C_BasePlayer::GetEyeAngles(ent));
        auto_ik.Init(pStudioHdr, angles, ent->GetAbsOrigin(), g_GlobalVars->curtime, 0, boneMask);
        boneSetup.CalcAutoplaySequences(pos, q, g_GlobalVars->curtime, &auto_ik);
    }
    else
    {
        boneSetup.CalcAutoplaySequences(pos, q, g_GlobalVars->curtime, NULL);
    }

    boneSetup.CalcBoneAdj(pos, q, &NET_FLOAT(ent, netvar.m_flEncodedController));
}

bool SetupBones(IClientEntity *ent, matrix3x4_t *pBoneToWorld, int boneMask)
{
    CStudioHdr *pStudioHdr = NET_VAR(ent, 0x880, CStudioHdr *);

    if (!pStudioHdr)
        return false;

    if (pBoneToWorld == nullptr)
    {
        pBoneToWorld = new matrix3x4_t[sizeof(matrix3x4_t) * MAXSTUDIOBONES];
    }

    int *entity_flags = (int *) ((uintptr_t) ent + 400);

    // EFL_SETTING_UP_BONES
    *entity_flags |= 1 << 3;

    Vector pos[MAXSTUDIOBONES];
    Quaternion q[MAXSTUDIOBONES];

    static uintptr_t m_pIK_offset = 0x568;

    Vector adjOrigin = ent->GetAbsOrigin();

    CIKContext **m_pIk = (reinterpret_cast<CIKContext **>(reinterpret_cast<uint64_t>(ent) + (m_pIK_offset)));
    QAngle angles = VectorToQAngle(re::C_BasePlayer::GetEyeAngles(ent));

    // One function seems to have pitch as it will do weird stuff with the bones, so we just do this as a fix
    QAngle angles2 = angles;
    angles2.x      = 0;
    angles2.z      = 0;

    if (*m_pIk)
    {
        // FIXME: pass this into Studio_BuildMatrices to skip transforms
        CBoneBitList boneComputed;
        // m_iIKCounter++;
        (*m_pIk)->Init(pStudioHdr, angles, adjOrigin, g_GlobalVars->curtime, 0 /*m_iIKCounter*/, boneMask);
        GetSkeleton(ent, pStudioHdr, pos, q, boneMask);

        (*m_pIk)->UpdateTargets(pos, q, pBoneToWorld, boneComputed);

        typedef void (*CalculateIKLocks_t)(IClientEntity *, float);
        static uintptr_t CalculateIKLocks_sig      = gSignatures.GetClientSignature("55 89 E5 57 56 53 81 EC EC 04 00 00");
        static CalculateIKLocks_t CalculateIKLocks = (CalculateIKLocks_t) CalculateIKLocks_sig;

        CalculateIKLocks(ent, g_GlobalVars->curtime);
        (*m_pIk)->SolveDependencies(pos, q, pBoneToWorld, boneComputed);
    }
    else
    {
        GetSkeleton(ent, pStudioHdr, pos, q, boneMask);
    }

    // m_flModelScale
    Studio_BuildMatrices(pStudioHdr, angles2, adjOrigin, pos, q, -1, NET_FLOAT(ent, netvar.m_flModelScale), // Scaling
                         pBoneToWorld, boneMask);

    // EFL_SETTING_UP_BONES
    *entity_flags &= ~(1 << 3);

    return true;
}
} // namespace setupbones_reconst
