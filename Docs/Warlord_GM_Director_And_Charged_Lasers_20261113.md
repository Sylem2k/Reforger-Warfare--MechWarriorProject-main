# Warlord OPFOR — Expanded GM Director, Laser Charge and Impact Audio (Nov 13, 2026)

## Scope

Single canonical boss: `{D60000000000000F}Prefabs/Vehicles/Mech/Enemies/Assault/Titan_WarLord_OPFOR.et`.

This branch **does not** change meshes, transforms, scales, hardpoints, damage values, shields, weapon range, or shared Mech Core scripts. Existing warhorn recording and first/repeat timer values stay in place.

## GM entity editor (12 options)

The custom Warlord attribute list is loaded via a **script hook** on `SCR_AttributesManagerEditorComponent` on both client and server, using `InsertAllAttributes` before vanilla initialization. This avoids a shared `Edit.conf` or `EditorModeEdit.et` override. The custom attribute implementations return null when the **selected editable entity** is not the Warlord.

1. **Warlord Weapons**: All Weapons, Missiles Only, Lasers Only, Cannons Only, Hold Fire (unchanged).
2. **Warlord Inaccuracy**: AI aim scatter x1–x5 (unchanged).
3. **Warlord Movement**: Patrol, Stop / Hold Here, GM Waypoints Only (unchanged).
4. **Waypoint Arrival (m)**: GM waypoint stop radius (unchanged).
5. **Vehicles Only**: Prevent the main AI banks from targeting dismounted infantry; Omni guns already exclude infantry by default.
6. **Top Siege Cannons**: Enable/disable two independent top cannon firing routines.
7. **Defensive Systems**: Enable/disable automatic point-defense and seismic stomp in **All Weapons** mode.
8. **Warhorn Enabled**: Temporarily mute scheduled horn cues without killing the scheduler.
9. **Laser Charge Enabled**: Add three-stage charge-up before the twin DRIVER arm-laser salvo and each Omni laser burst.
10. **Laser Charge (s)**: 0.75–3 seconds. The original charge sample lasts **1.25 s**, so that is the synchronized recommended value.
11. **Volley Pause x**: 0.5–4x the existing staggered AI bank pause. This affects the boss's staggered bank scheduler; not individual weapon damage or rate of fire.
12. **Extra Aim Settle (s)**: 0–8 additional seconds before a staggered AI station begins attacking a target.

## Sound timeline

**0.0 s**: reliable Warlord charge sound (three rising hum pulses).

**1.25 s** (default): server authorizes initial laser shot; the regular replicated shot path displays beams and plays the heavy laser fire event.

**On hit**: server broadcasts a reliable sound at the actual impact world coordinate; each nearby client plays its 3D event. The new single audio project has:

- `SOUND_WARLORD_LASER_CHARGE` — original 1.25 s `Warlord_LaserCharge_123.wav`.
- `SOUND_WARLORD_LASER_FIRE` — original existing `las_cannon.wav` recording, now assigned to Warlord laser shot FX.
- `SOUND_WARLORD_IMPACT_ENERGY` — existing `Las_Impact_Hard_01.wav`.
- `SOUND_WARLORD_IMPACT_KINETIC` — existing `Las_Impact_Hard_02.wav`.
- `SOUND_WARLORD_IMPACT_EXPLOSIVE` — new `Warlord_SiegeImpact.wav` original heavy detonation.

Source resources: `{A6111111000AA012}Sounds/Warlord/Warlord_Combat.acp` and both `.wav` files with their `.meta` files.

The audio project's falloff extends out to approximately **1,400 m**; audibility still depends on Workbench audio import, geometry, user volume, position, and the engine mixer.

### Impact coverage

- **Arm Las Cannons / AC cannons / top cannon rounds**: actual hit or terrain endpoint (no false sky-miss booms), with delay for physical rounds.
- **Autonomous Omni lasers**: instantaneous impact from server-side beam detonation.
- **Autonomous Omni kinetic + AA/flak**: actual clipped detonation.
- **LRM / rocket projectiles**: core rocket impact broadcast is extended by boss router to deliver an explosive impact cue at the authoritative detonation location.
- Impact cues are rate-limited per category (100–180 ms minimum) to protect 128-player sessions.

## Required test: script registration and UI

1. **Compile Scripts** in Workbench and check all sound `.acp` and `.wav` assets import; no compile/runtime validation has occurred in this branch.
2. Reload game and dedicated server with the **same mod build**, spawn a **fresh** OPFOR Warlord.
3. In GM, select the Warlord and enter **Edit Entity / Attributes**. The heading should correspond to the selected Warlord, **not Editing: Scenario properties**. See the four old and eight new rows. A non-Warlord must not show these controls.
4. Set `Vehicles Only` on/off and `Warhorn Enabled` on/off; check the **server** for `[RW WARLORD GM] vehiclesOnly=` and `[RW WARLORD GM] warhorn=`. If the panel shows rows but no server logs after Save, the values are not reaching the Warlord and must be diagnosed before gameplay testing.
5. Verify **Set Speed** and other native vehicle editing options remain intact.
6. Test Hold Fire, Top Siege Cannons, Defensive Systems, Volley Pause x and Extra Aim Settle individually, then restore to defaults.

## Required test: dedicated-client audio

Run a nearby client **within 100 m** of a fresh boss. Save **both server and that client's logs**.

Expected server:
```
[RW WARLORD LASER][SERVER CHARGE SEND]
[RW WARLORD LASER][DRIVER CHARGE START]
[RW WARLORD LASER][DRIVER FIRE AFTER CHARGE]
[RW WARLORD IMPACT][SERVER SEND]
```

Expected client:
```
[RW WARLORD LASER][CLIENT CHARGE RECEIVED]
[RW WARLORD AUDIO][CLIENT] event=SOUND_WARLORD_LASER_CHARGE valid=1 audible=...
[RW WARLORD IMPACT][CLIENT RECEIVED]
[RW WARLORD AUDIO][CLIENT] event=SOUND_WARLORD_IMPACT_ENERGY valid=1 audible=...
```

Other impact events are `SOUND_WARLORD_IMPACT_KINETIC` and `SOUND_WARLORD_IMPACT_EXPLOSIVE`. If server SEND exists but CLIENT RECEIVED is absent, investigate RPC receiver/relevance for the Warlord root router. If client receives but `valid=0`, fix local audio import/packaging. If `audible` is negative, inspect distance, mixer routing, and acoustic setup. **PLAY requests and valid resources alone do not prove the sound is audible.**

## Regression tests

- Verify current Warlord GM selection and waypoint behavior.
- With laser charge **disabled**, confirm immediate legacy firing.
- With laser charge **enabled**, confirm no main gun fire until the charge completes; after release/stop, no delayed shot should escape.
- Confirm that an Omni laser charges once before its first shot, then fires shots 2 and 3 at its original configured cadence.
- Test top cannons, lasers, LRMs and Omni flak against vehicle, building/terrain, and sky-miss endpoints.
- Confirm no script errors, duplicate effects, unexpected 128-player networking spikes, model displacement, or new dependency on CGS-Auto-Turrets.

**Status:** Source and resource static-reviewed. **Not compiled in Workbench and not yet tested on a dedicated server**.
