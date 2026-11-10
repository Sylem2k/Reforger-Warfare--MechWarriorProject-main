# WARLORD OPFOR — CURRENT PROJECT HANDOFF / START HERE
**Last prepared:** November 10, 2026
**Purpose:** Paste this GitHub document into a new ChatGPT conversation if the current conversation reaches its limit. This is a project-status handoff, **not** a request to alter meshes or prefabs.

> **IMMEDIATE STOP / OWNER INSTRUCTION (highest priority):** I have resized my Warlord XOB models locally and repositioned everything in Workbench, but **I HAVE NOT PUSHED THOSE NEW LOCAL CHANGES TO GITHUB YET**. The GitHub copy is temporarily **out of date**. **DO NOT change any vehicle scale, torso/hips/arms/legs offsets, hardpoint placement, torso pivot, model files, mesh references, prefab settings, or other Workbench positions until I push my local work and explicitly approve a change.** Only inspect/compare after my push.

## 1. My immediate next task
I will get home, push the latest **Warlord model XOB changes and my Workbench prefab adjustments**, and then ask for a **read-only comparison**. At that time:
1. Fetch the newest main-branch commits in **all three repositories**, not the old pinned snapshot below.
2. Confirm that the Warlord torso XOB uses the **same modeling-unit / scale basis** as my newly resized Warlord body XOB. This means **matching scale**, NOT making the torso physically as large as the whole body.
3. Confirm my updated prefab uses **vehicle scale = 1.0**, if that is what I saved locally; do not reset it to 2.0 just because old GitHub has `scale 2`.
4. Confirm the correct updated torso-only mesh is referenced in the torso prefab, and the existing hips/legs/arms are still in the positions I authored.
5. **Report discrepancies, do not silently fix them.** Ask before writing any model/transform/scale changes.
6. Confirm the correct single Warlord OPFOR prefab spawns rather than another smaller Titan. Compare spawn resource GUID, editable UI name, and actual MeshObject resource.

### Current unpushed local changes (user-reported; not verified via GitHub)
- I enlarged my source/model **XOB** to twice the former size, so the **vehicle prefab's scale can be 1 instead of 2**.
- I readjusted **all** parts/placements in local Workbench to suit the revised model and said everything was set back up.
- These changes may **not** be present on GitHub. No assistant may assume the old repository offsets represent the new work.
- I specifically asked: **make sure the torso equals the newly enlarged XOB's scale**. Compare first; do **not** adjust anything yet.
- I have seen an older/smaller Titan when spawning from Game Master, and previously saw an arm positioned far away. Whether these remain after my local adjustments is **unverified**.

## 2. Exactly ONE Warlord boss — never recreate the 128 test
**Authoritative boss prefab:**
`{D60000000000000F}Prefabs/Vehicles/Mech/Enemies/Assault/Titan_WarLord_OPFOR.et`

- The separate `Titan_WarLord_BOSS_128_Test.et` and `.et.meta` were **deleted** in PR #117. Do not reintroduce them.
- The working OPFOR boss originally had the complete arms/weapons and already **fired correctly**. The stripped 128 test prefab was not supposed to survive as a separate boss.
- Features intended for a 128-player fight were moved into the single OPFOR boss through the merged changes below.
- OPFOR boss is an **autonomous world boss** with many independently controlled guns, **not** the standard player-piloted mech. Keep the main combat system intact.
- GM spawn display name after registration fix: **OPFOR Warlord - Siege Overlord**.

## 3. Repositories, addon IDs, and dependency boundaries
| Repo | Purpose | Addon project ID | Addon GUID |
|---|---|---|---|
| https://github.com/Sylem2k/Mech-Core-CGS | Core Enfusion movement, aim, weapons, damage, repair, AI, HUD, torso twist | `MechCoreCGS` | `6A8C4B3A46C28E1B` |
| https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main | Actual Warlord/other mech prefabs, gameplay scripts, Mech Bay, catalog, GM registration | `ReforgerWarfareMWProject` | `6AA6E156BC274F13` |
| https://github.com/Sylem2k/CGS-MW-Assets | XOB/FBX/TXO meshes and textures, including Warlord models | `CGSMWAssets` | `6AAA31A473996E48` |

Do **not** use the old duplicate `CGS-Mech-Core` remote in place of the official `Mech-Core-CGS`.

### Snapshot observed on GitHub on November 10 (WILL BECOME STALE after my push)
- MW main commit: `127f78eda16717817477dae46f31708e5f8a9deb`
- Core main commit: `6ad41822af3a9e51b0303b43f28efab0b5628746`
- Assets main commit: `eeae5c7ef3190fe914935b1beb122e6906453803`

Treat the snapshot as a diagnostic reference, not a claim that I have pushed my current local Workbench changes.

