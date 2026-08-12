using UnrealBuildTool;

public class ReEchoAudio : ModuleRules
{
	public ReEchoAudio(ReadOnlyTargetRules Target) : base(Target)
	{
		// UE 5.8 requires a secondary runtime module to provide an explicit PCH.
		// Place it under Private/ and reference it here so every TU is force-fed
		// CoreUObject + Engine + the core ticker (GENERATED_BODY / FTicker, etc.).
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/ReEchoAudioPrivatePCH.h";

		// ReEchoAudio is a standalone runtime module. It must NOT depend on the
		// gameplay module ReEcho: gameplay publishes semantic event/state intent
		// only. See plans/33-audio-runtime-module-foundation.md.
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		// AudioMixer is pulled in so a future synth/procedural backend (per
		// shared/LESSONS.md AUDIO-U9) does not require a Build.cs change.
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AudioMixer"
		});
	}
}
