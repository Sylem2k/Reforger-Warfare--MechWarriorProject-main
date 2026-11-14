// Warlord GM storyline director. Authoritative server state; replicated to GM clients.
// Adapted from the user's CGS Auto Turrets editable-entity attribute pattern.
// This is boss-only, not a dependency on CGS-Auto-Turrets.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Game Master storyline settings for the Warlord: weapon profiles, inaccuracy, and movement orders.")]
class RW_WarlordGMDirectorComponentClass : ScriptComponentClass
{
    static RW_WarlordGMDirectorComponent FromEditorItem(Managed item)
    {
        SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
        if (!editable || !editable.GetOwner())
            return null;

        return RW_WarlordGMDirectorComponent.Cast(
            editable.GetOwner().FindComponent(RW_WarlordGMDirectorComponent));
    }

    static RW_WarlordGMDirectorComponent Find(IEntity mech)
    {
        if (!mech)
            return null;
        return RW_WarlordGMDirectorComponent.Cast(mech.FindComponent(RW_WarlordGMDirectorComponent));
    }
}

// Weapon profiles:
//   0 All weapons, 1 LRMs only, 2 lasers only, 3 cannons/flak only, 4 hold fire.
// Movement:
//   0 normal autonomous patrol, 1 stop at CURRENT position, 2 GM waypoints only.
// For GM waypoints, the existing hidden pilot must have its own AI group.
class RW_WarlordGMDirectorComponent : ScriptComponent
{
    [Attribute("false", UIWidgets.CheckBox, "Verbose storyline setting-change logs, OFF during Warlord leg diagnosis.", category: "Diagnostics")]
    protected bool m_bVerboseGMLogs;

    [Attribute("0", UIWidgets.Auto, "0=All 1=Missiles only 2=Lasers only 3=Cannons only 4=Hold fire.", category: "GM Storyline"), RplProp()]
    protected int m_iWeaponProfile;

    [Attribute("1", UIWidgets.Auto, "AI spread multiplier: 1 normal, 2 less accurate, 3 very inaccurate.", category: "GM Storyline"), RplProp()]
    protected float m_fInaccuracy;

    [Attribute("0", UIWidgets.Auto, "0=Patrol 1=Hold CURRENT position 2=GM waypoint only.", category: "GM Storyline"), RplProp()]
    protected int m_iMovementOrder;

    [Attribute("10", UIWidgets.Auto, "Arrive this close to a GM waypoint before stopping (metres).", category: "GM Storyline"), RplProp()]
    protected float m_fWaypointArrival;


    // Extra GM storyline controls. Defaults keep existing boss AI/defenses and pacing.
    [Attribute("false", UIWidgets.CheckBox, "Vehicle-only targets for the main AI gunners.", category: "GM Storyline"), RplProp()]
    protected bool m_bVehicleTargetsOnly;

    [Attribute("true", UIWidgets.CheckBox, "Allow the two independent top siege cannons.", category: "GM Storyline"), RplProp()]
    protected bool m_bTopCannonsEnabled;

    [Attribute("true", UIWidgets.CheckBox, "Allow defensive stomp and point-defense in All Weapons mode.", category: "GM Storyline"), RplProp()]
    protected bool m_bDefensesEnabled;

    [Attribute("true", UIWidgets.CheckBox, "Allow the scheduled world-boss warhorn.", category: "GM Storyline"), RplProp()]
    protected bool m_bWarhornEnabled;

    [Attribute("true", UIWidgets.CheckBox, "Charge before each Warlord laser salvo.", category: "GM Storyline"), RplProp()]
    protected bool m_bLaserChargeEnabled;

    [Attribute("1.25", UIWidgets.Auto, "Seconds for the three-stage laser charge-up.", category: "GM Storyline"), RplProp()]
    protected float m_fLaserChargeSeconds;

    [Attribute("1", UIWidgets.Auto, "Multiplier for AI downtime between volleys. Higher = fewer attacks.", category: "GM Storyline"), RplProp()]
    protected float m_fVolleyPauseScale;

    [Attribute("0", UIWidgets.Auto, "Additional seconds the AI must settle on a target before firing.", category: "GM Storyline"), RplProp()]
    protected float m_fAimSettleExtraSeconds;

    bool GetVehicleTargetsOnly() { return m_bVehicleTargetsOnly; }
    bool GetTopCannonsEnabled() { return m_bTopCannonsEnabled; }
    bool GetDefensesEnabled() { return m_bDefensesEnabled; }
    bool GetWarhornEnabled() { return m_bWarhornEnabled; }
    bool GetLaserChargeEnabled() { return m_bLaserChargeEnabled; }
    float GetLaserChargeSeconds() { return Math.Clamp(m_fLaserChargeSeconds, 0.75, 3.0); }
    float GetVolleyPauseScale() { return Math.Clamp(m_fVolleyPauseScale, 0.5, 4.0); }
    float GetAimSettleExtraSeconds() { return Math.Clamp(m_fAimSettleExtraSeconds, 0.0, 8.0); }

