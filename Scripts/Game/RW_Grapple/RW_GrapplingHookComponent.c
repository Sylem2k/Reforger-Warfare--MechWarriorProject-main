//------------------------------------------------------------------------------------------------
//! Reforger Warfare - handheld Grappling Hook test prototype.
//! Also supports Handgun_Grapling.et (M9): firing the normal pistol trigger
//! invokes grapple. ADS stays vanilla. OnProjectileShot event from the player
//! is used with an optional immediate MuzzleEffect event where available.
//! Server is authoritative; camera/weapon aim direction is used when available.
//! There is NO rope rendering, no fall-damage immunity and no wall-climbing yet.
//! With physics pull the player controller may reject imposed velocity on some builds;
//! an explicitly opt-in scripted movement test is exposed in Workbench.
[ComponentEditorProps(category: "Reforger Warfare/Grappling", description: "Experimental handheld grapple: server raycast and controlled character pull.")]
class RW_GrapplingHookComponentClass : ScriptComponentClass
{
}

class RW_GrapplingHookComponent : ScriptComponent
{
	[Attribute("50", UIWidgets.Auto, "Maximum hook distance in metres.", category: "Grapple")]
	protected float m_fMaxRange;

	[Attribute("18", UIWidgets.Auto, "Pull speed, metres per second.", category: "Grapple")]
	protected float m_fPullSpeed;

	[Attribute("2.0", UIWidgets.Auto, "Stop pulling when the player's chest is this close to the anchor (metres).", category: "Grapple")]
	protected float m_fStopDistance;

	[Attribute("3", UIWidgets.Auto, "Cooldown after activation, seconds.", category: "Grapple")]
	protected float m_fCooldown;

	[Attribute("6", UIWidgets.Auto, "Maximum grapple duration, seconds.", category: "Grapple")]
	protected float m_fMaxDuration;

	[Attribute("1.5", UIWidgets.Auto, "Approximate eye height for the server-side facing raycast.", category: "Grapple")]
	protected float m_fEyeHeight;

	[Attribute("0.65", UIWidgets.Auto, "One-time upwards release from the grounded character controller, metres. 0 disables the lift.", category: "Grapple Test")]
	protected float m_fGroundReleaseLift;

	[Attribute("false", UIWidgets.CheckBox, "EXPERIMENT ONLY: server moves the character with SetOrigin instead of velocity. Use only if native physics cannot pull a player; may cause camera snapping and movement collisions.", category: "Grapple Test")]
	protected bool m_bScriptedPositionFallback;

	[Attribute("false", UIWidgets.CheckBox, "Log start, stop and failed motion diagnostics.", category: "Grapple Test")]
	protected bool m_bDebug;

	[Attribute("false", UIWidgets.CheckBox, "M9 test: listen for actual pistol shots and activate grapple on left-click. ADS remains normal.", category: "Pistol Trigger")]
	protected bool m_bUseWeaponTrigger;

	[Attribute("true", UIWidgets.CheckBox, "M9 test: try to remove the normal fired bullet on the server. This is NOT a substitute for using a proper zero-damage test cartridge.", category: "Pistol Trigger")]
	protected bool m_bDeleteProjectile;

	protected IEntity m_pUser;
	protected vector m_vAnchor;
	protected vector m_vLastPosition;
	protected float m_fNoMotionSeconds;
	protected int m_iStartMs;
	protected int m_iNextUseMs;
	protected float m_fReleaseDelay;

