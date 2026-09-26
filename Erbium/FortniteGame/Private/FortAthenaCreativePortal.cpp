#include "pch.h"
#include "../Public/FortAthenaCreativePortal.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/BuildingSMActor.h"
#include "../../Erbium/Public/Configuration.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
static bool MemReadable(const void* Ptr, size_t Size)
{
    if (!Ptr || Size == 0)
        return false;
#ifdef _WIN32
    return !IsBadReadPtr(Ptr, Size);
#else
    return true;
#endif
}

namespace
{
    bool IsLiveObject(const UObject* Object)
    {
        return Object && MemReadable(Object, sizeof(UObject)) && Object->Class && MemReadable(Object->Class, sizeof(UObject));
    }

    const UFortCreativeRealEstatePlotItemDefinition* ResolveCreativePlot()
    {
        auto Plot = FindObject<UFortCreativeRealEstatePlotItemDefinition>(FConfiguration::CreativeTerrainPath);
        if (!Plot)
            Plot = FindObject<UFortCreativeRealEstatePlotItemDefinition>(L"/Game/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium");
        if (!Plot)
            Plot = FindObject<UFortCreativeRealEstatePlotItemDefinition>(L"/CR_Legacy/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium");
        return Plot;
    }

    typedef void (*FExecHandler)(UObject*, FFrame&);

    FExecHandler ServerStartInteractingOG = nullptr;
    FExecHandler ServerMoveSelectionSetOG = nullptr;
    FExecHandler ServerPlaceActorsAndClearMovementModeOG = nullptr;
    FExecHandler ServerClearMovementModeOG = nullptr;
    FExecHandler PhoneTargetStartInteractingOG = nullptr;

    AFortPlayerControllerAthena* FindControllerFromObject(UObject* Object)
    {
        UObject* Current = Object;
        while (Current && IsLiveObject(Current))
        {
            if (Current->IsA(AFortPlayerControllerAthena::StaticClass()))
                return (AFortPlayerControllerAthena*)Current;
            if (Current->IsA(AFortPlayerPawnAthena::StaticClass()))
            {
                auto Pawn = (AFortPlayerPawnAthena*)Current;
                if (Pawn->HasController() && IsLiveObject(Pawn->Controller) && Pawn->Controller->IsA(AFortPlayerControllerAthena::StaticClass()))
                    return (AFortPlayerControllerAthena*)Pawn->Controller;
            }
            Current = Current->Outer;
        }
        return nullptr;
    }

    void EnsureCreativeEditAuthority(AFortPlayerControllerAthena* PC)
    {
        if (!IsLiveObject(PC) || !IsLiveObject(PC->PlayerState))
            return;

        auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

        if (PC->HasOwnedPortal() && IsLiveObject(PC->OwnedPortal))
        {
            auto Portal = (AFortAthenaCreativePortal*)PC->OwnedPortal;
            if (Portal->HasLinkedVolume() && IsLiveObject(Portal->LinkedVolume))
            {
                auto LevelSave = (UFortLevelSaveComponent*)Portal->LinkedVolume->GetComponentByClass(UFortLevelSaveComponent::StaticClass());
                if (LevelSave)
                {
                    if (LevelSave->HasAccountIdOfOwner())
                        LevelSave->AccountIdOfOwner = PlayerState->UniqueId;
                    if (LevelSave->HasbIsLoaded())
                        LevelSave->bIsLoaded = true;
                    if (LevelSave->HasRestrictedPlotDefinition())
                    {
                        auto Plot = ResolveCreativePlot();
                        if (Plot)
                            LevelSave->RestrictedPlotDefinition = Plot;
                    }
                }
            }
        }

        if (PC->HasbIsCreativeModeEnabled())
        {
            PC->bIsCreativeModeEnabled = true;
            PC->OnRep_IsCreativeModeEnabled();
        }
        if (PC->HasbIsCreativeQuickbarEnabled())
            PC->bIsCreativeQuickbarEnabled = true;
        if (PC->HasbIsCreativeQuickmenuEnabled())
            PC->bIsCreativeQuickmenuEnabled = true;
    }

    void ServerStartInteractingHook(UObject* Context, FFrame& Stack)
    {
        UObject* NewActiveMovementMode = nullptr;
        TArray<uint8> NewSelectedActors;
        FTransform NewSelectionToWorld{};

        Stack.StepCompiledIn(&NewActiveMovementMode);
        Stack.StepCompiledIn(&NewSelectedActors);
        Stack.StepCompiledIn(&NewSelectionToWorld);
        Stack.IncrementCode();

        if (!IsLiveObject(Context))
            return;

        auto PlayerController = FindControllerFromObject(Context);
        if (PlayerController)
            EnsureCreativeEditAuthority(PlayerController);

        if (ServerStartInteractingOG && ServerStartInteractingOG != ServerStartInteractingHook)
        {
            auto Function = Stack.GetCurrentNativeFunction();
            Function->ExecFunction = (void*)ServerStartInteractingOG;
            Context->Call<void>(Function, NewActiveMovementMode, NewSelectedActors, NewSelectionToWorld);
            Function->ExecFunction = (void*)ServerStartInteractingHook;
        }
    }

