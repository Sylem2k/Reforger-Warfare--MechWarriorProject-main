// Reforger Warfare - Mech Bay service requests.
//
// The physical Mech Bay console is a local interaction. All health changes
// and crew job decisions go through the requesting player's OWN reliable
// PlayerController RPC and are independently validated on the server.
//
// siteIndex 0..3 = configured parts on the parked test mech.
// siteIndex -1 = hire crew for timed full repair.
modded class SCR_PlayerController
{
	protected int m_iRW_MBNextRepairRequestMs;

	void RW_MB_RequestService(RplId bayId, int siteIndex)
	{
		if (Replication.IsServer())
			RW_MB_RpcService(bayId, siteIndex);
		else
			Rpc(RW_MB_RpcService, bayId, siteIndex);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RW_MB_RpcService(RplId bayId, int siteIndex)
	{
		if (siteIndex < -1 || siteIndex > 3)
			return;

		int now = System.GetTickCount();
		if (now < m_iRW_MBNextRepairRequestMs)
			return;

		RplComponent rpl = RplComponent.Cast(Replication.FindItem(bayId));
		if (!rpl)
			return;
		IEntity bayEntity = rpl.GetEntity();
		if (!bayEntity)
			return;
		RW_MechBayComponent bay = RW_MechBayComponent.Cast(bayEntity.FindComponent(RW_MechBayComponent));
		if (!bay)
			return;

		IEntity requester = GetControlledEntity();
		if (!requester)
			return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || players.GetPlayerIdFromControlledEntity(requester) != GetPlayerId())
			return;

		// Rate-limit successful manual service requests to the action's 2s
		// duration. The client cannot spam 15% repairs in one frame.
		if (siteIndex == -1)
		{
			if (bay.ServerStartCrewRepair(requester))
				m_iRW_MBNextRepairRequestMs = now + 1000;
		}
		else if (bay.ServerRepairParkedPart(requester, siteIndex))
		{
			m_iRW_MBNextRepairRequestMs = now + 2000;
		}
	}
}
