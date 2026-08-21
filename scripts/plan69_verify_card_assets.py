"""Plan 69: verify card icon & art resolve exactly like C++ (no placeholder left unknowingly).

Run inside the editor (Output Log, Python mode):
    py "scripts/plan69_verify_card_assets.py"
It reads cards.csv and, for every card id, checks whether the texture the C++ would load
actually exists on disk AND loads via unreal.load_asset (the same as LoadObject at runtime).
Reports per-card icon hit/miss, art-by-tier hit/miss, and any fallback-to-placeholder.
"""
import csv
import os

import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_PATH = os.path.join(ROOT, "Content", "Data", "cards.csv")
ICON_DIR = os.path.join(ROOT, "Content", "SourceArt", "UI", "Cards", "Icon")
ART_DIR = os.path.join(ROOT, "Content", "SourceArt", "UI", "Cards", "Art")
PLACEHOLDER = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_CardIcon.T_UI_Shop_CardIcon"


def disk_icon_path(card_id):
    return os.path.join(ICON_DIR, f"T_UI_CardIcon_{card_id}.png")


def disk_art_path(tier):
    return os.path.join(ART_DIR, f"T_UI_CardTier{tier}.png")


def ue_load(package):
    """Mirror C++ LoadObject<UTexture2D>(nullptr, *Path). Returns bool success."""
    if not package:
        return False
    obj = unreal.load_asset(package)
    return obj is not None


def main():
    rows = list(csv.DictReader(open(CSV_PATH, encoding="utf-8-sig")))
    print(f"[Plan69-verify] cards.csv total = {len(rows)}")

    icon_hit, icon_miss, icon_placeholder = [], [], []
    art_hit, art_miss = [], []
    seen_tiers = set()

    for r in rows:
        cid = r["Id"]
        try:
            tier = int(r["Tier"])
        except ValueError:
            tier = 0
        name = r.get("DisplayName", "")

        # ---- icon ----
        pkg = f"/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{cid}.T_UI_CardIcon_{cid}"
        on_disk = os.path.isfile(disk_icon_path(cid))
        loaded = ue_load(pkg)
        line = f"{cid} | Tier{tier} | {name}"
        if loaded:
            icon_hit.append(line)
        else:
            icon_miss.append(line)
            # would fall back to placeholder
            if not ue_load(PLACEHOLDER):
                icon_placeholder.append(line + "  !! PLACEHOLDER MISSING !!")

        # ---- art (by tier) ----
        if tier >= 1:
            seen_tiers.add(tier)
            apkg = f"/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{tier}.T_UI_CardTier{tier}"
            if ue_load(apkg):
                art_hit.append(f"Tier{tier}")
            else:
                art_miss.append(f"Tier{tier}")

    print("\n==== ICON: resolved (dedicated) ====")
    for l in icon_hit:
        print(f"  OK   {l}")
    print(f"\n==== ICON: missing -> fallback placeholder ({len(icon_miss)}) ====")
    for l in icon_miss:
        print(f"  FALL {l}")
    if icon_placeholder:
        print("  !!! PLACEHOLDER ASSET ITSELF MISSING:")
        for l in icon_placeholder:
            print(f"    {l}")

    print(f"\n==== ART by Tier ====")
    print(f"  tiers present in csv: {sorted(seen_tiers)}")
    for t in sorted(seen_tiers):
        apkg = f"/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{t}.T_UI_CardTier{t}"
        ondisk = os.path.isfile(disk_art_path(t))
        loaded = ue_load(apkg)
        print(f"  Tier{t}: disk_png={ondisk} ue_load={loaded}")

    print(f"\n==== SUMMARY ====")
    print(f"  icon dedicated: {len(icon_hit)} / {len(rows)}")
    print(f"  icon fallback : {len(icon_miss)} / {len(rows)}")
    print(f"  art tiers ok  : {sorted(t for t in seen_tiers if ue_load(f'/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{t}.T_UI_CardTier{t}'))}")
    print("[Plan69-verify] done")


main()
