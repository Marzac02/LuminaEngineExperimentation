#pragma once

#include "Assets/AssetRequest.h"
#include "Containers/Array.h"
#include "Memory/SmartPtr.h"


namespace Lumina
{
	class FAssetRecord;
	
	class LUMINA_API FAssetManager final
	{
	public:

		FAssetManager();

		static FAssetManager& Get();
		
		TSharedPtr<FAssetRequest> LoadAsset(const FString& PackagePath, const FGuid& RequestedAsset);

		void NotifyAssetRequestCompleted(const TSharedPtr<FAssetRequest>& Request);

		void FlushAsyncLoading();
		
	private:

		TSharedPtr<FAssetRequest> TryFindActiveRequest(const FString& InAssetPath, const FGuid& GUID, bool& bAlreadyInQueue);
		void SubmitAssetRequest(const TSharedPtr<FAssetRequest>& Request);
	
	private:

		FMutex								RequestMutex;
		THashSet<TSharedPtr<FAssetRequest>>	ActiveRequests;
		
	};
	

	//--------------------------------------------------------------------------
	// Template definitions.
	//--------------------------------------------------------------------------
	
}
