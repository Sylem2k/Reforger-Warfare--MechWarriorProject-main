// REFORGER WARFARE - Independent structural arm-mount damage / wrench service test.
// This uses the existing CGS_MechRepairComponent health, location and tool rules.
// Neither weapon swapping nor the installed module prefab is touched.

class RW_MechArmServiceActionBase : RW_MechTrackServiceActionBase
{
	protected bool ValidArm(IEntity user)
	{
		CGS_MechRepairComponent repair = GetRepair();
		return repair && repair.IsWeaponMountSite(m_iRepairSiteIndex) && repair.CanService(user, m_iRepairSiteIndex) && repair.GetRepairTargetHealth01(m_iRepairSiteIndex) >= 0;
	}
}

// Temporary testing interaction. Enabled only on the explicit test frame.
class RW_MechArmDamageTestAction : RW_MechArmServiceActionBase
{
	[Attribute("175", UIWidgets.Auto, "HP removed from this structural arm hit zone per test hit.", category: "Arm Test")]
	protected float m_fTestDamage;

	override bool GetActionNameScript(out string outName)
	{
		outName = string.Format("TEST DAMAGE %1 (%2%% HP)", GetTrackLabel(), GetHealthPercent());
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		CGS_MechRepairComponent repair = GetRepair();
		return repair && repair.ArmDamageTestEnabled() && ValidArm(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!CanBeShownScript(user))
		{
			SetCannotPerformReason("Stand next to this arm mount on the parked test mech");
			return false;
		}
		if (GetHealthPercent() <= 0)
		{
			SetCannotPerformReason("Arm mount already disabled at 0%");
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
			repair.ServerTestDamageArm(m_iRepairSiteIndex, pUserEntity, m_fTestDamage);
	}
}

// Show permanent arm service points even at full health or without a wrench.
// Wrench ownership, missing HP, distance, and stationary state are checked
// before performing; the tool is revalidated on the server.
class RW_MechArmWrenchRepairAction : RW_MechArmServiceActionBase
{
	[Attribute("{33B2DFDCD0EBA3DB}Prefabs/Items/Equipment/Kits/RepairKit_01/Mech Wrench.et", UIWidgets.ResourceNamePicker, "Required Mech Wrench item.", "et")]
	protected ResourceName m_sTool;

	[Attribute("175", UIWidgets.Auto, "Legacy amount (unused): Core enforces 2% per field use or 15% per bay use.", category: "Repair")]
	protected float m_fRepairAmount;

	override bool GetActionNameScript(out string outName)
	{
		// Show the repair rate appropriate to the CURRENT parking location.
		IEntity mech = GetOwner();
		if (mech)
			mech = mech.GetRootParent();
		string rate = "2%";
		if (RW_MechBayComponent.FindBayFor(mech))
			rate = "15%";
		outName = string.Format("WRENCH REPAIR %1 %2 (%3%% HP)", rate, GetTrackLabel(), GetHealthPercent());
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		// In a Mech Bay, all parts are serviced from the single ground-level
		// service console instead of inaccessible tall-mech access points.
		IEntity mech = GetOwner();
		if (mech)
			mech = mech.GetRootParent();
		if (RW_MechBayComponent.FindBayFor(mech))
			return false;
		return ValidArm(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!CanBeShownScript(user))
		{
			SetCannotPerformReason("Stand next to this arm mount while the mech is stationary");
			return false;
		}
		if (!RW_MechWrench.HasToolCached(user, m_sTool))
		{
			SetCannotPerformReason("Mech Wrench required");
			return false;
		}
		IEntity mech = GetOwner();
		if (mech)
			mech = mech.GetRootParent();
		RW_MechBayComponent bay = RW_MechBayComponent.FindBayFor(mech);
		float allowed = 25.0;
		if (bay)
			allowed = 100.0;
		CGS_MechRepairComponent repair = GetRepair();
		if (!repair || repair.GetRepairTargetHealth01(m_iRepairSiteIndex) * 100.0 >= allowed - 0.001)
		{
			if (bay)
				SetCannotPerformReason("Arm mount already fully repaired");
			else
				SetCannotPerformReason("Field repairs capped at 25% - use a Mech Bay");
			return false;
		}
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !RW_MechWrench.HasTool(pUserEntity, m_sTool))
			return;

		CGS_MechRepairComponent repair = GetRepair();
		if (!repair || !repair.IsWeaponMountSite(m_iRepairSiteIndex))
			return;
		// Determine bay status on the server, never from client action text.
		IEntity mech = GetOwner();
		if (mech)
			mech = mech.GetRootParent();
		RW_MechBayComponent bay = RW_MechBayComponent.FindBayFor(mech);
		if (bay)
			repair.ServerRepairPartAtBay(m_iRepairSiteIndex, pUserEntity);
		else
			repair.ServerRepairPart(m_iRepairSiteIndex, pUserEntity, 0);
	}
}