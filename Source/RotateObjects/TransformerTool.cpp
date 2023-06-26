/*
MIT License

Copyright(c) 2020 Juan Marcelo Portillo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "TransformerTool.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

/* Gizmos */
#include "Gizmos/BaseGizmo.h"
#include "Gizmos/TranslationGizmo.h"
#include "Gizmos/RotationGizmo.h"
#include "Gizmos/ScaleGizmo.h"

#define LOCTEXT_NAMESPACE "FRuntimeTransformerModule"

DEFINE_LOG_CATEGORY(LogRuntimeTransformer);

#undef LOCTEXT_NAMESPACE

#define RTT_LOG(LogType, x, ...)  UE_LOG(LogRuntimeTransformer, LogType, TEXT(x), __VA_ARGS__)

// Sets default values
UTransformerTool::UTransformerTool()
{

	GizmoPlacement			= EGizmoPlacement::GP_OnLastSelection;
	CurrentTransformation	= ETransformationType::TT_Translation;
	CurrentDomain			= ETransformationDomain::TD_None;
	CurrentSpaceType		= ESpaceType::ST_World;
	TranslationGizmoClass	= ATranslationGizmo::StaticClass();
	RotationGizmoClass		= ARotationGizmo::StaticClass();
	ScaleGizmoClass			= AScaleGizmo::StaticClass();

	CloneReplicationCheckFrequency = 0.05f;
	MinimumCloneReplicationTime = 0.01f;

	bResyncSelection = false;
	bIgnoreNonReplicatedObjects = false;

	ResetDeltaTransform(AccumulatedDeltaTransform);
	ResetDeltaTransform(NetworkDeltaTransform);

	SetTransformationType(CurrentTransformation);
	SetSpaceType(CurrentSpaceType);

	bTransformUFocusableObjects = true;
	bRotateOnLocalAxis = false;
	bForceMobility = false;
	bToggleSelectedInMultiSelection = true;
	bComponentBased = false;
}

void UTransformerTool::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//Fill here if we need to replicate Properties. For now, nothing needs constant replication/check
}


void UTransformerTool::SetTransform(USceneComponent* Component, const FTransform& Transform)
{
	if (!Component) return;

    Component->SetWorldTransform(Transform);

}

void UTransformerTool::FilterHits(TArray<FHitResult>& outHits)
{
	//eliminate all outHits that have non-replicated objects
	if (bIgnoreNonReplicatedObjects)
	{
		TArray<FHitResult> checkHits = outHits;
		for (int32 i = 0; i < checkHits.Num(); ++i)
		{
			//don't remove Gizmos! They do not replicate by default 
			if (Cast<ABaseGizmo>(checkHits[i].GetActor()))
				continue;

			
			if (checkHits[i].GetActor()->IsSupportedForNetworking())
			{
				if (bComponentBased)
				{
					if (checkHits[i].Component->IsSupportedForNetworking())
						continue; //components - actor owner + themselves need to replicate
				}
				else
					continue; //actors only consider whether they replicate
			}
			/*RTT_LOG(Warning, "Removing (Actor: %s   ComponentHit:  %s) from hits because it is not supported for networking."
				, *checkHits[i].GetActor->GetName(), *checkHits[i].Component->GetName());*/
			outHits.RemoveAt(i);
		}
	}
}

void UTransformerTool::SetSpaceType(ESpaceType Type)
{
	CurrentSpaceType = Type;
	SetGizmo();

    // added by Talad
    if (Gizmo.IsValid() && playerController)
    {
        Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
            , playerController->PlayerCameraManager->GetActorForwardVector()
            , playerController->PlayerCameraManager->GetFOVAngle());
    }
}

ETransformationDomain UTransformerTool::GetCurrentDomain(bool& TransformInProgress) const
{
	TransformInProgress = (CurrentDomain != ETransformationDomain::TD_None);
	return CurrentDomain;
}

void UTransformerTool::ClearDomain()
{
	//Clear the Accumulated tranform when we stop Transforming
	ResetDeltaTransform(AccumulatedDeltaTransform);
	SetDomain(ETransformationDomain::TD_None);
}

bool UTransformerTool::GetMouseStartEndPoints(float TraceDistance, FVector& outStartPoint, FVector& outEndPoint)
{
	if (playerController)
	{
		FVector worldLocation, worldDirection;
		if (playerController->DeprojectMousePositionToWorld(worldLocation, worldDirection))
		{
			outStartPoint = worldLocation;
			outEndPoint = worldLocation + (worldDirection * TraceDistance);
			return true;
		}
	}
	return false;
}

