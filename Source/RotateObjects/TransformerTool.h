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

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TransformerTool.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogRuntimeTransformer, Log, All);

UENUM(BlueprintType)
enum class ETransformationType : uint8
{
    TT_NoTransform			UMETA(DisplayName = "None"),
    TT_Translation			UMETA(DisplayName = "Translation"),
    TT_Rotation				UMETA(DisplayName = "Rotation"),
    TT_Scale				UMETA(DisplayName = "Scale")
};

UENUM(BlueprintType)
enum class ESpaceType : uint8
{
    ST_None				UMETA(DisplayName = "None"),
    ST_Local			UMETA(DisplayName = "Local Space"),
    ST_World			UMETA(DisplayName = "World Space"),
};

UENUM(BlueprintType)
enum class ETransformationDomain : uint8
{
    TD_None				UMETA(DisplayName = "None"),

    TD_X_Axis			UMETA(DisplayName = "X Axis"),
    TD_Y_Axis			UMETA(DisplayName = "Y Axis"),
    TD_Z_Axis			UMETA(DisplayName = "Z Axis"),

    TD_XY_Plane			UMETA(DisplayName = "XY Plane"),
    TD_YZ_Plane			UMETA(DisplayName = "YZ Plane"),
    TD_XZ_Plane			UMETA(DisplayName = "XZ Plane"),

    TD_XYZ				UMETA(DisplayName = "XYZ"),

};

UENUM(BlueprintType)
enum class EGizmoPlacement : uint8
{
	GP_None					UMETA(DisplayName = "None"),
	GP_OnFirstSelection		UMETA(DisplayName = "On First Selection"),
	GP_OnLastSelection		UMETA(DisplayName = "On Last Selection"),
};

UCLASS(Blueprintable)
class  UTransformerTool : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	UTransformerTool();
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	//Sets the Transform for a Given Component and calls the 
	//Ufocusable transform function called if it implements the Interface
	void SetTransform(class USceneComponent* Component, const FTransform& Transform);

	//Used to Filter unwanted things from a list of OutHits.
	void FilterHits(TArray<FHitResult>& outHits);

