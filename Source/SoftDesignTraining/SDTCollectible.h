// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "NiagaraSystem.h"
#include "SDTCollectible.generated.h"

/**
 * 
 */
UCLASS()
class SOFTDESIGNTRAINING_API ASDTCollectible : public AStaticMeshActor
{
	GENERATED_BODY()
public:
    ASDTCollectible();

    void Collect(bool isAgent);
    void OnCooldownDone();
    bool IsOnCooldown();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float m_CollectCooldownDuration = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
        bool isMoveable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
        bool aiPlaySound = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
        bool aiPlayFx = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
        TObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
        UNiagaraSystem* VisualEffect;

    virtual void Tick(float deltaTime) override;
    virtual void BeginPlay() override;

    FVector initialPosition;

protected:
    FTimerHandle m_CollectCooldownTimer;
    TObjectPtr<UAudioComponent> AudioComponent;
};
