// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"

#include "DrawDebugHelpers.h"

void ASDTAIController::Tick(float deltaTime)
{

    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn) {
        return;
    }

    FHitResult HitResult;

    if (DetectWall(HitResult))
    {
        FVector AvoidanceDirection =
            FVector::CrossProduct(
                FVector::UpVector,
                HitResult.Normal
            ).GetSafeNormal();

        if (FVector::DotProduct(AvoidanceDirection, Direction) < 0.0f)
        {
            AvoidanceDirection *= -1.0f;
        }


        float RotationAmount = AvoidanceAngle * deltaTime;

        float TurnDirection =
            FVector::DotProduct(
                FVector::CrossProduct(
                    Direction,
                    AvoidanceDirection
                ),
                FVector::UpVector
            );

        if (TurnDirection < 0.0f)
        {
            RotationAmount *= -1.0f;
        }
        ControlledPawn->AddActorWorldRotation(
            FRotator(0.0f, RotationAmount, 0.0f),
            false,
            nullptr,
            ETeleportType::None
        );
        Direction = ControlledPawn->GetActorForwardVector().GetSafeNormal();
        Velocity = Direction * Velocity.Size();

    }



    Velocity += Direction * Acceleration * deltaTime;

    if (Velocity.Size() > MaxSpeed) {
        Velocity = Velocity.GetSafeNormal() * MaxSpeed;
    }

    ControlledPawn->AddMovementInput(Velocity.GetSafeNormal(), Velocity.Size() * deltaTime);

    if (!Velocity.IsNearlyZero())
    {
        ControlledPawn->SetActorRotation(
            Velocity.GetSafeNormal().Rotation()
        );
    }


}


bool ASDTAIController::DetectWall(FHitResult& HitResult) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }

    FVector Start = ControlledPawn->GetActorLocation();
    FVector End = Start + Direction.GetSafeNormal() * WallDetectionDistance;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledPawn);

    float SweepRadius = 100.0f;
    FCollisionShape CollisionShape =
    FCollisionShape::MakeSphere(SweepRadius);

    DrawDebugSphere(
        GetWorld(),
        Start,
        SweepRadius,
        16,
        FColor::Red,
        false,
        0.0f
    );


    return GetWorld()->SweepSingleByObjectType(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ObjectQueryParams,
        CollisionShape,
        QueryParams
    );


}

