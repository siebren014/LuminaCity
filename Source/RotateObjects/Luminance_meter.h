#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Luminance_meter.generated.h"

UCLASS()
class ROTATEOBJECTS_API ALuminance_meter : public AActor
{
	GENERATED_BODY()

		UFUNCTION(BlueprintCallable, Category = "Luminance_meter")
		float RadianceLuminance();

	UFUNCTION(BlueprintCallable, Category = "Unreal_Luminance")
		float UnrealLuminance();

	FViewport* Viewport;
	float Luminance;

public:
	// Sets default values for this actor's properties
	ALuminance_meter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};