// Copyright AmberleafCotton 2025. All Rights Reserved.
#pragma once

#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Roots.generated.h"

class ARTS_Actor;
class UConstructableModule;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRootsCompleted, URoots*, CompletedRoots);

DECLARE_LOG_CATEGORY_EXTERN(LogRoot, Log, All);

/**
 * Roots UObject manages individual root growth between buildings.
 * Handles spline creation, growth animation, and construction completion.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class DRAKTHYSPROJECT_API URoots : public UObject
{
	GENERATED_BODY()

public:
	URoots();

	// Initialize the root with main building and construction building
	UFUNCTION(BlueprintCallable, Category = "Roots")
	void Initialize(ARTS_Actor* InMainBuilding, ARTS_Actor* InConstructionBuilding);

	// Start the root growth process
	UFUNCTION(BlueprintCallable, Category = "Roots")
	void StartGrowing();

	// Stop the root growth process
	UFUNCTION(BlueprintCallable, Category = "Roots")
	void StopGrowing();

	UPROPERTY(BlueprintAssignable, Category = "Roots")
	FOnRootsCompleted OnRootsCompleted;

	// Root growth parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roots Config")
	UStaticMesh* RootMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roots Config")
	float SegmentLength = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roots Config")
	float GrowthRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roots Config")
	float GrowthTickInterval = 0.05f;

	// Get the world for timer management
	UWorld* GetWorld() const override;

protected:
	// Growth tick function
	UFUNCTION()
	void GrowTick();

private:
	// Building references
	UPROPERTY()
	ARTS_Actor* MainBuilding = nullptr;

	UPROPERTY()
	ARTS_Actor* ConstructionBuilding = nullptr;

	// Spline component for root path
	UPROPERTY()
	USplineComponent* RootsSpline = nullptr;

	// Current growing stem
	UPROPERTY()
	USplineMeshComponent* CurrentStem = nullptr;

	// Growth state
	float StemDistance = 0.0f;
	float GrowthAlpha = 0.0f;

	// Timer handle for growth
	FTimerHandle GrowthTimerHandle;
};
