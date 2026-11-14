//------------------------------------------------------------------------------------------------
// Warlord OPFOR world boss: auxiliary autonomous weapon bank.
//
// The standard Mech Core hardpoint component supports a maximum of 18 replicated
// hardpoints. The complete OPFOR boss already occupies all 18. The old 128-player
// prototype also had autonomous Omni/AA defenses. This BOSS-ONLY component carries
// those additional independent defenses without modifying the shared 18-slot
// loadout replication, the existing OPFOR arm guns, or the other mechs.
//
// Weapon prefabs have no RplComponent. As with BFA_MechHardpointComponent, spawn
// their attached instances locally on each machine in stable array order; only
// the server runs autonomous targeting, damage, and shot RPCs.
// The Torso slot is the parent so these guns inherit torso yaw exactly once.
//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class RW_WarlordAuxiliaryWeaponDefinition
{
    [Attribute("Aux Gun", UIWidgets.EditBox, "Unique world-boss battery name.", category: "Auxiliary Battery")]
    string m_sName;

    [Attribute("", UIWidgets.ResourceNamePicker, "Weapon module prefab, not a mesh-only prefab.", params: "et", category: "Auxiliary Battery")]
    ResourceName m_sPrefab;

    [Attribute("0 0 0", UIWidgets.Auto, "Position relative to the upper torso slot; X right, Y up, Z forward.", category: "Auxiliary Battery")]
    vector m_vLocalOffset;

    [Attribute("0 0 0", UIWidgets.Auto, "Initial local yaw, pitch, roll; turret aiming is applied on top.", category: "Auxiliary Battery")]
    vector m_vLocalAngles;
}

