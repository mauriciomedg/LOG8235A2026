// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SoftDesignTrainingTarget : TargetRules
{
  public SoftDesignTrainingTarget(TargetInfo Target) : base(Target)
  {
    Type = TargetType.Game;
	IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
    DefaultBuildSettings = BuildSettingsVersion.V7;
    ExtraModuleNames.Add("SoftDesignTraining");
  }
}