## 4. Merged GitHub change history (checked Nov 10)
All five PRs below were **merged** into MW main (the earlier chat may have called some of them 'draft' before later merges):
1. **PR #117:** https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main/pull/117 — delete stripped BOSS_128_Test prefab and use OPFOR only; correct old registry path.
2. **PR #118:** https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main/pull/118 — split torso/hips, migrate 128-player boss statistics, module catalog, 12 autonomous weapons.
3. **PR #119:** https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main/pull/119 — Warlord cover/line-of-sight fix, target vehicles first, reduce infantry/crew damage, boost boss hull/shield and repair a suspect torso pivot.
4. **PR #120:** https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main/pull/120 — make autonomous Omni laser impact/damage immediate with beam, preserving travel time for physical rounds.
5. **PR #121:** https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main/pull/121 — correct Game Master name and register one OPFOR boss; deduplicate registry/catalog entries.

**IMPORTANT:** Merged code is not the same as a confirmed in-game solution. Check Workbench script compilation, target behavior and server/client tests rather than assuming every PR worked perfectly.

## 5. Last inspected prefab and mesh state (GITHUB ONLY, not my pending local work)
At the old MW GitHub main snapshot:
- `Titan_WarLord_OPFOR.et` ends with **`scale 2`**. This is expected to change to **1** in my pending local version; **do not edit the old copy to force this**.
- Vehicle root MeshObject and the currently committed `Torso_Warlord_Camo.et` both reference `{528115F7F8BA3093}Common/Models/Titan_War_Lord/War_Lord_Body.xob`. That is a **full-body model reference**. Check after the new XOB push whether the torso now uses a separate, correct torso-only mesh. **Do not swap resources before I push.**
- `Torso` slot uses `{C628000000020106}Prefabs/Vehicles/Mech/Mech_Parts/WarLord/Torso_Warlord_Camo.et`.
- `Hips` slot uses `{FE37C2C97CDEB011}Prefabs/Vehicles/Mech/Mech_Parts/Boss/Warlord_Boss_Hips_.et`, with `War_Lord_Hips.xob`.
- Left and right boss cannon arm slots use `Warlord_DamageTest_Arm_Left.et` and `Warlord_DamageTest_Arm_Right.et`.
- `CGS_TorsoTwistComponent.m_bHideRootMesh=1` hides root body visuals while the `Torso` slot renders.
- An older PR restored torso PointInfo to `Offset -0.217 12.339 7.567`, but **that may be obsolete after my local model adjustments**. Do not touch the pivot until I have pushed and we have compared.
- Some old torso/weapon/arm slot offsets look very large or irregular; they are authored positions, NOT automatic proof of a defect once the locally rescaled meshes are imported.
- Last checked GM registry path: `Configs/Editor/PlaceableEntities/Vehicles/EnemyMechs.conf` contains only GUID `D60000000000000F`; display name is Warlord.

**Scale principle:** A mesh XOB's vertex coordinates, its import scale, the child slot transform, and the vehicle root's `scale` multiply into the final world size. If a model was made 2x larger in Blender/mesh export, don't apply another root 2x on top. **But don't assume the torso or child meshes were resized just because the body XOB was.** Compare their exported scale basis and visible match after the push.

## 6. Warlord mechanics and balance presently in GitHub
These were committed for the large 128-player event, but gameplay confirmation is pending:
- Boss hull: **600,000 HP**; shield **150,000 HP**, regen **600 HP/s** (with phase restores at **75/50/25%** hull); damage caps intended to slow TOS-1 and M777 from instantly ending the battle.
- Boss has **18 regular replicated hardpoints** (Mech Core max is 18; module IDs 1..1023) plus a Warlord-only **12 autonomous auxiliary Omni/AA weapons**, represented by local virtual indices 18..29 rather than expanding the shared hardpoint serializer.
- Heavy weapon banks should **prioritize hostile vehicles**; 12 auxiliary Omni weapons are vehicle-only by default. Existing main gunner stations may engage infantry when no vehicles are visible.
- Boss-specific weapons reduce infantry damage and avoid extra crew injury while shooting tanks (crew-share kinetic/explosive 0 in the relevant weapon prefabs).
- Anti-building-cover fix is in `Scripts/Game/RW_Boss/RW_WarlordCoverAwareFlak.c` and `RW_WarlordOmniTurret.c`: visible-target checks, impact clipping at walls, and damage-line checks. Inspect actual collision if a map building still fails to block fire.
- Omni laser beam and authoritative impact now intended to occur together; cannon and flak keep projectile flight delay.
- Boss arms should **remain at their original authored angle relative to the torso**, following torso left/right yaw, with **independent yaw locked to 0** and independent pitch around **-20..+40 degrees**. The torso twists at the waist while the lower hips/legs stay put.
- These values are event tuning and may need work after test fights. Don't undo the user's local placements in order to tweak damage.

