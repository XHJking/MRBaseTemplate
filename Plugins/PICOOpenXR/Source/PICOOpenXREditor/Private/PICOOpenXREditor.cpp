// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICOOpenXRRuntimeSettings.h"
#include "Modules/ModuleInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "OpenXRHMDSettings.h"

#define LOCTEXT_NAMESPACE "FPICOOpenXREditorModule"

class FPICOOpenXREditorModule
    : public IModuleInterface
{
    virtual void StartupModule() override
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

        if (SettingsModule != nullptr)
        {
            SettingsModule->RegisterSettings("Project", "Plugins", "PICOOpenXR",
                LOCTEXT("RuntimeSettingsName", "PICO OpenXR"),
                LOCTEXT("RuntimeSettingsDescription", "Project settings for PICO OpenXR Extension plugin"),
                GetMutableDefault<UPICOOpenXRRuntimeSettings>()
            );
        }

        UOpenXRHMDSettings* OpenXRHMDSettings = GetMutableDefault<UOpenXRHMDSettings>();
        if (OpenXRHMDSettings)
        {
            OpenXRHMDSettings->bIsFBFoveationEnabled = true; 
            OpenXRHMDSettings->UpdateSinglePropertyInConfigFile(OpenXRHMDSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOpenXRHMDSettings, bIsFBFoveationEnabled)), OpenXRHMDSettings->GetDefaultConfigFilename());
        }
    }

    virtual void ShutdownModule() override
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

        if (SettingsModule != nullptr)
        {
            SettingsModule->UnregisterSettings("Project", "Plugins", "PICOOpenXR");
        }
    }
};

IMPLEMENT_MODULE(FPICOOpenXREditorModule, PICOOpenXREditor);

#undef LOCTEXT_NAMESPACE