public:

	/*
	* This gets called everytime a Component / Actor is going to get added.
	* The default return is TRUE, but it can be overriden to check for additional things 
	* (e.g. checking if it implements an interface, has some property, is child of a class, etc)
	
	* @param OwnerActor: The Actor owning the Component Selected 
	* @param Component: The Component Selected (if it's an Actor Selected, this would be its RootComponent)

	* @return bool: Whether or not this Component should be added.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Transformer")
	bool ShouldSelect(AActor* OwnerActor, class USceneComponent* Component);

	//by default return true, override for custom logic
	virtual bool ShouldSelect_Implementation(AActor* OwnerActor
		, class USceneComponent* Component) { return true; }

	//Sets the Space of the Gizmo, whether its Local or World space.
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetSpaceType(ESpaceType Type);

	/*
	 * Gets the Current Domain
	 * If it returns ETransformationDomain::TD_None, then that means
	 * there is no Transformation in Progress.
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	ETransformationDomain GetCurrentDomain(bool& TransformInProgress) const;


	/**
	 * Sets the Current Domain to NONE. (Transforming in Progress will become false)
	 * Should be called when we are done transforming with the Gizmo.
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void ClearDomain();

	//Gets the Start and End Points of the Mouse based on the Player Controller possessing this pawn
	// returns true if outStartPoint and outEndPoint were given a successful value
	bool GetMouseStartEndPoints(float TraceDistance, FVector& outStartPoint, FVector& outEndPoint);

	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	 * This function only does the actual trace if there is a Player Controller Set
	 * @see SetupGizmos

	 * @param CollisionChannels - All the Channels to be considering during Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool MouseTraceByObjectTypes(float TraceDistance
		, TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);

	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	 * This function only does the actual trace if there is a Player Controller Set
	 * @see SetupGizmos

	 * @param TraceChannel - The Ray Collision Channel to be Considered in the Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool MouseTraceByChannel(float TraceDistance
		, TEnumAsByte<ECollisionChannel> TraceChannel
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);

	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	 * This function only does the actual trace if there is a Player Controller Set
	 * @see SetupGizmos

	 * @param ProfileName - The Profile Name to be used during the Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool MouseTraceByProfile(float TraceDistance
		, const FName& ProfileName
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);

	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	 * Note: This function does not Deselect the Objects selected if Trace doesn't select anything in any situation

	 * @param StartLocation - the starting Location of the trace, in World Space
	 * @param EndLocation - the ending location of the trace, in World Space
	 * @param CollisionChannels - All the Channels to be considering during Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool TraceByObjectTypes(const FVector& StartLocation
		, const FVector& EndLocation
		, TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);

    UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
    void MoveGizmoAction();

	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	  * Note: This function does not Deselect the Objects selected if Trace doesn't select anything in any situation

	 * @param StartLocation - the starting Location of the trace, in World Space
	 * @param EndLocation - the ending location of the trace, in World Space
	 * @param TraceChannel - The Ray Collision Channel to be Considered in the Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool TraceByChannel(const FVector& StartLocation
		, const FVector& EndLocation
		, TEnumAsByte<ECollisionChannel> TraceChannel
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);


	/**
	 * If a Gizmo is Present, (i.e. there is a Selected Object), then
	 * this test will prioritize finding a Gizmo, even if it is behind an object.
	 * If there is not a Gizmo present, the first Object encountered will be automatically Selected.

	  * Note: This function does not Deselect the Objects selected if Trace doesn't select anything in any situation

	 * @param StartLocation - the starting Location of the trace, in World Space
	 * @param EndLocation - the ending location of the trace, in World Space
	 * @param ProfileName - The Profile Name to be used during the Trace
	 * @param Ignored Actors	- The Actors to be Ignored during trace
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 * @return bool Whether there was an Object traced successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool TraceByProfile(const FVector& StartLocation
		, const FVector& EndLocation
		, const FName& ProfileName
		, TArray<AActor*> IgnoredActors
		, bool bAppendToList = false);


	/**
	 * If the Gizmo is currently in a Valid Domain,
	 * it will transform the Selected Object(s) through a valid domain.
	 * The transform is calculated with the given Ray Origin and Ray Direction in World Space (usually the Mouse World Position & Mouse World Direction)

	 * This function should be called if NO Player Controller has been Set

	 * @param LookingVector - The looking direction of the player (e.g. Camera Forward Vector)
	 * @param RayOrigin - The origin point (world space) of the Ray (e.g. the Mouse Position in World Space)
	 * @param RayDirection - the direction of the ray (in world space) (e.g. the Mouse Direction in World Space)

	 * @returnval FTransform - The delta transform calculated (after any snapping)
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	FTransform UpdateTransform(const FVector& LookingVector
		, const FVector& RayOrigin, const FVector& RayDirection);

	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void ApplyDeltaTransform(const FTransform& DeltaTransform);

	/**
	 * Processes the OutHits generated by Tracing and Selects either a Gizmo (priority) or
	 * if no Gizmo is present in the trace, the first object hit is selected.
	 *
	 * This is already called by the RuntimeTransformer built-in Trace Functions,
	 * but can be called manually if you wish to provide your own list of Hit Results (e.g. tracing with different configuration/method)
	 *
	 * @param HitResults - a list of the FHitResults that were generated by LineTracing
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	bool HandleTracedObjects(const TArray<FHitResult>& HitResults
		, bool bAppendToList = false);

	/*
	 * Called when the Gizmo State has changed (i.e. Domain has changed)
	 * @param GizmoType - the type of Gizmo that was changed (Translation, Rotation or Scale)
	 * @param bTransformInProgress - whether the Transform is currently in progress. This is basically a bool that evaluates to Domain != NONE
	 * @param Domain - The current domain that the Gizmo State changed to
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Transformer")
	void OnGizmoStateChanged(ETransformationType GizmoType, bool bTransformInProgress
		, ETransformationDomain Domain);

	virtual void OnGizmoStateChanged_Implementation(ETransformationType GizmoType
		, bool bTransformInProgress, ETransformationDomain Domain)
	{
		//this should be overriden for custom logic
	}

public:

	/**
	 * Whether to Set the System to work with Components (true)
	 * or to work with Actors (false)

	 @see bComponentBased
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetComponentBased(bool bIsComponentBased);

	/**
	 * Whether to Set the System to Rotate Multiple Objects around their own axis (true)
	 * or to work rotate around where the Gizmo is at (false)

	 @see bRotateLocalAxis
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetRotateOnLocalAxis(bool bRotateLocalAxis);

	/**
	 * Sets the Current Transformation (Translation, Rotation or Scale)
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetTransformationType(ETransformationType TransformationType);
	
	/*
	 * Enables/Disables Snapping for a given Transformation
	 * Snapping Value for the Given Transformation MUST NOT be 0 for Snapping to work

	 @see SetSnappingValue
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetSnappingEnabled(ETransformationType TransformationType, bool bSnappingEnabled);

	/*
	 * Sets a Snapping Value for a given Transformation
	 * Snapping Value MUST NOT be 0  and Snapping must be enabled for the given transformation for Snapping to work

	 @see SetSnappingEnabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SetSnappingValue(ETransformationType TransformationType, float SnappingValue);

	/*
	 * Gets the list of Selected Components.

	 @return outComponentList - the List of Currently Selected Components
	 @return outGizmoPlacedComponent - the Component in the list that currently has the Gizmo attached
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void GetSelectedComponents(TArray<class USceneComponent*>& outComponentList
		, class USceneComponent*& outGizmoPlacedComponent) const;

	TArray<class USceneComponent*> GetSelectedComponents() const;

public:


	/**
	 * Select Component adds a given Component to a list of components that will be used for the Runtime Transforms
	 * @param Component The component to add to the list.
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SelectComponent(class USceneComponent* Component, bool bAppendToList = false);

	/**
	 * Select Actor adds the Actor's Root Component to a list of components that will be used for the Runtime Transforms
	 * @param Actor The Actor whose Root Component will be added to the list.
	 * @param bAppendToList - If a selection happens, whether to append to the previously selected components or not
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SelectActor(AActor* Actor, bool bAppendToList = false);

	/**
	 * Selects all the Components in given list.
	 * @see SelectComponent func
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SelectMultipleComponents(const TArray<class USceneComponent*>& Components
		, bool bAppendToList = false);

	/**
	 * Selects all the Root Components of the Actors in given list.
	 * @see SelectActor func
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void SelectMultipleActors(const TArray<AActor*>& Actors
		, bool bAppendToList = false);

	/**
	 * Deselects a given Component, if found on the list.
	 * @param Component the Component to deselect
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void DeselectComponent(class USceneComponent* Component);

	/**
	 * Deselects a given Actor's Root Component, if found on the list.
	 * @param Actor whose RootComponent to deselect
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	void DeselectActor(AActor* Actor);

	/*
	* Deselects all the Selected Components that are in the list.

	* @param bDestroyComponents - whether to Deselect all Components and Destroy them!

	* @return the list of components that were Deselected. (list will be empty if bDestroyComponents is true)
	*/
	UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
	TArray<class USceneComponent*> DeselectAll(bool bDestroyDeselected = false);

