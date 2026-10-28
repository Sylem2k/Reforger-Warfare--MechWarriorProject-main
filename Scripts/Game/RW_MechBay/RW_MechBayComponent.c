//------------------------------------------------------------------------------------------------
//! REFORGER WARFARE - MECH BAY. Put this on a bay prefab. A mech parked within the radius can be
//! refitted with the Mech Lab from the bay's "Open Mech Bay" action, without the Mech Wrench.
//! Frames with "Swap Only At Mech Bay" can ONLY be refitted while parked in a bay.
//! ORDER FRAME: with the pad empty, "Open Mech Bay" shows the bay's frame list; pick one and the
//! bay builds the bare frame on the parking spot, then the Mech Lab opens on it.
//! Later: repair, rearm, cool-down and supply cost go here.

//------------------------------------------------------------------------------------------------
//! One frame the bay can build.
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class RW_BayFrameEntry
{
	[Attribute("", UIWidgets.EditBox, "Name in the order list. Empty = the frame's own name.")]
	string m_sName;

	[Attribute("", UIWidgets.ResourceNamePicker, "Mech prefab to build (spawns with its default, usually empty, loadout).", "et")]
	ResourceName m_sPrefab;
}

//------------------------------------------------------------------------------------------------
class RW_MechBayComponentClass : ScriptComponentClass
{
}

class RW_MechBayComponent : ScriptComponent
{
	[Attribute("Mech Bay", UIWidgets.EditBox, "Name shown in the Mech Lab.", category: "Mech Bay")]
	protected string m_sBayName;

	[Attribute("15", UIWidgets.Auto, "A mech whose centre is within this many metres of the parking spot counts as in the bay.", category: "Mech Bay")]
	protected float m_fRadius;

	[Attribute("0 0 0", UIWidgets.Auto, "Parking spot, relative to the bay (X right, Y up, Z forward). 0 0 0 = the bay's own origin.", category: "Mech Bay")]
	protected vector m_vParkingOffset;

	[Attribute("true", UIWidgets.CheckBox, "Refits here need no Mech Wrench.", category: "Mech Bay")]
	protected bool m_bNoToolNeeded;

	[Attribute("", UIWidgets.Object, "Frames this bay can build (Order Frame).", category: "Frames")]
	protected ref array<ref RW_BayFrameEntry> m_aFrames;

	[Attribute("20", UIWidgets.Auto, "Seconds between two builds at this bay.", category: "Frames")]
	protected float m_fBuildCooldown;

	[Attribute("0.5", UIWidgets.Auto, "The new mech appears this many metres above the ground at the parking spot.", category: "Frames")]
	protected float m_fSpawnHeight;

	[Attribute("0.75", UIWidgets.Auto, "Crew repair duration, seconds per missing HP percent on each installed track or test arm. Eg 100% damage = 75s for one part.", category: "Repair")]
	protected float m_fCrewSecondsPerMissingPercent;

	[Attribute("10", UIWidgets.Auto, "Minimum crew repair order duration in seconds.", category: "Repair")]
	protected float m_fCrewMinRepairSeconds;

	// Replicated informational job status; HP remains on original hit zones.
	[RplProp()]
	protected bool m_bCrewRepairing;
	[RplProp()]
	protected int m_iCrewRepairEndMs;
	protected IEntity m_pCrewRepairMech;


	// Last bay message (built / refused and why), shown in the order screen.
	[RplProp()]
	protected string m_sBayMessage;
	[RplProp()]
	protected int m_iBayMessageSeq;

	protected float m_fNextBuildTime;   // server
	// Last mech ordered here; only its ordering player can return it while it remains on the pad.
	protected IEntity m_pOrderedMech;   // server only
	protected int m_iOrderedByPlayerId; // server only
	protected IEntity m_pPendingMech;   // server: saved build waiting to be fitted
	protected ref RW_SavedBuild m_pPendingBuild;
	protected int m_iPendingTries;

