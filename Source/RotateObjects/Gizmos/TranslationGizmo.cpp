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

#include "TranslationGizmo.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

ATranslationGizmo::ATranslationGizmo()
{
	XY_PlaneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("XY Plane"));
	YZ_PlaneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("YZ Plane"));
	XZ_PlaneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("XZ Plane"));
	XYZ_Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("XYZ Sphere"));

	XY_PlaneBox->SetupAttachment(ScalingScene);
	YZ_PlaneBox->SetupAttachment(ScalingScene);
	XZ_PlaneBox->SetupAttachment(ScalingScene);
	XYZ_Sphere->SetupAttachment(ScalingScene);

	RegisterDomainComponent(XY_PlaneBox, ETransformationDomain::TD_XY_Plane);
	RegisterDomainComponent(YZ_PlaneBox, ETransformationDomain::TD_YZ_Plane);
	RegisterDomainComponent(XZ_PlaneBox, ETransformationDomain::TD_XZ_Plane);
	RegisterDomainComponent(XYZ_Sphere, ETransformationDomain::TD_XYZ);

}

FTransform ATranslationGizmo::GetDeltaTransform(const FVector& LookingVector
	, const FVector& RayStartPoint
	, const FVector& RayEndPoint
	, ETransformationDomain Domain)
{
	FTransform deltaTransform;
	deltaTransform.SetScale3D(FVector::ZeroVector); //used so that the default FVector(1.f, 1.f, 1.f) does not affect further scaling

	if (AreRaysValid())
	{
		const float Cos45Deg = 0.707;

		// the opposite direction of the Normal that is most perpendicular to the Looking Vector
		// will be the one we choose to be the normal to the Domain! (needs to be calculated for axis. For planes, it's straightforward)
		FVector planeNormal;

		// the direction of travel (only used for Axis Domains)
		FVector targetDirection(0.f);

		FVector forwardVector	= GetActorForwardVector();
		FVector rightVector		= GetActorRightVector();
		FVector upVector		= GetActorUpVector();


		switch (Domain)
		{
		case ETransformationDomain::TD_X_Axis:
		{
			targetDirection = forwardVector;
			if (FMath::Abs(FVector::DotProduct(LookingVector, rightVector)) > Cos45Deg) 
				planeNormal = rightVector;
			else planeNormal = upVector;
			break;
		}
		case ETransformationDomain::TD_Y_Axis:
		{
			targetDirection = rightVector;
			if (FMath::Abs(FVector::DotProduct(LookingVector, forwardVector)) > Cos45Deg) planeNormal = forwardVector;
			else planeNormal = upVector;
			break;
		}
		case ETransformationDomain::TD_Z_Axis:
		{
			targetDirection = upVector;
			if (FMath::Abs(FVector::DotProduct(LookingVector, forwardVector)) > Cos45Deg) planeNormal = forwardVector;
			else planeNormal = rightVector;
			break;
		}
		case ETransformationDomain::TD_XY_Plane:
			planeNormal = upVector;
			break;
		case ETransformationDomain::TD_YZ_Plane:
			planeNormal = forwardVector;
			break;
		case ETransformationDomain::TD_XZ_Plane:
			planeNormal = rightVector;
			break;
		case ETransformationDomain::TD_XYZ:
			planeNormal = LookingVector;
			break;
		}

		FPlane plane;
		plane.X = planeNormal.X;
		plane.Y = planeNormal.Y;
		plane.Z = planeNormal.Z;
		plane.W = FVector::PointPlaneDist(GetActorLocation(), FVector::ZeroVector, planeNormal);

		FVector deltaLocation =
			FMath::LinePlaneIntersection(RayStartPoint, RayEndPoint, plane)
			- FMath::LinePlaneIntersection(PreviousRayStartPoint, PreviousRayEndPoint, plane);

		switch (Domain)
		{
		case ETransformationDomain::TD_X_Axis:
		case ETransformationDomain::TD_Y_Axis:
		case ETransformationDomain::TD_Z_Axis:
			deltaLocation = deltaLocation.ProjectOnTo(targetDirection);
			break;
		}

		deltaTransform.SetLocation(deltaLocation);

	}

	UpdateRays(RayStartPoint, RayEndPoint);

	return deltaTransform;
}

FTransform ATranslationGizmo::GetSnappedTransform(FTransform& outCurrentAccumulatedTransform
	, const FTransform& DeltaTransform
	, ETransformationDomain Domain
	, float SnappingValue) const
{
	if (SnappingValue == 0.f) return DeltaTransform;

	FTransform result = DeltaTransform;
	FVector addedLocation = outCurrentAccumulatedTransform.GetLocation() + DeltaTransform.GetLocation();

	float domains = 1.f;

	switch (Domain)
	{
	case ETransformationDomain::TD_XY_Plane:
	case ETransformationDomain::TD_YZ_Plane:
	case ETransformationDomain::TD_XZ_Plane:
		domains = 2.f;
		break;
	case ETransformationDomain::TD_XYZ:
		domains = 3.f;
		break;
	}

	FVector snappedLocation = addedLocation.GetSafeNormal()
		* FMath::GridSnap(addedLocation.Size(), FMath::Sqrt(FMath::Square(SnappingValue) * domains));

	result.SetLocation(snappedLocation);
	outCurrentAccumulatedTransform.SetLocation(addedLocation - snappedLocation);
	return result;
}
