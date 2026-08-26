#include "ReEchoAudioEvents.h"

// Music states
const FName FReEchoAudioEvents::MusicMenu = FName(TEXT("Music.Menu"));
const FName FReEchoAudioEvents::MusicEncounter = FName(TEXT("Music.Encounter"));
const FName FReEchoAudioEvents::MusicBoss = FName(TEXT("Music.Boss"));
const FName FReEchoAudioEvents::MusicShop = FName(TEXT("Music.Shop"));
const FName FReEchoAudioEvents::MusicDeath = FName(TEXT("Music.Death"));
const FName FReEchoAudioEvents::MusicVictory = FName(TEXT("Music.Victory"));

// Ambience states
const FName FReEchoAudioEvents::AmbienceArena = FName(TEXT("Ambience.Arena"));
const FName FReEchoAudioEvents::AmbienceRain = FName(TEXT("Ambience.Rain"));

// UI
const FName FReEchoAudioEvents::UiHover = FName(TEXT("UI.Hover"));
const FName FReEchoAudioEvents::UiConfirm = FName(TEXT("UI.Confirm"));
const FName FReEchoAudioEvents::UiCancel = FName(TEXT("UI.Cancel"));
const FName FReEchoAudioEvents::UiError = FName(TEXT("UI.Error"));
const FName FReEchoAudioEvents::UiPurchase = FName(TEXT("UI.Purchase"));
const FName FReEchoAudioEvents::UiCardSelect = FName(TEXT("UI.CardSelect"));
const FName FReEchoAudioEvents::UiCardReveal = FName(TEXT("UI.CardReveal"));
const FName FReEchoAudioEvents::UiEquip = FName(TEXT("UI.Equip"));
const FName FReEchoAudioEvents::UiUnequip = FName(TEXT("UI.Unequip"));

// Combat
const FName FReEchoAudioEvents::CombatAttack = FName(TEXT("Combat.Attack"));
const FName FReEchoAudioEvents::CombatHit = FName(TEXT("Combat.Hit"));
const FName FReEchoAudioEvents::CombatBlock = FName(TEXT("Combat.Block"));
const FName FReEchoAudioEvents::CombatHurt = FName(TEXT("Combat.Hurt"));
const FName FReEchoAudioEvents::CombatKill = FName(TEXT("Combat.Kill"));
const FName FReEchoAudioEvents::CombatDeath = FName(TEXT("Combat.Death"));
const FName FReEchoAudioEvents::CombatReaction = FName(TEXT("Combat.Reaction"));

// Enemy
const FName FReEchoAudioEvents::EnemySpawn = FName(TEXT("Enemy.Spawn"));
const FName FReEchoAudioEvents::EnemyAttack = FName(TEXT("Enemy.Attack"));
const FName FReEchoAudioEvents::EnemyDeath = FName(TEXT("Enemy.Death"));
const FName FReEchoAudioEvents::BossSpawn = FName(TEXT("Boss.Spawn"));
const FName FReEchoAudioEvents::BossAttack = FName(TEXT("Boss.Attack"));
const FName FReEchoAudioEvents::BossDeath = FName(TEXT("Boss.Death"));

// Echo
const FName FReEchoAudioEvents::EchoSpawn = FName(TEXT("Echo.Spawn"));
const FName FReEchoAudioEvents::EchoAttack = FName(TEXT("Echo.Attack"));
const FName FReEchoAudioEvents::EchoEnd = FName(TEXT("Echo.End"));

// Misc
const FName FReEchoAudioEvents::CameraMove = FName(TEXT("CameraMove"));
const FName FReEchoAudioEvents::Revive = FName(TEXT("Revive"));
const FName FReEchoAudioEvents::ItemPickup = FName(TEXT("Item.Pickup"));
const FName FReEchoAudioEvents::FlowVictory = FName(TEXT("Flow.Victory"));
