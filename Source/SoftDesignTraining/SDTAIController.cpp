// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"

void ASDTAIController::Tick(float deltaTime)
{

    //Super::Tick(deltaTime);

    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn) {
        return;
    }

    Velocity += Direction * Acceleration * deltaTime;

    if (Velocity.Size() > MaxSpeed) {
        Velocity = Velocity.GetSafeNormal() * MaxSpeed;
    }

    ControlledPawn->AddMovementInput(Velocity.GetSafeNormal(), Velocity.Size() * deltaTime);

    if (!Velocity.IsNearlyZero()) {
        FRotator TargetRotation = Velocity.GetSafeNormal().Rotation();

        ControlledPawn->SetActorRotation(TargetRotation);
    }




}