private:

	/*
	The core functionality, but can be called by Selection of Multiple objects
	so as to not call UpdateGizmo every time
	*/
	void AddComponent_Internal(TArray<class USceneComponent*>& OutComponentList
		, class USceneComponent* Component);

	/*
	The core functionality, but can be called by Selection of Multiple objects
	so as to not call UpdateGizmo every time
	*/
	void DeselectComponent_Internal(TArray<class USceneComponent*>& OutComponentList
		, class USceneComponent* Component);
	void DeselectComponentAtIndex_Internal(TArray<class USceneComponent*>& OutComponentList
		, int32 Index);

	/**
	 * Creates / Replaces Gizmo with the Current Transformation.
	 * It destroys any current active gizmo to replace it.
	*/
	void SetGizmo();

	/**
	 * Updates the Gizmo Placement (Position)
	 * Called when an object was selected, deselected
	*/
	void UpdateGizmoPlacement();

	//Gets the respective assigned class for a given TransformationType
	UClass* GetGizmoClass(ETransformationType TransformationType) const;

	//Resets the transform to all Zeros (including Scale)
	static void ResetDeltaTransform(FTransform& Transform);

	void SetDomain(ETransformationDomain Domain);

public:

	/*
	 * Prints all the information regarding the Currently Selected Components
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug Runtime Transformer")
	void LogSelectedComponents();

	/*
	 * Calls the ServerClearDomain.
	 * Then it calls ServerApplyTransform and Resets the Accumulated Network Transform.

	 * @see ServerClearDomain
	 * @see ServerApplyTransform
	 */
	UFUNCTION(BlueprintCallable, Category = "Replicated Runtime Transformer")
	void ReplicateFinishTransform();

	//Tries to resync the Selections 
	void ResyncSelection();

    UFUNCTION(BlueprintCallable, Category = "Runtime Transformer")
    void SetupGizmos(APlayerController* playerController, UClass* translationClass, UClass* rotationClass, UClass* scaleClass);


	//Networking Variables
