
    #include "Roots.h"
    #include "RTS_Actor.h"
    #include "Engine/World.h"
    #include "Components/SplineComponent.h"
    #include "Components/SplineMeshComponent.h"
    #include "Engine/StaticMesh.h"
    #include "ConstructableModule/ConstructableModule.h"
    #include "Utilis/Libraries/RTSModuleFunctionLibrary.h"
    
    DEFINE_LOG_CATEGORY(LogRoot);
    
    URoots::URoots()
    {
    
    }
    
    void URoots::Initialize(ARTS_Actor* 
    InMainBuilding
    , ARTS_Actor* 
    InConstructionBuilding
    )
    {
        if (!InMainBuilding || !InConstructionBuilding)
        {
            UE_LOG(LogRoot, Warning, TEXT("Roots::Initialize: Invalid building parameters"));
            return;
        }
    
        MainBuilding = InMainBuilding;
        ConstructionBuilding = InConstructionBuilding;
    
        UE_LOG(LogRoot, Log, TEXT("Root initialized: MainBuilding=%s, ConstructionBuilding=%s"), 
            *MainBuilding->GetName(), *ConstructionBuilding->GetName());
    
        // Create spline component for roots
        RootsSpline = NewObject<USplineComponent>(MainBuilding);
        if (RootsSpline)
        {
            // Set the component to be movable for flexibility
            RootsSpline->SetMobility(EComponentMobility::Movable);
            
            // Register the component with the actor
            MainBuilding->AddInstanceComponent(RootsSpline);
            RootsSpline->RegisterComponent();
            
            // Attach to the main building's root component
            RootsSpline->AttachToComponent(MainBuilding->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
            
            // Make sure it's visible
            RootsSpline->SetVisibility(true);
            RootsSpline->SetHiddenInGame(false);
            
            // Set some visual properties
            RootsSpline->SetDrawDebug(true);
            
            UE_LOG(LogRoot, Log, TEXT("RootsSpline component created and registered"));
        }
    }
    
    void URoots::StartGrowing()
    {
        if (!MainBuilding || !ConstructionBuilding || !RootsSpline)
        {
            UE_LOG(LogRoot, Warning, TEXT("StartGrowing: Invalid state"));
            return;
        }
    
        UE_LOG(LogRoot, Log, TEXT("Starting root growth from %s to %s"), 
            *MainBuilding->GetName(), *ConstructionBuilding->GetName());
    
        // Clear existing spline points
        RootsSpline->ClearSplinePoints();
    
        // Add start point (main building)
        FVector StartLocation = MainBuilding->GetActorLocation();
        RootsSpline->AddSplinePoint(StartLocation, ESplineCoordinateSpace::World);
        UE_LOG(LogRoot, Log, TEXT("Added start point at: %s"), *StartLocation.ToString());
    
        // Add end point (construction building)
        FVector EndLocation = ConstructionBuilding->GetActorLocation();
        RootsSpline->AddSplinePoint(EndLocation, ESplineCoordinateSpace::World);
        UE_LOG(LogRoot, Log, TEXT("Added end point at: %s"), *EndLocation.ToString());
    
        // Update spline
        RootsSpline->UpdateSpline();
        
        // Log spline info
        float SplineLength = RootsSpline->GetSplineLength();
        UE_LOG(LogRoot, Log, TEXT("Spline created with length: %f"), SplineLength);
    
        // Reset growth state
        StemDistance = 0.0f;
        GrowthAlpha = 0.0f;
        CurrentStem = nullptr;
    
        // Start growth timer
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().SetTimer(GrowthTimerHandle, this, &URoots::GrowTick, GrowthTickInterval, true);
            UE_LOG(LogRoot, Log, TEXT("Growth timer started"));
        }
    }
    
    void URoots::GrowTick()
    {
        if (!MainBuilding || !ConstructionBuilding || !RootsSpline)
        {
            UE_LOG(LogRoot, Warning, TEXT("GrowTick: Invalid state, stopping growth"));
            StopGrowing();
            return;
        }
    
        float SplineLength = RootsSpline->GetSplineLength();
        
        // Check if we need more segments
        if (SplineLength <= StemDistance + SegmentLength)
        {
            // Root has reached the target - complete growth
            UE_LOG(LogRoot, Log, TEXT("Root growth completed to %s"), *ConstructionBuilding->GetName());
            StopGrowing();
            
            // Start self construction on the construction building
            if (UConstructableModule* ConstructableModule = URTSModuleFunctionLibrary::GetConstructableModule(ConstructionBuilding))
            {
                // Start self construction: 1 power every 1 second
                ConstructableModule->SelfConstruct();
                UE_LOG(LogRoot, Log, TEXT("Started self construction on %s"), *ConstructionBuilding->GetName());
            }
            else
            {
                UE_LOG(LogRoot, Warning, TEXT("No ConstructableModule found on %s"), *ConstructionBuilding->GetName());
            }
            
            // Broadcast completion event
            OnRootsCompleted.Broadcast(this);
            UE_LOG(LogRoot, Log, TEXT("Root growth completed, ready for cleanup"));
            return;
        }
    
        // Check if we need a new stem
        if (!CurrentStem)
        {
            // Create new spline mesh component for current stem
            CurrentStem = NewObject<USplineMeshComponent>(MainBuilding);
            if (CurrentStem)
            {
                // Set the SplineMeshComponent to be movable for flexibility
                CurrentStem->SetMobility(EComponentMobility::Movable);
                CurrentStem->RegisterComponent();
    
                // Register the component with the actor
                // MainBuilding->AddInstanceComponent(CurrentStem);
                // Attach to the spline component
                // CurrentStem->AttachToComponent(RootsSpline, FAttachmentTransformRules::KeepWorldTransform);
                
                // Set the mesh if one is configured
                if (RootMesh)
                {
                    CurrentStem->SetStaticMesh(RootMesh);
                    UE_LOG(LogRoot, Log, TEXT("Set root mesh: %s"), *RootMesh->GetName());
                }
                else
                {
                    UE_LOG(LogRoot, Warning, TEXT("No RootMesh configured!"));
                }
                
                // Make sure it's visible
                CurrentStem->SetVisibility(true);
                CurrentStem->SetHiddenInGame(false);
                
                UE_LOG(LogRoot, Log, TEXT("Created new root stem at distance: %f"), StemDistance);
            }
        }
    
        if (CurrentStem)
        {
            // Calculate start position and tangent
            FVector StartPos = RootsSpline->GetLocationAtDistanceAlongSpline(StemDistance, ESplineCoordinateSpace::Local);
            FVector StartTangent = RootsSpline->GetDirectionAtDistanceAlongSpline(StemDistance, ESplineCoordinateSpace::Local);
    
            // Calculate end position (lerped based on growth alpha)
            FVector EndPosBase = RootsSpline->GetLocationAtDistanceAlongSpline(StemDistance + SegmentLength, ESplineCoordinateSpace::Local);
            FVector EndPos = FMath::Lerp(StartPos, EndPosBase, GrowthAlpha);
    
            // Calculate end tangent
            FVector EndTangent = RootsSpline->GetDirectionAtDistanceAlongSpline(StemDistance + SegmentLength, ESplineCoordinateSpace::Local);
    
            // Update spline mesh
            CurrentStem->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
    
            // Update growth alpha
            GrowthAlpha += GrowthRate * GetWorld()->GetDeltaSeconds();
    
            // Check if current segment is complete
            if (GrowthAlpha >= 1.0f)
            {
                GrowthAlpha = 0.0f;
                StemDistance += SegmentLength;
                CurrentStem = nullptr;
                UE_LOG(LogRoot, Log, TEXT("Completed root segment, moving to distance: %f"), StemDistance);
            }
            
            // Log current growth progress every tick
            UE_LOG(LogRoot, Log, TEXT("Root growth tick - Distance: %f, Alpha: %f, Spline Length: %f"), 
                StemDistance, GrowthAlpha, SplineLength);
        }
    }
    
    void URoots::StopGrowing()
    {
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(GrowthTimerHandle);
            UE_LOG(LogRoot, Log, TEXT("Growth timer stopped"));
        }
        
        CurrentStem = nullptr;
    }
    
    UWorld* URoots::GetWorld() const
    {
        if (MainBuilding)
        {
            return MainBuilding->GetWorld();
        }
        return nullptr;
    }
    
    
    