	// The equipped player's OnProjectileShot is a vanilla Reforger weapon event.
	protected IEntity m_pFireEventUser;
	protected bool m_bRegisteredMuzzle;
	protected int m_iLastPistolShotMs;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.FRAME);

		// Reforger's regular muzzle event is immediate. Register only if this
		// inherited pistol base has the scripted muzzle-effect component.
		if (m_bUseWeaponTrigger)
		{
			SCR_MuzzleEffectComponent muzzleEffects = SCR_MuzzleEffectComponent.Cast(owner.FindComponent(SCR_MuzzleEffectComponent));
			if (muzzleEffects)
			{
				muzzleEffects.GetOnWeaponFired().Insert(OnMuzzleFired);
				m_bRegisteredMuzzle = true;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		DetachShotListener();
		if (m_bRegisteredMuzzle)
		{
			SCR_MuzzleEffectComponent muzzleEffects = SCR_MuzzleEffectComponent.Cast(owner.FindComponent(SCR_MuzzleEffectComponent));
			if (muzzleEffects)
				muzzleEffects.GetOnWeaponFired().Remove(OnMuzzleFired);
		}
		StopPull(false);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Find the active user from the pistol's inventory/hand hierarchy.
	protected ChimeraCharacter FindEquippedPlayer()
	{
		IEntity weaponEntity = GetOwner();
		if (!weaponEntity)
			return null;

		IEntity ancestor = weaponEntity.GetParent();
		while (ancestor)
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(ancestor);
			if (character && IsEquippedBy(character))
				return character;
			ancestor = ancestor.GetParent();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void DetachShotListener()
	{
		if (!m_pFireEventUser)
			return;

		EventHandlerManagerComponent events = EventHandlerManagerComponent.Cast(m_pFireEventUser.FindComponent(EventHandlerManagerComponent));
		if (events)
			events.RemoveScriptHandler("OnProjectileShot", this, OnPlayerShot);

		m_pFireEventUser = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateShotListener()
	{
		if (!m_bUseWeaponTrigger)
			return;

		IEntity active = FindEquippedPlayer();
		if (active == m_pFireEventUser)
			return;

		DetachShotListener();
		if (!active)
			return;

		EventHandlerManagerComponent events = EventHandlerManagerComponent.Cast(active.FindComponent(EventHandlerManagerComponent));
		if (!events)
		{
			if (m_bDebug)
				Print("[RW GRAPPLE] Active player lacks EventHandlerManagerComponent", LogLevel.WARNING);
			return;
		}

		events.RegisterScriptHandler("OnProjectileShot", this, OnPlayerShot);
		m_pFireEventUser = active;
		if (m_bDebug)
			Print("[RW GRAPPLE] Pistol trigger listener attached to player");
	}

	//------------------------------------------------------------------------------------------------
	//! Scripted muzzle callback (immediate, if inherited pistol supplies it).
	protected void OnMuzzleFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		if (!Replication.IsServer())
			return;
		ProcessPistolShot(projectileEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla player event, delayed until the next frame: catches pistol
	//! variants that do not expose SCR_MuzzleEffectComponent.
	protected void OnPlayerShot(int playerId, BaseWeaponComponent weapon, IEntity projectileEntity)
	{
		if (!Replication.IsServer() || !weapon || weapon.GetOwner() != GetOwner())
			return;

		ProcessPistolShot(projectileEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessPistolShot(IEntity projectileEntity)
	{
		if (!m_bUseWeaponTrigger || !Replication.IsServer())
			return;

		ChimeraCharacter user = FindEquippedPlayer();
		if (!user)
			return;

		// Remove bullet whenever possible. The player event can be delayed
		// so normal M9 rounds must NOT be used around other players for testing.
		if (m_bDeleteProjectile && projectileEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(projectileEntity);

		int now = System.GetTickCount();
		if (now - m_iLastPistolShotMs < 100)
			return;

		m_iLastPistolShotMs = now;

		vector aimDirection = user.GetTransformAxis(2);
		AimingComponent aiming = AimingComponent.Cast(user.FindComponent(AimingComponent));
		if (!aiming)
			aiming = AimingComponent.Cast(GetOwner().FindComponent(AimingComponent));

		if (aiming)
		{
			vector weaponDirection = aiming.GetAimingDirectionWorld();
			if (weaponDirection.LengthSq() > 0.001)
				aimDirection = weaponDirection;
		}

		if (aimDirection.LengthSq() > 0.001)
			aimDirection.Normalize();

		if (m_bDebug)
			Print(string.Format("[RW GRAPPLE] Pistol trigger detected, aim=%1", aimDirection));

		ServerToggleWithDirection(user, aimDirection);
	}

	//------------------------------------------------------------------------------------------------
	bool IsEquippedBy(IEntity user)
	{
		if (!user || !GetOwner() || !ChimeraCharacter.Cast(user))
			return false;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(user.FindComponent(CharacterControllerComponent));
		if (!controller)
			return false;

		if (controller.GetRightHandItem() == GetOwner())
			return true;

		// Firearms use the weapon manager, not the gadget right-hand item.
		BaseWeaponComponent currentWeapon = SCR_WeaponLib.GetCurrentWeaponComponent(ChimeraCharacter.Cast(user));
		return currentWeapon && currentWeapon.GetOwner() == GetOwner();
	}

	//------------------------------------------------------------------------------------------------
	bool IsActiveFor(IEntity user)
	{
		return m_pUser && m_pUser == user;
	}

	//------------------------------------------------------------------------------------------------
	bool CanUse(IEntity user)
	{
		if (!IsEquippedBy(user))
			return false;

		// Let the same player release at any time, even during the cooldown.
		if (IsActiveFor(user))
			return true;

		if (m_pUser)
			return false;

		return System.GetTickCount() >= m_iNextUseMs;
	}

	//------------------------------------------------------------------------------------------------
	//! One activation toggles fire/cancel. ScriptedUserAction is executed on the server.
	bool ServerToggle(IEntity user)
	{
		if (!user)
			return false;
		return ServerToggleWithDirection(user, user.GetTransformAxis(2));
	}

	//------------------------------------------------------------------------------------------------
	protected bool ServerToggleWithDirection(IEntity user, vector shotDirection)
	{
		if (!Replication.IsServer() || !GetGame() || !IsEquippedBy(user))
			return false;

		if (IsActiveFor(user))
		{
			StopPull(true);
			return true;
		}

		if (!CanUse(user))
			return false;

		// Never accept an arbitrary AI/remote entity in lieu of the player's character.
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;

		int playerId = players.GetPlayerIdFromControlledEntity(user);
		if (playerId <= 0 || players.GetPlayerControlledEntity(playerId) != user)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world || !user.GetPhysics())
			return false;

		vector direction = shotDirection;
		if (direction.LengthSq() <= 0.0001)
			return false;
		direction.Normalize();

		vector start = user.GetOrigin() + vector.Up * Math.Clamp(m_fEyeHeight, 0.8, 2.2);
		float range = Math.Clamp(m_fMaxRange, 3, 150);
		vector limit = start + direction * range;

		TraceParam ray = new TraceParam();
		ray.Start = start;
		ray.End = limit;
		ray.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		ray.LayerMask = EPhysicsLayerPresets.Projectile;
		array<IEntity> excluded = {};
		excluded.Insert(user);
		excluded.Insert(GetOwner());
		ray.ExcludeArray = excluded;

		float fraction = world.TraceMove(ray, null);
		if (fraction >= 1.0)
		{
			if (m_bDebug)
				Print("[RW GRAPPLE] No surface within range", LogLevel.WARNING);
			return false;
		}

		vector anchor = start + (limit - start) * fraction;
		if (vector.Distance(start, anchor) < Math.Max(m_fStopDistance + 0.75, 3.0))
			return false;

		// Lift only if there is empty room above the player's head; don't force
		// a player up through a roof. The native physics pull begins after release.
		float lift = Math.Clamp(m_fGroundReleaseLift, 0.0, 1.2);
		if (!m_bScriptedPositionFallback && lift > 0.01)
		{
			TraceParam liftTrace = new TraceParam();
			liftTrace.Start = start;
			liftTrace.End = start + vector.Up * lift;
			liftTrace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			liftTrace.LayerMask = EPhysicsLayerPresets.Projectile;
			liftTrace.ExcludeArray = excluded;
			if (world.TraceMove(liftTrace, null) >= 1.0)
				user.SetOrigin(user.GetOrigin() + vector.Up * lift);
		}

		m_pUser = user;
		m_vAnchor = anchor;
		m_vLastPosition = user.GetOrigin();
		m_fNoMotionSeconds = 0;
		m_fReleaseDelay = 0.08;
		m_iStartMs = System.GetTickCount();
		m_iNextUseMs = m_iStartMs + (int)(Math.Max(m_fCooldown, 0.0) * 1000.0);

		if (m_bDebug)
			Print(string.Format("[RW GRAPPLE] player=%1 anchor=%2 range=%3", playerId, anchor, vector.Distance(start, anchor)));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void StopPull(bool playerCancelled)
	{
		if (m_pUser && !m_bScriptedPositionFallback)
		{
			Physics body = m_pUser.GetPhysics();
			if (body)
				body.SetVelocity(vector.Zero);
		}
		if (m_bDebug && m_pUser)
			Print(string.Format("[RW GRAPPLE] stopped cancelled=%1 at=%2", playerCancelled, m_pUser.GetOrigin()));

		m_pUser = null;
		m_fReleaseDelay = 0;
		m_fNoMotionSeconds = 0;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !GetGame())
			return;

		// Equipped pistol may change ownership when stored, dropped, or swapped.
		// No global hooks or changes to other weapons.
		UpdateShotListener();

		if (!m_pUser)
			return;

		if (!IsEquippedBy(m_pUser))
		{
			StopPull(false);
			return;
		}

		int elapsed = System.GetTickCount() - m_iStartMs;
		if (elapsed > (int)(Math.Max(m_fMaxDuration, 0.5) * 1000.0))
		{
			StopPull(false);
			return;
		}

		if (m_fReleaseDelay > 0.0)
		{
			m_fReleaseDelay -= Math.Max(timeSlice, 0.0);
			return;
		}

		Physics body = m_pUser.GetPhysics();
		BaseWorld world = GetGame().GetWorld();
		if (!body || !world)
		{
			StopPull(false);
			return;
		}

		vector position = m_pUser.GetOrigin();
		vector chest = position + vector.Up * 1.1;
		vector toward = m_vAnchor - chest;
		float remaining = toward.Length();

		if (remaining <= Math.Max(m_fStopDistance, 0.75))
		{
			StopPull(false);
			return;
		}

		toward.Normalize();
		float speed = Math.Clamp(m_fPullSpeed, 3.0, 40.0);
		float dt = Math.Clamp(timeSlice, 0.0, 0.1);
		vector step = toward * Math.Min(speed * dt, remaining);

		// Check the immediate path, not the entire rope. Stop before striking
		// collision so the grappler doesn't phase through a wall or rooftop.
		TraceParam clearance = new TraceParam();
		clearance.Start = chest;
		clearance.End = chest + step;
		clearance.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		clearance.LayerMask = EPhysicsLayerPresets.Projectile;
		array<IEntity> ignored = {};
		ignored.Insert(m_pUser);
		ignored.Insert(GetOwner());
		clearance.ExcludeArray = ignored;

		if (world.TraceMove(clearance, null) < 0.95)
		{
			StopPull(false);
			return;
		}

		if (m_bScriptedPositionFallback)
		{
			m_pUser.SetOrigin(position + step);
		}
		else
		{
			body.SetVelocity(toward * speed);
			float moved = vector.Distance(position, m_vLastPosition);
			if (moved < 0.10)
				m_fNoMotionSeconds += dt;
			else
				m_fNoMotionSeconds = 0;

			// Never silently teleport if the network/character controller rejects
			// imposed velocity. Let the tester opt into SetOrigin in Workbench.
			if (m_fNoMotionSeconds > 0.65)
			{
				if (m_bDebug)
					Print("[RW GRAPPLE] Native player motion blocked. Try Scripted Position Fallback in a disposable test world.", LogLevel.WARNING);
				StopPull(false);
				return;
			}
		}

		m_vLastPosition = position;
	}
}
