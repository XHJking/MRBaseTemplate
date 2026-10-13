// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IOpenXRExtensionPlugin.h"
#include "PICO_InputFunctionLibrary.h"
#include "IOpenXRExtensionPluginDelegates.h"
#include "Runtime/Launch/Resources/Version.h"

class FControllerPICO : public IOpenXRExtensionPlugin
{
public:
	FControllerPICO();
	virtual ~FControllerPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("ControllerPICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostCreateInstance(XrInstance InInstance) override;
	virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext);
	virtual void PostCreateSession(XrSession InSession) override;
	virtual bool GetInteractionProfiles(XrInstance InInstance, TArray<FString>& OutKeyPrefixes, TArray<XrPath>& OutPaths, TArray<bool>& OutHasHaptics);
	virtual bool GetSuggestedBindings(XrPath InInteractionProfile, TArray<XrActionSuggestedBinding>& OutBindings);
	virtual void AttachActionSets(TSet<XrActionSet>& OutActionSets) override;
	virtual void GetActiveActionSetsForSync(TArray<XrActiveActionSet>& OutActiveSets) override;
	virtual void PostSyncActions(XrSession InSession) override;
	
	bool GetControllerBatteryLevel(const EControllerHand Hand, float& Level);
	void AddProfile(XrPath Profile);

private:
	XrSession Session = XR_NULL_HANDLE;
	XrInstance Instance = XR_NULL_HANDLE;
	XrActionsSyncInfo SyncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
	XrAction ControllerBatteryAction = XR_NULL_HANDLE;
	XrActionSet ControllerActionSet = XR_NULL_HANDLE;
	TArray<XrPath> SubactionPaths;
	TArray<XrPath> Profiles;
	TArray<XrActionStateFloat> ActionStateFloats;
	bool SuggestedBindings;
};