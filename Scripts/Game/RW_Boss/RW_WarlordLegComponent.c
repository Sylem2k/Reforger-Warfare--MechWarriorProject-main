// Warlord-only IK target coordinate correction.
// Mech Core's IK_Arm.agf builds its IK target from the J1/J2/J3 bind pose
// and then TRANSLATES that target by TargetX / TargetY / TargetZ.
// The generic BFA_MechLegComponent passes an absolute leg-model position.
// A Warlord FBX with an offset J3 bind marker can therefore stretch wildly.
// This subclass changes only the Warlord's leg IK target representation.
// Toggle "Bind-relative IK" OFF in Workbench for an A/B comparison.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord-only bind-relative J3 leg IK; does not alter shared Mech Core locomotion.")]
class RW_WarlordLegComponentClass : BFA_MechLegComponentClass
{
}

class RW_WarlordLegComponent : BFA_MechLegComponent
{
    [Attribute("true", UIWidgets.CheckBox, "For the Mech Core IK_Arm graph, supply foot target displacement relative to initial J3 bind pose instead of absolute leg-local position. Turn OFF to compare legacy walking.", category: "Warlord Leg Fix")]
    protected bool m_bBindRelativeJ3IK;

    protected bool m_bWarlordBindCaptured;
    protected vector m_vWarlordJ3BindModel;
    protected int m_iWarlordIKDebugSamples;

    // A leg is a child of the mech's SlotManager, so CoordToParent() is a
    // parent-space transform, NOT guaranteed to be a world-space position.
    // Base GetFootHome() uses CoordToParent(J3 model rest) and then passes it
    // to world-space ground tracing. On an attached Warlord leg, that can
    // steer both feet toward a common incorrect origin.
    // Rebuild the ORIGINAL unanimated J3 position using this leg entity's
    // complete WORLD transform. Do not query the animated J3 every tick.
    [Attribute("true", UIWidgets.CheckBox, "Use unanimated J3 model-space rest transformed into world space for planted foot home. OFF uses original Core coordinate conversion.", category: "Warlord Leg Fix")]
    protected bool m_bJ3RestWorldSpace;

    protected bool m_bJ3WorldRestLogged;

    override protected vector GetFootHome()
    {
        if (!m_bJ3RestWorldSpace || !m_bFootRestAtLastBone || !m_bFootRestCaptured)
            return super.GetFootHome();

        IEntity leg = GetOwner();
        if (!leg)
            return super.GetFootHome();

        // Engine entity world matrix includes parent hip mount transform and
        // scale. This keeps the original canted hip/knee/ankle bind geometry.
        vector worldMatrix[4];
        leg.GetWorldTransform(worldMatrix);

        vector home = worldMatrix[3]
            + worldMatrix[0] * m_vFootRestModel[0]
            + worldMatrix[1] * m_vFootRestModel[1]
            + worldMatrix[2] * m_vFootRestModel[2];

        if (!m_bJ3WorldRestLogged)
        {
            vector legacyHome = leg.CoordToParent(m_vFootRestModel);
            vector difference = home - legacyHome;
            Print(string.Format("[RW WARLORD FOOT WORLD][REST] leg=%1 left=%2 J3Model=%3 originWorld=%4 J3World=%5 legacyParent=%6 worldDelta=%7",
                leg, m_bIsLeftLeg, m_vFootRestModel, leg.GetOrigin(), home, legacyHome, difference.Length()));
            m_bJ3WorldRestLogged = true;
        }

        // Original Warlord prefabs leave FootRestOffset at zero.
        // Retain standard optional root-relative foot offsets if enabled.
        if (m_vFootRestOffset.LengthSq() < 0.000001)
            return home;

        IEntity body = m_BodyEntity;
        if (!body)
            body = leg.GetRootParent();
        if (!body)
            return home;

        vector right = body.GetTransformAxis(0);
        vector up = body.GetTransformAxis(1);
        vector forward = body.GetTransformAxis(2);
        float side = 1.0;
        if (m_bIsLeftLeg)
            side = -1.0;

        return home
            + right * (m_vFootRestOffset[0] * side)
            + up * m_vFootRestOffset[1]
            + forward * m_vFootRestOffset[2];
    }

    override bool InitLeg(BFA_MechLegPair pair, IEntity bodyEntity, bool isLeftLeg)
    {
        bool initialized = super.InitLeg(pair, bodyEntity, isLeftLeg);
        if (!initialized || !m_LastBone)
            return initialized;

        // Snapshot once at initialization, before the procedural leg starts
        // driving IK. Do NOT read a moving J3 every frame (feedback loop).
        vector restMat[4];
        m_LastBone.GetModelTransform(restMat);
        m_vWarlordJ3BindModel = restMat[3];
        m_bWarlordBindCaptured = true;

        Print(string.Format("[RW WARLORD LEG IK][INIT] leg=%1 left=%2 j3BindModel=%3 bindRelative=%4",
            GetOwner(), isLeftLeg, m_vWarlordJ3BindModel, m_bBindRelativeJ3IK));
        return initialized;
    }

    override void ApplyIKToAnimation()
    {
        if (!m_bBindRelativeJ3IK || !m_bWarlordBindCaptured)
        {
            super.ApplyIKToAnimation();
            return;
        }

        if (!m_AnimController)
            return;

        // Absolute planted/swing target in this leg entity's model space.
        vector absoluteModelTarget = GetOwner().CoordToLocal(m_vCurrentIK);
        absoluteModelTarget[1] = absoluteModelTarget[1] - Math.Max(m_fFootSoleDrop, 0.0);

        // IK_Arm.agf: Create IK2 Target from bind pose, then Translate
        // TargetX / TargetY / TargetZ. Therefore send the displacement
        // FROM that initial pose, not the absolute target position.
        vector targetDelta = absoluteModelTarget - m_vWarlordJ3BindModel;

        if (m_iWarlordIKDebugSamples < 3)
        {
            Print(string.Format("[RW WARLORD LEG IK][TARGET] leg=%1 absolute=%2 bindJ3=%3 delta=%4",
                GetOwner(), absoluteModelTarget, m_vWarlordJ3BindModel, targetDelta));
            m_iWarlordIKDebugSamples++;
        }

        m_AnimController.SetFloatVariable(m_vIKTargetVars[0], targetDelta[0]);
        m_AnimController.SetFloatVariable(m_vIKTargetVars[1], targetDelta[1]);
        m_AnimController.SetFloatVariable(m_vIKTargetVars[2], targetDelta[2]);
    }
}
