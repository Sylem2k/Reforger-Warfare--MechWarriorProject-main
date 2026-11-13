//
// The server schedules one cue and broadcasts a small reliable event.
// Every listening client plays the same 3D cue locally. No looping,
// per-frame ticking, extra entities, or effects on AI/weapon balance.
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Randomized Warlord warhorn with replicated client audio.")]
class RW_WarlordWarhornComponentClass : ScriptComponentClass
{
}

class RW_WarlordWarhornComponent : ScriptComponent
{
    [Attribute("true", UIWidgets.CheckBox, "Enable the Warlord's occasional warhorn.", category: "Warlord Warhorn")]
    protected bool m_bEnabled;

    [Attribute("{A6111111000AA002}Sounds/Warlord/Warlord_Warhorn.acp", UIWidgets.ResourceNamePicker, "Warhorn audio project. Requires Warlord_Warhorn.wav beside the .acp.", params: "acp", category: "Warlord Warhorn")]
    protected ResourceName m_sSoundProject;

    [Attribute("SOUND_WARLORD_HORN", UIWidgets.EditBox, "Event name in the warhorn audio project.", category: "Warlord Warhorn")]
    protected string m_sSoundEvent;

    [Attribute("0 20 0", UIWidgets.Auto, "Horn origin in Warlord-local metres, X right / Y up / Z forward.", category: "Warlord Warhorn")]
    protected vector m_vHornOffset;

    [Attribute("35", UIWidgets.Auto, "Shortest initial delay after spawn, in seconds.", category: "Warlord Warhorn")]
    protected float m_fFirstDelayMin;

    [Attribute("95", UIWidgets.Auto, "Longest initial delay after spawn, in seconds.", category: "Warlord Warhorn")]
    protected float m_fFirstDelayMax;

    [Attribute("120", UIWidgets.Auto, "Shortest repeat delay, in seconds.", category: "Warlord Warhorn")]
    protected float m_fRepeatDelayMin;

    [Attribute("310", UIWidgets.Auto, "Longest repeat delay, in seconds.", category: "Warlord Warhorn")]
    protected float m_fRepeatDelayMax;

    [Attribute("0.25", UIWidgets.Auto, "Probability of an extra-long silence before the next blast (0..1).", category: "Warlord Warhorn")]
    protected float m_fLongSilenceChance;

    [Attribute("false", UIWidgets.CheckBox, "Log scheduled horn calls and playback for testing.", category: "Warlord Warhorn")]
    protected bool m_bDebugWarhorn;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (m_bDebugWarhorn)
        {
            Print(string.Format("[RW WARLORD HORN][INIT] server=%1 enabled=%2 console=%3",
                Replication.IsServer(), m_bEnabled, System.IsConsoleApp()));
            Print(string.Format("[RW WARLORD HORN][INIT] project=%1 event=%2",
                m_sSoundProject, m_sSoundEvent));
        }