private:

	/*
	* Ignore Non-Replicated Objects means that the objects that do not satisfy
	* the replication conditions will become unselectable. This only takes effect if using the ServerTracing
	* The Replication Conditions:
	* - For an actor, replicating must be on
	* - For a component, both its owner and itself need to be replicating
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Replicated Runtime Transformer", meta = (AllowPrivateAccess = "true"))
	bool bIgnoreNonReplicatedObjects;

	/*
	 * Optional minimum time to wait for all Cloned objects to fully replicate and are selectable.
	 * It is not required, but there are occassions (especially when cloning multiple objects at once)
	 * where the newly spawned clone objects are not selected on the client side because the object, 
	 * although has begun play, is not still fully network addressable on the client side and so a nullptr is passed 
	 * (so no selection occurs).
	 * The time it actually takes to Replicate can be more because it also waits for all clone objects to have begun play.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Replicated Runtime Transformer", meta = (AllowPrivateAccess = "true"))
	float MinimumCloneReplicationTime;

	//The frequency at which checks are done on newly spawned clones. Whether they are suitable for replication.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Replicated Runtime Transformer", meta = (AllowPrivateAccess = "true"))
	float CloneReplicationCheckFrequency;

	FTransform	NetworkDeltaTransform;

	//List of clone actor/components that need replication but haven't been replicated yet
	TArray<class USceneComponent*> UnreplicatedComponentClones;

	FTimerHandle	CheckUnrepTimerHandle;
	FTimerHandle	ResyncSelectionTimerHandle;


	//Other Vars
private:

	//The Current Space being used, whether it is Local or World.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	ESpaceType CurrentSpaceType;

	//The Transform Accumulated for Snapping
	FTransform AccumulatedDeltaTransform;

	/**
	 * GizmoClasses are variables that specified which Gizmo to spawn for each
	 * transformation. This can even be childs of classes that are already defined
	 * to allow the user to customize gizmo functionality
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gizmo", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ATranslationGizmo> TranslationGizmoClass;

	/**
	 * GizmoClasses are variables that specified which Gizmo to spawn for each
	 * transformation. This can even be childs of classes that are already defined
	 * to allow the user to customize gizmo functionality
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gizmo", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ARotationGizmo> RotationGizmoClass;

	/**
	 * GizmoClasses are variables that specified which Gizmo to spawn for each
	 * transformation. This can even be childs of classes that are already defined
	 * to allow the user to customize gizmo functionality
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gizmo", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AScaleGizmo> ScaleGizmoClass;

	UPROPERTY()
	TWeakObjectPtr<class ABaseGizmo> Gizmo;

	// Tell which Domain is Selected. If NONE, then that means that there is no Selected Objects, or
	// that the Gizmo has not been hit yet.
	ETransformationDomain CurrentDomain;

	//Tell where the Gizmo should be placed when multiple objects are selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	EGizmoPlacement GizmoPlacement;

	// Var that tells which is the Current Transformation taking place
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	ETransformationType CurrentTransformation;

	/**
	 * Array storing Selected Components. Although a quick O(1) removal is needed (like a Set),
	 * it is Crucial that we maintain the order of the elements as they were selected
	 */
	TArray<class USceneComponent*> SelectedComponents;

	/*
	* Map storing the Snap values for each transformation
	* bSnappingEnabled must be true AND, the value for the current transform MUST NOT be 0 for these values to take effect.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	TMap<ETransformationType, float> SnappingValues;

	/**
	* Whether Snapping an Object for each Transformation is enabled or not.
	* SnappingValue for each Transformation must also NOT be zero for it to work 
	* (if, snapping value is 0 for a transformation, no snapping will take place)

	* @see SetSnappingValue function & SnappingValues Map var
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	TMap<ETransformationType, bool> SnappingEnabled;

	/**
	* Whether to Force Mobility on items that are not Moveable
	* if true, Mobility on Components will be changed to Moveable (WARNING: does not set it back to its original mobility!)
	* if false, no movement transformations will be attempted on Static/Stationary Components

	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	bool bForceMobility;

	/*
	 * This property only matters when multiple objects are selected.
	 * Whether multiple objects should rotate on their local axes (true) or on the axes the Gizmo is in (false)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	bool bRotateOnLocalAxis;

	/**
	 * Whether to Apply the Transforms to objects that Implement the UFocusable Interface.
	 * if True, the Transforms will be applied.
	 * if False, the Transforms will not be applied.

	 * IN BOTH Situations, the UFocusable Objects have IFocusable::OnNewDeltaTransformation called.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	bool bTransformUFocusableObjects;

	//Property that checks whether a CLICK on an already selected object should deselect the object or not.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	bool bToggleSelectedInMultiSelection;

	/*
	 * Property that checks whether Components are considered in trace 
	 or the Actors are.
	 * This property affects how Cloning, Tracing is done and Interface checking is done
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Transformations", meta = (AllowPrivateAccess = "true"))
	bool bComponentBased;

	//Whether we need to Sync with Server if there is a mismatch in number of Selections.
	bool bResyncSelection;

    // player controller referenced by this tool
    APlayerController* playerController;
};
