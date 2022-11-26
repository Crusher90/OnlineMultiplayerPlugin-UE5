// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

bool UMenu::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &ThisClass::QuitButtonClicked);
    }
    return true;
}

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;
    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    bIsFocusable = true;

    if (GetWorld())
    {
        APlayerController *PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }
    UGameInstance *GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
    }
}

void UMenu::HostButtonClicked()
{
    HostButton->SetIsEnabled(false);
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
    }
}

void UMenu::JoinButtonClicked()
{
    JoinButton->SetIsEnabled(false);
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UMenu::QuitButtonClicked()
{
    if (GetWorld())
    {
        UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
    }
}

void UMenu::MenuTearDown()
{
    RemoveFromParent();
    if (GetWorld())
    {
        auto PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(false);
        }
    }
}

void UMenu::OnLevelRemovedFromWorld(ULevel *InLevel, UWorld *InWorld)
{
    MenuTearDown();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, FString::Printf(TEXT("Created Session Successfully")));
        }
        if (GetWorld())
        {
            GetWorld()->ServerTravel(PathToLobby);
        }
        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->StartSession();
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Failed to Create Session")));
        }
        HostButton->SetIsEnabled(true);
    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult> &SessionResults, bool bWasSuccessful)
{
    if (MultiplayerSessionsSubsystem == nullptr)
        return;
    for (auto Result : SessionResults)
    {
        FString SettingsValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
        if (SettingsValue == MatchType)
        {
            MultiplayerSessionsSubsystem->JoinSession(Result);
            return;
        }
    }
    JoinButton->SetIsEnabled(true);
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem *Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Purple, FString::Printf(TEXT("Address is %s"), *Address));
            }
            APlayerController *PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if (PlayerController)
            {
                PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
            }
        }
    }
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Started Session Successfully Menu call")));
        }
    }
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Silver, FString::Printf(TEXT("Destroyed Session Successfully Menu call")));
        }
    }
}