        if (!Replication.IsServer() || !m_bEnabled || !GetGame() || !GetGame().GetCallqueue())
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][INIT] no scheduler on this instance (client, disabled, or no callqueue)");
            return;
        }

        if (m_bDebugWarhorn)
            Print("[RW WARLORD HORN][SERVER] scheduler starting");
        ScheduleWarhorn(m_fFirstDelayMin, m_fFirstDelayMax);
    }

    override void OnDelete(IEntity owner)
    {
        if (m_bDebugWarhorn)
            Print("[RW WARLORD HORN][DELETE] owner deleted, timer stopped");
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(TriggerWarhorn);

        super.OnDelete(owner);
    }

    protected void ScheduleWarhorn(float minSeconds, float maxSeconds)
    {
        if (!Replication.IsServer() || !GetOwner() || !GetGame() || !GetGame().GetCallqueue())
            return;

        float minimum = Math.Max(minSeconds, 1.0);
        float maximum = Math.Max(maxSeconds, minimum);
        float delay = Math.RandomFloat(minimum, maximum);

        // A quarter of the repeat cycles are intentionally much quieter.
        if (minSeconds == m_fRepeatDelayMin && Math.RandomFloat(0.0, 1.0) < Math.Clamp(m_fLongSilenceChance, 0.0, 1.0))
            delay += Math.RandomFloat(60.0, 155.0);

        if (m_bDebugWarhorn)
            Print(string.Format("[RW WARLORD HORN][SERVER][SCHEDULE] next horn in %1 seconds", delay));

        GetGame().GetCallqueue().CallLater(TriggerWarhorn, (int)(delay * 1000.0), false);
    }

    protected void TriggerWarhorn()
    {
        if (m_bDebugWarhorn)
            Print(string.Format("[RW WARLORD HORN][SERVER][TRIGGER] server=%1 enabled=%2 owner=%3",
                Replication.IsServer(), m_bEnabled, GetOwner()));

        if (!Replication.IsServer() || !m_bEnabled || !GetOwner())
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][SERVER][SKIP] cannot trigger");
            return;
        }

        // GM may mute the warhorn without disabling the component timer.
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(GetOwner());
        if (director && !director.GetWarhornEnabled())
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][SERVER][SKIP] muted by GM storyline");
            ScheduleWarhorn(m_fRepeatDelayMin, m_fRepeatDelayMax);
            return;
        }

        // Do not broadcast the boss horn after the Warlord is destroyed.
        DamageManagerComponent damage = DamageManagerComponent.Cast(GetOwner().FindComponent(DamageManagerComponent));
        if (damage && damage.GetState() == EDamageState.DESTROYED)
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][SERVER][SKIP] boss destroyed");
            return;
        }

        if (!System.IsConsoleApp())
            PlayWarhornLocal();
        else if (m_bDebugWarhorn)
            Print("[RW WARLORD HORN][SERVER] dedicated server: local playback skipped (expected)");

        RW_WarlordBossWeaponRouterComponent router = RW_WarlordBossWeaponRouterComponent.Cast(
            GetOwner().FindComponent(RW_WarlordBossWeaponRouterComponent));

        if (router)
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][SERVER][RPC SEND] using replicated Warlord weapon router");
            router.ServerBroadcastWarlordWarhorn();
        }
        else
        {
            Print("[RW WARLORD HORN][SERVER][RPC ERROR] Warlord weapon router missing; client sound not delivered", LogLevel.WARNING);
        }

        // Server keeps the single timer; clients never schedule a horn.
        ScheduleWarhorn(m_fRepeatDelayMin, m_fRepeatDelayMax);
    }

    // Invoked by the Warlord's already-replicated weapon router on client
    // machines. The horn component itself sends no RPCs (dedicated MP fix).
    void ClientPlayFromRouterRpc()
    {
        if (Replication.IsServer())
            return;

        if (m_bDebugWarhorn)
            Print(string.Format("[RW WARLORD HORN][CLIENT][RPC RECEIVED] console=%1 project=%2 event=%3",
                System.IsConsoleApp(), m_sSoundProject, m_sSoundEvent));

        if (System.IsConsoleApp())
        {
            if (m_bDebugWarhorn)
                Print("[RW WARLORD HORN][CLIENT][AUDIO SKIP] console instance has no audio output");
            return;
        }

        PlayWarhornLocal();
    }

    protected void PlayWarhornLocal()
    {
        IEntity boss = GetOwner();
        if (!boss || m_sSoundProject.IsEmpty() || m_sSoundEvent.IsEmpty())
        {
            if (m_bDebugWarhorn)
                Print(string.Format("[RW WARLORD HORN][AUDIO SKIP] boss=%1 project=%2 event=%3",
                    boss, m_sSoundProject, m_sSoundEvent));
            return;
        }

        if (m_bDebugWarhorn)
        {
            Resource soundProject = Resource.Load(m_sSoundProject);
            if (!soundProject || !soundProject.IsValid())
                Print(string.Format("[RW WARLORD HORN][AUDIO ERROR] resource missing/invalid: %1", m_sSoundProject), LogLevel.WARNING);
            else
                Print(string.Format("[RW WARLORD HORN][AUDIO RESOURCE OK] %1", m_sSoundProject));
        }

        // Use the owner's world-space basis to place the audio emitter
        // at its configured local offset without the unavailable CoordToWorld().
        vector soundMat[4];
        boss.GetWorldTransform(soundMat);
        soundMat[3] = soundMat[3]
            + soundMat[0] * m_vHornOffset[0]
            + soundMat[1] * m_vHornOffset[1]
            + soundMat[2] * m_vHornOffset[2];

        if (m_bDebugWarhorn)
        {
            float audible = AudioSystem.IsAudible(m_sSoundProject, m_sSoundEvent, soundMat[3]);
            Print(string.Format("[RW WARLORD HORN][CLIENT][AUDIO AUDIBILITY] result=%1 (negative means unavailable or inaudible) position=%2",
                audible, soundMat[3]));
            Print(string.Format("[RW WARLORD HORN][PLAY REQUEST] position=%1 event=%2",
                soundMat[3], m_sSoundEvent));
        }

        BFA_MechWeaponModuleComponent.BFA_MechPlaySound(m_sSoundProject, m_sSoundEvent, soundMat);

        if (m_bDebugWarhorn)
            Print("[RW WARLORD HORN][PLAY SUBMITTED] AudioSystem.PlayEvent called; this does not confirm audibility");
    }
}
