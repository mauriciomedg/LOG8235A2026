// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "SDTAIController.generated.h"

class UStateMachine;
class ACharacter;
/**
 * 
 */
UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public AAIController
{
    GENERATED_BODY()
public:
    ASDTAIController(const FObjectInitializer& ObjectInitializer);

    virtual void Tick(float deltaTime) override;
private:
    FVector Velocity = FVector::ZeroVector;

    //FVector Direction = FVector(0.0f,1.0f,0.0f);

    float Acceleration = 100.0f;
    float MaxSpeed = 66.0f;

    float WallDetectionDistance = 400.0f;
    float AvoidanceAngle = 90.0f;

    void Navigation(const FVector& DesiredDirection, float deltaTime);

    bool DetectWall(const FVector& DesiredDirection, TArray<FHitResult>& Hits) const;

    UPROPERTY()
    TObjectPtr<UStateMachine> StateMachine;

};
