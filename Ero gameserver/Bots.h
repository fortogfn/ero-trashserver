#pragma once
#include "ue.h"
#include <intrin.h>

vector<class Bot2*> Bots2{};

class Bot2
{
public:
	ABP_PhoebePlayerController_C* PC;
	AFortPlayerPawnAthena* Pawn;
	bool bTickEnabled = false;
	AActor* CurrentTarget = nullptr;
	UFortWeaponItemDefinition* Weapon = nullptr;
	FGuid WeaponGuid;
	int shotCounter = 0;
	uint64_t tick_counter = 0;

public:
	Bot2(AFortPlayerPawnAthena* Pawn)
	{
		this->Pawn = Pawn;
		PC = (ABP_PhoebePlayerController_C*)Pawn->Controller;
		Bots2.push_back(this);
	}

public:
	FVector FindllamaSpawn(AFortAthenaMapInfo* MapInfo, FVector Center, float Radius)
	{
		static FVector* (*PickSupplyDropLocationOriginal)(AFortAthenaMapInfo * MapInfo, FVector * outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOriginal)(__int64(GetModuleHandleA(0)) + 0x18848f0);

		if (!PickSupplyDropLocationOriginal)
			return FVector(0, 0, 0);


		FVector Out = FVector(0, 0, 0);
		auto ahh = PickSupplyDropLocationOriginal(MapInfo, &Out, __int64(&Center), Radius);
		return Out;
	}
	bool spawned = false;

	void Spawnllama() {
		auto MI = GetGameState()->MapInfo;
		int numPlayers = GetGameState()->GameMemberInfoArray.Members.Num();

		FFortAthenaAIBotRunTimeCustomizationData runtimeData;
		runtimeData.CustomSquadId = 3 + numPlayers;

		FRotator RandomYawRotator{};
		RandomYawRotator.Yaw = (float)rand() * 0.010986663f;

		int Radius = 100000;
		FVector Location = FindllamaSpawn(MI, FVector(1, 1, 10000), (float)Radius);

		auto Llama = SpawnActor<AFortAthenaSupplyDrop>(MI->LlamaClass.Get(), Location, RandomYawRotator);

		auto GroundLocation = Llama->FindGroundLocationAt(Location);

		Llama->K2_DestroyActor();

		spawned = true;
	}

	FGuid GetItem(UFortItemDefinition* Def)
	{
		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid;
		}
		return FGuid();
	}

	void GiveItem(UFortItemDefinition* ODef, int Count = 1, bool equip = true)
	{
		UFortItemDefinition* Def = ODef;
		if (Def->GetName() == "AGID_Boss_Tina") {
			Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/Boss/WID_Boss_Tina.WID_Boss_Tina");
		}
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->OwnerInventory = PC->Inventory;

		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			Item->ItemEntry.LoadedAmmo = GetAmmoForDef((UFortWeaponItemDefinition*)Def);
		}

		PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->Inventory->Inventory.ItemInstances.Add(Item);
		PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->Inventory->HandleInventoryLocalUpdate();
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) && equip)
		{
			this->Weapon = (UFortWeaponItemDefinition*)ODef;
			this->WeaponGuid = GetItem(Def);
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Def, GetItem(Def));
		}
	}

	virtual void OnPerceptionSensed(AActor* SourceActor, FAIStimulus& Stimulus)
	{
		if (Stimulus.bSuccessfullySensed == 1 && PC->LineOfSightTo(SourceActor, FVector(), true) && Pawn->GetDistanceTo(SourceActor) < 5000)
		{
			CurrentTarget = SourceActor;
			cout << "bSuccessfullySensed" << endl;
		}
	}
};


// will bot shoot
int RandomNumGen() {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	int randomNumber = (std::rand() % 26) + 1;
	//std::cout << "Random number between 1 and 3: " << randomNumber << std::endl;
	return randomNumber;
}