[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord auxiliary Omni/AA guns outside the core 18-hardpoint limit.")]
class RW_WarlordAuxiliaryBatteryComponentClass : ScriptComponentClass
{
}

class RW_WarlordAuxiliaryBatteryComponent : ScriptComponent
{
    [Attribute("false", UIWidgets.CheckBox, "Extra aux setup traces, OFF during hip tests.", category: "Diagnostics")]
    protected bool m_bVerboseAuxLogs;

    // Keep this above Mech Core's 0..17 real hardpoints. The boss-only weapon
    // router maps 18+ directly to these locally created modules for shot FX.
    static const int FIRST_VIRTUAL_HARDPOINT = 18;
    // Keep outside the auxiliary 18-29 range and outside the 18 core slots.
    static const int TOP_RIGHT_VIRTUAL_HARDPOINT = 48;
    static const int TOP_LEFT_VIRTUAL_HARDPOINT = 49;

    [Attribute("Torso", UIWidgets.EditBox, "Upper body slot whose yaw the battery follows.", category: "Auxiliary Battery")]
    protected string m_sParentSlot;

    [Attribute("", UIWidgets.Object, "Autonomous defense modules from the retired 128-player configuration.", category: "Auxiliary Battery")]
    protected ref array<ref RW_WarlordAuxiliaryWeaponDefinition> m_aWeapons;

    protected ref array<IEntity> m_aEntities = {};
    protected ref array<BFA_MechWeaponModuleComponent> m_aLiveModules = {};
    protected bool m_bBuilt;
    protected int m_iResolveAttempts;

    // Both large top guns already exist in SlotManager. Bind the existing
    // slotted CGS Titan Cannon modules instead of spawning duplicate geometry.
    [Attribute("true", UIWidgets.CheckBox, "Enable the two existing Warlord top Titan cannons as independent boss siege guns.", category: "Warlord Top Cannons")]
    protected bool m_bEnableTopCannons;
    [Attribute("5.5", UIWidgets.Auto, "Minimum seconds between individual top-cannon attacks.", category: "Warlord Top Cannons")]
    protected float m_fTopCannonPauseMin;
    [Attribute("9.5", UIWidgets.Auto, "Maximum seconds between individual top-cannon attacks.", category: "Warlord Top Cannons")]
    protected float m_fTopCannonPauseMax;

    protected CGS_Titan_Cannon m_pTopRight;
    protected CGS_Titan_Cannon m_pTopLeft;
    protected int m_iTopBindAttempts;
    protected bool m_bTopFireTickRunning;
    protected int m_iTopRightNextMs;
    protected int m_iTopLeftNextMs;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (GetGame() && GetGame().GetCallqueue())
        {
            GetGame().GetCallqueue().CallLater(BuildBattery, 150, false);
            if (m_bEnableTopCannons)
                GetGame().GetCallqueue().CallLater(BindTopCannons, 350, false);
        }
    }

    override void OnDelete(IEntity owner)
    {
        if (GetGame() && GetGame().GetCallqueue())
        {
            GetGame().GetCallqueue().Remove(BuildBattery);
            GetGame().GetCallqueue().Remove(BindTopCannons);
            GetGame().GetCallqueue().Remove(TickTopCannons);
        }
        m_bTopFireTickRunning = false;
        m_pTopRight = null;
        m_pTopLeft = null;

        // All locally-created modules are children of the torso slot and are
        // destroyed with it. Never delete another mod's weapon or a core slot.
        m_aLiveModules.Clear();
        m_aEntities.Clear();
        super.OnDelete(owner);
    }

    // Called by the boss-only BFA weapon router for remote muzzle/beam FX.
    BFA_MechWeaponModuleComponent GetModuleForVirtualIndex(int hardpointIndex)
    {
        int slot = hardpointIndex - FIRST_VIRTUAL_HARDPOINT;
        if (slot < 0 || slot >= m_aLiveModules.Count())
            return null;
        return m_aLiveModules[slot];
    }


    BFA_MechWeaponModuleComponent GetTopCannonModuleForVirtualIndex(int index)
    {
        if (index == TOP_RIGHT_VIRTUAL_HARDPOINT)
            return m_pTopRight;
        if (index == TOP_LEFT_VIRTUAL_HARDPOINT)
            return m_pTopLeft;
        return null;
    }

    protected void BindTopCannons()
    {
        if (!GetOwner() || !GetGame())
            return;

        SlotManagerComponent slots = SlotManagerComponent.Cast(GetOwner().FindComponent(SlotManagerComponent));
        if (slots)
        {
            EntitySlotInfo rightSlot = slots.GetSlotByName("Cannon_top_R");
            EntitySlotInfo leftSlot = slots.GetSlotByName("Cannon_top_L");

            if (!m_pTopRight && rightSlot && rightSlot.GetAttachedEntity())
            {
                m_pTopRight = CGS_Titan_Cannon.Cast(
                    rightSlot.GetAttachedEntity().FindComponent(CGS_Titan_Cannon));
                if (m_pTopRight)
                    m_pTopRight.BindToHardpoint(GetOwner(), TOP_RIGHT_VIRTUAL_HARDPOINT,
                        "WARLORD_TOP_RIGHT", BFA_EMechHardpointClass.UTILITY, "DRIVER", 2);
            }

            if (!m_pTopLeft && leftSlot && leftSlot.GetAttachedEntity())
            {
                m_pTopLeft = CGS_Titan_Cannon.Cast(
                    leftSlot.GetAttachedEntity().FindComponent(CGS_Titan_Cannon));
                if (m_pTopLeft)
                    m_pTopLeft.BindToHardpoint(GetOwner(), TOP_LEFT_VIRTUAL_HARDPOINT,
                        "WARLORD_TOP_LEFT", BFA_EMechHardpointClass.UTILITY, "DRIVER", 2);
            }
        }

        // Only the server simulates and decides to fire. All clients bind
        // locally so the existing module-shot RPC can resolve the right FX.
        if (Replication.IsServer() && !m_bTopFireTickRunning && (m_pTopRight || m_pTopLeft))
        {
            m_bTopFireTickRunning = true;
            int now = System.GetTickCount();
            m_iTopRightNextMs = now + 2000;
            m_iTopLeftNextMs = now + 4000;
            GetGame().GetCallqueue().CallLater(TickTopCannons, 500, true);
        }

        if ((!m_pTopRight || !m_pTopLeft) && m_iTopBindAttempts < 40)
        {
            m_iTopBindAttempts++;
            GetGame().GetCallqueue().CallLater(BindTopCannons, 250, false);
        }
        else if ((!m_pTopRight || !m_pTopLeft) && m_iTopBindAttempts >= 40)
        {
            Print("[RW WARLORD CANNON] One or both Cannon_top slots are missing or have no CGS_Titan_Cannon.", LogLevel.WARNING);
        }
    }

    protected int NextTopCannonDelayMs()
    {
        float pause = Math.RandomFloat(
            Math.Max(m_fTopCannonPauseMin, 1.0),
            Math.Max(m_fTopCannonPauseMax, Math.Max(m_fTopCannonPauseMin, 1.0)));
        return (int)(pause * 1000.0);
    }

    // Reuse the verified DRIVER target. No separate 128-player entity query.
    // Each of the two slotted heavy guns fires on its own randomized clock.
    protected void TickTopCannons()
    {
        if (!Replication.IsServer() || !GetOwner() || !m_bEnableTopCannons)
            return;

        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(GetOwner());
        if (director && !director.AllowsTopCannons())
            return;

        RW_WarlordMultiTargetAICrewComponent ai = RW_WarlordMultiTargetAICrewComponent.Cast(
            GetOwner().FindComponent(RW_WarlordMultiTargetAICrewComponent));
        if (!ai)
            return;

        IEntity target = ai.GetWarlordDriverTarget();
        if (!target)
            return;

        vector targetPoint = RW_WarlordCoverVisibility.AimPoint(target);
        int now = System.GetTickCount();

        if (m_pTopRight && m_pTopRight.IsBoundToHardpoint() && now >= m_iTopRightNextMs)
        {
            IEntity gunRight = m_pTopRight.GetOwner();
            if (gunRight && RW_WarlordCoverVisibility.IsClear(
                GetOwner(), gunRight, gunRight.GetOrigin(), targetPoint, target))
            {
                m_pTopRight.ServerTriggerDown(null);
                m_pTopRight.ServerTriggerUp(null);
                m_iTopRightNextMs = now + NextTopCannonDelayMs();
            }
        }

        if (m_pTopLeft && m_pTopLeft.IsBoundToHardpoint() && now >= m_iTopLeftNextMs)
        {
            IEntity gunLeft = m_pTopLeft.GetOwner();
            if (gunLeft && RW_WarlordCoverVisibility.IsClear(
                GetOwner(), gunLeft, gunLeft.GetOrigin(), targetPoint, target))
            {
                m_pTopLeft.ServerTriggerDown(null);
                m_pTopLeft.ServerTriggerUp(null);
                m_iTopLeftNextMs = now + NextTopCannonDelayMs();
            }
        }
    }

    protected void BuildBattery()
    {
        if (m_bBuilt)
            return;

        IEntity mech = GetOwner();
        if (!mech || !GetGame())
            return;

        SlotManagerComponent slots = SlotManagerComponent.Cast(mech.FindComponent(SlotManagerComponent));
        EntitySlotInfo torsoSlot;
        if (slots)
            torsoSlot = slots.GetSlotByName(m_sParentSlot);

        IEntity torso;
        if (torsoSlot)
            torso = torsoSlot.GetAttachedEntity();

        // Slots may attach later on a dedicated server / joining MP client.
        // Avoid root fallback: firing from the non-twisting hips is incorrect.
        if (!torso)
        {
            m_iResolveAttempts++;
            if (m_iResolveAttempts <= 40 && GetGame().GetCallqueue())
                GetGame().GetCallqueue().CallLater(BuildBattery, 250, false);
            else
                Print("[RW WARLORD AUX] Torso slot missing; auxiliary weapons not started.", LogLevel.WARNING);
            return;
        }

        m_bBuilt = true;
        if (!m_aWeapons)
            return;

        int builtCount = 0;
        for (int i = 0; i < m_aWeapons.Count(); i++)
        {
            RW_WarlordAuxiliaryWeaponDefinition definition = m_aWeapons[i];
            m_aEntities.Insert(null);
            m_aLiveModules.Insert(null);

            if (!definition || definition.m_sPrefab.IsEmpty())
                continue;

            Resource prefab = Resource.Load(definition.m_sPrefab);
            if (!prefab || !prefab.IsValid())
            {
                Print(string.Format("[RW WARLORD AUX] Invalid prefab for %1: %2", definition.m_sName, definition.m_sPrefab), LogLevel.WARNING);
                continue;
            }

            EntitySpawnParams spawn = new EntitySpawnParams();
            spawn.TransformMode = ETransformMode.WORLD;
            torso.GetWorldTransform(spawn.Transform);

            IEntity gun = GetGame().SpawnEntityPrefab(prefab, mech.GetWorld(), spawn);
            if (!gun)
            {
                Print(string.Format("[RW WARLORD AUX] Failed to spawn %1", definition.m_sName), LogLevel.WARNING);
                continue;
            }

           torso.AddChild(gun, -1);

            vector local[4];
            Math3D.AnglesToMatrix(definition.m_vLocalAngles, local);
            local[3] = definition.m_vLocalOffset;
            gun.SetLocalTransform(local);
            gun.Update();

            BFA_MechWeaponModuleComponent module = BFA_MechWeaponModuleComponent.Cast(gun.FindComponent(BFA_MechWeaponModuleComponent));
            if (!module)
            {
                Print(string.Format("[RW WARLORD AUX] %1 has no weapon module component", definition.m_sName), LogLevel.WARNING);
                SCR_EntityHelper.DeleteEntityAndChildren(gun);
                continue;
            }

            int virtualIndex = FIRST_VIRTUAL_HARDPOINT + i;
            module.BindToHardpoint(mech, virtualIndex, definition.m_sName, BFA_EMechHardpointClass.UTILITY, "DRIVER", 4);
            m_aEntities[i] = gun;
            m_aLiveModules[i] = module;
            builtCount++;
        }

        if (m_bVerboseAuxLogs)
            Print(string.Format("[RW WARLORD AUX] %1 autonomous turret modules attached to the torso (%2 configured).", builtCount, m_aWeapons.Count()));
    }
}

