#include "Luminance_meter.h"

// Sets default values
ALuminance_meter::ALuminance_meter()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALuminance_meter::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ALuminance_meter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float ALuminance_meter::RadianceLuminance()
{
	//FViewport* Viewport;
	//FIntPoint MousePosition;
	Viewport = GetWorld()->GetGameViewport()->Viewport;
	/*UWorld* World = GetWorld();*/

	int32 x_pos;
	int32 y_pos;
	x_pos = Viewport->GetMouseX();
	y_pos = Viewport->GetMouseY();

	FVector2D PixelLocation = FVector2D(x_pos, y_pos);

	/*FVector LightLocation;
	ULightComponent* LightComponent = World->GetLightAtLocation(PixelLocation, LightLocation);*/

	TArray<FColor> PixelData;
	PixelData.AddZeroed(1);

	FIntRect rect(x_pos - 5, y_pos - 5, x_pos + 5, y_pos + 5);

	Viewport->ReadPixels(
		PixelData,
		FReadSurfaceDataFlags(),
		rect
	);

	FColor PixelColor = PixelData[0];
	uint8 Red = PixelColor.R;
	uint8 Green = PixelColor.G;
	uint8 Blue = PixelColor.B;

	// Do something with the RGB values
	Luminance = ((0.265 * Red) + (0.670 * Green) + (0.065 * Blue));
	return Luminance;
}

float ALuminance_meter::UnrealLuminance()
{
	//FViewport* Viewport;
	//FIntPoint MousePosition;
	Viewport = GetWorld()->GetGameViewport()->Viewport;
	/*UWorld* World = GetWorld();*/

	int32 x_pos;
	int32 y_pos;
	x_pos = Viewport->GetMouseX();
	y_pos = Viewport->GetMouseY();

	FVector2D PixelLocation = FVector2D(x_pos, y_pos);

	/*FVector LightLocation;
	ULightComponent* LightComponent = World->GetLightAtLocation(PixelLocation, LightLocation);*/

	TArray<FColor> PixelData;
	PixelData.AddZeroed(1);

	FIntRect rect(x_pos - 5, y_pos - 5, x_pos + 5, y_pos + 5);

	Viewport->ReadPixels(
		PixelData,
		FReadSurfaceDataFlags(),
		rect
	);

	FColor PixelColor = PixelData[0];
	uint8 Red = PixelColor.R;
	uint8 Green = PixelColor.G;
	uint8 Blue = PixelColor.B;

	// Do something with the RGB values
	Luminance = ((0.299 * Red) + (0.587 * Green) + (0.114 * Blue));
	return Luminance;
}