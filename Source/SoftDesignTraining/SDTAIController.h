// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "SDTAIController.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public AAIController
{
    GENERATED_BODY()
public:
    virtual void Tick(float deltaTime) override;
private:
    FVector Velocity = FVector::ZeroVector;

    FVector Direction = FVector(0.0f,1.0f,0.0f);

    //temporary values
    float Acceleration = 500.0f;
    float MaxSpeed = 300.0f;



};
