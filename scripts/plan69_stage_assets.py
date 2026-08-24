#!/usr/bin/env python3
"""Plan 69 资产搬运/重命名脚本（纯 Python，无需 UE）。

读取 c:/Users/gavynqiu/Documents/miniGame/_plan69_incoming/{art,icon}，
按 CardId 重命名为 T_UI_Card_{id}.png / T_UI_CardIcon_{id}.png，
搬运到工作树 Content/SourceArt/UI/Cards/{Art,Icon}/。
"""
import os
import csv
import shutil

INCOMING = r"c:/Users/gavynqiu/Documents/miniGame/_plan69_incoming"
WORKTREE = r"c:/Users/gavynqiu/Documents/miniGame/ReEcho-plan69-trait-card-art"

CARD_IDS = [
    "G_1_01", "G_1_02", "G_1_03", "G_1_04", "G_1_05", "G_1_06", "G_1_07", "G_1_08",
    "G_2_04", "G_2_05", "G_2_06", "G_2_07", "G_2_08", "G_2_09", "G_2_10", "G_2_12",
    "G_2_13", "G_2_14", "G_2_15", "G_2_16", "G_2_17",
    "G_3_01", "G_3_02", "G_3_03", "G_3_04", "G_3_05", "G_3_07", "G_3_09", "G_3_10",
    "G_3_11", "G_3_12", "G_3_13", "G_3_14", "G_3_16", "G_3_17", "G_3_19", "G_3_20",
    "G_3_21", "G_3_22",
]


def stage(folder: str, dest_sub: str, prefix: str):
    src = os.path.join(INCOMING, folder)
    dest = os.path.join(WORKTREE, "Content", "SourceArt", "UI", "Cards", dest_sub)
    os.makedirs(dest, exist_ok=True)

    # 方式二：manifest.csv 映射
    mapping = {}
    manifest = os.path.join(INCOMING, "manifest.csv")
    if os.path.exists(manifest):
        with open(manifest, newline="", encoding="utf-8-sig") as f:
            for row in csv.DictReader(f):
                cid = (row.get("card_id") or "").strip()
                if not cid:
                    continue
                val = (row.get(folder) or "").strip()
                if val:
                    mapping[cid] = val

    covered = set()
    for cid in CARD_IDS:
        direct = os.path.join(src, f"{cid}.png")
        if os.path.exists(direct):
            src_file = direct
        elif cid in mapping:
            cand = mapping[cid]
            src_file = cand if os.path.isabs(cand) else os.path.join(src, cand)
            if not os.path.exists(src_file):
                src_file = os.path.join(INCOMING, os.path.basename(cand))
        else:
            src_file = None

        if src_file and os.path.exists(src_file):
            dst = os.path.join(dest, f"T_UI_Card{prefix}{cid}.png")
            shutil.copy2(src_file, dst)
            covered.add(cid)

    missing = [c for c in CARD_IDS if c not in covered]
    print(f"[{folder}] staged {len(covered)}/{len(CARD_IDS)}; missing: {missing}")
    return missing


if __name__ == "__main__":
    m_art = stage("art", "Art", "_Card_")
    m_icon = stage("icon", "Icon", "_CardIcon_")
    if m_art or m_icon:
        print("WARNING: 有卡缺少 art 或 icon，导入后这些卡将使用回退/隐藏。")
    else:
        print("OK: 42 张卡的 art + icon 已全部就位。")
