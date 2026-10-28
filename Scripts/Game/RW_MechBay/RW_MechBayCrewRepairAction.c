// Mech Bay timed crew-repair request. The job, permissions and finish time
// are authored by RW_MechBayComponent on the server (not by client GUI).
class RW_MechBayCrewRepairAction : ScriptedUserAction
{
	protected RW_MechBayComponent m_Bay;

	override void Init(IEntity owner, GenericComponent manager)
	{
		m_Bay = RW_MechBayComponent.Cast(owner.FindComponent(RW_MechBayComponent));
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

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Bay)
			return;
		m_Bay.ServerStartCrewRepair(pUserEntity);
	}
}
