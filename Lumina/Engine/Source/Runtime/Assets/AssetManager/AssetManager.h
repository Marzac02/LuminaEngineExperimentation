#pragma once

#include "EASTL/atomic.h"
#include "Assets/AssetRequest.h"
#include "Containers/Array.h"
#include "Core/Threading/Thread.h"
#include "Memory/SmartPtr.h"
#include "Subsystems/Subsystem.h"


namespace Lumina
{
	class FAssetRecord;
	
	class LUMINA_API FAssetManager : public ISubsystem
	{
	public:

		FAssetManager();
		~FAssetManager() override;

		void Initialize() override;
		void Deinitialize() override;
		TSharedPtr<FAssetRequest> LoadAsset(const FString& InAssetPath);

		void NotifyAssetRequestCompleted(const TSharedPtr<FAssetRequest>& Request);

		void FlushAsyncLoading();
		
	private:

		TSharedPtr<FAssetRequest> TryFindActiveRequest(const FString& InAssetPath, bool& bAlreadyInQueue);
		void SubmitAssetRequest(const TSharedPtr<FAssetRequest>& Request);
	
	private:
		
		THashSet<TSharedPtr<FAssetRequest>>	ActiveRequests;
		eastl::atomic<int32>				OutstandingTasks{0};
		
	};
	

	//--------------------------------------------------------------------------
	// Template definitions.
	//--------------------------------------------------------------------------
	
}
