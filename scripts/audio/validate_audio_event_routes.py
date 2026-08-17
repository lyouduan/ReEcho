#!/usr/bin/env python3
"""Verify that every catalog event has a stable constant and a production route."""

from __future__ import annotations

import csv
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
EVENT_DEFINITIONS = ROOT / "Source" / "ReEchoAudio" / "Private" / "ReEchoAudioEvents.cpp"
CATALOG = ROOT / "Content" / "Data" / "audio_events.csv"
PRODUCTION_ROOT = ROOT / "Source" / "ReEcho"
DEFINITION_PATTERN = re.compile(
    r"FReEchoAudioEvents::(?P<member>\w+)\s*=\s*FName\(TEXT\(\"(?P<event>[^\"]+)\"\)\)"
)


def main() -> None:
    definitions = dict(DEFINITION_PATTERN.findall(EVENT_DEFINITIONS.read_text(encoding="utf-8")))
    with CATALOG.open("r", encoding="utf-8-sig", newline="") as handle:
        catalog_ids = {row["EventId"] for row in csv.DictReader(handle)}

    production = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in PRODUCTION_ROOT.rglob("*")
        if path.suffix in {".cpp", ".h"} and "Tests" not in path.parts
    )
    routed_members = {
        member
        for member in definitions
        if f"FReEchoAudioEvents::{member}" in production
    }
    routed_ids = {definitions[member] for member in routed_members}

    errors: list[str] = []
    missing_constants = sorted(catalog_ids - set(definitions.values()))
    stale_constants = sorted(set(definitions.values()) - catalog_ids)
    missing_routes = sorted(catalog_ids - routed_ids)
    if missing_constants:
        errors.append(f"catalog ids without constants: {missing_constants}")
    if stale_constants:
        errors.append(f"constants absent from catalog: {stale_constants}")
    if missing_routes:
        errors.append(f"catalog ids without production routes: {missing_routes}")
    if errors:
        raise SystemExit("Plan46 audio route validation failed:\n" + "\n".join(errors))

    print(
        f"Plan46 audio routes validated: {len(catalog_ids)} catalog ids, "
        f"{len(definitions)} constants, {len(routed_ids)} production routes"
    )


if __name__ == "__main__":
    main()
