# Warlord OPFOR — warhorn from my recorded video

I supplied a complete 6.6-second warhorn video, and the audio was extracted as
**Warlord_Warhorn.wav**, 44.1 kHz mono PCM.

## Important: the WAV is NOT committed yet
The GitHub connector supports text file updates but did not upload the binary WAV.
I must add the WAV to this exact branch before merging it:

`Sounds/Warlord/Warlord_Warhorn.wav`

The matching `.wav.meta` and `.acp` files are already committed on this branch.
The `.acp` event is **SOUND_WARLORD_HORN**.

## My GitHub Desktop steps
1. Switch to `feat/warlord-user-recorded-warhorn-20261111`.
2. Extract `Warlord_Warhorn_Import_Package.zip` into my local MW addon folder.
3. Check that the WAV exists at `Sounds/Warlord/Warlord_Warhorn.wav`.
4. In GitHub Desktop, commit that new WAV to the **same branch** and push.
5. Reload my Workbench addon, check script/audio compilation, spawn a new boss and listen.
6. If everything works, merge the PR. Do not merge the PR before step 4.

## Planned in-game timing
- First blast: random **35–95 seconds** after spawning.
- Subsequent blasts: random **120–310 seconds** apart.
- 25% of repeat intervals get an additional **60–155 seconds** of silence.
- Sound originates roughly **20 metres above the Warlord's root**.
- Scheduling is server-only; clients play the same 3D sound through one broadcast RPC.
- Horn stops scheduling when the Warlord is destroyed.
- The original recording plays in full; no pitch alteration or edited sample.

For a fast Workbench check, temporarily adjust the boss component's
First Delay Min/Max to `3` / `5`, then restore `35` / `95`.

## Files and scope
- `Scripts/Game/RW_Boss/RW_WarlordWarhornComponent.c`
- `Prefabs/Vehicles/Mech/Enemies/Assault/Titan_WarLord_OPFOR.et` (one component added)
- `Sounds/Warlord/Warlord_Warhorn.acp`
- `Sounds/Warlord/Warlord_Warhorn.acp.meta`
- `Sounds/Warlord/Warlord_Warhorn.wav.meta`
- **Add locally:** `Sounds/Warlord/Warlord_Warhorn.wav`

No movement, shields, weapons, cannon mounts, damage, point defense, meshes or scale changed.
