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

    [Attribute("Torso", UIWidgets.EditBox, "Upper body slot whose yaw the battery follows.", category: "Auxiliary Battery")]
    protected string m_sParentSlot;

    [Attribute("", UIWidgets.Object, "Autonomous defense modules from the retired 128-player configuration.", category: "Auxiliary Battery")]
    protected ref array<ref RW_WarlordAuxiliaryWeaponDefinition> m_aWeapons;

    protected ref array<IEntity> m_aEntities = {};
    protected ref array<BFA_MechWeaponModuleComponent> m_aLiveModules = {};
    protected bool m_bBuilt;
    protected int m_iResolveAttempts;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().CallLater(BuildBattery, 150, false);
    }

    override void OnDelete(IEntity owner)
    {
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(BuildBattery);

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

            torso.AddChild(gun);

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
    override protected BFA_MechWeaponModuleComponent FindLocalModule(int hardpointIndex)
    {
        if (hardpointIndex >= RW_WarlordAuxiliaryBatteryComponent.FIRST_VIRTUAL_HARDPOINT)
        {
            RW_WarlordAuxiliaryBatteryComponent battery = RW_WarlordAuxiliaryBatteryComponent.Cast(
                GetOwner().FindComponent(RW_WarlordAuxiliaryBatteryComponent)
            );
            if (battery)
                return battery.GetModuleForVirtualIndex(hardpointIndex);
        }

        return super.FindLocalModule(hardpointIndex);
    }
}