    void SetVehicleTargetsOnly(bool enabled)
    {
        if (!Replication.IsServer()) return;
        m_bVehicleTargetsOnly = enabled;
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] vehiclesOnly=%1", enabled));
    }

    void SetTopCannonsEnabled(bool enabled)
    {
        if (!Replication.IsServer()) return;
        m_bTopCannonsEnabled = enabled;
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] topCannons=%1", enabled));
    }

    void SetDefensesEnabled(bool enabled)
    {
        if (!Replication.IsServer()) return;
        m_bDefensesEnabled = enabled;
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] defenses=%1", enabled));
    }

    void SetWarhornEnabled(bool enabled)
    {
        if (!Replication.IsServer()) return;
        m_bWarhornEnabled = enabled;
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] warhorn=%1", enabled));
    }

    void SetLaserChargeEnabled(bool enabled)
    {
        if (!Replication.IsServer()) return;
        m_bLaserChargeEnabled = enabled;
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] laserCharge=%1", enabled));
    }

    void SetLaserChargeSeconds(float seconds)
    {
        if (!Replication.IsServer()) return;
        m_fLaserChargeSeconds = Math.Clamp(seconds, 0.75, 3.0);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] laserChargeSeconds=%1", m_fLaserChargeSeconds));
    }

    void SetVolleyPauseScale(float multiplier)
    {
        if (!Replication.IsServer()) return;
        m_fVolleyPauseScale = Math.Clamp(multiplier, 0.5, 4.0);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] volleyPauseScale=%1", m_fVolleyPauseScale));
    }

    void SetAimSettleExtraSeconds(float seconds)
    {
        if (!Replication.IsServer()) return;
        m_fAimSettleExtraSeconds = Math.Clamp(seconds, 0.0, 8.0);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] extraAimSettle=%1", m_fAimSettleExtraSeconds));
    }

    int GetWeaponProfile() { return m_iWeaponProfile; }
    float GetInaccuracy() { return m_fInaccuracy; }
    int GetMovementOrder() { return m_iMovementOrder; }
    float GetWaypointArrival() { return m_fWaypointArrival; }

    void SetWeaponProfile(int profile)
    {
        if (!Replication.IsServer())
            return;
        m_iWeaponProfile = Math.Clamp(profile, 0, 4);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] weapon profile=%1", m_iWeaponProfile));
    }

    void SetInaccuracy(float scale)
    {
        if (!Replication.IsServer())
            return;
        m_fInaccuracy = Math.Clamp(scale, 1.0, 5.0);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] inaccuracy=%1", m_fInaccuracy));
    }

    void SetMovementOrder(int order)
    {
        if (!Replication.IsServer())
            return;
        m_iMovementOrder = Math.Clamp(order, 0, 2);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] movement=%1 (0 patrol, 1 hold, 2 GM waypoint)", m_iMovementOrder));
    }

    void SetWaypointArrival(float metres)
    {
        if (!Replication.IsServer())
            return;
        m_fWaypointArrival = Math.Clamp(metres, 3.0, 35.0);
        Replication.BumpMe();
        if (m_bVerboseGMLogs)
            Print(string.Format("[RW WARLORD GM] waypoint arrival=%1 m", m_fWaypointArrival));
    }

    bool AllowsStationGroup(string station, int group)
    {
        if (m_iWeaponProfile == 0)
            return true;
        if (m_iWeaponProfile == 4 || group < 1)
            return false;

        // Virtual AC and LRM gunners BOTH use group 3. Classify by STATION
        // instead of disabling the entire group, or Missiles Only would fail.
        if (station == "BOSS_AC_L" || station == "BOSS_AC_R")
            return m_iWeaponProfile == 3;

        if (station == "BOSS_LRM_L1" || station == "BOSS_LRM_L2" ||
            station == "BOSS_LRM_R1" || station == "BOSS_LRM_R2" ||
            station == "BOSS_LRM_R3")
            return m_iWeaponProfile == 1;

        // On the default Warlord, DRIVER group 1 is the arm Las Cannons.
        // DRIVER group 2 is the target designator, NOT missiles.
        if (station == "DRIVER")
            return m_iWeaponProfile == 2 && group == 1;

        return false;
    }

    bool AllowsTopCannons()
    {
        return m_bTopCannonsEnabled && (m_iWeaponProfile == 0 || m_iWeaponProfile == 3);
    }

    bool AllowsAutoBattery(bool isLaser, bool isCannon)
    {
        if (m_iWeaponProfile == 0)
            return true;
        if (m_iWeaponProfile == 2)
            return isLaser;
        if (m_iWeaponProfile == 3)
            return isCannon;
        return false;
    }

    bool AllowsDefenses()
    {
        // Explicit weapon profile selections also suppress automatic stomp and
        // point-defense effects. Only All Weapons enables these auxiliary systems.
        return m_bDefensesEnabled && m_iWeaponProfile == 0;
    }

    vector AddAimError(vector worldAim, vector mechPos, float targetTime, int stationNumber)
    {
        if (m_fInaccuracy <= 1.0)
            return worldAim;

        // Slow-moving, deterministic miss cone: does not jitter on every frame
        // and uses the existing aim replication. Larger range means wider miss.
        float metres = Math.Clamp(vector.Distance(mechPos, worldAim) * 0.015, 3.0, 24.0);
        float error = (m_fInaccuracy - 1.0) * metres;
        float phase = targetTime * 0.45 + stationNumber * 1.91;
        worldAim[0] = worldAim[0] + Math.Sin(phase) * error;
        worldAim[1] = worldAim[1] + Math.Sin(phase * 0.63) * error * 0.25;
        worldAim[2] = worldAim[2] + Math.Cos(phase * 0.77) * error;
        return worldAim;
    }
}
