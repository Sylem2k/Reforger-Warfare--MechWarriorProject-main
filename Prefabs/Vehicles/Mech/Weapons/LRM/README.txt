LRM pods (Catapult-style back racks) for MechWarrior Project

Two blank prefabs, each one = TWO racks (left + right of the back):
  LRM15_Pair.et   Module Id LRM15_PAIR   2 x 15 tubes = 30 missiles a volley   GUID {FEFE3F482AB3D8B8}
  LRM20_Pair.et   Module Id LRM20_PAIR   2 x 20 tubes = 40 missiles a volley   GUID {8AB285438FD079E6}
Both are in Configs/RW/RW_Equipment.conf as MISSILE, LARGE (9 t and 12 t).

GUIDANCE (homing + designator)
  Built on the same CGS_Titan_Missile component as Laser_Missile_Arm.et, set to:
    m_bHoming 1, m_bLockTargets 1   - lock a target and the missiles chase it
    m_eGuidance BOTH                 - lock-on OR the laser spot from a Target Designator
    m_fLaserConeDeg 120              - how far off-axis a laser spot can be and still be picked up
    m_fLoftAngle 45                  - missiles arc up first (LRM look)
    m_fTurnRate 45                   - how hard they can turn (lower = easier to dodge)
    m_fArmingDistance 60             - no damage closer than 60 m (LRM minimum range)
  To designate: put TARGET_DESIGNATOR on the SHOULDER, turn it on (toggle), and fire the LRMs
  at the spot. A teammate's designator works too.

WHEN THE MODEL IS DONE
  1. MeshObject > Object: pick your model. It currently uses Left_Right_4_Pod.xob as a placeholder.
  2. m_aBarrelPoints: one PointInfo per tube, in firing order. They are laid out as a placeholder
     grid (left rack x = -0.6, right rack x = +0.6, facing +Z). In Workbench drag each one onto its
     tube opening. Keep exactly 30 (LRM15) / 40 (LRM20), or change m_iMagazineSize to match.
  3. m_MuzzlePoint: the centre of the pod (used for aim / crosshair).
  4. Missiles must leave along +Z of the prefab - rotate the model, not the points.

PUTTING IT ON A MECH
  Needs a hardpoint whose frame rule is MISSILE (or ANY) and max size LARGE - a heavy / Catapult
  frame. The Warden's BACK is MEDIUM, so it will refuse these ("too big for this hardpoint").
  Add to the mech's BFA_MechHardpointComponent > m_aModules:
    m_iId          next free number (1-31)
    m_sLabel       "LRM-15 x2"
    m_iClass       3                          (same class as the BACK hardpoint)
    m_sWeaponModuleId "LRM15_PAIR"
    m_sPrefab      "{FEFE3F482AB3D8B8}Prefabs/Vehicles/Mech/Weapons/LRM/LRM15_Pair.et"
    m_fSwapTime    5
  (same for LRM20_PAIR / {8AB285438FD079E6}...LRM20_Pair.et)

CHECK IN PLAY (the CGS component's source isn't in this repo, so these are best guesses)
  - One press should send the whole volley (one missile per barrel point, 60 ms apart =
    m_iSalvoIntervalMs). If it only fires one missile per press, tell Claude / check the
    CGS_Titan_Missile attributes in Workbench for a salvo-count setting.
  - m_fHeatPerShot 1.5: if heat is counted per missile, a full LRM20 volley = 60 heat. If it is
    counted per press, raise it to ~45 (LRM15) / ~60 (LRM20).
  - Damage 60 direct + 25 splash per missile. Compare: Laser Missiles do 250 per missile x 4.
