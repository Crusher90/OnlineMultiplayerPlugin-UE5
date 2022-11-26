// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
    IOnlineSubsystem *Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        SessionInterface = Subsystem->GetSessionInterface();
    }
    CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete);
    FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete);
    JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
    StartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete);
    DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete);
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
    if(!SessionInterface.IsValid())
        return;

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if(ExistingSession != nullptr)
    {
        bCreateSessionOnDestroy = true;
        LastNumPublicConnections = NumPublicConnections;
        LastMatchType = MatchType;
        DestroySession();
    }
    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
    LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
    LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
    LastSessionSettings->NumPublicConnections = NumPublicConnections;
    LastSessionSettings->bShouldAdvertise = true;
    LastSessionSettings->bUseLobbiesIfAvailable = true;
    LastSessionSettings->bAllowJoinInProgress = true;
    LastSessionSettings->bUsesPresence = true;
    LastSessionSettings->BuildUniqueId = 1;
    LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    const auto LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if(!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        MultiplayerOnCreateSessionComplete.Broadcast(false);
    }
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
    LastSessionSearch->MaxSearchResults = MaxSearchResults;
    LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
    LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

    const ULocalPlayer *LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

        MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
    }
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult &SessionResult)
{
    if (!SessionInterface.IsValid())
    {
        MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
        return;
    }

    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

    const ULocalPlayer *LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

        MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
    }
}

void UMultiplayerSessionsSubsystem::StartSession()
{
    if(!SessionInterface.IsValid())
    {
        MultiplayerOnStartSessionComplete.Broadcast(false);
        return;
    }
    StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
    bool bStartSession = SessionInterface->StartSession(NAME_GameSession);
    if(bStartSession)
    {
        MultiplayerOnStartSessionComplete.Broadcast(bStartSession);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Started Session Successfully")));
        }
    }
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
    if(!SessionInterface.IsValid())
    {
        MultiplayerOnDestroySessionComplete.Broadcast(false);
        return;
    }
    DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
    if(!SessionInterface->DestroySession(NAME_GameSession))
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        MultiplayerOnDestroySessionComplete.Broadcast(false);
    }
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }
    MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }
    if (LastSessionSearch->SearchResults.Num() <= 0)
    {
        MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
        return;
    }
    MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if(!SessionInterface.IsValid()) return;
    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
    MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(!SessionInterface.IsValid())
        return;
    if(bWasSuccessful)
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
        MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
    }
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(!SessionInterface.IsValid())
        return;
    SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    if(bWasSuccessful && bCreateSessionOnDestroy)
    {
        bCreateSessionOnDestroy = false;
        CreateSession(LastNumPublicConnections, LastMatchType);
    }
    MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}
