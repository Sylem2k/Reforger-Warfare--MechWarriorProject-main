REFORGER WARFARE - TEST CHASSIS (MW Project)
============================================
These are independent test-prefabs cloned from Med_Mech_Frame_B.
The original Med_Mech_Frame_B and Med_Mech_Base prefabs are preserved.
Each new chassis has unique entity, component, nested-resource, and .et.meta GUIDs.
The medium-body MeshObject is TEMPORARY: replace it with your own mesh later.
New weapon mounts have PointInfo offsets but are not yet animated rig/bone mounts.

Mech Bay roster:
Cricket (Light)             existing Light test prefab, renamed in UI
Warden (Medium)             existing medium test prefab
Cricket (Light - Alt)       existing light Frame B, unchanged loadout
Warden (Medium - Alt)       existing medium Frame B, unchanged loadout
Heavy Judiciar - Gun        new
Heavy Judiciar - Missile    new
Assault Executioner - Gun   new
Assault Executioner - Missile new

NAME                           MAX  EMPTY  PAYLOAD  HP  ARM/GUN  BACK/MISSILE  SUPPORT
Heavy Judiciar - Gun            80    49      31     8      5           2         1
Heavy Judiciar - Missile        80    47      33     8      2           5         1
Assault Executioner - Gun      100    60      40    11      7           3         1
Assault Executioner - Missile  100    56      44    12      3           8         1

Core hardpoint classes: ARM=1, SHOULDER=2, BACK=3.
Weapon group 1: arms, torso guns; group 2: shoulder; group 3: back missile racks.
The existing single pilot/group mask (7) remains compatible.
The RW_FrameHardpoint rules explicitly restrict added mounts to BALLISTIC,
ENERGY or MISSILE and allow items up to LARGE.
Existing weapon IDs (including 100..119) are unchanged.

Notes for modeling:
- The PointInfo Offset positions are rough test coordinates based on the
  temporary medium model. Reposition them for each completed 3D model.
- New mounts have no m_sAimMountId or attach bone assigned. They attach to
  the mech root and won't automatically follow animated gun turrets/arms.
- The original two arm aim mounts still use the medium frame's mount IDs.
- Edit the new prefab's MeshObject once your own body model is ready.
- Adjust collision, movement, armor and hit zones to suit the final models.
- No Core changes are required; all four use fewer than 18 hardpoints.
- TESTING NEEDED: Resource load, spawn, mounting, saved builds, server
  replication, part spawn offsets and piloted firing (not yet verified).
