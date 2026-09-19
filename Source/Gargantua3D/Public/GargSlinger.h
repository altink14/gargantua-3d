#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GargSlinger.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * The player.
 *
 * A third-person character, because this game is about looking at creatures and
 * a first-person view would hide the thing it is about.
 *
 * Input is bound through the classic axis mappings declared in
 * Config/DefaultInput.ini rather than Enhanced Input assets. Those assets can
 * only be authored in the editor, and a blockout that cannot be walked without
 * first opening the editor and making four assets is not much of a blockout.
 * This is the part most likely to be replaced once the game has real content.
 */
UCLASS()
class GARGANTUA3D_API AGargSlinger : public ACharacter
{
	GENERATED_BODY()

public:
	AGargSlinger();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* Input) override;
	virtual void BeginPlay() override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<USpringArmComponent> Boom;

	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UCameraComponent> Camera;
};
