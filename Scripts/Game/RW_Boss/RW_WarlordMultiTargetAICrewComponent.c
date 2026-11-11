//------------------------------------------------------------------------------------------------
// Warlord world boss: target distribution across virtual AI gunner stations.
// Reuses the original CGS_MechBrainHooks firing logic, weapon routing, LOS,
// faction checks, burst timing, and server-authoritative aim replication.
// The base AI applies a distance penalty to already-assigned targets; for a
// 128-player boss that still causes multiple stations to focus the same nearest
// target. This class first tries an untaken hostile and falls back to the base
// selection only when no distinct visible target is available.
//
// Virtual BOSS_LEFT / BOSS_RIGHT stations are defined on the prefab. MechBrain
// runs them without occupants. No per-frame entity search is added: this only
// runs when the existing AI station scan interval expires.
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "World boss multi-gunner AI: distribute gunner banks among distinct hostile targets.")]
class RW_WarlordMultiTargetAICrewComponentClass : BFA_MechAICrewComponentClass
{
}

class RW_WarlordMultiTargetAICrewComponent : BFA_MechAICrewComponent
{
    [Attribute("7", UIWidgets.Auto, "Minimum seconds to stay locked on a valid visible target. Prevents turrets switching every scan.", category: "Warlord Targeting")]
    protected float m_fMinimumTargetHoldSeconds;

    // Warlord-only staggered firing; shared Mech Core and player control stay unchanged.
    [Attribute("true", UIWidgets.CheckBox, "Use staggered, randomized AI battery volleys instead of synchronized bursts.", category: "Warlord Firing")]
    protected bool m_bStaggeredBatteryFire;

    [Attribute("0.3", UIWidgets.Auto, "Minimum extra delay after acquiring a new target (seconds).", category: "Warlord Firing")]
    protected float m_fOpeningDelayMin;
    [Attribute("3.0", UIWidgets.Auto, "Maximum extra delay after acquiring a new target (seconds).", category: "Warlord Firing")]
    protected float m_fOpeningDelayMax;

    [Attribute("1.3", UIWidgets.Auto, "Minimum active firing burst duration (seconds).", category: "Warlord Firing")]
    protected float m_fBurstLengthMin;
    [Attribute("2.7", UIWidgets.Auto, "Maximum active firing burst duration (seconds).", category: "Warlord Firing")]
    protected float m_fBurstLengthMax;

    [Attribute("3.5", UIWidgets.Auto, "Minimum pause between bursts (seconds).", category: "Warlord Firing")]
    protected float m_fVolleyPauseMin;
    [Attribute("7.5", UIWidgets.Auto, "Maximum pause between bursts (seconds).", category: "Warlord Firing")]
    protected float m_fVolleyPauseMax;

    // The DRIVER alternates its arm-cannon group (1) and shoulder group (2).
    // Other virtual gunners keep their own assigned weapon group and target.
    protected int m_iNextDriverAttackGroup = 1;

    // The separately slotted top cannons are NOT core hardpoint modules.
    // Let their boss-only fire controller reuse the DRIVER's verified AI target
    // rather than performing two more expensive scans across 128 players.
    IEntity GetWarlordDriverTarget()
    {
        Faction bossFaction = GetMechFaction();
        if (!bossFaction)
            return null;

        foreach (BFA_MechAIStationState state : m_aStations)
        {
            if (!state || state.m_sStationId != "DRIVER")
                continue;

            IEntity target = state.m_pTarget;
            if (!target || !IsValidTarget(target) || !IsHostile(target, bossFaction))
                return null;

            if (!HasLineOfSight(GetAimPoint(target), target))
                return null;

            return target;
        }

        return null;
    }

