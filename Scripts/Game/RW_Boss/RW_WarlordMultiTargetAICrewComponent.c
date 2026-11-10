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
}
