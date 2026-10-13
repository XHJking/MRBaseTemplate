// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "PICO_IMRModule.h"
#include "PICO_MR.h"

class FPICOOpenXRMRModule : public IPICOOpenXRMRModule
{
public:
	static inline FPICOOpenXRMRModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FPICOOpenXRMRModule>("PICOOpenXRMR");
	}

	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
	
};