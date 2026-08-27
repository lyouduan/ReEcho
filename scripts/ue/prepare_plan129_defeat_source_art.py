"""Extract editable round-defeat source layers from the supplied Figma SVG."""

from __future__ import annotations

import argparse
import csv
import hashlib
import shutil
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)
ET.register_namespace("xlink", "http://www.w3.org/1999/xlink")


@dataclass(frozen=True)
class LayerExport:
    name: str
    child_indices: tuple[int, ...]
    view_box: tuple[float, float, float, float]
    description: str


EXPORTS = (
    LayerExport(
        "DefeatSummaryPanel",
        (4, 5, 6, 7, 8, 9),
        (199.0, 341.0, 1405.0, 500.0),
        "失败结算纸张、上下横边和已选卡牌标题底条；不含文字和卡槽。",
    ),
    LayerExport(
        "DefeatCharacter",
        (36,),
        (1195.0, 192.0, 552.0, 695.0),
        "本轮失败角色立绘。",
    ),
    LayerExport(
        "DefeatRestartButton",
        (37, 38, 39),
        (502.0, 808.0, 405.0, 136.0),
        "重开整局按钮底图；按批准效果复用棕色返回主菜单底板，按钮文字由 UMG TextBlock 提供。",
    ),
    LayerExport(
        "DefeatMainMenuButton",
        (43,),
        (943.0, 808.0, 405.0, 136.0),
        "返回主菜单按钮底图；按钮文字由 UMG TextBlock 提供。",
    ),
    LayerExport(
        "DefeatCardSlot",
        (23, 24),
        (460.0, 592.0, 144.0, 170.0),
        "单个已选卡牌空槽；WBP 中复制为五个可独立移动的 Image。",
    ),
    LayerExport(
        "DefeatTimeShard",
        (17,),
        (1139.0, 400.0, 56.0, 62.0),
        "时间碎片图标。",
    ),
    LayerExport(
        "DefeatWitheredFlowerLeft",
        (47, 48),
        (200.0, 599.0, 312.0, 326.0),
        "失败结算左下枯萎花装饰。",
    ),
    LayerExport(
        "DefeatWitheredFlowerRight",
        (49, 50),
        (1108.0, 234.0, 200.0, 278.0),
        "失败结算右上枯萎花装饰。",
    ),
)


def find_edge() -> Path:
    for candidate in (
        Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
        Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
    ):
        if candidate.is_file():
            return candidate
    raise RuntimeError("Microsoft Edge was not found; cannot rasterize SVG fragments")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def build_fragment(source_root: ET.Element, export: LayerExport) -> ET.Element:
    root = ET.Element(
        f"{{{SVG_NS}}}svg",
        {
            "width": str(int(export.view_box[2])),
            "height": str(int(export.view_box[3])),
            "viewBox": " ".join(str(value) for value in export.view_box),
            "fill": "none",
        },
    )
    source_children = list(source_root)
    content = source_children[1]
    fragment_group = ET.SubElement(root, f"{{{SVG_NS}}}g")
    for index in export.child_indices:
        fragment_group.append(
            ET.fromstring(ET.tostring(list(content)[index], encoding="unicode"))
        )
    root.append(ET.fromstring(ET.tostring(source_children[2], encoding="unicode")))
    return root


def render_svg(edge: Path, svg_path: Path, png_path: Path, width: int, height: int) -> None:
    command = (
        str(edge),
        "--headless",
        "--disable-gpu",
        "--hide-scrollbars",
        "--no-first-run",
        "--default-background-color=00000000",
        f"--window-size={width},{height}",
        f"--screenshot={png_path}",
        svg_path.resolve().as_uri(),
    )
    subprocess.run(command, check=True, capture_output=True)
    with png_path.open("rb") as stream:
        header = stream.read(26)
    if len(header) != 26 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise RuntimeError(f"Edge did not create a valid PNG: {png_path}")
    actual_width, actual_height = struct.unpack(">II", header[16:24])
    if (actual_width, actual_height) != (width, height) or header[25] != 6:
        raise RuntimeError(f"Unexpected RGBA raster for {png_path.name}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--svg", type=Path, required=True)
    parser.add_argument("--css", type=Path)
    parser.add_argument(
        "--output", type=Path, default=Path("Content/SourceArt/UI/Formal/RoundDefeat")
    )
    args = parser.parse_args()
    if not args.svg.is_file():
        raise FileNotFoundError(args.svg)
    if args.css and not args.css.is_file():
        raise FileNotFoundError(args.css)

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source_root = ET.parse(args.svg).getroot()
    source_children = list(source_root)
    if len(source_children) < 3 or len(list(source_children[1])) < 51:
        raise RuntimeError("The supplied defeat SVG no longer matches the approved Figma export")

    shutil.copy2(args.svg, output / "RoundDefeat_Figma.svg")
    rows: list[dict[str, str | int]] = []
    with tempfile.TemporaryDirectory(prefix="reecho_defeat_svg_") as temporary:
        temporary_dir = Path(temporary)
        for export in EXPORTS:
            fragment_svg = temporary_dir / f"{export.name}.svg"
            fragment_png = output / f"{export.name}.png"
            ET.ElementTree(build_fragment(source_root, export)).write(
                fragment_svg, encoding="utf-8", xml_declaration=True
            )
            width, height = int(export.view_box[2]), int(export.view_box[3])
            render_svg(find_edge(), fragment_svg, fragment_png, width, height)
            rows.append(
                {
                    "asset": export.name,
                    "png": fragment_png.name,
                    "width": width,
                    "height": height,
                    "sha256": sha256(fragment_png),
                    "description": export.description,
                }
            )

    # The supplied SVG still contains an obsolete green restart fill.  The
    # approved reference uses the same brown plate for both actions, so keep
    # the two UMG images independent while deriving both from the brown plate.
    restart_png = output / "DefeatRestartButton.png"
    main_menu_png = output / "DefeatMainMenuButton.png"
    shutil.copy2(main_menu_png, restart_png)
    for row in rows:
        if row["asset"] == "DefeatRestartButton":
            row["sha256"] = sha256(restart_png)

    if args.css:
        shutil.copy2(args.css, output / "RoundDefeat_Figma.css")
    with (output / "_SourceManifest.csv").open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=("asset", "png", "width", "height", "sha256", "description")
        )
        writer.writeheader()
        writer.writerows(rows)
    print(f"Prepared {len(rows)} editable defeat layers in {output}")


if __name__ == "__main__":
    main()
