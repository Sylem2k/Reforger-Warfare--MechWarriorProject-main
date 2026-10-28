// Reforger Warfare - central, ground-level Mech Bay repair station.
//
// Four instances on one UserActionContext form a native scrollable action
// list: left/right track and left/right structural arm. The repair points on
// tall mech meshes are never used for Mech Bay maintenance.
//
// Server receives a reliable authenticated request through the local
// player's SCR_PlayerController and checks bay/tool/repair target again.
class RW_MechBayManualRepairAction : ScriptedUserAction
{
	[Attribute("0", UIWidgets.Auto, "Repair site index on the parked mech: 0 left track, 1 right track, 2 left arm, 3 right arm.", category: "Mech Bay")]
	protected int m_iRepairSiteIndex;

	protected RW_MechBayComponent m_Bay;

	override void Init(IEntity owner, GenericComponent manager)
	{
		m_Bay = RW_MechBayComponent.Cast(owner.FindComponent(RW_MechBayComponent));
	}

	protected CGS_MechRepairComponent FindRepair()
	{
		if (!m_Bay)
			return null;
		IEntity mech = m_Bay.FindParkedMech();
		if (!mech)
			return null;
		return CGS_MechRepairComponent.Cast(mech.FindComponent(CGS_MechRepairComponent));
	}

	protected string GetPartLabel()
	{
		CGS_MechRepairComponent repair = FindRepair();
		if (repair && m_iRepairSiteIndex >= 0 && m_iRepairSiteIndex < repair.GetRepairSiteCount())
			return repair.GetRepairSiteLabel(m_iRepairSiteIndex);

		switch (m_iRepairSiteIndex)
		{
			case 0: return "LEFT TRACK";
			case 1: return "RIGHT TRACK";
			case 2: return "LEFT ARM";
			case 3: return "RIGHT ARM";
		}
		return "PART";
	}

	override bool GetActionNameScript(out string outName)
	{
		string name = GetPartLabel();
		CGS_MechRepairComponent repair = FindRepair();
		if (!repair)
		{
			outName = "WRENCH REPAIR 15% " + name;
			return true;
		}
		float health01 = repair.GetRepairTargetHealth01(m_iRepairSiteIndex);
		if (health01 < 0)
		{
			outName = "WRENCH REPAIR 15% " + name + " (not installed)";
			return true;
		}
		int percentage = (int)Math.Round(health01 * 100.0);
		outName = "WRENCH REPAIR 15% " + name + " (" + percentage + "% HP)";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		// The bay console is always discoverable, including when a mech is
		// healthy or a player does not have the wrench.
		return m_Bay != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_Bay || !m_Bay.CanUseServiceConsole(user))
		{
			SetCannotPerformReason("Use the Mech Bay ground service station");
			return false;
		}
		if (m_Bay.IsCrewRepairing())
		{
			SetCannotPerformReason("Crew repair already in progress");
			return false;
		}
		CGS_MechRepairComponent repair = FindRepair();
		if (!repair)
		{
			SetCannotPerformReason("Park a repairable mech in the Mech Bay");
			return false;
		}
		if (m_iRepairSiteIndex < 0 || m_iRepairSiteIndex >= repair.GetRepairSiteCount())
		{
			SetCannotPerformReason("Repair target is not configured");
			return false;
		}
		if (!repair.IsTrackSite(m_iRepairSiteIndex) && !repair.IsArmSite(m_iRepairSiteIndex))
		{
			SetCannotPerformReason("Part is not enabled for repairs yet");
			return false;
		}
		float health01 = repair.GetRepairTargetHealth01(m_iRepairSiteIndex);
		if (health01 < 0)
		{
			SetCannotPerformReason("The selected part is not installed");
			return false;
		}
		if (health01 >= 0.99999)
		{
			SetCannotPerformReason("This part is already fully repaired");
			return false;
		}
		if (!RW_MechWrench.HasToolCached(user, RW_MechWrench.DEFAULT_TOOL))
		{
			SetCannotPerformReason("Mech Wrench required for self repair");
			return false;
		}
		return true;
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_Bay || pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		RplComponent rpl = RplComponent.Cast(m_Bay.GetOwner().FindComponent(RplComponent));
		if (!rpl)
			return;
		controller.RW_MB_RequestService(rpl.Id(), m_iRepairSiteIndex);
	}
}
