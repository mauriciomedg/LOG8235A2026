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

    FHitResult HitResult;

    if (DetectWall(DesiredDirection, HitResult))
    {
        FVector AvoidanceDirection =
            FVector::CrossProduct(
                FVector::UpVector,
                HitResult.Normal
            ).GetSafeNormal();

        if (FVector::DotProduct(AvoidanceDirection, DesiredDirection) < 0.0f)
        {
            AvoidanceDirection *= -1.0f;
        }


        float RotationAmount = AvoidanceAngle * deltaTime;

        float TurnDirection =
            FVector::DotProduct(
                FVector::CrossProduct(
                    DesiredDirection,
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
        //DesiredDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal();
        Velocity = DesiredDirection * Velocity.Size();

    }

    Velocity += DesiredDirection * Acceleration * deltaTime;

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

void ASDTAIController::Tick(float deltaTime)
{
    FVector OutDirection;
    StateMachine->Run(GetPawn(), OutDirection);

    Navigation(OutDirection, deltaTime);
}


bool ASDTAIController::DetectWall(const FVector& DesiredDirection, FHitResult& HitResult) const
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