UClass* UTransformerTool::GetGizmoClass(ETransformationType TransformationType) const /* private */
{
	//Assign correct Gizmo Class depending on given Transformation
	switch (CurrentTransformation)
	{
	case ETransformationType::TT_Translation:	return TranslationGizmoClass;
	case ETransformationType::TT_Rotation:		return RotationGizmoClass;
	case ETransformationType::TT_Scale:			return ScaleGizmoClass;
	default:									return nullptr;
	}
}

void UTransformerTool::ResetDeltaTransform(FTransform& Transform)
{
	Transform = FTransform();
	Transform.SetScale3D(FVector::ZeroVector);
}

void UTransformerTool::SetDomain(ETransformationDomain Domain)
{
	CurrentDomain = Domain;

	if (Gizmo.IsValid())
		Gizmo->SetTransformProgressState(CurrentDomain != ETransformationDomain::TD_None
			, CurrentDomain);
}

bool UTransformerTool::MouseTraceByObjectTypes(float TraceDistance
	, TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels
	, TArray<AActor*> IgnoredActors, bool bAppendToList)
{
	FVector start, end;
	bool bTraceSuccessful = false;
	if (GetMouseStartEndPoints(TraceDistance, start, end))
	{
		bTraceSuccessful = TraceByObjectTypes(start, end, CollisionChannels
			, IgnoredActors, bAppendToList);

		if (!bTraceSuccessful && !bAppendToList)
		    DeselectAll(false);
	}
	return bTraceSuccessful;
}

bool UTransformerTool::MouseTraceByChannel(float TraceDistance
	, TEnumAsByte<ECollisionChannel> TraceChannel, TArray<AActor*> IgnoredActors
	, bool bAppendToList)
{
	FVector start, end;
	bool bTraceSuccessful = false;
	if (GetMouseStartEndPoints(TraceDistance, start, end))
	{
		bTraceSuccessful = TraceByChannel(start, end, TraceChannel
			, IgnoredActors, bAppendToList);
		if (!bTraceSuccessful && !bAppendToList)
		    DeselectAll(false);
	}
	return false;
}

bool UTransformerTool::MouseTraceByProfile(float TraceDistance
	, const FName& ProfileName
	, TArray<AActor*> IgnoredActors
	, bool bAppendToList)
{
	FVector start, end;
	bool bTraceSuccessful = false;
	if (GetMouseStartEndPoints(TraceDistance, start, end))
	{

		bTraceSuccessful = TraceByProfile(start, end, ProfileName
			, IgnoredActors, bAppendToList);
		if (!bTraceSuccessful && !bAppendToList)
		    DeselectAll(false);

	}
	return bTraceSuccessful;
}

bool UTransformerTool::TraceByObjectTypes(const FVector& StartLocation
	, const FVector& EndLocation
	, TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels
	, TArray<AActor*> IgnoredActors
	, bool bAppendToList)
{
	if (UWorld* world = GetWorld())
	{
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		FCollisionQueryParams CollisionQueryParams;

		//Add All Given Collisions to the Array
		for (auto& cc : CollisionChannels)
			CollisionObjectQueryParams.AddObjectTypesToQuery(cc);

		CollisionQueryParams.AddIgnoredActors(IgnoredActors);

		TArray<FHitResult> OutHits;
		if (world->LineTraceMultiByObjectType(OutHits, StartLocation, EndLocation
			, CollisionObjectQueryParams, CollisionQueryParams))
		{
			FilterHits(OutHits);
			return HandleTracedObjects(OutHits, bAppendToList);
		}
	}
	return false;
}

bool UTransformerTool::TraceByChannel(const FVector& StartLocation
	, const FVector& EndLocation
	, TEnumAsByte<ECollisionChannel> TraceChannel
	, TArray<AActor*> IgnoredActors
	, bool bAppendToList)
{
	if (UWorld* world = GetWorld())
	{
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors(IgnoredActors);

		TArray<FHitResult> OutHits;
		if (world->LineTraceMultiByChannel(OutHits, StartLocation, EndLocation
			, TraceChannel, CollisionQueryParams))
		{
			FilterHits(OutHits);
			return HandleTracedObjects(OutHits, bAppendToList);
		}
	}
	return false;
}