void TickBots()
{
	// AFortGameStateAthena
	for (auto bot : Bots2)
	{
		if (bot->bTickEnabled && bot->Pawn && !bot->Pawn->bIsDBNO && bot->CurrentTarget)
		{
			if (bot->Pawn->CurrentWeapon && bot->Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
			{
				auto BotPos = bot->Pawn->K2_GetActorLocation();
				auto TargetPos = bot->CurrentTarget->K2_GetActorLocation();
				float Distance = bot->Pawn->GetDistanceTo(bot->CurrentTarget);

				if (Distance > 800)
				{
					bot->PC->MoveToActor(bot->CurrentTarget, 0, true, false, true, nullptr, true);
				}
				else
				{
					bot->PC->StopMovement();
					if (!bot->Pawn->bIsCrouched && GetMath()->RandomBoolWithWeight(0.05f))
					{
						bot->Pawn->Crouch(false);
					}
					else if (bot->Pawn->bIsCrouched)
					{
						bot->Pawn->UnCrouch(false);
					}
				}

				int WillShoot = RandomNumGen();

				if (bot->Weapon)
				{
					bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);

					if (bot->tick_counter % 30 == 0) {
						if (bot->Weapon) {
							bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
							bot->Pawn->PawnStopFire(0); // some reason if i dont add this they will fire even tho there is no player or anything for it to trigger a fire
						}
					}

					if (bot->shotCounter >= 0)
					{
						if (WillShoot == 1) {
							printf("[EON AI]: Can Hit player!\n");

							float RandomYaw = GetMath()->RandomFloatInRange(-120.0f, 120.0f);
							float RandomPitch = GetMath()->RandomFloatInRange(-90.0f, 90.0f);

							FVector TargetPosTest{ TargetPos.X + RandomPitch, TargetPos.Y + RandomYaw, TargetPos.Z };

							auto TestRot = GetMath()->FindLookAtRotation(BotPos, TargetPosTest);
							bot->PC->SetControlRotation(TestRot);
							bot->Pawn->PawnStartFire(0);

						}
						else {
							float RandomYaw = GetMath()->RandomFloatInRange(-280.0f, 280.0f);
							float RandomPitch = GetMath()->RandomFloatInRange(-120.0f, 120.0f);

							FVector TargetPosFake{ TargetPos.X + RandomPitch, TargetPos.Y + RandomYaw, TargetPos.Z };
							auto TestRot = GetMath()->FindLookAtRotation(BotPos, TargetPosFake);
							bot->PC->SetControlRotation(TestRot);
							bot->Pawn->PawnStartFire(0);
						}
					}
				}
			}
		}
		else if (bot->bTickEnabled && bot->Pawn && bot->Pawn->bIsDBNO)
		{
			bot->CurrentTarget = nullptr;
			bot->PC->StopMovement();
		}
	}
}

wchar_t* (*OnPerceptionSensedOG)(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus);
wchar_t* OnPerceptionSensed(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus)
{
	if (SourceActor->IsA(AFortPlayerPawnAthena::StaticClass()) && Cast<AFortPlayerPawnAthena>(SourceActor)->Controller && !Cast<AFortPlayerPawnAthena>(SourceActor)->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass()) /*!Cast<AFortPlayerPawnAthena>(SourceActor)->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass())*/)
	{
		for (auto bot : Bots2)
		{
			if (bot->PC == PC)
			{
				bot->OnPerceptionSensed(SourceActor, Stimulus);
			}
		}
	}
	return OnPerceptionSensedOG(PC, SourceActor, Stimulus);
}
void (*OnPossessedPawnDiedOG)(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
void OnPossessedPawnDied(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
{
	Bot2* KilledBot = nullptr;
	for (size_t i = 0; i < Bots2.size(); i++)
	{
		auto bot = Bots2[i];
		if (bot && bot->PC == PC)
		{
			if (bot->Pawn->GetName().starts_with("BP_Pawn_DangerGrape_")) {
				goto nodrop;
			}
			else {
				KilledBot = bot;
			}
		}
	}
	PC->PlayerBotPawn->SetMaxShield(0);
	for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()))
			continue;
		auto Def = PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition;
		if (Def->GetName() == "AGID_Athena_Keycard_Yacht") {
			goto nodrop;
		}
		if (Def->GetName() == "WID_Boss_Tina") {
			Def = KilledBot->Weapon;
		}
		SpawnPickup(PC->Pawn->K2_GetActorLocation(), Def, PC->Inventory->Inventory.ReplicatedEntries[i].Count, PC->Inventory->Inventory.ReplicatedEntries[i].LoadedAmmo, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
		int Ammo = 0;
		int AmmoC = 0;
		if (Def->GetName() == "WID_Boss_Hos_MG") {
			Ammo = 60;
			AmmoC = 60;
		}
		else if (Def->GetName().starts_with("WID_Assault_LMG_Athena_")) {
			Ammo = 45;
			AmmoC = 45;
		}
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			UFortWeaponItemDefinition* Def2 = (UFortWeaponItemDefinition*)Def;
			SpawnPickup(PC->Pawn->K2_GetActorLocation(), Def2->GetAmmoWorldItemDefinition_BP(), AmmoC != 0 ? AmmoC : GetAmmoForDef(Def2), Ammo != 0 ? Ammo : GetAmmoForDef(Def2), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
		}
	}

nodrop:
	for (size_t i = 0; i < Bots2.size(); i++)
	{
		auto bot = Bots2[i];
		if (bot && bot->PC == PC)
		{
			Bots2.erase(Bots2.begin() + i);
			break;
		}
	}

	return OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
}

void spawnMeowscles();