    override protected IEntity FindBestTarget(BFA_MechAIStationState forState = null)
    {
        if (!forState || !m_bSpreadTargets)
            return super.FindBestTarget(forState);

        // Keep an existing vehicle target so the heavy arms can settle and
        // damage the hull. Infantry never blocks acquisition of a visible tank.
        IEntity held = forState.m_pTarget;
        if (held && Vehicle.Cast(held) && forState.m_fTargetTime < Math.Max(m_fMinimumTargetHoldSeconds, 0))
        {
            Faction heldFaction = GetMechFaction();
            if (heldFaction && IsValidTarget(held) && IsHostile(held, heldFaction) && HasLineOfSight(GetAimPoint(held), held))
                return held;
        }

        BaseWorld world = GetGame().GetWorld();
        Faction mechFaction = GetMechFaction();
        IEntity owner = GetOwner();
        if (!world || !mechFaction || !owner)
            return super.FindBestTarget(forState);

        float range = Math.Max(m_fEngageRange, 10.0);
        float rangeSq = range * range;
        float freeVehicleDistance = rangeSq;
        float sharedVehicleDistance = rangeSq;
        float freeInfantryDistance = rangeSq;
        float sharedInfantryDistance = rangeSq;

        IEntity freeVehicle;
        IEntity sharedVehicle;
        IEntity freeInfantry;
        IEntity sharedInfantry;

        m_aQuery.Clear();
        world.QueryEntitiesBySphere(owner.GetOrigin(), range, CollectQueryEntity, null, EQueryEntitiesFlags.DYNAMIC);

        foreach (IEntity candidate : m_aQuery)
        {
            if (!candidate || candidate == owner || candidate.GetRootParent() == owner)
                continue;

            bool soldier = ChimeraCharacter.Cast(candidate) != null;
            bool vehicle = Vehicle.Cast(candidate) != null;
            if (!soldier && !vehicle)
                continue;

            // Vehicle crew are never direct AI targets while seated.
            if (soldier && candidate.GetParent())
                continue;
            if (!IsAlive(candidate) || !IsHostile(candidate, mechFaction))
                continue;

            float distanceSq = vector.DistanceSq(candidate.GetOrigin(), owner.GetOrigin());
            if (distanceSq >= rangeSq)
                continue;

            bool alreadyTaken = IsTargetTakenByOtherStation(candidate, forState);

            // Skip expensive visibility traces if this target cannot improve
            // the corresponding vehicle/infantry and free/shared tier.
            if (vehicle)
            {
                if (!alreadyTaken && distanceSq >= freeVehicleDistance)
                    continue;
                if (alreadyTaken && distanceSq >= sharedVehicleDistance)
                    continue;
            }
            else
            {
                if (!alreadyTaken && distanceSq >= freeInfantryDistance)
                    continue;
                if (alreadyTaken && distanceSq >= sharedInfantryDistance)
                    continue;
            }

            if (!HasLineOfSight(GetAimPoint(candidate), candidate))
                continue;

            if (vehicle)
            {
                if (alreadyTaken)
                {
                    sharedVehicleDistance = distanceSq;
                    sharedVehicle = candidate;
                }
                else
                {
                    freeVehicleDistance = distanceSq;
                    freeVehicle = candidate;
                }
            }
            else
            {
                if (alreadyTaken)
                {
                    sharedInfantryDistance = distanceSq;
                    sharedInfantry = candidate;
                }
                else
                {
                    freeInfantryDistance = distanceSq;
                    freeInfantry = candidate;
                }
            }
        }

        m_aQuery.Clear();

        // Target an exposed hostile vehicle first, even when other stations
        // are already attacking it. Only engage infantry when no tank is seen.
        if (freeVehicle)
            return freeVehicle;
        if (sharedVehicle)
            return sharedVehicle;
        if (freeInfantry)
            return freeInfantry;
        if (sharedInfantry)
            return sharedInfantry;

        return null;
    }

    // Keep one consistent, nonnegative range even if an editor enters reversed limits.
    protected float RandomFiringTime(float minimum, float maximum)
    {
        float low = Math.Max(minimum, 0.0);
        float high = Math.Max(maximum, low);
        return Math.RandomFloat(low, high);
    }

