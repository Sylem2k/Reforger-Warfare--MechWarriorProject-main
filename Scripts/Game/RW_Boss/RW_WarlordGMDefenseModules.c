// Warlord-only wrappers for autonomous defensive weapons.
// Their Core implementations are unchanged for every other mech.
[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Point defense respecting Warlord GM story fire profile.")]
class RW_WarlordGMPointDefenseClass : BFA_MechPointDefenseModuleComponentClass
{
}

class RW_WarlordGMPointDefense : BFA_MechPointDefenseModuleComponent
{
    protected bool StoryAllows()
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(m_pMechOwner);
        return !director || director.AllowsDefenses();
    }

    override protected void ScanTick()
    {
        if (!StoryAllows())
        {
            // Keep scan delta short on reactivation.
            m_iLastScanMs = GetWorldTimeMs();
            return;
        }
        super.ScanTick();
    }

    override protected void FireManualBurst()
    {
        if (!StoryAllows())
            return;
        super.FireManualBurst();
    }
}

[ComponentEditorProps(category: "Reforger Warfare/Boss", description: "Seismic stomp respecting Warlord GM story fire profile.")]
class RW_WarlordGMStompClass : CGS_ForcePulseModuleComponentClass
{
}

class RW_WarlordGMStomp : CGS_ForcePulseModuleComponent
{
    protected bool StoryAllows()
    {
        RW_WarlordGMDirectorComponent director = RW_WarlordGMDirectorComponentClass.Find(m_pMechOwner);
        return !director || director.AllowsDefenses();
    }

    override protected bool TryFireShot(IEntity instigator)
    {
        if (!StoryAllows())
            return false;
        return super.TryFireShot(instigator);
    }

    // A GM may switch to Hold Fire after a stomp has already started charging.
    override protected void Pulse(int token)
    {
        if (!StoryAllows())
        {
            if (token == m_iToken)
                m_bCharging = false;
            return;
        }
        super.Pulse(token);
    }
}
