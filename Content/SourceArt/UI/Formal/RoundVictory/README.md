# Formal Round Victory source art

The authoritative visual reference is `RoundVictory_Figma.svg`, copied from the user-supplied
Figma export. `RoundVictory_Figma.css` is retained as layout evidence only; Unreal does not load
the CSS at runtime.

The PNG files in this directory are transparent, independently editable source layers extracted
from the SVG by `scripts/ue/prepare_plan129_victory_source_art.py`:

- `VictorySummaryPanel.png`: result paper and decorative frame, without baked text or card slots.
- `VictoryCharacter.png`: victory character illustration.
- `VictoryRoseRight.png`: right-side rose decoration.
- `VictoryContinueButton.png`: button art without baked label.
- `VictoryCardSlot.png`: empty selected-card sample slot.
- `VictoryTimeShard.png`: time-shard result icon.

Runtime textures are imported under `/Game/ReEcho/Textures/UI/Formal/RoundVictory`. Text, values,
hit targets, and layout remain separate UMG widgets in `WBP_ReEchoRestart` so they can be moved and
resized in the Designer without regenerating these images.
