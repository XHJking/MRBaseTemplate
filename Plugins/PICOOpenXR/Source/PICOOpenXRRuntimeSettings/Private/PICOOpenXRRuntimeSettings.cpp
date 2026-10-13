// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICOOpenXRRuntimeSettings.h"

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManager.h"
#include "Engine/RendererSettings.h"
#include "Misc/ConfigUtilities.h"
#include "Engine/RendererSettings.h"

DEFINE_LOG_CATEGORY(LogPICOOpenXRSettings);

IMPLEMENT_MODULE(FDefaultModuleImpl, PICOOpenXRRuntimeSettings);

UPICOOpenXRRuntimeSettings::UPICOOpenXRRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(TEXT("/Script/PICOOpenXRRuntimeSettings.PICOOpenXRRuntimeSettings"), *GEngineIni, ECVF_SetByProjectSetting);
}

void UPICOOpenXRRuntimeSettings::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	if (GConfig)
	{
		const FString Value = FString("<meta-data android:name=\"") + "pvr.app.type" + "\" android:value=\"" + "vr" + "\" />";
		const FString DefaultEnginePath = FConfigCacheIni::NormalizeConfigIniPath(FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini")));
		GConfig->LoadFile(DefaultEnginePath);
		GConfig->SetString(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("ExtraActivitySettings"), *Value, DefaultEnginePath);
		GConfig->Flush(false);
		GConfig->UnloadFile(DefaultEnginePath);
	}

	ToggleOcclusionCulling();
#endif
}

bool UPICOOpenXRRuntimeSettings::GetBoolConfigByKey(const FString& InKeyName)
{
	if(const FConfigSection* Section = GConfig->GetSection(TEXT("/Script/PICOOpenXRRuntimeSettings.PICOOpenXRRuntimeSettings"), false, GEngineIni))
	{
		for(FConfigSectionMap::TConstIterator It(*Section); It; ++It)
		{
			const FString& KeyString = It.Key().GetPlainNameString(); 
			const FString& ValueString = It.Value().GetValue();

			if (KeyString==InKeyName)
			{
				const FString& NewValueString = UE::ConfigUtilities::ConvertValueFromHumanFriendlyValue(*ValueString);

				if (NewValueString == TEXT("1"))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UPICOOpenXRRuntimeSettings::ToggleOcclusionCulling()
{
	URendererSettings* RendererSettings = GetMutableDefault<URendererSettings>();
	RendererSettings->bOcclusionCulling = !bDisableOcclusionCulling;
	RendererSettings->UpdateSinglePropertyInConfigFile(RendererSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(URendererSettings, bOcclusionCulling)), RendererSettings->GetDefaultConfigFilename());
}

#if WITH_EDITOR
void UPICOOpenXRRuntimeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GConfig->Flush(false);

	FString Dst = FPaths::ProjectDir() / TEXT("pico_splash.png");
	if (!(bUsingOSSplash && IFileManager::Get().Copy(*Dst, *OSSplashScreen.FilePath, true) == COPY_OK))
	{
		IFileManager::Get().Delete(*Dst, true);
	}

	static const FName EnableSemanticsAlignWithVertex = GET_MEMBER_NAME_CHECKED(UPICOOpenXRRuntimeSettings, bSemanticsAlignWithVertex);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == EnableSemanticsAlignWithVertex)
	{
		if (bSemanticsAlignWithVertex == true)
		{
			bSemanticsAlignWithTriangle = false;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UPICOOpenXRRuntimeSettings, bSemanticsAlignWithTriangle)), GetDefaultConfigFilename());
		}
	}

	static const FName EnableSemanticsAlignWithTriangle = GET_MEMBER_NAME_CHECKED(UPICOOpenXRRuntimeSettings, bSemanticsAlignWithTriangle);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == EnableSemanticsAlignWithTriangle)
	{
		if (bSemanticsAlignWithTriangle == true)
		{
			bSemanticsAlignWithVertex = false;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UPICOOpenXRRuntimeSettings, bSemanticsAlignWithVertex)), GetDefaultConfigFilename());
		}
	}

	ToggleOcclusionCulling();
}

#endif
