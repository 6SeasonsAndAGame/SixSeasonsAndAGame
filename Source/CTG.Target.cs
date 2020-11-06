// None

using UnrealBuildTool;
using System.Collections.Generic;

public class CTGTarget : TargetRules
{
	public CTGTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "CTG" } );
	}
}