bool UTransformerTool::TraceByProfile(const FVector& StartLocation
	, const FVector& EndLocation
	, const FName& ProfileName, TArray<AActor*> IgnoredActors
	, bool bAppendToList)
{
	if (UWorld* world = GetWorld())
	{
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors(IgnoredActors);

		TArray<FHitResult> OutHits;
		if (world->LineTraceMultiByProfile(OutHits, StartLocation, EndLocation
			, ProfileName, CollisionQueryParams))
		{
			FilterHits(OutHits);
			return HandleTracedObjects(OutHits, bAppendToList);
		}
	}
	return false;
}

#include "Kismet/GameplayStatics.h"

// was inside Tick()
void UTransformerTool::MoveGizmoAction()
{
    if (!Gizmo.IsValid()) return;

    if (playerController)
    {
        FVector worldLocation, worldDirection;
        if (playerController->IsLocalController() && playerController->PlayerCameraManager)
        {
            if (playerController->DeprojectMousePositionToWorld(worldLocation, worldDirection))
            {
                FTransform deltaTransform = UpdateTransform(playerController->PlayerCameraManager->GetActorForwardVector()
                    , worldLocation, worldDirection);

                NetworkDeltaTransform = FTransform(
                    deltaTransform.GetRotation() * NetworkDeltaTransform.GetRotation(),
                    deltaTransform.GetLocation() + NetworkDeltaTransform.GetLocation(),
                    deltaTransform.GetScale3D() + NetworkDeltaTransform.GetScale3D());
            }

        }
    }

    //Only consider Local View
    if (playerController)
    {
        if (playerController->PlayerCameraManager)
        {
            Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
                , playerController->PlayerCameraManager->GetActorForwardVector()
                , playerController->PlayerCameraManager->GetFOVAngle());
        }
    }

    Gizmo->UpdateGizmoSpace(CurrentSpaceType); //ToDo: change when this is called to improve performance when a gizmo is there without doing anything
}


FTransform UTransformerTool::UpdateTransform(const FVector& LookingVector
	, const FVector& RayOrigin
	, const FVector& RayDirection)
{
	FTransform deltaTransform;
	deltaTransform.SetScale3D(FVector::ZeroVector);

	if (!Gizmo.IsValid() || CurrentDomain == ETransformationDomain::TD_None) 
		return deltaTransform;

	FVector rayEnd = RayOrigin + 1'000'000'00 * RayDirection;

	FTransform calcDeltaTransform = Gizmo->GetDeltaTransform(LookingVector, RayOrigin, rayEnd, CurrentDomain);

	//The delta transform we are actually going to apply (same if there is no Snapping taking place)
	deltaTransform = calcDeltaTransform;

	/* SNAPPING LOGIC */
	bool* snappingEnabled = SnappingEnabled.Find(CurrentTransformation);
	float* snappingValue = SnappingValues.Find(CurrentTransformation);

	if (snappingEnabled && *snappingEnabled && snappingValue)
			deltaTransform = Gizmo->GetSnappedTransform(AccumulatedDeltaTransform
				, calcDeltaTransform, CurrentDomain, *snappingValue);
				//GetSnapped Transform Modifies Accumulated Delta Transform by how much Snapping Occurred
	
	ApplyDeltaTransform(deltaTransform);
	return deltaTransform;
}

void UTransformerTool::ApplyDeltaTransform(const FTransform& DeltaTransform)
{
	bool* snappingEnabled = SnappingEnabled.Find(CurrentTransformation);
	float* snappingValue = SnappingValues.Find(CurrentTransformation);

	for (auto& sc : SelectedComponents)
	{
		if (!sc) continue;
		if (bForceMobility || sc->Mobility == EComponentMobility::Type::Movable)
		{
			const FTransform& componentTransform = sc->GetComponentTransform();

			FQuat deltaRotation = DeltaTransform.GetRotation();

			FVector deltaLocation = componentTransform.GetLocation()
				- Gizmo->GetActorLocation();

			//DeltaScale is Unrotated Scale to Get Local Scale since World Scale is not supported
			FVector deltaScale = componentTransform.GetRotation()
				.UnrotateVector(DeltaTransform.GetScale3D());


			if (false == bRotateOnLocalAxis)
				deltaLocation = deltaRotation.RotateVector(deltaLocation);

			FTransform newTransform(
				deltaRotation * componentTransform.GetRotation(),
				//adding Gizmo Location + prevDeltaLocation 
				// (i.e. location from Gizmo to Object after optional Rotating)
				// + deltaTransform Location Offset
				deltaLocation + Gizmo->GetActorLocation() + DeltaTransform.GetLocation(),
				deltaScale + componentTransform.GetScale3D());


			/* SNAPPING LOGIC PER COMPONENT */
			if (snappingEnabled && *snappingEnabled && snappingValue)
				newTransform = Gizmo->GetSnappedTransformPerComponent(componentTransform
					, newTransform, CurrentDomain, *snappingValue);

			sc->SetMobility(EComponentMobility::Type::Movable);
			SetTransform(sc, newTransform);
		}
		else
		{
			RTT_LOG(Warning, "Transform will not affect Component [%s] as it is NOT Moveable!", *sc->GetName());
		}
			
	}
}

