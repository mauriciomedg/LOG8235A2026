// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SDTAIController.generated.h"

class UStateMachine;

UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public AAIController
{
    GENERATED_BODY()

public:
    ASDTAIController(const FObjectInitializer& ObjectInitializer);

    virtual void Tick(float DeltaTime) override;

private:
    // Mouvement
    FVector Velocity = FVector::ZeroVector;
    float Acceleration = 900.0f;
    float MaxSpeed = 500.0f;

    // Murs
    float WallDetectionDistance = 180.0f;
    float WallSweepRadius = 45.0f;
    float WallAvoidanceAngle = 180.0f;

    // Death floor
    float DeathFloorDetectionDistance = 220.0f;
    float DeathFloorSweepRadius = 45.0f;
    float DeathFloorAvoidanceSpeed = 300.0f;
    bool bAvoidingDeathFloor = false;
    FVector DeathFloorAvoidanceDirection = FVector::ZeroVector;

    UPROPERTY()
    TObjectPtr<UStateMachine> StateMachine;

    void Navigation(const FVector& DesiredDirection, float DeltaTime);

    bool DetectWall(const FVector& Direction, TArray<FHitResult>& Hits) const;
    bool DetectDeathFloor(const FVector& Direction, TArray<FHitResult>& Hits) const;

    FVector ComputeWallAvoidanceDirection(
        const FVector& CurrentDirection, const FVector& DesiredDirection,
        const TArray<FHitResult>& Hits, float DeltaTime) const;
    FVector ChooseDeathFloorAvoidanceDirection(const FVector& DesiredDirection) const;
    FVector MakeDeathFloorDirectionSafe(const FVector& ProposedDirection, const TArray<FHitResult>& DeathFloorHits) const;
};
