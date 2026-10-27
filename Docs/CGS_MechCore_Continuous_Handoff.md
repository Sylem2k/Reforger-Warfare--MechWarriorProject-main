# CGS MECH CORE / MW PROJECT — CONTINUOUS HANDOFF
Updated: 2026-10-27. This is my living first-person development log. I update this EXACT file after every meaningful change, including commits, attempted fixes, compile results, in-game tests, blockers and next steps. I do not replace it with another dated handoff.

## Repositories and rules
- Official published Core: https://github.com/Sylem2k/Mech-Core-CGS
- MW project (prefabs, frame setup, Mech Bay and content): https://github.com/Sylem2k/Reforger-Warfare--MechWarriorProject-main
- Assets: https://github.com/Sylem2k/CGS-MW-Assets
- Duplicate legacy Core: https://github.com/Sylem2k/CGS-Mech-Core — do not use for new development or confuse with published Core. Both contain the same addon GUID.
- I work on two PCs. My Workbench addons directory on one PC is C:\Users\Dylan\OneDrive\Documents\My Games\ArmaReforgerWorkbench\addons. My older local Core folder there is called CGS-Mech-Core and is connected to the DUPLICATE remote; do not assume it pulls official Core changes.
- I require fresh isolated feature branches, no unapproved main merges, file names unchanged for existing scripts, and full updated source rather than snippets. Changes only count as tested after I compile and test them in Workbench.
- Preserve previously working walker/tracked/hover movement, engine Start/Stop, cockpit buttons, weapon switching, and existing HUD.
- Keep this file synchronized in Core AND MW after every update. Record branch, commit, exact paths touched, test results, and outstanding steps. If a feature touches only one repository, still update both handoffs so either one can resume development.

## Current working and incomplete systems
- Engine starts OFF; pilot-only Start/Stop actions request authoritative replicated engine changes through SCR_PlayerController. An engine-running interlock blocks movement when engine OFF or STARTING. Earlier gameplay test confirmed controls and interlock worked, but subsequent branch combinations caused invisible buttons.
- Cockpit action fix uses HasLocalEffectOnlyScript() for both actions and larger, reachable action contexts. Main branches have these changes; user still needs to retest combined current builds.
- Walker, tracked, hover locomotion previously worked. Track and hover part prefabs now have BFA_MechPartHitZone configurations (700 HP per element); existing CGS_MechPartsPanel maps their SlotManager slots to left/right leg HUD. Compilation and game testing pending.
- CGS_MechRepairComponent is a READ-ONLY registry resolving named SlotManager attachments and installed hardpoint modules. It does NOT create actions, inflict damage or heal. PointInfo alone is not a usable action.
- Wrench in MW currently handles refitting and inventory tool verification; it does NOT yet heal parts. Need actual server-authoritative wrench repair action, development-only damage action, health API verification and track damage effects.
- MW Mech Bay owns loadout installation; SlotManager owns structural assemblies. Repair should not automatically spawn weapons or restore removed modules.
- No complete test of track HP -> degraded motion -> destroy -> wrench repair -> restored motion exists yet. Track/hover damage performance penalties and weapon HP% HUD are incomplete.
- Current prefabs have provisioned track service positions, not confirmed reachable by gameplay. The test must wire real repair actions to these points.
- Tracked/hover drive audio remains unresolved separately. Do not overwrite movement to fix audio.

## Latest code state (2026-10-27)
- Official Core main last checked: 61337309e1a43f16f54136fbbfceab9b2a54b393
- MW main last checked: 0912af7dfc33127209d1b0e115318efa04161915
- Assets main last checked: 7f543a8e57df9bc14e7dab825f990c24666344dd
- Core working backup: backup/working-engine-movement-20261027
- MW working backup: backup/working-assault-tracked-engine-20261027
- Core track/hover HUD test: feat/track-hover-hud-20261027
- MW track/hover prefab test: feat/track-hover-damage-hud-20261027
- Core/MW combined engine-button fix: fix/track-test-start-stop-20261027; changes exist on main at time of audit.
- The duplicate CGS-Mech-Core and official Mech-Core-CGS main trees matched in source after synchronization; remaining difference was resourceDatabase.rdb. Duplicate's PR #1 was prepared, not an alternative official source. Avoid wiping local Workbench resourceDatabase.rdb changes.

## Next acceptance test
1. Verify official Core and MW project clones point at the intended remotes; backup work from both PCs before migrating duplicate Core.
2. Compile both current main versions in Workbench and confirm Start and Stop controls are reachable from DRIVER seat, engine transitions OFF/STARTING/RUNNING, and tracked mech moves.
3. Check track and hover parts appear on the existing damage HUD.
4. Implement real external per-slot repair actions for Track_L1/L2/R1/R2 and hover equivalents, with server-side wrench verification and independent part HP. Include safe development-only damage commands/actions.
5. Implement track/hover movement degradation using validated part HP without rewriting working locomotion; test server and clients.
6. Extend later to walker legs, weapon hardpoints, and weapon HP% without disrupting existing HUD.
7. Consolidate shared systems gradually into official Core while keeping MW prefabs/config and Assets separate.

## Change log (append newest changes here, keeping prior entries)
- 2026-10-27: Established the living handoff convention in both Core and MW on independent docs-only feature branches. No gameplay script, prefab or main code changed. This file is the authoritative continuity note until updated again.

## MW-specific notes
MW owns Assault_Gravemarch_Gun_Tracked.et, track/hover part prefabs, existing RW_MechWrenchActions.c and RW_MechBayComponent.c. Current Assault Tracked has four configured track repair PointInfos but no repair user actions. Do not change fixed SlotManager assemblies to implement weapon swapping.
