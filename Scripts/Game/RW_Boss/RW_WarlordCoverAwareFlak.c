//------------------------------------------------------------------------------------------------
// Warlord OPFOR world boss: server-only physical cover checks.
//
// All Warlord Omni and AA/flak batteries must respect buildings, walls, static
// geometry and terrain. The original anti-air module leads the target and
// schedules damage at the predicted burst point without checking the path.
// In addition, Core's splash protection only tests TraceEnt; a WORLD hit has
// TraceEnt == null, which can let its damage pass through static geometry.
//
// Do not change the shared CGS anti-air or mech components used by other mechs.
//------------------------------------------------------------------------------------------------
class RW_WarlordCoverVisibility
{
    // This is not a "target found" test; it is a physical bullet/beam path.
    // Self-collisions are ignored one by one. Every other obstruction fails
    // closed, including WORLD hits without an entity.
    static bool IsClear(IEntity mech, IEntity weapon, vector from, vector to, IEntity target = null)
    {
        BaseWorld world = GetGame().GetWorld();
        if (!world || !mech || vector.DistanceSq(from, to) < 0.0001)
            return false;

        IEntity targetRoot;
        if (target)
        {
            targetRoot = target.GetRootParent();
            if (!targetRoot)
                targetRoot = target;
        }

        array<IEntity> excludes = { mech };
        if (weapon && weapon != mech)
            excludes.Insert(weapon);

        for (int tries = 0; tries < 20; tries++)
        {
            TraceParam ray = new TraceParam();
            ray.Start = from;
            ray.End = to;
            ray.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            ray.LayerMask = EPhysicsLayerPresets.Projectile;
            ray.ExcludeArray = excludes;

            float fraction = world.TraceMove(ray, null);
            if (fraction >= 1.0)
                return TerrainClear(world, from, to);

            IEntity hit = ray.TraceEnt;
            if (!hit)
                return false; // Static WORLD geometry or terrain, not empty air.

            IEntity root = hit.GetRootParent();
            if (hit == mech || root == mech || (weapon && (hit == weapon || root == weapon)))
            {
                if (excludes.Find(hit) >= 0)
                    return false;
                excludes.Insert(hit);
                continue;
            }

            if (target && (hit == target || hit == targetRoot || root == target || root == targetRoot))
                return TerrainClear(world, from, to);

            return false; // Building, wall, other vehicle, other character, tree, etc.
        }
        return false;
    }

    // For the autonomous shell simulation, explode at the FIRST physical
    // obstruction along the real firing line. Never teleport the detonation
    // through a house to the predicted position on the other side.
    static vector ClipImpact(IEntity mech, IEntity weapon, vector from, vector wanted)
    {
        BaseWorld world = GetGame().GetWorld();
        if (!world || !mech || vector.DistanceSq(from, wanted) < 0.0001)
            return from;

        array<IEntity> excludes = { mech };
        if (weapon && weapon != mech)
            excludes.Insert(weapon);

        for (int tries = 0; tries < 20; tries++)
        {
            TraceParam ray = new TraceParam();
            ray.Start = from;
            ray.End = wanted;
            ray.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            ray.LayerMask = EPhysicsLayerPresets.Projectile;
            ray.ExcludeArray = excludes;

            float fraction = world.TraceMove(ray, null);
            if (fraction >= 1.0)
                return wanted;

            IEntity hit = ray.TraceEnt;
            if (hit)
            {
                IEntity root = hit.GetRootParent();
                if (hit == mech || root == mech || (weapon && (hit == weapon || root == weapon)))
                {
                    if (excludes.Find(hit) >= 0)
                        return from; // fail closed on an unresolvable self hit
                    excludes.Insert(hit);
                    continue;
                }
            }

            return from + (wanted - from) * Math.Clamp(fraction, 0, 1);
        }
        return from;
    }

    static vector AimPoint(IEntity target)
    {
        if (!target)
            return vector.Zero;

        if (ChimeraCharacter.Cast(target))
            return target.GetOrigin() + vector.Up * 1.0;

        vector minBounds, maxBounds;
        target.GetWorldBounds(minBounds, maxBounds);
        return (minBounds + maxBounds) * 0.5;
    }

    // Terrain-only fallback: physics traces still handle building meshes.
    protected static bool TerrainClear(BaseWorld world, vector from, vector to)
    {
        vector delta = to - from;
        int steps = Math.Min((int)(delta.Length() / 6.0), 80);
        if (steps <= 1)
            return true;

        for (int i = 1; i < steps; i++)
        {
            vector point = from + delta * ((float)i / (float)steps);
            if (point[1] <= world.GetSurfaceY(point[0], point[2]) + 0.2)
                return false;
        }
        return true;
    }
}

