// Warlord Game Master attribute registration, entity-only and multiplayer-safe.
// Extends the vanilla editor attribute manager at script initialization.
// No override of the shared EditorModeEdit.et resource or global Edit.conf.
modded class SCR_AttributesManagerEditorComponentClass
{
    protected bool m_bRW_WarlordAttributesAppended;

    void RW_AppendWarlordAttributes(notnull SCR_EditorAttributeList list)
    {
        if (m_bRW_WarlordAttributesAppended)
            return;

        m_bRW_WarlordAttributesAppended = true;
        list.InsertAllAttributes(m_aAttributes);
    }
}

modded class SCR_AttributesManagerEditorComponent
{
    protected static const ResourceName RW_WARLORD_ATTRIBUTES = "{AB12112600000002}Configs/Editor/AttributeLists/RW_Warlord_Edit.conf";
    protected static ref SCR_EditorAttributeList s_RW_WarlordAttributes;

    // Register before vanilla initialization so it initializes our classes on
    // both clients (GM UI) and the authoritative server (WriteVariable).
    override void EOnEditorInit()
    {
        RW_RegisterWarlordAttributes();
        super.EOnEditorInit();
    }

    override void EOnEditorInitServer()
    {
        RW_RegisterWarlordAttributes();
        super.EOnEditorInitServer();
    }

    protected void RW_RegisterWarlordAttributes()
    {
        SCR_AttributesManagerEditorComponentClass data =
            SCR_AttributesManagerEditorComponentClass.Cast(GetEditorComponentData());
        if (!data)
            return;

        if (!s_RW_WarlordAttributes)
            s_RW_WarlordAttributes = RW_LoadWarlordAttributes();

        if (s_RW_WarlordAttributes)
            data.RW_AppendWarlordAttributes(s_RW_WarlordAttributes);
    }

    protected SCR_EditorAttributeList RW_LoadWarlordAttributes()
    {
        Resource resource = Resource.Load(RW_WARLORD_ATTRIBUTES);
        if (!resource || !resource.IsValid())
        {
            Print("[RW WARLORD GM] Attribute resource missing: " + RW_WARLORD_ATTRIBUTES, LogLevel.WARNING);
            return null;
        }

        BaseResourceObject sourceResource = resource.GetResource();
        if (!sourceResource)
            return null;

        SCR_EditorAttributeList list = SCR_EditorAttributeList.Cast(
            BaseContainerTools.CreateInstanceFromContainer(sourceResource.ToBaseContainer()));
        if (!list)
            Print("[RW WARLORD GM] Attribute list failed to load from resource container", LogLevel.WARNING);
        else
            // Successful registration is intentionally quiet; warnings above remain visible.

        return list;
    }
}
