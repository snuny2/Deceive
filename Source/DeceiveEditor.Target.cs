using UnrealBuildTool;

public class DeceiveEditorTarget : TargetRules
{
	public DeceiveEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Deceive");
	}
}
