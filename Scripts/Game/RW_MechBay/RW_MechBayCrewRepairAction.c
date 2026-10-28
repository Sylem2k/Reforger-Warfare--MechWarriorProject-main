// Mech Bay timed crew-repair request. The job, permissions and finish time
// are authored by RW_MechBayComponent on the server (not by client GUI).
class RW_MechBayCrewRepairAction : ScriptedUserAction
{
	protected RW_MechBayComponent m_Bay;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Bay = RW_MechBayComponent.Cast(pOwnerEntity.FindComponent(RW_MechBayComponent));
	}

	override bool GetActionNameScript(out string outName)
	{
		if (m_Bay && m_Bay.IsCrewRepairing())
			outName = string.Format("CREW REPAIR - %1s remaining", m_Bay.GetCrewRepairSecondsRemaining());
		else
			outName = "MECH BAY: ORDER CREW REPAIR";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		// Keep the repair-order option visible even when the pad is empty or
		// parts are healthy (the usage reason is shown below).
		return m_Bay != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_Bay)
			return false;
		if (m_Bay.IsCrewRepairing())
		{
			SetCannotPerformReason("Crew repair already in progress");
			return false;
		}
		if (!m_Bay.CanUseServiceConsole(user))
		{
			SetCannotPerformReason("Use the Mech Bay ground service station");
			return false;
		}
		IEntity mech = m_Bay.FindParkedMech();
		if (!mech)
		{
			SetCannotPerformReason("Park a mech inside the bay first");
			return false;
		}
		if (!m_Bay.CanOperate(user, mech))
		{
			SetCannotPerformReason("Approach the Mech Bay controls");
			return false;
		}
		if (m_Bay.GetCrewRepairWorkPercent(mech) <= 0.001)
		{
			SetCannotPerformReason("Tracks and arm mounts are already repaired");
			return false;
		}
		return true;
	}

	// The shield generator's proven pattern: local interaction -> caller's
	// PlayerController -> reliable server RPC -> authoritative validation.
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
		controller.RW_MB_RequestService(rpl.Id(), -1);
	}
}