## 7. What was observed vs what is still unverified
### Observations from play and user messages
- The original OPFOR Warlord **fired its guns**, unlike the old stripped 128 prefab.
- Boss at one point fired **through buildings**; target LOS changes were merged but final in-game validation is not recorded in this conversation.
- Boss weapons at one point **de-crewed tanks** more often than destroying them, were **too lethal to infantry**, and it was **too vulnerable to TOS-1 / M777**; balance fixes were merged, but final validation is not recorded.
- Arms appeared detached/far out during torso movement; a pivot edit was merged, but **the user has since re-authored local model positions**.
- Omni laser impact explosion appeared **later than the beam**; timing fix was merged; no definitive subsequent Workbench test result here.
- Game Master showed a smaller/older Titan and the OPFOR entry was hard to find; GM registry correction was merged. It is **not yet established** whether the wrong entity actually spawned or the Warlord's old mesh/scale was simply showing.
- Most recently, the user said the updated model and vehicle placements were corrected locally, but **they still need to push them home-to-GitHub**.

### Pending tests when the user resumes
1. **Before changing anything:** verify newly pushed commit SHAs, file names, XOB resource meta GUIDs, model import sizes, prefab `scale 1`, correct torso mesh reference, and old/new body dimensions.
2. Compare body/torso/hips/legs/arms model coordinate basis against the authored local slots. Ensure the **torso visual and collision match the revised model**, not the outdated full-body visual.
3. Spawn **fresh** `D60000000000000F` from GM and confirm correct appearance. Check the selected editor entity's resource reference if a small Titan appears again.
4. At neutral and ±yaw, verify waist does not orbit, arms stay attached to the upper body and can pitch only, while hips/legs don't follow the twist.
5. Compile/reload all mods. Validate 18 core modules and 12 auxiliaries, no new resource GUID or script errors.
6. On a dedicated server with a remote client, verify tank-first targeting, vehicle hull damage vs crew loss, cover LOS, and laser beam/impact sync.
7. Bombard the Warlord with TOS-1 and M777 to tune the length of a **fun** 128-player boss encounter (not instant death for infantry, not instant boss destruction).
8. Record actual logs and results before updating statuses to "working".

## 8. Other Mech Core project context
- The official Core provides walker, tracked, hover movement; cockpit-only Start/Stop engine controls; pilot HUD, weapon groups, heat, torso twist, hit zones, AI companion brain, mech shield, and repair targets.
- **Max 18 hardpoints**, **8 stations**, **10-bit module IDs 1..1023**.
- Mech Bay, Mech Lab, gear catalog, saved builds and wrench tool actions live in the MW Gameplay repo.
- The current Core script has server-authoritative part repair methods. MW has real track and arm wrench repair action classes and Mech Bay service/crew repair, but actual behavior depends on the prefab's configured repair sites and ActionsManager contexts; not every mech automatically has repairs.
- Controls reference: `Mech-Core-CGS/Configs/System/README_MECH_CONTROLS.txt`.
- Broader historical handoff: `Mech-Core-CGS/Docs/CGS_MechCore_Continuous_Handoff.md` and its MW counterpart. Those logs contain dated test experiments and must not override my newer instructions above.
- A separate **14-page PDF** Mech Core README/User Guide was supplied in the earlier conversation on November 10, 2026. It is an orientation guide, not a substitute for this current pending-local-changes handoff.

## 9. Workflow rules for a future assistant
- Read this handoff **first**; then compare GitHub with the latest user-pushed local changes.
- **Do not change anything** until I explicitly request the fix. I have already adjusted all Warlord visuals on my local PC.
- Prefer **full updated scripts/files** using their **original filenames** when I request code changes.
- For actual code changes, create an isolated feature branch/PR for review, compile/test in Workbench first, and don't claim unverified fixes as working.
- Never create or suggest a second Warlord 128 prefab; improve **OPFOR only**.
- Write development notes **in first person as if I wrote them**.
- Respect existing weapon placement, damage setup, working boss AI and 128-player server performance.
- If the chat hits context limit, give the next assistant this GitHub path:
  `Docs/WARLORD_OPFOR_CURRENT_HANDOFF_START_HERE_2026-11-10.md`

## 10. First message I can paste into a new chat
> Continue my Arma Reforger **Warlord OPFOR** world-boss project. Read `Docs/WARLORD_OPFOR_CURRENT_HANDOFF_START_HERE_2026-11-10.md` in `Sylem2k/Reforger-Warfare--MechWarriorProject-main` first. We have ONE boss, GUID `D60000000000000F`. I have enlarged my XOB meshes, made local Workbench corrections, and will push them to GitHub. **Do not make any prefab, scale, pivot, arm or mesh adjustments until you compare the latest push and ask first.** Verify my torso mesh uses the same scale as the resized Warlord XOB, and that the root prefab is at scale 1. After that, we can validate the boss visuals and combat.