bool UTransformerTool::HandleTracedObjects(const TArray<FHitResult>& HitResults
	, bool bAppendToList)
{
	//Assign as None just in case we don't hit Any Gizmos
	ClearDomain();

	//Search for our Gizmo (if Valid) First before Selecting any item
	if (Gizmo.IsValid())
	{
		for (auto& hitResult : HitResults)
		{
			if (Gizmo == hitResult.GetActor()) //check if it's OUR gizmo
			{
				//Check which Domain of Gizmo was Hit from the Test
				if (USceneComponent* componentHit = Cast<USceneComponent>(hitResult.Component))
				{
					SetDomain(Gizmo->GetTransformationDomain(componentHit));
					if (CurrentDomain != ETransformationDomain::TD_None)
					{
						Gizmo->SetTransformProgressState(true, CurrentDomain);
						return true; //finish only if the component actually has a domain, else continue
					}
				}
			}
		}
	}

	for (auto& hits : HitResults)
	{
		if (Cast<ABaseGizmo>(hits.GetActor())) {
			continue; //ignore other Gizmos.
		}

		if (bComponentBased)
			SelectComponent(Cast<USceneComponent>(hits.GetComponent()), bAppendToList);
		else
			SelectActor(hits.GetActor(), bAppendToList);

		return true; //don't process more!
	}

	return false;
}

void UTransformerTool::SetComponentBased(bool bIsComponentBased)
{
	auto selectedComponents = DeselectAll();
	bComponentBased = bIsComponentBased;
	if(bComponentBased)
		SelectMultipleComponents(selectedComponents, false);
	else
	{
		TArray<AActor*> actors;
		for (auto& c : selectedComponents)
			actors.Add(c->GetOwner());
		SelectMultipleActors(actors, false);
	}

    // added by Talad
    if (Gizmo.IsValid() && playerController)
    {
        Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
            , playerController->PlayerCameraManager->GetActorForwardVector()
            , playerController->PlayerCameraManager->GetFOVAngle());
    }

}

void UTransformerTool::SetRotateOnLocalAxis(bool bRotateLocalAxis)
{
	bRotateOnLocalAxis = bRotateLocalAxis;
}

void UTransformerTool::SetTransformationType(ETransformationType TransformationType)
{
	//Don't continue if these are the same.
	if (CurrentTransformation == TransformationType) return;

	if (TransformationType == ETransformationType::TT_NoTransform)
		RTT_LOG(Warning, "Setting Transformation Type to None!");

	CurrentTransformation = TransformationType;

	//Clear the Accumulated tranform when we have a new Transformation
	ResetDeltaTransform(AccumulatedDeltaTransform);

	UpdateGizmoPlacement();

    // added by Talad
    if (Gizmo.IsValid() && playerController)
    {
        Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
            , playerController->PlayerCameraManager->GetActorForwardVector()
            , playerController->PlayerCameraManager->GetFOVAngle());
    }
}

void UTransformerTool::SetSnappingEnabled(ETransformationType TransformationType, bool bSnappingEnabled)
{
	SnappingEnabled.Add(TransformationType, bSnappingEnabled);
}

void UTransformerTool::SetSnappingValue(ETransformationType TransformationType, float SnappingValue)
{
	SnappingValues.Add(TransformationType, SnappingValue);
}

void UTransformerTool::GetSelectedComponents(TArray<class USceneComponent*>& outComponentList
	, USceneComponent*& outGizmoPlacedComponent) const
{
	outComponentList = SelectedComponents;
	if (Gizmo.IsValid())
		outGizmoPlacedComponent = Gizmo->GetParentComponent();
}

TArray<USceneComponent*> UTransformerTool::GetSelectedComponents() const
{
	return SelectedComponents;
}

