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
    // Count only physical cannon shells. The server puts a visual-only flag in
    // the unused tracer-length field of the EXISTING shot RPC. Thus every client
    // agrees on exactly which shell is a tracer, even with packet loss.
    // No extra RPC, network entity, damage, or shared Mech Core change.
    protected ref array<int> m_aCannonShotCounts = {};

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

        return super.ServerBroadcastModuleShot(module, start, end, hit, speed,
            length, thickness, serverImpact);
    }

    override protected void PlayShotLocal(
        int hardpointIndex, int shotId, vector start, vector end,
        int flags, float speed, float length, float thickness)
    {
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
