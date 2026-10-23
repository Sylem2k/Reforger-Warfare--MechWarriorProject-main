//------------------------------------------------------------------------------------------------
//! Reforger Warfare: ammo-independent grappling input from the actual local player controller.
//!
//! M9-derived guns keep their default ADS (right mouse) and equip/pickup behavior.
//! The normal CharacterFire input (left mouse) is read on the local player controller,
//! regardless of whether the firearm successfully fires a projectile.
//! The controller has player-owned RPC authority and resolves the EQUIPPED
//! RW_GrapplingHookComponent again on the server. No weapon-entity RPC ownership
//! or inferred item parenting is required.
//
//! Test only with an EMPTY M9 magazine: the vanilla weapon can still shoot
//! ordinary damaging bullets if loaded. No general changes to other firearms.
modded class SCR_PlayerController
{
	protected bool m_bRWGrappleFireWasDown;

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(float timeSlice)
	{
		super.OnUpdate(timeSlice);

		if (!GetGame() || GetGame().GetPlayerController() != this)
			return;

		InputManager inputs = GetGame().GetInputManager();
		if (!inputs)
			return;

		// Track held state every frame. One left-click sends one request.
		// CharacterFire is the game's normal shoot input, not a new binding.
		bool fireDown = inputs.GetActionValue("CharacterFire") > 0.5;
		bool justPressed = inputs.GetActionTriggered("CharacterFire") || (fireDown && !m_bRWGrappleFireWasDown);
		m_bRWGrappleFireWasDown = fireDown;

		if (!justPressed)
			return;

		ChimeraCharacter user = ChimeraCharacter.Cast(GetControlledEntity());
		if (!user)
			return;

		BaseWeaponComponent weapon = SCR_WeaponLib.GetCurrentWeaponComponent(user);
		if (!weapon)
			return;

		RW_GrapplingHookComponent grapple = RW_GrapplingHookComponent.Cast(weapon.GetOwner().FindComponent(RW_GrapplingHookComponent));
		if (!grapple || !grapple.WantsControllerFireInput())
			return;

		vector direction = user.GetTransformAxis(2);
		AimingComponent aim = AimingComponent.Cast(user.FindComponent(AimingComponent));
		if (!aim)
			aim = AimingComponent.Cast(weapon.GetOwner().FindComponent(AimingComponent));
		if (aim)
		{
			vector aimed = aim.GetAimingDirectionWorld();
			if (aimed.LengthSq() > 0.001)
				direction = aimed;
		}

		if (direction.LengthSq() < 0.001)
			return;
		direction.Normalize();

		Print(string.Format("[RW GRAPPLE CTRL] Local FIRE input detected. Equipped=%1", weapon.GetOwner()));

		if (Replication.IsServer())
		{
			grapple.ServerPlayerControllerFire(user, direction);
			return;
		}

		// This RPC is owned by the player's controller, not by their gun.
		Rpc(RW_RpcAskGrappleFire, direction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RW_RpcAskGrappleFire(vector direction)
	{
		if (!Replication.IsServer())
			return;

		ChimeraCharacter user = ChimeraCharacter.Cast(GetControlledEntity());
		if (!user)
			return;

		BaseWeaponComponent weapon = SCR_WeaponLib.GetCurrentWeaponComponent(user);
		if (!weapon)
		{
			Print("[RW GRAPPLE CTRL] Server: no current weapon", LogLevel.WARNING);
			return;
		}

		RW_GrapplingHookComponent grapple = RW_GrapplingHookComponent.Cast(weapon.GetOwner().FindComponent(RW_GrapplingHookComponent));
		if (!grapple || !grapple.WantsControllerFireInput())
		{
			Print("[RW GRAPPLE CTRL] Server: current weapon lacks active grapple component", LogLevel.WARNING);
			return;
		}

		Print(string.Format("[RW GRAPPLE CTRL] Server received fire input. Player=%1 weapon=%2", user, weapon.GetOwner()));
		grapple.ServerPlayerControllerFire(user, direction);
	}
}
