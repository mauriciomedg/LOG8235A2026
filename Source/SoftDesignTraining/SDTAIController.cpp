// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"
#include "StateMachine.h"
#include "DrawDebugHelpers.h"

ASDTAIController::ASDTAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    StateMachine = CreateDefaultSubobject<UStateMachine>("AIStateMachine");
}

void ASDTAIController::Navigation(const FVector& DesiredDirection, float deltaTime)
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn) {
        return;
    }

    TArray<FHitResult> Hits;

    FVector MovementDirection = DesiredDirection.GetSafeNormal();

    if (DetectWall(MovementDirection, Hits))
    {
        FVector AvoidanceDirection = FVector::ZeroVector;

        for (const FHitResult& Hit : Hits)
        {
            if (Hit.bBlockingHit)
            {
                AvoidanceDirection += Hit.Normal;
            }
        }

        AvoidanceDirection.Normalize();

        const float CrossZ =
            FVector::CrossProduct(
                MovementDirection,
                AvoidanceDirection
            ).Z;

        const float TurnSign = FMath::Sign(CrossZ);

        const float RotationAmount =
            TurnSign * AvoidanceAngle * deltaTime;

        // Rotate the actual movement direction
        MovementDirection =
            MovementDirection.RotateAngleAxis(
                RotationAmount,
                FVector::UpVector
            );

        Velocity =
            MovementDirection * Velocity.Size();
    }

    Velocity +=
        MovementDirection * Acceleration * deltaTime;

    if (Velocity.Size() > MaxSpeed)
    {
        Velocity =
            Velocity.GetSafeNormal() * MaxSpeed;
    }

    ControlledPawn->AddMovementInput(
        Velocity.GetSafeNormal(),
        Velocity.Size() * deltaTime
    );

    if (!Velocity.IsNearlyZero())
    {
        ControlledPawn->SetActorRotation(
            Velocity.GetSafeNormal().Rotation()
        );
    }
}

void ASDTAIController::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
    FVector OutDirection;
    StateMachine->Run(GetPawn(), OutDirection);

    Navigation(OutDirection, deltaTime);
}


bool ASDTAIController::DetectWall(const FVector& DesiredDirection, TArray<FHitResult>& Hits) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }

    FVector Start = ControlledPawn->GetActorLocation();
    FVector End = Start + DesiredDirection.GetSafeNormal() * WallDetectionDistance;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledPawn);

    float SweepRadius = 100.0f;
    FCollisionShape CollisionShape =
        FCollisionShape::MakeSphere(SweepRadius);

    //DrawDebugSphere(
    //    GetWorld(),
    //    Start,
    //    SweepRadius,
    //    16,
    //    FColor::Red,
    //    false,
    //    0.0f
    //);


    return GetWorld()->SweepMultiByObjectType(
        Hits,
        Start,
        End,
        FQuat::Identity,
        ObjectQueryParams,
        CollisionShape,
        QueryParams
    );


}

