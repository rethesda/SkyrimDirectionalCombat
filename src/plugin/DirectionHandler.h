#pragma once

#include "UIMenu.h"
#include "Direction.h"
#include "Utils.h"

#include <shared_mutex>
#include <unordered_set>
#include "parallel_hashmap/phmap.h"


class DirectionHandler
{
public:

	DirectionHandler()
	{
		TR = nullptr;
		TL = nullptr;
		BL = nullptr;
		BR = nullptr;
		Debuff = nullptr;
		Unblockable = nullptr;
		NPCKeyword = nullptr;
		ParryVFX = nullptr;
		BattleaxeKeyword = nullptr;
		PikeKeyword = nullptr;
	}
	static DirectionHandler* GetSingleton()
	{
		static DirectionHandler obj;
		return std::addressof(obj);
	}

	bool HasDirectionalPerks(RE::Actor* actor) const;
	bool HasBlockAngle(RE::Actor* attacker, RE::Actor* target) const;
	void AddDirectional(RE::Actor* actor, RE::TESObjectWEAP* weapon);
	void SwitchDirectionLeft(RE::Actor* actor);
	void SwitchDirectionUp(RE::Actor* actor);
	void SwitchDirectionDown(RE::Actor* actor);
	void SwitchDirectionRight(RE::Actor* actor);
	void WantToSwitchTo(RE::Actor* actor, Directions dir, bool force = false);
	RE::SpellItem* DirectionToPerk(Directions dir) const;
	RE::SpellItem* GetDirectionalPerk(RE::Actor* actor) const;
	Directions PerkToDirection(RE::SpellItem* perk) const;
	inline Directions GetCurrentDirection(RE::Actor* actor) const
	{
		auto Iter = ActiveDirections.find(actor->GetHandle());
		if (Iter != ActiveDirections.end())
		{
			return Iter->second;
		}
		return Directions::TR;
	}
	void RemoveDirectionalPerks(RE::Actor* actor);
	void UIDrawAngles(RE::Actor* actor);
	bool DetermineMirrored(RE::Actor* actor);

	void Initialize();
	void UpdateCharacter(RE::Actor* actor, float delta);
	void Update(float delta);
	void SendAnimationEvent(RE::Actor* actor);
	void DebuffActor(RE::Actor* actor);
	void AddCombo(RE::Actor* actor);
	inline unsigned GetRepeatCount(RE::Actor* actor) const
	{
		auto Iter = ComboDatas.find(actor->GetHandle());
		if (Iter != ComboDatas.end())
		{
			return Iter->second.repeatCount;
		}
		return 0;
	}
	inline bool IsUnblockable(RE::Actor* actor) const
	{
		return UnblockableActors.contains(actor->GetHandle());
	}
	inline bool HasImperfectParry(RE::Actor* actor) const
	{
		return ImperfectParry.contains(actor->GetHandle());
	}

	void Cleanup();

	void StartedAttackWindow(RE::Actor* actor)
	{
		SendAnimationEvent(actor);
		InAttackWin.insert(actor->GetHandle());
	}

	void EndedAttackWindow(RE::Actor* actor)
	{
		SendAnimationEvent(actor);
		InAttackWin.erase(actor->GetHandle());
	}
private:
	void CleanupActor(RE::Actor* actor);
	RE::SpellItem* TR;
	RE::SpellItem* TL;
	RE::SpellItem* BL;
	RE::SpellItem* BR;
	
	RE::SpellItem* Unblockable;
	RE::BGSPerk* Debuff;
	RE::TESObjectACTI* ParryVFX;
	RE::BGSKeyword* NPCKeyword;

	// This is so incredibly specific i hate having it
	RE::BGSKeyword* BattleaxeKeyword;
	RE::BGSKeyword* PikeKeyword;

	// is mapping really better?
	// 5 compares versus hashing?
	//std::map<RE::BGSPerk*, Directions> PerkToDirection;
	//std::map<Directions, RE::BGSPerk*> DirectionToPerk;
	 

	bool CanSwitch(RE::Actor* actor);
	void SwitchDirectionSynchronous(RE::Actor* actor, Directions dir);
	struct DirectionSwitch
	{
		Directions dir;
		float timeLeft;
	};
	// The direction switches are queued as we don't want instant guard switches
	phmap::parallel_flat_hash_map<RE::ActorHandle, DirectionSwitch> DirectionTimers;

	// The transition is slower than the actual guard break time since it looks better,
	// so we need to queue the forceidle events as skyrim does not allow blending multiple animations
	// during blending another animation transition
	phmap::parallel_flat_hash_map<RE::ActorHandle, std::queue<float>> AnimationTimer;

	struct ComboData
	{
		// circular array
		std::vector<Directions> lastAttackDirs;
		int currentIdx = 0;
		int repeatCount = 0;
		int size = 0;
		float timeLeft = 0.f;
	};

	// Metadata to handle combos and punishing repeated attacks
	phmap::parallel_flat_hash_map<RE::ActorHandle, ComboData> ComboDatas;

	// To switch directions after the hitframe but still in attack state for fluid animations
	// set uses hash so is fast?
	phmap::parallel_flat_hash_set<RE::ActorHandle> InAttackWin;

	// Have to record directions here
	// Because of the way skyrim handles spells, we need these to be totally synchronous
	phmap::parallel_flat_hash_map<RE::ActorHandle, Directions> ActiveDirections;

	// Set to determine who is unblockable
	phmap::parallel_flat_hash_set<RE::ActorHandle> UnblockableActors;

	// Determine who has an imperfect parry
	phmap::parallel_flat_hash_set<RE::ActorHandle> ImperfectParry;
};