    protected int ChooseSequencedGroup(BFA_MechAIStationState state)
    {
        if (!state || !m_pCore)
            return 0;

        // Each virtual boss gunner has a dedicated offensive group. Preserve
        // that group's normal weapon routing and existing vehicle-first targeting.
        if (state.m_sStationId != "DRIVER")
            return ChooseWeaponGroup(state.m_sStationId, state.m_pTarget);

        // Group 1 is the arm pair, group 2 the shoulder. Toggle per burst
        // so disabling Fire All Groups does not permanently silence one bank.
        array<int> available = {};
        m_pCore.GetUsableWeaponGroups("DRIVER", available);
        bool arms = available.Contains(1) && GroupHasOffensiveWeapons("DRIVER", 1);
        bool shoulder = available.Contains(2) && GroupHasOffensiveWeapons("DRIVER", 2);

        if (arms && shoulder)
        {
            int selected = m_iNextDriverAttackGroup;
            if (selected != 1 && selected != 2)
                selected = 1;
            if (selected == 1)
                m_iNextDriverAttackGroup = 2;
            else
                m_iNextDriverAttackGroup = 1;
            return selected;
        }
        if (arms)
            return 1;
        if (shoulder)
            return 2;

        return ChooseWeaponGroup("DRIVER", state.m_pTarget);
    }

    // Boss-only version of the base station burst loop. Target checks, LOS,
    // replication and fire routing are the same; only the firing schedule changes.
    override protected void TickStation(BFA_MechAIStationState state, float dt)
    {
        if (!m_bStaggeredBatteryFire)
        {
            super.TickStation(state, dt);
            return;
        }

        if (!state || !m_pAim || !m_pRouter || !m_pCore)
            return;

        state.m_fRescanTimer -= dt;
        if (!IsValidTarget(state.m_pTarget) || state.m_fRescanTimer <= 0)
        {
            state.m_fRescanTimer = Math.Max(m_fScanInterval, 0.1);
            IEntity next = FindBestTarget(state);
            if (next != state.m_pTarget)
            {
                CeaseFire(state);
                state.m_pTarget = next;
                state.m_fTargetTime = 0;
                // Every bank starts on a different clock, including reacquisition.
                state.m_fBurstTimer = RandomFiringTime(m_fOpeningDelayMin, m_fOpeningDelayMax);
                if (m_bDebug && next)
                    Print(string.Format("[RW WARLORD FIRE] %1 target=%2 openingDelay=%3",
                        state.m_sStationId, next, state.m_fBurstTimer));
            }
        }

        if (!state.m_pTarget)
        {
            CeaseFire(state);
            return;
        }

        vector aimWorld = GetAimPoint(state.m_pTarget);
        m_pAim.ServerSetStationAim(state.m_sStationId, GetOwner().CoordToLocal(aimWorld));

        state.m_fTargetTime += dt;
        if (state.m_fTargetTime < Math.Max(m_fAimSettleTime, 0.0))
            return;

        if (!HasLineOfSight(aimWorld, state.m_pTarget))
        {
            CeaseFire(state);
            return;
        }

        state.m_fBurstTimer -= dt;
        if (state.m_iFiringGroup == 0)
        {
            if (state.m_fBurstTimer > 0)
                return;

            int group = ChooseSequencedGroup(state);
            if (group < 1)
                return;

            m_pCore.ServerSelectWeaponGroup(state.m_sStationId, group);
            m_pRouter.ServerSetFireHeld(state.m_sStationId, group, true);
            state.m_iFiringGroup = group;
            state.m_fBurstTimer = Math.Max(RandomFiringTime(m_fBurstLengthMin, m_fBurstLengthMax), 0.1);

            if (m_bDebug)
                Print(string.Format("[RW WARLORD FIRE] %1 group=%2 burst=%3",
                    state.m_sStationId, group, state.m_fBurstTimer));
        }
        else if (state.m_fBurstTimer <= 0)
        {
            CeaseFire(state);
            state.m_fBurstTimer = Math.Max(RandomFiringTime(m_fVolleyPauseMin, m_fVolleyPauseMax), 0.1);
        }
    }

}
