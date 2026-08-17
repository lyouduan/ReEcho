#pragma once

#include "CoreMinimal.h"

/**
 * Stable FName constants for audio events and state ids.
 *
 * Gameplay and the catalog reference these instead of scattering raw strings.
 * Do NOT add a hardcoded constant per WeaponId/EnemyId: weapon and enemy
 * variants are passed through VariantId / SourceCategory and resolved by the
 * (future) designer catalog instead.
 */
struct REECHOAUDIO_API FReEchoAudioEvents
{
	// ---- Music states (used as Music-channel state ids) ----
	static const FName MusicMenu;		// "Music.Menu"
	static const FName MusicEncounter; // "Music.Encounter"
	static const FName MusicBoss;		// "Music.Boss"
	static const FName MusicShop;		// "Music.Shop"
	static const FName MusicDeath;		// "Music.Death"
	static const FName MusicVictory;	// "Music.Victory"

	// ---- Ambience states (used as Ambience-channel state ids) ----
	static const FName AmbienceArena; // "Ambience.Arena"
	static const FName AmbienceRain;	// "Ambience.Rain"

	// ---- UI one-shot events ----
	static const FName UiHover;		// "UI.Hover"
	static const FName UiConfirm;		// "UI.Confirm"
	static const FName UiCancel;		// "UI.Cancel"
	static const FName UiError;		// "UI.Error"
	static const FName UiPurchase;		// "UI.Purchase"
	static const FName UiCardSelect;	// "UI.CardSelect"

	// ---- Combat one-shot events ----
	static const FName CombatAttack; // "Combat.Attack"
	static const FName CombatHit;		// "Combat.Hit"
	static const FName CombatBlock;		// "Combat.Block"
	static const FName CombatHurt;		// "Combat.Hurt"
	static const FName CombatKill;		// "Combat.Kill"
	static const FName CombatDeath;		// "Combat.Death"

	// ---- Enemy one-shot events ----
	static const FName EnemySpawn;		// "Enemy.Spawn"
	static const FName EnemyAttack;		// "Enemy.Attack"
	static const FName EnemyDeath;		// "Enemy.Death"
	static const FName BossSpawn;		// "Boss.Spawn"
	static const FName BossAttack;		// "Boss.Attack"
	static const FName BossDeath;		// "Boss.Death"

	// ---- Echo (player echo ability) one-shot events ----
	static const FName EchoSpawn; // "Echo.Spawn"
	static const FName EchoAttack; // "Echo.Attack"
	static const FName EchoEnd;	 // "Echo.End"

	// ---- Misc ----
	static const FName CameraMove; // "CameraMove"
	static const FName Revive;	  // "Revive" (death-restart arrival; does not create revive gameplay)
};
