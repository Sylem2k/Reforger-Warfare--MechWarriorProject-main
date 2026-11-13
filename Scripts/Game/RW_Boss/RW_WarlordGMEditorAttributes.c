// Warlord Game Master entity-edit controls, following the working
// CGS-Auto-Turrets Editor Attributes pattern. Hide rows on non-Warlord entities.
// Values are submitted to the authoritative server by GM's attribute manager.

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMWeaponsEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateInt(director.GetWeaponProfile());
    }

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        array<string> options = { "All Weapons", "Missiles Only", "Lasers Only", "Cannons Only", "Hold Fire" };
        m_aValues.Clear();
        foreach (int i, string option : options)
        {
            SCR_EditorAttributeFloatStringValueHolder entry = new SCR_EditorAttributeFloatStringValueHolder();
            entry.SetName(option);
            entry.SetFloatValue(i);
            m_aValues.Insert(entry);
        }
        outEntries.Insert(new SCR_BaseEditorAttributeFloatStringValues(m_aValues));
        return outEntries.Count();
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetWeaponProfile(var.GetInt());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMInaccuracyEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(director.GetInaccuracy());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetInaccuracy(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMMovementEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateInt(director.GetMovementOrder());
    }

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        array<string> options = { "Autonomous Patrol", "Stop / Hold Here", "GM Waypoints Only" };
        m_aValues.Clear();
        foreach (int i, string option : options)
        {
            SCR_EditorAttributeFloatStringValueHolder entry = new SCR_EditorAttributeFloatStringValueHolder();
            entry.SetName(option);
            entry.SetFloatValue(i);
            m_aValues.Insert(entry);
        }
        outEntries.Insert(new SCR_BaseEditorAttributeFloatStringValues(m_aValues));
        return outEntries.Count();
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetMovementOrder(var.GetInt());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMArrivalEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(director.GetWaypointArrival());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetWaypointArrival(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMVehicleOnlyEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateBool(director.GetVehicleTargetsOnly());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetVehicleTargetsOnly(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMTopCannonsEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateBool(director.GetTopCannonsEnabled());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetTopCannonsEnabled(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMDefensesEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateBool(director.GetDefensesEnabled());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetDefensesEnabled(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMWarhornEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateBool(director.GetWarhornEnabled());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetWarhornEnabled(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMLaserChargeEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateBool(director.GetLaserChargeEnabled());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetLaserChargeEnabled(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMLaserChargeSecondsEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(director.GetLaserChargeSeconds());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetLaserChargeSeconds(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMVolleyPauseScaleEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(director.GetVolleyPauseScale());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetVolleyPauseScale(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class RW_WarlordGMExtraAimSettleEditorAttribute : SCR_BaseValueListEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (!director)
            return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(director.GetAimSettleExtraSeconds());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.FromEditorItem(item);
        if (director && var)
            director.SetAimSettleExtraSeconds(var.GetFloat());
    }
}
