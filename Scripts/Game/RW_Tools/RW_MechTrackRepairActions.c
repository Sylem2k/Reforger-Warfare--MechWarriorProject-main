// REFORGER WARFARE - Track-only damage/repair test actions.
// Add to the Assault Tracked mech ActionsManager at the LEFT/RIGHT track
// service contexts. These actions do NOT control the engine, refit weapons,
// restore the hull, or change locomotion behavior.
//
// Test damage uses the real BFA_MechPartHitZone on each slot-attached track.
// Wrench repair restores that same hit zone, one part at a time.
// All HP-changing calls are executed and checked on the authoritative server.

class RW_MechTrackServiceActionBase : ScriptedUserAction
{
	[Attribute("0", UIWidgets.Auto, "Index of target in CGS_MechRepairComponent.m_aRepairSites. 0=LEFT TRACK, 1=RIGHT TRACK.", category: "Track Repair")]
	protected int m_iRepairSiteIndex;

	protected CGS_MechRepairComponent GetRepair()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;
		IEntity root = owner.GetRootParent();
		if (!root)
			root = owner;
		return CGS_MechRepairComponent.Cast(root.FindComponent(CGS_MechRepairComponent));
	}

	protected bool ValidTrack(IEntity user)
	{
		CGS_MechRepairComponent repair = GetRepair();
		return repair && repair.IsTrackSite(m_iRepairSiteIndex) && repair.CanService(user, m_iRepairSiteIndex) && repair.GetRepairTargetHealth01(m_iRepairSiteIndex) >= 0;
	}

	protected string GetTrackLabel()
	{
		CGS_MechRepairComponent repair = GetRepair();
		if (!repair)
			return "TRACK";
		return repair.GetRepairSiteLabel(m_iRepairSiteIndex);
	}

	protected int GetHealthPercent()
	{
		CGS_MechRepairComponent repair = GetRepair();
		if (!repair)
			return -1;
		float ratio = repair.GetRepairTargetHealth01(m_iRepairSiteIndex);
		if (ratio < 0)
			return -1;
		return (int)Math.Round(ratio * 100.0);
	}
}

// TEMPORARY developmental test control, enabled only for explicitly configured
// test prefabs. Disabled by default in the Core repair registry.
class RW_MechTrackDamageTestAction : RW_MechTrackServiceActionBase
{
	[Attribute("175", UIWidgets.Auto, "HP taken from the selected track with one test hit.", category: "Track Test")]
	protected float m_fTestDamage;

	override bool GetActionNameScript(out string outName)
	{
		outName = string.Format("TEST DAMAGE %1 (%2%% HP)", GetTrackLabel(), GetHealthPercent());
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		CGS_MechRepairComponent repair = GetRepair();
		return repair && repair.TrackDamageTestEnabled() && ValidTrack(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!CanBeShownScript(user))
		{
			SetCannotPerformReason("Stand beside a valid track on the test mech");
			return false;
		}
		if (GetHealthPercent() <= 0)
		{
			SetCannotPerformReason("Track is already destroyed");
			return false;
		}
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;
		CGS_MechRepairComponent repair = GetRepair();
		if (repair)
			repair.ServerTestDamageTrack(m_iRepairSiteIndex, pUserEntity, m_fTestDamage);
	}
}

// Field repair; requires the existing MW Mech Wrench, checked again on server.
// This never changes weapon loadout or installs a replacement track.
class RW_MechTrackWrenchRepairAction : RW_MechTrackServiceActionBase
{
	[Attribute("{33B2DFDCD0EBA3DB}Prefabs/Items/Equipment/Kits/RepairKit_01/Mech Wrench.et", UIWidgets.ResourceNamePicker, "Required Mech Wrench item.", "et")]
	protected ResourceName m_sTool;

	[Attribute("175", UIWidgets.Auto, "Hit points restored per completed wrench action.", category: "Repair")]
	protected float m_fRepairAmount;

	override bool GetActionNameScript(out string outName)
	{
		outName = string.Format("WRENCH REPAIR %1 (%2%% HP)", GetTrackLabel(), GetHealthPercent());
		return true;
	}

	// The service point is ALWAYS shown for a present track, including at 100% HP
	// and to players not holding the wrench (vanilla-style discoverability).
	// Tool and actual missing HP are checked only when trying to perform repair.
	override bool CanBeShownScript(IEntity user)
	{
		return ValidTrack(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!CanBeShownScript(user))
		{
			SetCannotPerformReason("Stand next to this track while the mech is stationary");
			return false;
		}
		if (!RW_MechWrench.HasToolCached(user, m_sTool))
		{
			SetCannotPerformReason("Mech Wrench required");
			return false;
		}
		if (GetHealthPercent() >= 100)
		{
			SetCannotPerformReason("Track already fully repaired");
			return false;
		}
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !RW_MechWrench.HasTool(pUserEntity, m_sTool))
			return;
		CGS_MechRepairComponent repair = GetRepair();
		if (repair)
			repair.ServerRepairPart(m_iRepairSiteIndex, pUserEntity, m_fRepairAmount);
	}
}
