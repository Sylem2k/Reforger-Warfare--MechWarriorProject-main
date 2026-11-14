# Warlord weapon tracer variants — source-asset handoff

This addition is **boss-only visual presentation**. It does not modify shot timing, damage, client/server RPCs, muzzle coordinates, network ownership, projectile physics, AI, weapon sound or impact sound.

## New visual resources

The four .et prefabs are in `Prefabs/Vehicles/Mech/Weapons/Boss/Tracers/`, with unique .et.meta GUIDs.

| Name | Expected CGS-MW-Assets .xob ID and path | Boss use |
|---|---|---|
| Warlord_Tracer_Big.et | `{3BCBDD5B7EF389EA}Common/Models/Titan_War_Lord/Tracers/Tracer_Big.xob` | `Warlord_Boss_AC10.et` (heavy siege AC/10) |
| Warlord_Tracer_Standard.et | `{2403DB7297B80ED4}Common/Models/Titan_War_Lord/Tracers/Tracer.xob` | `Warlord_Omni_Cannon.et` (autonomous kinetic cannon) |
| Warlord_Tracer_Long.et | `{5F25A72BE82BA8A7}Common/Models/Titan_War_Lord/Tracers/Tracer_Long.xob` | `Warlord_top_Cannon_Right.et` (both top siege gun slots) |
| Warlord_Tracer_Blue.et | `{05FA3236E0D468C7}Common/Models/Titan_War_Lord/Tracers/Tracer_Blue.xob` | Available, *not assigned automatically* |

Each prefab contains only a `GenericEntity` and a `MeshObject`, with neutral `scale 1`. The Mech Core shot renderer spawns the resource locally and moves it using its existing server-authoritative start/end/speed. Adjust **only prefab scale** after visually inspecting the source models' dimensions; do not alter network physics.

## Important: new assets must be pushed before merging

When this integration branch was prepared, none of the four `Tracers/*.xob` files were visible in `CGS-MW-Assets/main`. The model resource paths and GUIDs were supplied directly and are **not yet verified against the source .xob.meta files**.

Before merging:

1. Push the four source `.fbx`, compiled `.xob`, and their `.xob.meta` files into `CGS-MW-Assets/Common/Models/Titan_War_Lord/Tracers/`, or whichever project is providing those resources at runtime.
2. Open each `.xob.meta` and check its `Name` GUID exactly matches the model ID listed above. If Workbench regenerates a GUID, update these four new tracer prefabs to match the final GUID.
3. Run Reforger Workbench import/rebuild. Spawn each tracer `.et` in a test level to verify the mesh is present, its nose points along local +Z (the shot renderer's direction vector), and the model origin sits on the centerline.
4. Load the OPFOR Warlord. Confirm its AC/10, autonomous Omni cannons, and both top siege guns produce visually distinct tracers on a **remote multiplayer client**. Fire in both forward and rotated headings; watch for backwards tracer rotation.
5. Inspect visual scale / thickness / duration at both near and long range; adjust prefabs without changing shot physics. Compare server FPS and client visual entity counts with multi-weapon volleys.
6. Check that the laser/charge sound work from the recent PR is unchanged. Mech Core's **hitscan beam renderer assumes a 0.07 m × 0.45 m lasbolt source**, so `Tracer_Blue.xob` has deliberately **not** been assigned to `m_sBeamPrefab` until its dimensions and pivot are known.

This patch is not Workbench-compiled or multiplayer-tested and must not be merged while the model resources are unresolved.
