// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 *
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected:
	virtual bool Initialize() override;

	virtual void OnLevelRemovedFromWorld(ULevel *InLevel, UWorld *InWorld) override;

	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult> &SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

private:
	class UMultiplayerSessionsSubsystem *MultiplayerSessionsSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UButton *HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton *JoinButton;

	UPROPERTY(meta = (BindWidget))
	UButton *QuitButton;

	int32 NumPublicConnections = 4;

	FString MatchType = FString(TEXT("FreeForAll"));

	FString PathToLobby = {TEXT("")};

private:
	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	UFUNCTION()
	void QuitButtonClicked();

	void MenuTearDown();
};