//------------------------------------------------------------------------------------------------
// Isolated Warlord weapon router. Virtual hardpoint indices are local to the
// boss's auxiliary component, NOT entries in the 18-slot player loadout.
// Reusing the normal router's shot RPC preserves multiplayer muzzle/beam FX.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Warlord shot router with auxiliary turret FX lookup.")]
class RW_WarlordBossWeaponRouterComponentClass : BFA_MechWeaponComponentClass
{
}

class RW_WarlordBossWeaponRouterComponent : BFA_MechWeaponComponent
{
    [Attribute("false", UIWidgets.CheckBox, "Verbose laser, impact audio and warhorn RPC logs, OFF for hip investigation.", category: "Diagnostics")]
    protected bool m_bVerboseWarlordLogs;

    // Count only physical cannon shells. The server puts a visual-only flag in
    // the unused tracer-length field of the EXISTING shot RPC. Thus every client
    // agrees on exactly which shell is a tracer, even with packet loss.
    // No extra RPC, network entity, damage, or shared Mech Core change.
    protected ref array<int> m_aCannonShotCounts = {};

    // Dedicated boss-only impact cue. A shot RPC may be dropped on an MP
    // client, and the core impact path needs a local hardpoint module.
    // This reliable audio-only message carries the actual impact location.
    // Effects and authoritative damage remain in the existing weapon system.
    protected int m_iLastImpactKineticMs;
    protected int m_iLastImpactEnergyMs;
    protected int m_iLastImpactExplosiveMs;
    // Locality-aware coalescing: one far-away impact must not silence a
    // separate wall/ground strike near another player in the same 100 ms.
    protected vector m_vLastImpactKinetic;
    protected vector m_vLastImpactEnergy;
    protected vector m_vLastImpactExplosive;
    protected int m_iImpactAudioLogs;
    protected int m_iChargeAudioLogs;
    protected bool m_bDriverLaserRequested;
    protected bool m_bDriverLaserCharging;