	protected static ref array<RW_MechBayComponent> s_aBays;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aBays)
			s_aBays = {};
		s_aBays.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(ApplyPendingBuild);
			GetGame().GetCallqueue().Remove(ServerFinishCrewRepair);
		}
		if (s_aBays)
			s_aBays.RemoveItem(this);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	string GetBayName()
	{
		return m_sBayName;
	}

	float GetRadius()
	{
		return m_fRadius;
	}

	bool NoToolNeeded()
	{
		return m_bNoToolNeeded;
	}

	//------------------------------------------------------------------------------------------------
	vector GetParkingSpot()
	{
		return GetOwner().CoordToParent(m_vParkingOffset);
	}

	//------------------------------------------------------------------------------------------------
	//! Is this mech parked in this bay?
	bool Contains(IEntity mech)
	{
		if (!mech || !GetOwner())
			return false;
		return vector.Distance(mech.GetOrigin(), GetParkingSpot()) <= m_fRadius;
	}

	//------------------------------------------------------------------------------------------------
	//! Is this player close enough to work the bay (at the bay or at the parked mech)?
	bool CanOperate(IEntity user, IEntity mech)
	{
		if (!user || !GetOwner())
			return false;
		if (vector.Distance(user.GetOrigin(), GetOwner().GetOrigin()) <= m_fRadius + 10)
			return true;
		return mech && vector.Distance(user.GetOrigin(), mech.GetOrigin()) <= 25;
	}

	//------------------------------------------------------------------------------------------------
	//! The bay this mech is parked in, or null.
	static RW_MechBayComponent FindBayFor(IEntity mech)
	{
		if (!mech || !s_aBays)
			return null;
		foreach (RW_MechBayComponent bay : s_aBays)
		{
			if (bay && bay.Contains(mech))
				return bay;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsAtBay(IEntity mech)
	{
		return FindBayFor(mech) != null;
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Crew repair: one server-authoritative timed order at this bay.
	// Only tested structural tracks/arms are included for now. Do not modify
	// swappable weapon loadout, frame, or hull.
	bool IsCrewRepairing()
	{
		return m_bCrewRepairing;
	}

	int GetCrewRepairSecondsRemaining()
	{
		if (!m_bCrewRepairing || !GetGame() || !GetGame().GetWorld())
			return 0;
		return Math.Max(0, (int)Math.Ceil((m_iCrewRepairEndMs - GetGame().GetWorld().GetWorldTime()) * 0.001));
	}

	// Damage percentages are summed so more damaged components take longer.
	// The client uses this for the button; the server recalculates on request.
	float GetCrewRepairWorkPercent(IEntity mech)
	{
		if (!mech)
			return 0;
		CGS_MechRepairComponent repair = CGS_MechRepairComponent.Cast(mech.FindComponent(CGS_MechRepairComponent));
		if (!repair)
			return 0;
		float missingPercent = 0;
		for (int i = 0; i < repair.GetRepairSiteCount(); i++)
		{
			if (!repair.IsTrackSite(i) && !repair.IsArmSite(i))
				continue;
			float health01 = repair.GetRepairTargetHealth01(i);
			if (health01 < 0)
				continue;
			missingPercent += (1.0 - health01) * 100.0;
		}
		return missingPercent;
	}

	bool ServerStartCrewRepair(IEntity user)
	{
		if (!Replication.IsServer() || !user || !ChimeraCharacter.Cast(user) || m_bCrewRepairing)
			return false;

		IEntity mech = FindParkedMech();
		if (!mech || !Contains(mech) || !CanOperate(user, mech) || BFA_MechExtension.IsMechDestroyed(mech))
			return false;

		// Authoritative owner permissions and stationary/no-crew checks.
		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(mech);
		PlayerManager players = GetGame().GetPlayerManager();
		int playerId = 0;
		if (players)
			playerId = players.GetPlayerIdFromControlledEntity(user);
		if (playerId <= 0 || (frame && frame.IsBayReservedForOtherPlayer(playerId)))
			return false;

		Physics physics = mech.GetPhysics();
		if (physics && physics.GetVelocity().Length() > 0.5)
			return false;

		BaseCompartmentManagerComponent seats = BaseCompartmentManagerComponent.Cast(mech.FindComponent(BaseCompartmentManagerComponent));
		if (!seats)
			return false;
		array<BaseCompartmentSlot> compartments = {};
		seats.GetCompartments(compartments);
		foreach (BaseCompartmentSlot compartment : compartments)
		{
			if (compartment && compartment.GetOccupant())
				return false;
		}

		float missing = GetCrewRepairWorkPercent(mech);
		if (missing <= 0.001)
			return false;
		int jobMs = (int)(Math.Max(m_fCrewMinRepairSeconds, missing * Math.Max(0.01, m_fCrewSecondsPerMissingPercent)) * 1000.0);
		m_pCrewRepairMech = mech;
		m_bCrewRepairing = true;
		m_iCrewRepairEndMs = GetGame().GetWorld().GetWorldTime() + jobMs;
		Replication.BumpMe();
		Print(string.Format("[CGS BAY CREW] START: missing summed=%1%%, duration=%2s", missing, jobMs * 0.001));
		ServerBayMessage(string.Format("Crew repairing %1 - %2 seconds", mech.GetName(), jobMs * 0.001));
		GetGame().GetCallqueue().Remove(ServerFinishCrewRepair);
		GetGame().GetCallqueue().CallLater(ServerFinishCrewRepair, jobMs, false);
		return true;
	}

	protected void ServerFinishCrewRepair()
	{
		if (!Replication.IsServer() || !m_bCrewRepairing)
			return;

		IEntity mech = m_pCrewRepairMech;
		m_pCrewRepairMech = null;
		m_bCrewRepairing = false;
		m_iCrewRepairEndMs = 0;
		Replication.BumpMe();

		if (!mech || !Contains(mech) || BFA_MechExtension.IsMechDestroyed(mech))
		{
			ServerBayMessage("Crew repair canceled: mech left bay or was destroyed");
			return;
		}

		Physics physics = mech.GetPhysics();
		if (physics && physics.GetVelocity().Length() > 0.5)
		{
			ServerBayMessage("Crew repair canceled: mech is moving");
			return;
		}

		CGS_MechRepairComponent repair = CGS_MechRepairComponent.Cast(mech.FindComponent(CGS_MechRepairComponent));
		if (!repair)
		{
			ServerBayMessage("Crew repair canceled: no repair system");
			return;
		}

		int repaired = 0;
		for (int i = 0; i < repair.GetRepairSiteCount(); i++)
		{
			if (!repair.IsTrackSite(i) && !repair.IsArmSite(i))
				continue;
			if (repair.ServerCrewRepairPart(i))
				repaired++;
		}
		ServerBayMessage(string.Format("Crew repair completed: %1 parts restored", repaired));
	}

	//------------------------------------------------------------------------------------------------
	int GetFrameCount()
	{
		if (!m_aFrames)
			return 0;
		return m_aFrames.Count();
	}

	RW_BayFrameEntry GetFrameEntry(int index)
	{
		if (!m_aFrames || index < 0 || index >= m_aFrames.Count())
			return null;
		return m_aFrames[index];
	}

	string GetBayMessage()
	{
		return m_sBayMessage;
	}

	int GetBayMessageSeq()
	{
		return m_iBayMessageSeq;
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerBayMessage(string message)
	{
		m_sBayMessage = message;
		m_iBayMessageSeq++;
		Replication.BumpMe();
		Print("[RW MECH BAY] " + m_sBayName + ": " + message);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: build frame 'index' on the pad for this player.
	bool ServerOrderFrame(int index, IEntity user, int playerId, RW_SavedBuild build = null)
	{
		if (!Replication.IsServer())
			return false;

		RW_BayFrameEntry entry = GetFrameEntry(index);
		if (!entry || entry.m_sPrefab.IsEmpty())
		{
			ServerBayMessage("REFUSED: no such frame");
			return false;
		}
		// The RPC runs through the requesting PlayerController; verify that its
		// controlled entity actually belongs to that player before locking ownership.
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerId <= 0 || !playerManager || !user || playerManager.GetPlayerIdFromControlledEntity(user) != playerId)
		{
			ServerBayMessage("REFUSED: could not verify ordering player");
			return false;
		}
		if (vector.Distance(user.GetOrigin(), GetOwner().GetOrigin()) > m_fRadius + 10)
		{
			ServerBayMessage("REFUSED: too far from the bay");
			return false;
		}
		if (m_pPendingMech)
		{
			ServerBayMessage("REFUSED: bay is still fitting the previous order");
			return false;
		}
		if (FindParkedMech())
		{
			ServerBayMessage("REFUSED: pad occupied - move the mech off the pad first");
			return false;
		}
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (now < m_fNextBuildTime)
		{
			ServerBayMessage(string.Format("REFUSED: crew busy, ready in %1 s", Math.Ceil(m_fNextBuildTime - now)));
			return false;
		}

		Resource resource = Resource.Load(entry.m_sPrefab);
		if (!resource || !resource.IsValid())
		{
			ServerBayMessage("REFUSED: frame prefab missing");
			return false;
		}

		// On the ground at the parking spot, facing the way the bay faces.
		BaseWorld world = GetOwner().GetWorld();
		vector spot = GetParkingSpot();
		// Preserve the authored pad elevation for raised or indoor mech bays.
		// Ground-level bays use the terrain surface when the offset is lower.
		spot[1] = Math.Max(spot[1], world.GetSurfaceY(spot[0], spot[2])) + m_fSpawnHeight;
		vector angles = GetOwner().GetYawPitchRoll();
		vector mat[4];
		Math3D.AnglesToMatrix(Vector(angles[0], 0, 0), mat);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = mat[0];
		params.Transform[1] = mat[1];
		params.Transform[2] = mat[2];
		params.Transform[3] = spot;

		IEntity mech = GetGame().SpawnEntityPrefab(resource, world, params);
		if (!mech)
		{
			ServerBayMessage("REFUSED: could not build the frame");
			return false;
		}

		m_pOrderedMech = mech;
		m_iOrderedByPlayerId = playerId;
		m_fNextBuildTime = now + Math.Max(m_fBuildCooldown, 0);
		string name = entry.m_sName;
		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(mech);
		if (frame)
			frame.ServerSetBayOrderOwnerId(playerId);

		// A companion frame is reserved for the same authenticated player at birth.
		CGS_MechBrainComponent brain = CGS_MechBrainComponent.Find(mech);
		if (brain)
			brain.ServerBindCompanionOwner(playerId);
		if (name.IsEmpty() && frame)
			name = frame.GetFrameName();
		if (!build)
		{
			ServerBayMessage(name + " built - fit it out");
			return true;
		}

		// Saved build: fit it once the new mech has settled.
		ServerBayMessage(name + " built - fitting \"" + build.m_sName + "\"...");
		m_pPendingMech = mech;
		m_pPendingBuild = build;
		m_iPendingTries = 0;
		GetGame().GetCallqueue().Remove(ApplyPendingBuild);
		GetGame().GetCallqueue().CallLater(ApplyPendingBuild, 1500, false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyPendingBuild()
	{
		if (!m_pPendingBuild)
			return;
		if (!m_pPendingMech)
		{
			ServerBayMessage("REFUSED: mech was removed before its saved build could be fitted");
			m_pPendingBuild = null;
			return;
		}
		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(m_pPendingMech);
		BFA_MechHardpointComponent hardpoints = BFA_MechHardpointComponent.Cast(m_pPendingMech.FindComponent(BFA_MechHardpointComponent));
		if (!frame || !hardpoints)
		{
			ServerBayMessage("REFUSED: spawned frame is missing mech components");
			m_pPendingMech = null;
			m_pPendingBuild = null;
			return;
		}
		if (m_pPendingBuild.m_aModules.Count() != hardpoints.GetHardpointCount())
		{
			ServerBayMessage("REFUSED: saved build hardpoint count does not match this frame");
			m_pPendingMech = null;
			m_pPendingBuild = null;
			return;
		}

		// One entry per hardpoint (older saves may be shorter).
		array<int> modules = {};
		for (int i = 0; i < hardpoints.GetHardpointCount(); i++)
		{
			int id = 0;
			if (i < m_pPendingBuild.m_aModules.Count())
				id = m_pPendingBuild.m_aModules[i];
			modules.Insert(id);
		}

		string reason;
		if (frame.ServerApplyBuild(modules, m_pPendingBuild.m_iHeatsinks, reason))
		{
			ServerBayMessage("\"" + m_pPendingBuild.m_sName + "\" fitted - crew at work");
			m_pPendingMech = null;
			m_pPendingBuild = null;
			return;
		}

		// Still settling / busy: try again a few times. Rules broken: tell the player.
		m_iPendingTries++;
		if (m_iPendingTries < 6 && (reason.Contains("still") || reason.Contains("already")))
		{
			GetGame().GetCallqueue().CallLater(ApplyPendingBuild, 1000, false);
			return;
		}
		ServerBayMessage("REFUSED: saved build doesn't fit any more - " + reason + " (fix it in the Mech Lab)");
		m_pPendingMech = null;
		m_pPendingBuild = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Only the player who ordered the current pad mech can return it. The server owns the decision.
	//! This cannot delete an arbitrary mech parked in the bay, a crewed mech or a frame being fitted.
	bool ServerDeleteOrderedMech(IEntity mech, int playerId, IEntity user)
	{
		if (!Replication.IsServer())
			return false;

		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(mech);
		if (!mech || !frame)
			return false;

		if (playerId <= 0 || m_iOrderedByPlayerId != playerId || m_pOrderedMech != mech)
		{
			frame.ServerCrewMessage("REFUSED: only the player who ordered this mech can delete it");
			return false;
		}

		if (!user || !CanOperate(user, mech) || !Contains(mech) || FindParkedMech() != mech)
		{
			frame.ServerCrewMessage("REFUSED: mech must be on its original Mech Bay pad");
			return false;
		}

		if (m_pPendingMech == mech)
		{
			frame.ServerCrewMessage("REFUSED: wait for the saved build to finish");
			return false;
		}

		BFA_MechHardpointComponent hardpoints = BFA_MechHardpointComponent.Cast(mech.FindComponent(BFA_MechHardpointComponent));
		if (hardpoints && hardpoints.IsSwapInProgress())
		{
			frame.ServerCrewMessage("REFUSED: wait for the crew to finish fitting weapons");
			return false;
		}

		// Never delete a mech with a player or AI in any seat.
		BaseCompartmentManagerComponent seats = BaseCompartmentManagerComponent.Cast(mech.FindComponent(BaseCompartmentManagerComponent));
		if (!seats)
		{
			frame.ServerCrewMessage("REFUSED: could not verify mech seats are empty");
			return false;
		}
		array<BaseCompartmentSlot> slots = {};
		seats.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (slot && slot.GetOccupant())
			{
				frame.ServerCrewMessage("REFUSED: someone is still inside the mech");
				return false;
			}
		}

		// Clear server ownership before deletion; reset cooldown so the correct frame can be ordered.
		m_pOrderedMech = null;
		m_iOrderedByPlayerId = 0;
		m_fNextBuildTime = 0;
		ServerBayMessage("Mech deleted - pad ready for another order");
		SCR_EntityHelper.DeleteEntityAndChildren(mech);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The mech parked closest to the parking spot, or null. Walks the frame list (a handful of
	//! mechs) - no area scan, so it is cheap enough for the action prompt.
	IEntity FindParkedMech()
	{
		IEntity best;
		float bestDist = m_fRadius;
		vector spot = GetParkingSpot();
		foreach (RW_MechFrameComponent frame : RW_MechFrameComponent.GetAll())
		{
			if (!frame)
				continue;
			IEntity mech = frame.GetOwner();
			if (!mech || BFA_MechExtension.IsMechDestroyed(mech))
				continue;
			float dist = vector.Distance(mech.GetOrigin(), spot);
			if (dist <= bestDist)
			{
				bestDist = dist;
				best = mech;
			}
		}
		return best;
	}
}

//------------------------------------------------------------------------------------------------
//! On the bay: opens the Mech Lab for the mech parked in it.
class RW_MechBayOpenAction : ScriptedUserAction
{
	protected RW_MechBayComponent m_Bay;
	protected IEntity m_pCachedMech;
	protected float m_fNextLook;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Bay = RW_MechBayComponent.Cast(pOwnerEntity.FindComponent(RW_MechBayComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity ParkedMech()
	{
		if (!m_Bay || !GetGame().GetWorld())
			return null;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (now >= m_fNextLook)
		{
			m_fNextLook = now + 0.5;
			m_pCachedMech = m_Bay.FindParkedMech();
		}
		return m_pCachedMech;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return m_Bay != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		IEntity mech = ParkedMech();
		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(mech);
		if (frame && GetGame())
		{
			PlayerController pc = GetGame().GetPlayerController();
			// Client UI check only; a headless server has no local PlayerController.
			// The server RPC performs the authoritative ownership validation.
			if (pc && frame.IsBayReservedForOtherPlayer(pc.GetPlayerId()))
			{
				SetCannotPerformReason("Another player owns this bay order");
				return false;
			}
		}
		if (!mech && m_Bay.GetFrameCount() == 0)
		{
			SetCannotPerformReason("No mech parked in the bay");
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		IEntity mech = ParkedMech();
		RW_MechFrameComponent frame = RW_MechFrameComponent.Find(mech);
		if (frame)
			outName = "Open Mech Bay (" + frame.GetFrameName() + ")";
		else if (m_Bay && m_Bay.GetFrameCount() > 0)
			outName = "Mech Bay: Order Frame";
		else
			outName = "Open Mech Bay";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;
		m_fNextLook = 0;
		IEntity mech = ParkedMech();
		if (mech)
			RW_MechLabMenu.OpenFor(mech);
		else if (m_Bay.GetFrameCount() > 0)
			RW_MechLabMenu.OpenOrder(m_Bay);
	}
}
