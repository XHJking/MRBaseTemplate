// Copyright Epic Games, Inc. All Rights Reserved.

#include "PICO_LiveLinkHandTrackingSourceFactory.h"
#include "PICO_IHandTrackingModule.h"

#include "ILiveLinkClient.h"

#define LOCTEXT_NAMESPACE "PICOOpenXRHandTracking"

FText ULiveLinkHandTrackingSourceFactoryPICO::GetSourceDisplayName() const
{
	return LOCTEXT("HandTrackingLiveLinkSourceName", "Windows Mixed Reality Hand Tracking Source");
}

FText ULiveLinkHandTrackingSourceFactoryPICO::GetSourceTooltip() const
{
	return LOCTEXT("HandTrackingLiveLinkSourceTooltip", "Windows Mixed Reality Hand Tracking Key Points Source");
}

ULiveLinkHandTrackingSourceFactoryPICO::EMenuType ULiveLinkHandTrackingSourceFactoryPICO::GetMenuType() const
{
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		ILiveLinkClient& LiveLinkClient = IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);

		if (!IPICOOpenXRHandTrackingModule::Get().IsLiveLinkSourceValid() || !LiveLinkClient.HasSourceBeenAdded(IPICOOpenXRHandTrackingModule::Get().GetLiveLinkSource()))
		{
			return EMenuType::MenuEntry;
		}
	}
	return EMenuType::Disabled;
}

TSharedPtr<ILiveLinkSource> ULiveLinkHandTrackingSourceFactoryPICO::CreateSource(const FString& ConnectionString) const
{
	return IPICOOpenXRHandTrackingModule::Get().GetLiveLinkSource();
}

#undef LOCTEXT_NAMESPACE
