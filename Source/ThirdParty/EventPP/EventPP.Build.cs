using System.IO;
using UnrealBuildTool;

public class EventPP: ModuleRules {
    public EventPP(ReadOnlyTargetRules Target) : base(Target) {
        Type = ModuleType.External;
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
    }
}