    // Boss-only reliable charge cue; all nearby clients hear the same 1-2-3
    // rising hum. Server remains authoritative for starting actual shots.
    void ServerBroadcastWarlordLaserCharge(vector position)
    {
        if (!Replication.IsServer())
            return;

        if (m_bVerboseWarlordLogs && m_iChargeAudioLogs < 8)
        {
            Print(string.Format("[RW WARLORD LASER][SERVER CHARGE SEND] at=%1", position));
            m_iChargeAudioLogs++;
        }
        if (!System.IsConsoleApp())
            PlayBossCombatAudioLocal(position, "SOUND_WARLORD_LASER_CHARGE");

        Rpc(RpcDo_WarlordLaserCharge, position);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_WarlordLaserCharge(vector position)
    {
        if (Replication.IsServer() || System.IsConsoleApp())
            return;
        if (m_bVerboseWarlordLogs && m_iChargeAudioLogs < 8)
        {
            Print("[RW WARLORD LASER][CLIENT CHARGE RECEIVED]");
            m_iChargeAudioLogs++;
        }
        PlayBossCombatAudioLocal(position, "SOUND_WARLORD_LASER_CHARGE");
    }

    protected void PlayBossCombatAudioLocal(vector position, string eventName)
    {
        if (System.IsConsoleApp())
            return;

        ResourceName project = "{A6111111000AA012}Sounds/Warlord/Warlord_Combat.acp";
        if (m_bVerboseWarlordLogs && m_iImpactAudioLogs < 20)
        {
            Resource resource = Resource.Load(project);
            bool valid = resource && resource.IsValid();
            float audible = AudioSystem.IsAudible(project, eventName, position);
            Print(string.Format("[RW WARLORD AUDIO][CLIENT] event=%1 valid=%2 audible=%3 pos=%4",
                eventName, valid, audible, position));
            m_iImpactAudioLogs++;
        }

        vector soundMat[4];
        Math3D.MatrixIdentity4(soundMat);
        soundMat[3] = position;
        BFA_MechWeaponModuleComponent.BFA_MechPlaySound(project, eventName, soundMat);
    }

    // Charge the pair of DRIVER arm Las Cannons once per held-fire salvo,
    // not once per individual shot and never on a proxy/client.
    override void ServerSetFireHeld(string stationId, int weaponGroup, bool held)
    {
        if (!Replication.IsServer())
            return;

        if (stationId != "DRIVER" || weaponGroup != 1)
        {
            super.ServerSetFireHeld(stationId, weaponGroup, held);
            return;
        }

        if (!held)
        {
            m_bDriverLaserRequested = false;
            m_bDriverLaserCharging = false;
            if (GetGame() && GetGame().GetCallqueue())
                GetGame().GetCallqueue().Remove(ServerFinishDriverLaserCharge);
            super.ServerSetFireHeld(stationId, weaponGroup, false);
            return;
        }

        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(GetOwner());
        if (!director || !director.GetLaserChargeEnabled())
        {
            super.ServerSetFireHeld(stationId, weaponGroup, true);
            return;
        }

        if (m_bDriverLaserRequested)
            return;

        m_bDriverLaserRequested = true;
        m_bDriverLaserCharging = true;
        int delayMs = Math.Max((int)(director.GetLaserChargeSeconds() * 1000.0), 750);
        ServerBroadcastWarlordLaserCharge(GetOwner().GetOrigin() + vector.Up * 18.0);
        if (m_bVerboseWarlordLogs)
            Print(string.Format("[RW WARLORD LASER][DRIVER CHARGE START] delayMs=%1", delayMs));

        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().CallLater(ServerFinishDriverLaserCharge, delayMs, false);
        else
        {
            m_bDriverLaserCharging = false;
            m_bDriverLaserRequested = false;
        }
    }

    protected void ServerFinishDriverLaserCharge()
    {
        if (!Replication.IsServer() || !m_bDriverLaserRequested || !m_bDriverLaserCharging || !GetOwner())
            return;

        m_bDriverLaserCharging = false;
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(GetOwner());
        if (director && !director.AllowsStationGroup("DRIVER", 1))
            return;

        if (m_bVerboseWarlordLogs)
            Print("[RW WARLORD LASER][DRIVER FIRE AFTER CHARGE]");
        super.ServerSetFireHeld("DRIVER", 1, true);
    }

    override void OnDelete(IEntity owner)
    {
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(ServerFinishDriverLaserCharge);
        super.OnDelete(owner);
    }

    // 0 = kinetic, 1 = energy, 2 = explosive. Group nearby impacts to avoid
    // reliable-RPC floods from the Warlord's many simultaneous batteries.
    void ServerBroadcastBossImpactAudio(vector position, int kind)
    {
        if (!Replication.IsServer())
            return;

        int now = System.GetTickCount();
        int previous = 0;
        int minGap = 100;
        vector previousPosition = vector.Zero;
        if (kind == 1)
        {
            previous = m_iLastImpactEnergyMs;
            previousPosition = m_vLastImpactEnergy;
            minGap = 130;
        }
        else if (kind == 2)
        {
            previous = m_iLastImpactExplosiveMs;
            previousPosition = m_vLastImpactExplosive;
            minGap = 180;
        }
        else
        {
            previous = m_iLastImpactKineticMs;
            previousPosition = m_vLastImpactKinetic;
        }

        // Suppress a rapid cluster of essentially coincident impacts, but
        // never suppress a hit at a different location across the battlefield.
        // In particular, a far miss cannot mute a shot striking a nearby wall.
        if (previous > 0 && now - previous < minGap
            && vector.DistanceSq(previousPosition, position) < 144.0)
            return;

        if (kind == 1)
        {
            m_iLastImpactEnergyMs = now;
            m_vLastImpactEnergy = position;
        }
        else if (kind == 2)
        {
            m_iLastImpactExplosiveMs = now;
            m_vLastImpactExplosive = position;
        }
        else
        {
            m_iLastImpactKineticMs = now;
            m_vLastImpactKinetic = position;
        }

        if (m_bVerboseWarlordLogs && m_iImpactAudioLogs < 12)
        {
            Print(string.Format("[RW WARLORD IMPACT][SERVER SEND] kind=%1 at=%2", kind, position));
            m_iImpactAudioLogs++;
        }

        if (!System.IsConsoleApp())
            PlayBossImpactAudioLocal(position, kind);

        Rpc(RpcDo_BossImpactAudio, position, kind);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_BossImpactAudio(vector position, int kind)
    {
        // Hosted server already played its own local sound.
        if (Replication.IsServer() || System.IsConsoleApp())
            return;

        if (m_bVerboseWarlordLogs && m_iImpactAudioLogs < 20)
            Print(string.Format("[RW WARLORD IMPACT][CLIENT RECEIVED] kind=%1 pos=%2", kind, position));
        PlayBossImpactAudioLocal(position, kind);
    }

    protected void PlayBossImpactAudioLocal(vector position, int kind)
    {
        if (System.IsConsoleApp())
            return;

        string eventName = "SOUND_WARLORD_IMPACT_KINETIC";
        if (kind == 1)
            eventName = "SOUND_WARLORD_IMPACT_ENERGY";
        else if (kind == 2)
            eventName = "SOUND_WARLORD_IMPACT_EXPLOSIVE";

        PlayBossCombatAudioLocal(position, eventName);
    }

    // Core Titan LRMs/rockets resolve impacts using this authoritative call,
    // not the initial shot event. Send sound at the actual detonation position.
    // This complements the Warlord's hitscan and autonomous flak paths.
    override void ServerBroadcastModuleImpact(ResourceName particle, vector position, int shotId = 0, int hardpointIndex = -1)
    {
        super.ServerBroadcastModuleImpact(particle, position, shotId, hardpointIndex);
        if (Replication.IsServer())
            ServerBroadcastBossImpactAudio(position, 2);
    }

    // Reuse this router's established replication path: its shot/audio RPCs
    // already reach the Warlord's multiplayer clients. The warhorn component
    // handles timing and audio; this only transports a reliable event cue.
    void ServerBroadcastWarlordWarhorn()
    {
        if (!Replication.IsServer())
            return;

        if (m_bVerboseWarlordLogs)
            Print("[RW WARLORD HORN][ROUTER][SERVER SEND] broadcasting warhorn cue through weapon router");
        Rpc(RpcDo_WarlordWarhornToClients);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_WarlordWarhornToClients()
    {
        // Dedicated server has no speakers; a hosted server plays its own cue
        // in the scheduler. Only client replicas play the broadcasted cue.
        if (Replication.IsServer())
            return;

        if (m_bVerboseWarlordLogs)
            Print("[RW WARLORD HORN][ROUTER][CLIENT RECEIVED] warhorn RPC arrived");

        IEntity boss = GetOwner();
        if (!boss)
        {
            Print("[RW WARLORD HORN][ROUTER][CLIENT ERROR] boss entity unavailable", LogLevel.WARNING);
            return;
        }

        RW_WarlordWarhornComponent warhorn = RW_WarlordWarhornComponent.Cast(
            boss.FindComponent(RW_WarlordWarhornComponent));
        if (!warhorn)
        {
            Print("[RW WARLORD HORN][ROUTER][CLIENT ERROR] warhorn component unavailable", LogLevel.WARNING);
            return;
        }

        warhorn.ClientPlayFromRouterRpc();
    }

    protected bool IsBossKineticCannon(BFA_MechWeaponModuleComponent module)
    {
        if (!module)
            return false;

        if (RW_WarlordBossAutocannon.Cast(module) || CGS_Titan_Cannon.Cast(module))
            return true;

        // This class also handles energy Omni beams; exclude those.
        return RW_WarlordOmniTurret.Cast(module) && module.GetModuleId() == "Warlord_Omni_Cannon";
    }

    // The base router broadcasts this float to all clients as a debug tracer
    // length. For non-beam guns that length is otherwise presentation-only.
    // A negative value means: use a dim kinetic shell visual (all shots still
    // render); every fourth shot uses the bright projectile/tracer FX.
    // World geometry (walls, roofs, roads, rocks) often has TraceEnt == null.
    // Core's hitscan returns a precise clipped endpoint for those collisions,
    // but 'hit' is false because no entity was struck. Probe only a small
    // neighbourhood around that endpoint to distinguish an actual physical
    // surface from a clean maximum-range shot through the air.
    protected bool IsWarlordPhysicalSurfaceAtShotEnd(vector start, vector end, BFA_MechWeaponModuleComponent module)
    {
        if (!GetGame() || !GetGame().GetWorld())
            return false;

        BaseWorld world = GetGame().GetWorld();

        // Terrain is a physical collision even when it has no entity.
        if (end[1] <= world.GetSurfaceY(end[0], end[2]) + 0.75)
            return true;

        vector toEnd = end - start;
        float distance = toEnd.Length();
        if (distance < 0.1)
            return false;

        vector direction = toEnd * (1.0 / distance);

        TraceParam probe = new TraceParam();
        // The server already resolved the actual shot. Rechecking its FINAL
        // two metres, plus a short overrun, avoids replaying a long ray and
        // catches walls even when the hit point is on a physics boundary.
        probe.Start = end - direction * Math.Min(distance * 0.25, 2.0);
        probe.End = end + direction * 0.65;
        probe.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        probe.LayerMask = EPhysicsLayerPresets.Projectile;

        array<IEntity> excludes = {};
        if (GetOwner())
            excludes.Insert(GetOwner());
        if (module && module.GetOwner())
            excludes.Insert(module.GetOwner());
        probe.ExcludeArray = excludes;

        float fraction = world.TraceMove(probe, null);
        if (fraction >= 1.0)
            return false;

        vector contact = probe.Start + (probe.End - probe.Start) * fraction;
        // Do not make a fake impact if a different obstruction happens to
        // be far away from the authoritative endpoint.
        return vector.DistanceSq(contact, end) <= 1.44;
    }

    override int ServerBroadcastModuleShot(
        BFA_MechWeaponModuleComponent module,
        vector start, vector end, bool hit, float speed,
        float length, float thickness, bool serverImpact = false)
    {
        if (Replication.IsServer() && IsBossKineticCannon(module))
        {
            int hp = module.GetHardpointIndex();
            if (hp >= 0 && hp < 64)
            {
                while (m_aCannonShotCounts.Count() <= hp)
                    m_aCannonShotCounts.Insert(0);

                int shots = m_aCannonShotCounts[hp] + 1;
                m_aCannonShotCounts[hp] = shots;
                if (Math.Mod(shots, 4) != 0)
                    length = -Math.Max(Math.AbsFloat(length), 1.0);
            }
        }

        int shotId = super.ServerBroadcastModuleShot(module, start, end, hit, speed,
            length, thickness, serverImpact);

        // Hitscan cannons and beams have no later server-impact callback.
        // Schedule their audio at the actual endpoint after visual travel.
        // Server-simulated LRMs and flak send audio on actual detonation instead.
        // An entity hit is only one type of collision. A shot can miss its
        // target and still hit world geometry with no TraceEnt (e.g. a wall).
        // A clean maximum-range sky shot must stay silent at its endpoint.
        bool physicalImpact = hit;
        if (!hit && !serverImpact && module && Replication.IsServer())
            physicalImpact = IsWarlordPhysicalSurfaceAtShotEnd(start, end, module);

        if (Replication.IsServer() && physicalImpact && !serverImpact && module
            && !RW_WarlordCoverAwareFlakComponent.Cast(module))
        {
            int kind = 0;
            BFA_MechWeaponFX fx = module.GetFX();
            if (fx && !fx.m_sBeamPrefab.IsEmpty())
                kind = 1;

            int delayMs = 0;
            if (kind != 1)
            {
                float travel = vector.Distance(start, end) / Math.Max(speed, 1.0);
                delayMs = Math.Clamp((int)(travel * 1000.0), 0, 3000);
            }

            if (delayMs > 0 && GetGame() && GetGame().GetCallqueue())
                GetGame().GetCallqueue().CallLater(ServerBroadcastBossImpactAudio, delayMs, false, end, kind);
            else
                ServerBroadcastBossImpactAudio(end, kind);
        }
        return shotId;
    }

    override protected void PlayShotLocal(
        int hardpointIndex, int shotId, vector start, vector end,
        int flags, float speed, float length, float thickness)
    {
        // Main Warlord Las Cannons inherit their old fire sound from the
        // generic arm prefab. Swap only their audio cue during this shot.
        // Do not change the shared Mech Core module or model offsets.
        BFA_MechWeaponModuleComponent laserModule = FindLocalModule(hardpointIndex);
        if (BFA_MechLasCannonModuleComponent.Cast(laserModule))
        {
            BFA_MechWeaponFX laserFX = laserModule.GetFX();
            if (laserFX)
            {
                ResourceName savedProject = laserFX.m_sSoundProject;
                string savedEvent = laserFX.m_sFireSoundEvent;
                laserFX.m_sSoundProject = "{A6111111000AA012}Sounds/Warlord/Warlord_Combat.acp";
                laserFX.m_sFireSoundEvent = "SOUND_WARLORD_LASER_FIRE";
                super.PlayShotLocal(hardpointIndex, shotId, start, end, flags,
                    speed, Math.AbsFloat(length), thickness);
                laserFX.m_sSoundProject = savedProject;
                laserFX.m_sFireSoundEvent = savedEvent;
                return;
            }
        }

        if (length >= 0)
        {
            super.PlayShotLocal(hardpointIndex, shotId, start, end, flags,
                speed, length, thickness);
            return;
        }

        BFA_MechWeaponModuleComponent module = FindLocalModule(hardpointIndex);
        if (!IsBossKineticCannon(module))
        {
            super.PlayShotLocal(hardpointIndex, shotId, start, end, flags,
                speed, Math.AbsFloat(length), thickness);
            return;
        }

        BFA_MechWeaponFX fx = module.GetFX();
        if (!fx)
        {
            super.PlayShotLocal(hardpointIndex, shotId, start, end, flags,
                speed, Math.AbsFloat(length), thickness);
            return;
        }

        // Every cannon round now gets a visible travelling shell.
        // Only shot 4/8/12/... uses the brighter amber tracer prefab.
        // Do not turn non-tracers into invisible shots.
        ResourceName savedProjectile = fx.m_sProjectilePrefab;
        ResourceName savedTrail = fx.m_sTrailParticle;
        fx.m_sProjectilePrefab = "{EA61110000000002}Prefabs/Vehicles/Mech/Weapons/Boss/Warlord_CannonShell.et";
        fx.m_sTrailParticle = "";
        super.PlayShotLocal(hardpointIndex, shotId, start, end, flags,
            speed, Math.AbsFloat(length), thickness);
        fx.m_sProjectilePrefab = savedProjectile;
        fx.m_sTrailParticle = savedTrail;
    }

    override protected BFA_MechWeaponModuleComponent FindLocalModule(int hardpointIndex)
    {
        if (hardpointIndex >= RW_WarlordAuxiliaryBatteryComponent.FIRST_VIRTUAL_HARDPOINT)
        {
            RW_WarlordAuxiliaryBatteryComponent battery = RW_WarlordAuxiliaryBatteryComponent.Cast(
                GetOwner().FindComponent(RW_WarlordAuxiliaryBatteryComponent)
            );
            if (battery)
            {
                BFA_MechWeaponModuleComponent top = battery.GetTopCannonModuleForVirtualIndex(hardpointIndex);
                if (top)
                    return top;
                return battery.GetModuleForVirtualIndex(hardpointIndex);
            }
        }

        return super.FindLocalModule(hardpointIndex);
    }
}
