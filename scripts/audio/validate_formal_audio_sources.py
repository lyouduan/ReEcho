"""Verify the exact user-delivered Plan114 direct-replacement source bytes."""

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

EXPECTED_SHA256 = {
    "Design/Audio/Source/Music/Music_Menu.mp3": "dd598665d3ae5624dea06956974d3ca844abf9c860c0cb0edd5ad37bd393d880",
    "Design/Audio/Source/Music/Music_Shop.mp3": "9bd5830a0a2166c592223d7f355e04df22df1cefa81fa8c041a0c19a26f3f4a0",
    "Design/Audio/Source/Music/Music_Boss.mp3": "8e13193b3775008079c507bd9b147f5769a5ded7b01f811de4baaad8457d9fc3",
    "Design/Audio/Source/Formal/UI/UI_Hover.mp3": "67d37f70b23249af1dcfed8156cb53e35660c4689b9c51b56e92ded846f22333",
    "Design/Audio/Source/Formal/UI/UI_Confirm.mp3": "64d9331804512e308f52ecbac75e0124b79b6ed46144ec740d9b6129a7880156",
    "Design/Audio/Source/Formal/UI/UI_Cancel.wav": "c5a7cd3ed51c37861898155fdb024700d92ee2e8a188713dd65692c47d36821b",
    "Design/Audio/Source/Formal/UI/UI_Purchase.wav": "c252826d1a1fc3eb45ef2a3586dc7ea155c013af2a8df549eb0d99e1c4fb529d",
    "Design/Audio/Source/Formal/UI/UI_CardSelect.wav": "6be0d30f185abd5df1aed287cdfb1f293b5b3f6030911e7f2f0a35c6b15bb5c5",
    "Design/Audio/Source/Formal/Combat/Combat_Hurt.wav": "4cd439549c4ed1f258dcd9fffb5edf9f7618eb1b276227557d3e401a1d9f62ee",
    "Design/Audio/Source/Formal/Combat/Combat_Death.wav": "b9ea39cae4c135c9c522d7bb06fd584dd315e8a0489bc51f3407c3cc6e733b5e",
    "Design/Audio/Source/Formal/Enemy/Enemy_Death.mp3": "d69fe3c797559854561325f0a4625def8f22042e19d4e2e34f4a0eb910c607ca",
    "Design/Audio/Source/Formal/Enemy/Enemy_Spawn_Source.wav": "c6fd22f3536f66a852de21ae620a0922f8872e8c24a49d292c123eaa987d9703",
    "Design/Audio/Source/Formal/Boss/Boss_Death.wav": "6dc0c505aa964aedcf4e29138fb577f3fccad060e94b58ed641f8a218f8db237",
    "Design/Audio/Source/Formal/Flow/CameraMove.wav": "902e15545e4afe4258009e7c6848941fc0181c9c93aa438f152672024d72a236",
    "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_01.mp3": "1df71196d411fa11b5c1a7269488059be967fe5070d1c2b6ed3dd4155a805bbc",
    "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_04.wav": "1a58ea16575dccabab8acb393383ee725386472793fbfe9befd13ab50ebcb2e1",
    "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_08.mp3": "cfdfc405152eadbc3f31f51f900b555ada70b93a02308a60295429245d534506",
    "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_09.wav": "3630dca5877c81653ef5a5049bb4f98f51ed2cb26cb1411e85908375de1d3bb5",
    "Design/Audio/Source/Formal/Variants/CombatHit/Flame.mp3": "b9c5d877622bd70cb77e55ebe3d3f8aac583f885cc7ea6ceb505d3729cecb8a9",
    "Design/Audio/Source/Formal/Variants/CombatHit/Grass.wav": "741dff2f7f42bccf5d35c73ec0f361c1480dbc2ae98ee7a34f1fe94f9514cf47",
    "Design/Audio/Source/Formal/Variants/CombatHit/Lightning.wav": "1cc5eab6416b0a7675527d0d00c93f765b6849bf219e3c768f6c11d545ad18df",
    "Design/Audio/Source/Formal/Variants/CombatHit/Water.wav": "caa2dc1adc4dca8de23955a690c4e6616c587f2b5711bf390d38a095662778be",
    "Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_1.mp3": "e44508de4be1bfc786ed597afdb00dd510d5067b979ae07d1c62dbec4e5a70e4",
    "Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_2.mp3": "65930d4240bc8ecec8a92609b81fddd26f595c64702fcf158b0e39c5b5c5d3bb",
    "Design/Audio/Source/Formal/Variants/MusicEncounter/Stage_3.mp3": "343d3efedd5c4547b71f6347028d3c8375d55e8aa920f4454e2c7e2e121da4c7",
}


def main() -> None:
    errors: list[str] = []
    for relative_path, expected in EXPECTED_SHA256.items():
        source = ROOT / relative_path
        if not source.is_file():
            errors.append(f"missing: {relative_path}")
            continue
        actual = hashlib.sha256(source.read_bytes()).hexdigest()
        if actual != expected:
            errors.append(f"hash mismatch: {relative_path}: expected {expected}, found {actual}")
    if errors:
        raise SystemExit("Plan114 formal audio source validation failed:\n" + "\n".join(errors))
    print(f"[PASS] Plan114 formal audio sources verified: {len(EXPECTED_SHA256)} files")


if __name__ == "__main__":
    main()