    void ServerMoveSelectionSetHook(UObject* Context, FFrame& Stack)
    {
        UObject* BehaviorHandlingMove = nullptr;
        FTransform NewSelectionToWorld{};
        bool bShouldUpdateOwningClient = false;

        auto Function = Stack.GetCurrentNativeFunction();
        if (Function && Function->GetOffset("BehaviorHandlingMove") != (uint32)-1)
            Stack.StepCompiledIn(&BehaviorHandlingMove);
        Stack.StepCompiledIn(&NewSelectionToWorld);
        if (Function && Function->GetOffset("bShouldUpdateOwningClient") != (uint32)-1)
            Stack.StepCompiledIn(&bShouldUpdateOwningClient);
        Stack.IncrementCode();

        if (!IsLiveObject(Context))
            return;

        auto PlayerController = FindControllerFromObject(Context);
        if (PlayerController)
        {
            EnsureCreativeEditAuthority(PlayerController);

            if (PlayerController->HasCreativePlotLinkedVolume() && IsLiveObject(PlayerController->CreativePlotLinkedVolume))
            {
     
                FVector VolumeLoc = PlayerController->CreativePlotLinkedVolume->K2_GetActorLocation();
                if ((NewSelectionToWorld.Translation - VolumeLoc).SizeSquared() > 100000000.0) 
                    return;
            }
        }
        if (ServerMoveSelectionSetOG && ServerMoveSelectionSetOG != ServerMoveSelectionSetHook)
        {
            Function->ExecFunction = (void*)ServerMoveSelectionSetOG;
            if (Function->GetOffset("BehaviorHandlingMove") != (uint32)-1)
                Context->Call<void>(Function, BehaviorHandlingMove, NewSelectionToWorld, bShouldUpdateOwningClient);
            else
                Context->Call<void>(Function, NewSelectionToWorld, bShouldUpdateOwningClient);
            Function->ExecFunction = (void*)ServerMoveSelectionSetHook;
        }
    }

    void ServerPlaceActorsAndClearMovementModeHook(UObject* Context, FFrame& Stack)
    {
        UObject* BehaviorHandlingMove = nullptr;
        FTransform TargetTransform{};

        auto Function = Stack.GetCurrentNativeFunction();
        if (Function && Function->GetOffset("BehaviorHandlingMove") != (uint32)-1)
            Stack.StepCompiledIn(&BehaviorHandlingMove);
        Stack.StepCompiledIn(&TargetTransform);
        Stack.IncrementCode();

        if (!IsLiveObject(Context))
            return;

        auto PlayerController = FindControllerFromObject(Context);
        if (PlayerController)
            EnsureCreativeEditAuthority(PlayerController);

        if (ServerPlaceActorsAndClearMovementModeOG && ServerPlaceActorsAndClearMovementModeOG != ServerPlaceActorsAndClearMovementModeHook)
        {
            Function->ExecFunction = (void*)ServerPlaceActorsAndClearMovementModeOG;
            if (Function->GetOffset("BehaviorHandlingMove") != (uint32)-1)
                Context->Call<void>(Function, BehaviorHandlingMove, TargetTransform);
            else
                Context->Call<void>(Function, TargetTransform);
            Function->ExecFunction = (void*)ServerPlaceActorsAndClearMovementModeHook;
        }
    }

    void ServerClearMovementModeHook(UObject* Context, FFrame& Stack)
    {
        bool bExited = false;
        auto Function = Stack.GetCurrentNativeFunction();
        if (Function && Function->GetOffset("bExited") != (uint32)-1)
            Stack.StepCompiledIn(&bExited);
        Stack.IncrementCode();

        if (!IsLiveObject(Context))
            return;

        auto PlayerController = FindControllerFromObject(Context);
        if (PlayerController)
            EnsureCreativeEditAuthority(PlayerController);

        if (ServerClearMovementModeOG && ServerClearMovementModeOG != ServerClearMovementModeHook)
        {
            Function->ExecFunction = (void*)ServerClearMovementModeOG;
            if (Function->GetOffset("bExited") != (uint32)-1)
                Context->Call<void>(Function, bExited);
            else
                Context->Call<void>(Function);
            Function->ExecFunction = (void*)ServerClearMovementModeHook;
        }
    }