[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord flak with real LOS, projectile impact clipping, and covered-blast protection.")]
class RW_WarlordCoverAwareFlakComponentClass : CGS_AntiAirModuleComponentClass
{
}

// Applies to the Warlord's anti-air turrets and is the parent of the Warlord
// Omni guns. Normal CGS_AntiAirModuleComponent is NOT modified.
class RW_WarlordCoverAwareFlakComponent : CGS_AntiAirModuleComponent
{
    // The Omni laser is hitscan, while the boss cannon and flak remain
    // simulated projectiles. Enable only on the actual beam-laser prefab.
    [Attribute("false", UIWidgets.CheckBox, "Make beam impact and damage happen at shot time instead of waiting for simulated shell flight. For beam weapons only.", category: "Warlord Timing")]
    protected bool m_bInstantBeamImpact;

    // A beam hits the target's CURRENT position. Do not lead the target as
    // if the laser travelled at the shell velocity; keep turret aim aligned
    // with the authoritative instant ray and impact effect.
    override protected vector PredictAimPoint(vector start)
    {
        if (!m_bInstantBeamImpact || !m_pTarget || !m_FX || m_FX.m_sBeamPrefab.IsEmpty())
            return super.PredictAimPoint(start);

        vector mins, maxs;
        m_pTarget.GetWorldBounds(mins, maxs);
        return (mins + maxs) * 0.5;
    }

    // Direct per-shot check, not only at target selection time: a player who
    // runs behind a building during a burst must immediately be protected.
    override protected bool TryFireShot(IEntity instigator)
    {
        if (!m_pTarget || !m_pMechOwner)
            return false;

        if (!RW_WarlordCoverVisibility.IsClear(
            m_pMechOwner, GetOwner(), GetShotStart(),
            RW_WarlordCoverVisibility.AimPoint(m_pTarget), m_pTarget))
            return false;

        return super.TryFireShot(instigator);
    }

    // The base flak sim schedules a detonation at a predicted position even if
    // a building lies between the muzzle and that position. Intercept it first.
    override protected void FireShot(IEntity instigator)
    {
        if (!m_pTarget || !m_pMechOwner)
            return;

        vector start = GetShotStart();

        vector mins, maxs;
        m_pTarget.GetWorldBounds(mins, maxs);
        vector targetPos = (mins + maxs) * 0.5;

        float speed = Math.Max(m_fShellSpeed, 50);
        float distance = vector.Distance(start, targetPos);
        vector lead = PredictAimPoint(start);

        float miss = (m_fMissRadius + distance * 0.01 * m_fMissPer100m) * GetSpreadScale();
        vector scatter = Vector(Math.RandomFloat(-1, 1), Math.RandomFloat(-1, 1), Math.RandomFloat(-1, 1));
        if (scatter.LengthSq() > 1)
            scatter.Normalize();
        vector wanted = lead + scatter * miss;

        vector burst = RW_WarlordCoverVisibility.ClipImpact(m_pMechOwner, GetOwner(), start, wanted);

        // The client draws a beam across its full length immediately. For a
        // hitscan laser, detonate on the SERVER in the same shot tick, too.
        // This keeps visible impact, damage and beam aligned even at 1 km.
        // Still use the physics-clipped burst point: walls and roofs block it.
        BroadcastShot(start, burst, true, speed, m_fTracerLength, m_fTracerThickness);
        if (m_bInstantBeamImpact && m_FX && !m_FX.m_sBeamPrefab.IsEmpty())
        {
            Detonate(burst, instigator, m_iShellToken);
            return;
        }

        // Physical flak and autocannons keep the normal range-dependent
        // travel delay and their wall-clipped detonation positions.
        float flight = vector.Distance(start, burst) / speed;
        int delayMs = (int)(flight * 1000);
        if (delayMs < 20)
        {
            Detonate(burst, instigator, m_iShellToken);
            return;
        }
        GetGame().GetCallqueue().CallLater(Detonate, delayMs, false, burst, instigator, m_iShellToken);
    }

    // Core splash damage checks blocker != null, but a static WORLD hit has
    // TraceEnt == null. This extra boss-only gate blocks that damage path.
    // The impact point is supplied by the parent splash code as hitPos.
    override protected float DealDamage(IEntity hitEntity, float amount, EDamageType type, vector hitPos, vector direction, IEntity instigator)
    {
        if (!hitEntity || !m_pMechOwner)
            return 0;

        vector start = hitPos + vector.Up * 0.25;
        vector targetPoint = RW_WarlordCoverVisibility.AimPoint(hitEntity);

        if (!RW_WarlordCoverVisibility.IsClear(m_pMechOwner, GetOwner(), start, targetPoint, hitEntity))
            return 0;

        return super.DealDamage(hitEntity, amount, type, hitPos, direction, instigator);
    }
}