void UTransformerTool::SelectComponent(class USceneComponent* Component
	, bool bAppendToList)
{
	if (!Component) return;

	if (ShouldSelect(Component->GetOwner(), Component))
	{
		if (false == bAppendToList)
			DeselectAll();
		AddComponent_Internal(SelectedComponents, Component);
		UpdateGizmoPlacement();

        // added by Talad
        if (Gizmo.IsValid() && playerController)
        {
            Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
                , playerController->PlayerCameraManager->GetActorForwardVector()
                , playerController->PlayerCameraManager->GetFOVAngle());
        }
	}
}

void UTransformerTool::SelectActor(AActor* Actor
	, bool bAppendToList)
{
	if (!Actor) return;

	if (ShouldSelect(Actor, Actor->GetRootComponent()))
	{
		if (false == bAppendToList)
			DeselectAll();
		AddComponent_Internal(SelectedComponents, Actor->GetRootComponent());
		UpdateGizmoPlacement();

        // added by Talad
        if (Gizmo.IsValid() && playerController)
        {
            Gizmo->ScaleGizmoScene(playerController->PlayerCameraManager->GetCameraLocation()
                , playerController->PlayerCameraManager->GetActorForwardVector()
                , playerController->PlayerCameraManager->GetFOVAngle());
        }
	}
}

void UTransformerTool::SelectMultipleComponents(const TArray<USceneComponent*>& Components
	, bool bAppendToList)
{
	bool bValidList = false;

	for (auto& c : Components)
	{
		if (!c) continue;
		if (!ShouldSelect(c->GetOwner(), c)) continue;

		if (false == bAppendToList)
		{
			DeselectAll();
			bAppendToList = true;
			//only run once. This is not place outside in case a list is empty or contains only invalid components
		}
		bValidList = true;
		AddComponent_Internal(SelectedComponents, c);
	}

	if(bValidList) UpdateGizmoPlacement();
}

void UTransformerTool::SelectMultipleActors(const TArray<AActor*>& Actors
	, bool bAppendToList)
{
	bool bValidList = false;
	for (auto& a : Actors)
	{
		if (!a) continue;
		if (!ShouldSelect(a, a->GetRootComponent())) continue;

		if (false == bAppendToList)
		{
			DeselectAll();
			bAppendToList = true;
			//only run once. This is not place outside in case a list is empty or contains only invalid components
		}

		bValidList = true;
		AddComponent_Internal(SelectedComponents, a->GetRootComponent());
	}
	if(bValidList) UpdateGizmoPlacement();
}

void UTransformerTool::DeselectComponent(USceneComponent* Component)
{
	if (!Component) return;
	DeselectComponent_Internal(SelectedComponents, Component);
	UpdateGizmoPlacement();
}

void UTransformerTool::DeselectActor(AActor* Actor)
{
	if (Actor)
		DeselectComponent(Actor->GetRootComponent());
}

TArray<USceneComponent*> UTransformerTool::DeselectAll(bool bDestroyDeselected)
{
	TArray<USceneComponent*> componentsToDeselect = SelectedComponents;
	for (auto& i : componentsToDeselect)
		DeselectComponent(i);
		//calling internal so as not to modify SelectedComponents until the last bit!
	SelectedComponents.Empty();
	UpdateGizmoPlacement();

	if (bDestroyDeselected)
	{
		for (auto& c : componentsToDeselect)
		{
			if (!IsValid(c)) continue; //a component that was in the same actor destroyed will be pending kill
			if (AActor* actor = c->GetOwner())
			{
				//We destroy the actor if no components are left to destroy, or the system is currently ActorBased
				if (bComponentBased && actor->GetComponents().Num() > 1)
					c->DestroyComponent(true);
				else
					actor->Destroy();
			}
		}
	}

	return componentsToDeselect;
}

void UTransformerTool::AddComponent_Internal(TArray<USceneComponent*>& OutComponentList
	, USceneComponent* Component)
{
	//if (!Component) return; //assumes that previous have checked, since this is Internal.

	int32 Index = OutComponentList.Find(Component);

	if (INDEX_NONE == Index) //Component is not in list
	{
		OutComponentList.Emplace(Component);
	}
	else if (bToggleSelectedInMultiSelection)
		DeselectComponentAtIndex_Internal(OutComponentList, Index);
}

void UTransformerTool::DeselectComponent_Internal(TArray<USceneComponent*>& OutComponentList
	, USceneComponent* Component)
{
	//if (!Component) return; //assumes that previous have checked, since this is Internal.

	int32 Index = OutComponentList.Find(Component);
	if (INDEX_NONE != Index)
		DeselectComponentAtIndex_Internal(OutComponentList, Index);
}

