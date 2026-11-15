// Warlord-only 275 metre napalm siege projector.
// Uses the existing Omni AI, physical-cover clipping, shot/impact router and
// dedicated-server-authoritative damage. No shared Mech Core modifications.
class RW_WarlordNapalmPatch
{
    vector m_vPosition;
    IEntity m_pInstigator;
    int m_iExpireAt;
    int m_iNextTick;
}

[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord 200-300m napalm stream and persistent area fire.")]
class RW_WarlordNapalmProjectorClass : RW_WarlordOmniTurretClass
{
}

class RW_WarlordNapalmProjector : RW_WarlordOmniTurret
{
    [Attribute("6.5", UIWidgets.Auto, "How long a napalm impact burns (seconds).", category: "Warlord Napalm")]
    protected float m_fNapalmDuration;
    [Attribute("1", UIWidgets.Auto, "Time between burn damage ticks (seconds).", category: "Warlord Napalm")]
    protected float m_fNapalmTickSeconds;
    [Attribute("4.5", UIWidgets.Auto, "Burn damage per tick, before normal target-type multipliers.", category: "Warlord Napalm")]
    protected float m_fNapalmTickDamage;
    [Attribute("9", UIWidgets.Auto, "Burning zone radius in metres.", category: "Warlord Napalm")]
    protected float m_fNapalmRadius;
    [Attribute("6", UIWidgets.Auto, "Cap active burning zones per flamethrower for server performance.", category: "Warlord Napalm")]
    protected int m_iMaxNapalmZones;
    [Attribute("6", UIWidgets.Auto, "Refresh, rather than stack, nearby patches within this radius.", category: "Warlord Napalm")]
    protected float m_fPatchMergeRadius;

    [Attribute("24", UIWidgets.Auto, "Half-width of the ground spray at maximum range, metres; jet sweeps left and right across this area.", category: "Warlord Napalm")]
    protected float m_fSprayHalfWidth;

    [Attribute("500", UIWidgets.Auto, "Milliseconds between replicated ground-fire visual clusters; damage packets remain independent.", category: "Warlord Napalm")]
    protected int m_iImpactVisualIntervalMs;

    protected int m_iSprayPulse;
    protected int m_iNextImpactVisualMs;
    protected ref array<ref RW_WarlordNapalmPatch> m_aPatches = {};
    protected bool m_bNapalmTickRunning;

    override void OnDelete(IEntity owner)
    {
        if (GetGame() && GetGame().GetCallqueue())
        {
            GetGame().GetCallqueue().Remove(NapalmLand);
            GetGame().GetCallqueue().Remove(TickBurnZones);
        }
        m_bNapalmTickRunning = false;
        m_aPatches.Clear();
        super.OnDelete(owner);
    }

    // Each packet travels toward the surface near a hostile target.
    // Use actual world collision, so walls and roofs block it.
    override protected void FireShot(IEntity instigator)
    {
        if (!Replication.IsServer() || !m_pTarget || !m_pMechOwner || !GetGame() || !GetGame().GetWorld())
            return;

        BaseWorld world = GetGame().GetWorld();
        vector start = GetShotStart();
        vector aim = PredictAimPoint(start);
        // Wide sweeping wash, not independent rifle-like shots at one point.
        // Move the jet across thirteen lanes left-to-right in a repeating
        // pattern. Small jitter breaks up visibly regular spacing. Width
        // grows with range and respects the existing GM inaccuracy multiplier.
        vector horizontal = aim - start;
        horizontal[1] = 0;
        if (horizontal.LengthSq() < 0.01)
            horizontal = GetOwner().GetTransformAxis(2);
        horizontal[1] = 0;
        horizontal.Normalize();

        float distanceToTarget = vector.Distance(start, aim);
        float rangeFactor = Math.Clamp(distanceToTarget / Math.Max(m_fRange, 1), 0.2, 1.0);
        float spreadFactor = Math.Clamp(GetSpreadScale(), 0.75, 2.0);
        float halfWidth = Math.Clamp(m_fSprayHalfWidth * rangeFactor * spreadFactor, 0.0, 32.0);

        int lane = Math.Mod(m_iSprayPulse, 13) - 6;
        m_iSprayPulse++;
        float sideways = lane * (halfWidth / 6.0) + Math.RandomFloat(-1.5, 1.5);
        float foreAft = Math.RandomFloat(-halfWidth * 0.3, halfWidth * 0.3);
        float x = aim[0] + horizontal[2] * sideways + horizontal[0] * foreAft;
        float z = aim[2] - horizontal[0] * sideways + horizontal[2] * foreAft;
        vector wanted = Vector(x, world.GetSurfaceY(x, z) + 0.35, z);
        vector ray = wanted - start;
        float dist = ray.Length();
        if (dist < 0.5)
            return;

        if (dist > m_fRange)
            wanted = start + ray * (Math.Max(m_fRange, 1.0) / dist);

        vector impact = RW_WarlordCoverVisibility.ClipImpact(m_pMechOwner, GetOwner(), start, wanted);
        if (vector.DistanceSq(start, impact) < 1.0)
            return;

        float speed = Math.Max(m_fShellSpeed, 25.0);
        int shotId = BroadcastShot(start, impact, true, speed, m_fTracerLength, m_fTracerThickness, true);
        int flightMs = Math.Clamp((int)(1000.0 * vector.Distance(start, impact) / speed), 20, 4000);
        ScriptCallQueue queue = GetGame().GetCallqueue();
        if (queue)
            queue.CallLater(NapalmLand, flightMs, false, impact, instigator, m_iShellToken, shotId);
        else
            NapalmLand(impact, instigator, m_iShellToken, shotId);
    }

    protected void NapalmLand(vector impact, IEntity instigator, int token, int shotId)
    {
        if (!Replication.IsServer() || token != m_iShellToken || !m_pMechOwner || !GetOwner())
            return;

        DealSplashDamage(impact, m_fBurstDamage, m_fBurstRadius, EDamageType.INCENDIARY, instigator);
        RegisterPatch(impact, instigator);

        // Reliable impact RPC: creates the fire patch visual on all clients.
        // The router separately sends the existing Warlord impact sound.
        RW_WarlordBossWeaponRouterComponent router = RW_WarlordBossWeaponRouterComponent.Cast(
            m_pMechOwner.FindComponent(RW_WarlordBossWeaponRouterComponent));
        if (!router)
            return;

        // Continuous napalm sends many legitimate server-side damage packets.
        // Do not create one 7-8 second fire prefab and reliable impact RPC for
        // EVERY packet. Broadcast visual clusters at a bounded interval;
        // skipped packets still travel, land and deal their normal damage.
        int now = GetGame().GetWorld().GetWorldTime();
        if (now < m_iNextImpactVisualMs)
            return;
        m_iNextImpactVisualMs = now + Math.Clamp(m_iImpactVisualIntervalMs, 250, 2000);

        ResourceName particle = "";
        if (m_FX)
            particle = m_FX.m_sImpactParticle;
        router.ServerBroadcastModuleImpact(particle, impact, shotId, m_iHardpointIndex);
    }

    protected void RegisterPatch(vector position, IEntity instigator)
    {
        if (!GetGame() || !GetGame().GetWorld())
            return;

        int now = GetGame().GetWorld().GetWorldTime();
        int lifeMs = (int)(1000.0 * Math.Max(m_fNapalmDuration, 0.1));
        float merge = Math.Max(m_fPatchMergeRadius, 0);
        foreach (RW_WarlordNapalmPatch patch : m_aPatches)
        {
            if (!patch || vector.DistanceSq(patch.m_vPosition, position) > merge * merge)
                continue;

            patch.m_iExpireAt = Math.Max(patch.m_iExpireAt, now + lifeMs);
            patch.m_pInstigator = instigator;
            return;
        }

        int maxZones = Math.Clamp(m_iMaxNapalmZones, 1, 12);
        while (m_aPatches.Count() >= maxZones)
            m_aPatches.Remove(0);

        RW_WarlordNapalmPatch next = new RW_WarlordNapalmPatch();
        next.m_vPosition = position;
        next.m_pInstigator = instigator;
        next.m_iExpireAt = now + lifeMs;
        next.m_iNextTick = now + Math.Max((int)(1000.0 * m_fNapalmTickSeconds), 200);
        m_aPatches.Insert(next);

        if (!m_bNapalmTickRunning && GetGame().GetCallqueue())
        {
            m_bNapalmTickRunning = true;
            GetGame().GetCallqueue().CallLater(TickBurnZones, 250, true);
        }
    }

    protected void TickBurnZones()
    {
        if (!Replication.IsServer() || !GetOwner() || !m_pMechOwner || !GetGame() || !GetGame().GetWorld())
        {
            StopBurnZones();
            return;
        }

        int now = GetGame().GetWorld().GetWorldTime();
        int stepMs = Math.Max((int)(1000.0 * Math.Max(m_fNapalmTickSeconds, 0.2)), 200);
        for (int i = m_aPatches.Count() - 1; i >= 0; i--)
        {
            RW_WarlordNapalmPatch patch = m_aPatches[i];
            if (!patch || now >= patch.m_iExpireAt)
            {
                m_aPatches.Remove(i);
                continue;
            }
            if (now < patch.m_iNextTick)
                continue;

            patch.m_iNextTick = now + stepMs;
            DealSplashDamage(patch.m_vPosition, m_fNapalmTickDamage,
                m_fNapalmRadius, EDamageType.INCENDIARY, patch.m_pInstigator);
        }

        if (m_aPatches.IsEmpty())
            StopBurnZones();
    }

    protected void StopBurnZones()
    {
        if (m_bNapalmTickRunning && GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(TickBurnZones);
        m_bNapalmTickRunning = false;
    }
}

// A self-cleaning visual fire patch. Mech Core spawns the prefab locally on
// every client when a napalm packet arrives, then removes it after 7 seconds.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Napalm impact fire, client presentation only.")]
class RW_WarlordNapalmFirePatchVisualClass : ScriptComponentClass
{
}

class RW_WarlordNapalmFirePatchVisual : ScriptComponent
{
    [Attribute("{79DD3A5F1F3069CD}Particles/Vehicle/Vehicle_fire_ground_medium.ptc", UIWidgets.ResourceNamePicker, "Fire particle.", params: "ptc", category: "Napalm FX")]
    protected ResourceName m_sFireParticle;

    [Attribute("12", UIWidgets.Auto, "Spread local-only ground fire around the impact, metres.", category: "Napalm FX")]
    protected float m_fGroundVisualRadius;

    protected ref array<ParticleEffectEntity> m_aFires = {};

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (!owner || System.IsConsoleApp() || m_sFireParticle.IsEmpty())
            return;

        // One authority-sent impact creates a broad, deterministic cluster
        // on each client. These are NOT damage entities, have no physics or
        // replication, and are all cleaned up with the parent prefab.
        float radius = Math.Clamp(m_fGroundVisualRadius, 0, 16);
        for (int i = 0; i < 5; i++)
        {
            vector offset = vector.Zero;
            if (i == 1)
                offset = Vector(radius, 0, 0);
            else if (i == 2)
                offset = Vector(-radius, 0, 0);
            else if (i == 3)
                offset = Vector(0, 0, radius);
            else if (i == 4)
                offset = Vector(0, 0, -radius);

            ParticleEffectEntitySpawnParams params = new ParticleEffectEntitySpawnParams();
            Math3D.MatrixIdentity4(params.Transform);
            params.Transform[3] = owner.GetOrigin() + offset;
            ParticleEffectEntity effect = ParticleEffectEntity.SpawnParticleEffect(m_sFireParticle, params);
            if (effect)
                m_aFires.Insert(effect);
        }
    }

    override void OnDelete(IEntity owner)
    {
        foreach (ParticleEffectEntity fire : m_aFires)
        {
            if (fire)
                delete fire;
        }
        m_aFires.Clear();
        super.OnDelete(owner);
    }
}
