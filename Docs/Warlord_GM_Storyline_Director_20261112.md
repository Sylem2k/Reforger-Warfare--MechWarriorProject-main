# Warlord — GM Storyline Director (test build)

I reused the **CGS-Auto-Turrets** Game Master edit-entity pattern in my
MechWarrior gameplay addon. I did not add the turret addon as a runtime dependency.
The Warlord already has `SCR_EditableVehicleComponent`; the new edit attributes
only appear when that entity has `RW_WarlordGMDirectorComponent`.

## Edit Warlord in Game Master

1. Spawn **OPFOR Warlord - Siege Overlord** in GM.
2. Open its **Edit Entity / Attributes** screen (same flow as the auto turret).
3. Find category **Warlord Storyline**:
   - **Warlord Weapons**: All Weapons, Missiles Only, Lasers Only,
     Cannons Only, Hold Fire.
   - **Warlord Inaccuracy** slider, 1.0-5.0: 1 = original;
     2-3 = much less accurate. The missile/rack AI receives off-centre aim and
     autonomous AA/Omni turret spread grows. No damage values are changed.
   - **Warlord Movement**: Autonomous Patrol, Stop / Hold Here,
     GM Waypoints Only.
   - **Waypoint Arrival (m)** slider: stop radius from 3 to 35 metres.
4. Confirm the changes. I should see `[RW WARLORD GM]` lines in the
   **server** log. The values use RplProp replication for GM client display.

## Give a destination on the map

The existing **CGS Mech Brain** already supports standard GM waypoints through
its hidden driver's `SCR_AIGroup`. This boss originally left
`m_sPilotGroupPrefab` blank, so the hidden pilot had no controllable AI group.
My test prefab now spawns the base AI group with the hidden pilot.

1. In GM, locate/select the **Warlord's hidden-pilot AI group** in the AI groups.
   This depends on the group being visible/commandable in the mission's GM UI;
   this part must be verified in Workbench and on the dedicated server.
2. Give it a standard **Move** waypoint at the desired map position.
3. Set Warlord Movement to **GM Waypoints Only** so it does NOT choose its
   own random patrol destinations after the waypoint.
4. Set **Stop / Hold Here** to halt its driver inputs at any time.
5. Use **Autonomous Patrol** to let it wander again. With an active GM
   waypoint, the existing Core brain gives the group waypoint priority.

The boss never uses sideways strafing. Game Master movement orders drive the
existing IK walker controller, not a new teleport / drag / physics system.
The turret and weapon AI remain independent of movement orders.

## Weapon profiles

- **All Weapons**: untouched behaviour.
- **Missiles Only**: BOSS_LRM virtual stations (LRM-10 and LRM-15) fire;
  AC virtual stations, DRIVER arm lasers, autonomous AA/Omni,
  top siege cannons, point defence and seismic stomp stop.
- **Lasers Only**: DRIVER group-1 arm Las Cannons and autonomous Omni
  lasers fire; the target designator remains aim-only.
- **Cannons Only**: AC virtual stations, auto Omni kinetic / AA flak
  and top siege cannons fire; missile racks and lasers stop.
- **Hold Fire**: all the above offensive and automatic defensive modules
  stop new firing. An already-launched missile in flight is not deleted;
  the engine completes its existing projectile.

This profile is implemented at each Warlord-specific weapon control path,
not by globally turning off the shared Mech Core hardpoint system.

## Files changed

- `Scripts/Game/RW_Boss/RW_WarlordGMDirectorComponent.c`
- `Scripts/Game/RW_Boss/RW_WarlordGMEditorAttributes.c`
- `Scripts/Game/RW_Boss/RW_WarlordGMDefenseModules.c`
- `Scripts/Game/RW_Boss/RW_WarlordMultiTargetAICrewComponent.c`
- `Scripts/Game/RW_Boss/RW_WarlordCoverAwareFlak.c`
- `Scripts/Game/RW_Boss/RW_WarlordAuxiliaryBatteryComponent.c`
- `Prefabs/Vehicles/Mech/Enemies/Assault/Titan_WarLord_OPFOR.et`
- `Prefabs/Vehicles/Mech/Enemies/Assault/Warlord_Point_Defense.et`
- `Prefabs/Vehicles/Mech/Enemies/Assault/Warlord_Stomp_Pulse.et`
- `Configs/Editor/AttributeLists/Edit.conf`
- `Configs/Editor/AttributeCategories/RW_Warlord.conf` and metadata

## Required tests before merge

- Compile Scripts and check resource loading of the new Editor attribute list,
  category GUID and Enfusion class registration.
- Edit a **live** Warlord in GM. Confirm all four controls are visible and
  changing each gives a server log line and persists on a connected client.
- Test Missiles Only against a vehicle and verify independent cannons, lasers,
  AA, stomp, and point defense do not fire. Switch back to All Weapons.
- Test inaccuracy at 1 versus 3 against a stationary target; check effects on
  missiles, main Las Cannons and autonomous batteries.
- Test movement Hold, Patrol, and **GM Move waypoint** via hidden-pilot AI group.
- Ensure 128-player server works, no log spam, no changes to weapon damage,
  sound resources, model or existing leg socket transforms.
- If GM Move waypoint does not appear/select, inspect the visibility/permissions
  for the new AI group. Do not assume the UI is working merely because the
  group was spawned.
- **Compatibility caution:** both this addon and CGS-Auto-Turrets declare
  `Configs/Editor/AttributeLists/Edit.conf`. If both are mounted at the same
  time, Workbench may report a resource override/collision. Verify whether the
  loaded GM AttributeList actually includes *both* sets of custom entries
  before merging. A consolidated edit attribute list may be required.

This code has NOT been compiled in Workbench or dedicated-server-tested.