void UTransformerTool::DeselectComponentAtIndex_Internal(
	TArray<USceneComponent*>& OutComponentList
	, int32 Index)
{
	//if (!Component) return; //assumes that previous have checked, since this is Internal.
	if (OutComponentList.IsValidIndex(Index))
	{
		USceneComponent* Component = OutComponentList[Index];
		OutComponentList.RemoveAt(Index);
	}

}

void UTransformerTool::SetGizmo()
{

	//If there are selected components, then we see whether we need to create a new gizmo.
	if (SelectedComponents.Num() > 0)
	{
		bool bCreateGizmo = true;
		if (Gizmo.IsValid())
		{
			if (CurrentTransformation == Gizmo->GetGizmoType())
			{
				bCreateGizmo = false; // do not create gizmo if there is already a matching gizmo
			}
			else
			{
				// Destroy the current gizmo as the transformation types do not match
				Gizmo->Destroy();
				Gizmo.Reset();
			}
		}

		if (bCreateGizmo)
		{
			if (UWorld* world = GetWorld())
			{
				UClass* GizmoClass = GetGizmoClass(CurrentTransformation);
				if (GizmoClass)
				{
					Gizmo = Cast<ABaseGizmo>(world->SpawnActor(GizmoClass));
					Gizmo->OnGizmoStateChange.AddDynamic(this, &UTransformerTool::OnGizmoStateChanged);
				}
			}
		}
	}
	//Since there are no selected components, we must destroy any gizmos present
	else
	{
		if (Gizmo.IsValid())
		{
			Gizmo->Destroy();
			Gizmo.Reset();
		}
	}


}

void UTransformerTool::UpdateGizmoPlacement()
{
	SetGizmo();
	//means that there are no active gizmos (no selections) so nothing to do in this func
	if (!Gizmo.IsValid()) return;

	FDetachmentTransformRules detachmentRules(EDetachmentRule::KeepWorld, false);

	// detach from any actors it may be currently attached to
	Gizmo->DetachFromActor(detachmentRules);
	Gizmo->SetActorTransform(FTransform()); //Reset Transformation

	USceneComponent* ComponentToAttachTo = nullptr;

	switch (GizmoPlacement)
	{
	case EGizmoPlacement::GP_OnFirstSelection: 
		ComponentToAttachTo = SelectedComponents[0]; break;
	case EGizmoPlacement::GP_OnLastSelection:
		ComponentToAttachTo = SelectedComponents.Last(); break;
	}

	if (ComponentToAttachTo)
	{
		FAttachmentTransformRules attachmentRules 
			= FAttachmentTransformRules::SnapToTargetIncludingScale;
		Gizmo->AttachToComponent(ComponentToAttachTo, attachmentRules);
	}

	Gizmo->UpdateGizmoSpace(CurrentSpaceType);
}


void UTransformerTool::LogSelectedComponents()
{

	RTT_LOG(Log, "******************** SELECTED COMPONENTS LOG START ********************");
	RTT_LOG(Log, "   * Selected Component Count: %d", SelectedComponents.Num());
	RTT_LOG(Log, "   * -------------------------------- ");
	for (int32 i = 0; i < SelectedComponents.Num(); ++i)
	{
		USceneComponent* cmp = SelectedComponents[i];
		FString message = "Component: ";
		if (cmp)
		{
			message += cmp->GetName() + "\tOwner: ";
			if (AActor* owner = cmp->GetOwner())
				message += owner->GetName();
			else
				message += TEXT("[INVALID]");
		}
		else
			message += TEXT("[INVALID]");

		RTT_LOG(Log, "   * [%d] %s", i, *message);
	}

	RTT_LOG(Log, "******************** SELECTED COMPONENTS LOG END   ********************");
}


void UTransformerTool::ReplicateFinishTransform()
{
	//ServerClearDomain();
	//ServerApplyTransform(NetworkDeltaTransform);
	//ResetDeltaTransform(NetworkDeltaTransform);
}


void UTransformerTool::SetupGizmos(APlayerController* pController, UClass* translationClass, UClass* rotationClass, UClass* scaleClass)
{
    playerController = pController;
    TranslationGizmoClass = translationClass;
    RotationGizmoClass = rotationClass;
    ScaleGizmoClass = scaleClass;
}

#undef RTT_LOG