    void PhoneTargetStartInteractingHook(UObject* Context, FFrame& Stack)
    {
        TArray<UObject*> Targets;
        FTransform DragStart{};
        Stack.StepCompiledIn(&Targets);
        Stack.StepCompiledIn(&DragStart);
        Stack.IncrementCode();

        if (!IsLiveObject(Context))
            return;

        auto PlayerController = FindControllerFromObject(Context);
        if (PlayerController)
            EnsureCreativeEditAuthority(PlayerController);


        if (PhoneTargetStartInteractingOG && PhoneTargetStartInteractingOG != PhoneTargetStartInteractingHook)
        {
            auto Function = Stack.GetCurrentNativeFunction();
            Function->ExecFunction = (void*)PhoneTargetStartInteractingOG;
            Context->Call<void>(Function, Targets, DragStart);
            Function->ExecFunction = (void*)PhoneTargetStartInteractingHook;
        }
    }
} 

void AFortMinigameSettingsBuilding::BeginPlay(AFortMinigameSettingsBuilding* Settings)
{
    return BeginPlayOG(Settings);
}

AFortAthenaCreativePortal* AFortAthenaCreativePortal::Create(AFortPlayerControllerAthena* PlayerController)
{
    auto World = UWorld::GetWorld();
    if (!World || !PlayerController || !PlayerController->PlayerState || !World->GameState)
        return nullptr;

    auto GameState = (AFortGameStateAthena*)World->GameState;
    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

    if (!GameState->HasCreativePortalManager() || !GameState->CreativePortalManager)
        return nullptr;

    AFortAthenaCreativePortal* Portal = nullptr;

    if (GameState->CreativePortalManager->HasAvailablePortals() && GameState->CreativePortalManager->AvailablePortals.Num() > 0)
    {
        Portal = (AFortAthenaCreativePortal*)GameState->CreativePortalManager->AvailablePortals[0];
        GameState->CreativePortalManager->AvailablePortals.Remove(0);
        if (GameState->CreativePortalManager->HasUsedPortals())
            GameState->CreativePortalManager->UsedPortals.Add(Portal);
    }

    if (!Portal || !Portal->HasLinkedVolume() || !Portal->LinkedVolume)
    {
        printf("[PLATINUM] Failed to find portal!\n");
        return nullptr;
    }

    printf("[PLATINUM] Assigned portal %s to %s\n", Portal->Name.ToString().c_str(), PlayerController->Name.ToString().c_str());

    if (Portal->HasbIsPublishedPortal())
    {
        Portal->bIsPublishedPortal = false;
        Portal->OnRep_PublishedPortal();
    }
    if (Portal->HasOwningPlayer())
    {
        Portal->OwningPlayer = PlayerState->UniqueId;
        Portal->OnRep_OwningPlayer();
    }
    if (Portal->HasbPortalOpen())
    {
        Portal->bPortalOpen = true;
        Portal->OnRep_PortalOpen();
    }

    auto Plot = ResolveCreativePlot();
    if (!Plot || !Plot->HasBasePlayset())
        return Portal;

    auto IslandPlayset = Plot->BasePlayset.Get();
    if (!IslandPlayset)
        return Portal;

    auto LevelSaveComponent = (UFortLevelSaveComponent*)Portal->LinkedVolume->GetComponentByClass(UFortLevelSaveComponent::StaticClass());
    if (LevelSaveComponent)
    {
        if (LevelSaveComponent->HasAccountIdOfOwner())
            LevelSaveComponent->AccountIdOfOwner = PlayerState->UniqueId;
        if (LevelSaveComponent->HasbIsLoaded())
            LevelSaveComponent->bIsLoaded = true;
        if (LevelSaveComponent->HasRestrictedPlotDefinition())
            LevelSaveComponent->RestrictedPlotDefinition = Plot;
    }

    auto LevelStreamComponent = (UPlaysetLevelStreamComponent*)Portal->LinkedVolume->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass());
    if (LevelStreamComponent)
    {
        if (auto SetPlayset = LevelStreamComponent->GetFunction("SetPlayset"))
            LevelStreamComponent->Call<void>(SetPlayset, IslandPlayset);
        else if (LevelStreamComponent->HasCurrentPlayset())
            LevelStreamComponent->CurrentPlayset = IslandPlayset;
    }

    auto LoadPlayset = (void (*)(UPlaysetLevelStreamComponent*))FindLoadPlayset();
    if (LoadPlayset && LevelStreamComponent)
        LoadPlayset(LevelStreamComponent);

    if (Portal->LinkedVolume->HasVolumeState())
    {
        Portal->LinkedVolume->VolumeState = 3;
        Portal->LinkedVolume->OnRep_VolumeState();
    }
    if (Portal->LinkedVolume->HasbNeverAllowSaving())
        Portal->LinkedVolume->bNeverAllowSaving = false;
    Portal->LinkedVolume->ForceNetUpdate();

    if (PlayerController->HasOwnedPortal())
        PlayerController->OwnedPortal = Portal;
    if (PlayerController->HasCreativePlotLinkedVolume())
    {
        PlayerController->CreativePlotLinkedVolume = Portal->LinkedVolume;
        PlayerController->OnRep_CreativePlotLinkedVolume();
    }

    printf("[Creative] Setup portal!\n");
    return Portal;
}