AFortPlayerPawnAthena* (*SpawnBotOG)(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData);
AFortPlayerPawnAthena* SpawnBot(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData)
{
	if (__int64(_ReturnAddress()) - __int64(GetModuleHandleW(0)) == 0x1A4153F)
		return SpawnBotOG(BotManager, SpawnLoc, SpawnRot, BotData, RuntimeBotData);

	spawnMeowscles();

	cout << BotData->GetFullName() << endl;

	if (BotData->GetFullName().contains("MANG_POI_Yacht"))
	{
		BotData = StaticLoadObject<UFortAthenaAIBotCustomizationData>("/Game/Athena/AI/MANG/BotData/BotData_MANG_POI_HDP.BotData_MANG_POI_HDP");
	}

	if (BotData->CharacterCustomization->CustomizationLoadout.Character->GetName() == "CID_556_Athena_Commando_F_RebirthDefaultA")
	{
		std::string Tag = RuntimeBotData.PredefinedCosmeticSetTag.TagName.ToString();
		if (Tag == "Athena.Faction.Alter") {
			BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanBad.CID_NPC_Athena_Commando_M_HenchmanBad");
		}
		else if (Tag == "Athena.Faction.Ego") {
			BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanGood.CID_NPC_Athena_Commando_M_HenchmanGood");
		}
	}

	AActor* SpawnLocator = SpawnActor<ADefaultPawn>(SpawnLoc, SpawnRot);
	AFortPlayerPawnAthena* Ret = BotMutator->SpawnBot(BotData->PawnClass, SpawnLocator, SpawnLoc, SpawnRot, true);
	AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Ret->Controller;
	PC->CosmeticLoadoutBC = BotData->CharacterCustomization->CustomizationLoadout;
	for (size_t i = 0; i < BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations.Num(); i++)
	{
		UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(Conv_NameToString(BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());

		if (Spec)
		{
			for (size_t i = 0; i < Spec->CharacterParts.Num(); i++)
			{
				UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(Conv_NameToString(Spec->CharacterParts[i].ObjectID.AssetPathName).ToString());
				Ret->ServerChoosePart(Part->CharacterPartType, Part);
			}
		}
	}

	Ret->CosmeticLoadout = BotData->CharacterCustomization->CustomizationLoadout;
	Ret->OnRep_CosmeticLoadout();

	cout << BotData->CharacterCustomization->CustomizationLoadout.Character->GetName() << endl;

	SpawnLocator->K2_DestroyActor();
	DWORD CustomSquadId = RuntimeBotData.CustomSquadId;
	BYTE TrueByte = 1;
	BYTE FalseByte = 0;
	BotManagerSetupStuffIdk(__int64(BotManager), __int64(Ret), __int64(BotData->BehaviorTree), 0, &CustomSquadId, 0, __int64(BotData->StartupInventory), __int64(BotData->BotNameSettings), 0, &FalseByte, 0, &TrueByte, RuntimeBotData);

	Bot2* bot = new Bot2(Ret);

	for (int32 i = 0; i < BotData->StartupInventory->Items.Num(); i++)
	{
		bool equip = true;
		if (BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Athena_FloppingRabbit") || BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Boss_Adventure_GH")) {
			equip = false;
		}
		bot->GiveItem(BotData->StartupInventory->Items[i], 1, equip);
		if (auto Data = Cast<UFortWeaponItemDefinition>(BotData->StartupInventory->Items[i]))
		{
			if (Data->GetAmmoWorldItemDefinition_BP() && Data->GetAmmoWorldItemDefinition_BP() != Data)
			{
				bot->GiveItem(Data->GetAmmoWorldItemDefinition_BP(), 99999);
			}
		}
	}
	bot->bTickEnabled = true;
	return Ret;
}

void spawnMeowscles() {
	static bool meowsclesSpawned = false;
	if (meowsclesSpawned) return;
	meowsclesSpawned = true;

	UFortAthenaAIBotCustomizationData* customization = StaticLoadObject<UFortAthenaAIBotCustomizationData>("/Game/Athena/AI/MANG/BotData/BotData_MANG_POI_HMW.BotData_MANG_POI_HMW");
	FFortAthenaAIBotRunTimeCustomizationData runtimeData{};
	runtimeData.CustomSquadId = 0;

	FVector SpawnLocs[] = {
		// First spawn location
		{
			-69728.0,
			81376.0,
			5684.0
		},
		// Second spawn location
		{
			-70912.0,
			81376.0,
			5684.0
		},
		// Third spawn location
		{
			-67696.0,
			81540.0,
			5672.0
		},
	};

	FRotator Rotation = {
		0.0,
		-179.9999f,
		0.0
	};

	int32 SpawnCount = sizeof(SpawnLocs) / sizeof(FVector);
	int32 randomIndex = static_cast<int32>(GetMath()->RandomFloatInRange(0.f, static_cast<float>(SpawnCount) - 1));

	auto Meowscles = SpawnBot(GetGameMode()->ServerBotManager, SpawnLocs[randomIndex], Rotation, customization, runtimeData);
	if (!Meowscles) {
		return;
	}
	Meowscles->SetMaxShield(400);
	Meowscles->SetShield(400);
}

