// Warlord-only AI weapon aiming. Other MechCore/MW weapons are unchanged.
// The standard Fire At Aim Point helper only corrects weapons registered on
// BFA_MechWeaponMountComponent; boss rack hardpoints are mounted directly
// onto the root and must read the DRIVER station's replicated aim instead.
// All corrections happen on the authoritative server through normal weapon fire.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord AI LRM: uses shared AI aim for locks and launches.")]
class RW_WarlordBossLRMClass : RW_LRMWeaponClass
{
}

class RW_WarlordBossLRM : RW_LRMWeapon
{
    // One bass-heavy accent for each missile volley, not one accent for every
    // missile in the salvo. The original rocket launch sound remains intact.
    [Attribute("{D7FD0CE25E74A7A8}Sounds/Weapons/Machineguns/UK59/Cannon_shot.acp", UIWidgets.ResourceNamePicker, "Optional launch accent project. Existing local cannon thump is an initial placeholder until a dedicated missile recording is supplied.", params: "acp", category: "Warlord Audio")]
    protected ResourceName m_sLaunchAccentProject;

    [Attribute("SOUND_SHOT_2", UIWidgets.EditBox, "Event within the launch accent audio project.", category: "Warlord Audio")]
    protected string m_sLaunchAccentEvent;

    [Attribute("1.2", UIWidgets.Auto, "Minimum seconds between heavy launch accents on this rack.", category: "Warlord Audio")]
    protected float m_fLaunchAccentCooldown;

    protected int m_iNextAccentTick;

    override void ClientOnShot(vector start, vector end)
    {
        super.ClientOnShot(start, end);

        if (!GetOwner() || m_sLaunchAccentProject.IsEmpty() || m_sLaunchAccentEvent.IsEmpty())
            return;

        int now = System.GetTickCount();
        if (now < m_iNextAccentTick)
            return;

        m_iNextAccentTick = now + (int)(Math.Max(m_fLaunchAccentCooldown, 0.1) * 1000.0);
        vector soundMat[4];
        Math3D.MatrixIdentity4(soundMat);
        soundMat[3] = start;
        BFA_MechWeaponModuleComponent.BFA_MechPlaySound(
            m_sLaunchAccentProject, m_sLaunchAccentEvent, soundMat);
    }

    // Server-confirmed LRM explosion. Audio goes over the Warlord root router
    // rather than relying on a locally spawned rocket component on each client.
    override protected void BroadcastImpact(vector position, IEntity mechOwner, int shotId = 0)
    {
        super.BroadcastImpact(position, mechOwner, shotId);

        if (!Replication.IsServer() || !mechOwner)
            return;

        RW_WarlordBossWeaponRouterComponent router = RW_WarlordBossWeaponRouterComponent.Cast(
            mechOwner.FindComponent(RW_WarlordBossWeaponRouterComponent));
        if (router)
            router.ServerBroadcastBossImpactAudio(position, 2);
    }

    override protected void ApplyFireAtAimPoint(vector start, inout vector direction)
    {
        if (!m_bFireAtAimPoint || !m_pMechOwner)
        {
            super.ApplyFireAtAimPoint(start, direction);
            return;
        }

        vector aimPoint;
        if (!GetLiveAimPoint(aimPoint))
        {
            super.ApplyFireAtAimPoint(start, direction);
            return;
        }

        vector toAim = aimPoint - start;
        if (toAim.LengthSq() <= 0.01)
            return;

        toAim.Normalize();
        // Boss launchers intentionally cover the full arc around the Titan.
        // Accuracy still depends on lock stability, missile spread and steering.
        direction = toAim;
    }
}

[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord AI autocannon: fires at shared station aim with normal spread.")]
class RW_WarlordBossAutocannonClass : BFA_MechAutocannonModuleComponentClass
{
}

class RW_WarlordBossAutocannon : BFA_MechAutocannonModuleComponent
{
    override protected void ApplyFireAtAimPoint(vector start, inout vector direction)
    {
        if (!m_bFireAtAimPoint || !m_pMechOwner)
        {
            super.ApplyFireAtAimPoint(start, direction);
            return;
        }

        BFA_MechAimComponent aim = BFA_MechAimComponent.Cast(
            m_pMechOwner.FindComponent(BFA_MechAimComponent)
        );
        if (!aim)
        {
            super.ApplyFireAtAimPoint(start, direction);
            return;
        }

        vector aimPoint = m_pMechOwner.CoordToParent(
            aim.GetAimLocalPointForStation(GetControlStation())
        );
        vector toAim = aimPoint - start;
        if (toAim.LengthSq() <= 0.01)
            return;

        toAim.Normalize();
        direction = toAim;
    }
}