void AFortAthenaCreativePortal::TeleportPlayerToLinkedVolume(UObject* Context, FFrame& Stack)
{
    AFortPlayerPawnAthena* PlayerPawn = nullptr;
    bool bUseSpawnTags;
    Stack.StepCompiledIn(&PlayerPawn);
    Stack.StepCompiledIn(&bUseSpawnTags);
    Stack.IncrementCode();
    auto Portal = (AFortAthenaCreativePortal*)Context;

    if (!Portal || !PlayerPawn || !PlayerPawn->Controller || !Portal->HasLinkedVolume() || !Portal->LinkedVolume)
        return;

    auto PlayerController = (AFortPlayerControllerAthena*)PlayerPawn->Controller;
    static auto CreativePhone = FindObject<UFortWeaponItemDefinition>(L"/Game/Athena/Items/Weapons/Prototype/WID_CreativeTool.WID_CreativeTool");
    if (CreativePhone && PlayerController->WorldInventory)
    {
        auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) { return entry.ItemDefinition == CreativePhone; }, FFortItemEntry::Size());
        if (!ItemEntry)
        {
            auto Stats = AFortInventory::GetStats(CreativePhone);
            int32 ClipSize = Stats ? Stats->ClipSize : 0;
            PlayerController->WorldInventory->GiveItem(CreativePhone, 1, ClipSize);
            PlayerController->ClientCreativePhoneCreated();
        }
    }

    if (PlayerController->HasbIsCreativeQuickbarEnabled())
    {
        auto Old = PlayerController->bIsCreativeQuickbarEnabled;
        PlayerController->bIsCreativeQuickbarEnabled = true;
        PlayerController->OnRep_IsCreativeQuickbarEnabled(Old);
    }
    if (PlayerController->HasbIsCreativeQuickmenuEnabled())
        PlayerController->bIsCreativeQuickmenuEnabled = true;
    if (PlayerController->HasbIsCreativeModeEnabled())
    {
        PlayerController->bIsCreativeModeEnabled = true;
        PlayerController->OnRep_IsCreativeModeEnabled();
    }

    if (PlayerController->HasCreativePlotLinkedVolume())
    {
        PlayerController->CreativePlotLinkedVolume = Portal->LinkedVolume;
        PlayerController->OnRep_CreativePlotLinkedVolume();
    }

    auto Location = Portal->LinkedVolume->K2_GetActorLocation();
    Location.Z = 10000;
    PlayerPawn->K2_TeleportTo(Location, FRotator());
    PlayerPawn->BeginSkydiving(false);
}

void AFortAthenaCreativePortal::Hook()
{
    if (!GetDefaultObj())
        return;

    Hooking::ExecHook(GetDefaultObj()->GetFunction("TeleportPlayerToLinkedVolume"), TeleportPlayerToLinkedVolume);

    if (auto MoveToolClass = SDK::FindClass("FortCreativeMoveTool"))
    {
        if (auto MoveToolDefault = MoveToolClass->GetDefaultObj())
        {
            if (auto Func = MoveToolDefault->GetFunction("ServerStartInteracting"))
                Hooking::ExecHook(Func, ServerStartInteractingHook, ServerStartInteractingOG);
            if (auto Func = MoveToolDefault->GetFunction("ServerMoveSelectionSet"))
                Hooking::ExecHook(Func, ServerMoveSelectionSetHook, ServerMoveSelectionSetOG);
            if (auto Func = MoveToolDefault->GetFunction("ServerPlaceActorsAndClearMovementMode"))
                Hooking::ExecHook(Func, ServerPlaceActorsAndClearMovementModeHook, ServerPlaceActorsAndClearMovementModeOG);
            if (auto Func = MoveToolDefault->GetFunction("ServerClearMovementMode"))
                Hooking::ExecHook(Func, ServerClearMovementModeHook, ServerClearMovementModeOG);
            if (auto Func = MoveToolDefault->GetFunction("ServerStartInteractingWithTargets"))
                Hooking::ExecHook(Func, PhoneTargetStartInteractingHook, PhoneTargetStartInteractingOG);
        }
    }
}

void AFortMinigameSettingsBuilding::Hook()
{
    if (!GetDefaultObj())
        return;
}