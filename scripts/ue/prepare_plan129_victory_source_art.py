"""Extract editable round-victory source layers from the supplied Figma SVG.

The SVG embeds several large source images and vector masks.  This tool keeps
the original 1920x1080 coordinate system, isolates the visual layers that UMG
needs, and lets Chromium rasterize each fragment with alpha.  Runtime text and
interaction remain UMG widgets rather than being baked into these images.
"""

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
		"VictorySummaryPanel",
		(4, 5, 6, 7, 8, 9, 42),
		(199.0, 341.0, 1405.0, 500.0),
		"结算纸张、上下横边、已选卡牌标题底条与左侧玫瑰；不含文字和卡槽。",
	),
	LayerExport(
		"VictoryCharacter",
		(36,),
		(1195.0, 192.0, 552.0, 695.0),
		"本轮胜利红帽角色立绘。",
	),
	LayerExport(
		"VictoryRoseRight",
		(41,),
		(1134.0, 276.0, 216.0, 160.0),
		"结算板与角色交界处的右侧玫瑰。",
	),
	LayerExport(
		"VictoryContinueButton",
		(37,),
		(784.0, 816.0, 405.0, 136.0),
		"继续按钮底图；按钮文字由 UMG TextBlock 提供。",
	),
	LayerExport(
		"VictoryCardSlot",
		(23, 24),
		(460.0, 592.0, 144.0, 170.0),
		"单个已选卡牌空槽；WBP 中复制为五个可独立移动的 Image。",
	),
	LayerExport(
		"VictoryTimeShard",
		(17,),
		(1139.0, 400.0, 56.0, 62.0),
		"时间碎片图标。",
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
		fragment_group.append(ET.fromstring(ET.tostring(list(content)[index], encoding="unicode")))
	# Definitions contain clip paths, image payloads, patterns, masks and fills.
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
	if not png_path.is_file():
		raise RuntimeError(f"Edge did not create {png_path}")
	with png_path.open("rb") as stream:
		header = stream.read(26)
	if len(header) != 26 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
		raise RuntimeError(f"Edge did not create a valid PNG: {png_path}")
	actual_width, actual_height = struct.unpack(">II", header[16:24])
	if (actual_width, actual_height) != (width, height):
		raise RuntimeError(
			f"Unexpected raster size for {png_path.name}: {(actual_width, actual_height)}"
		)
	if header[25] != 6:
		raise RuntimeError(
			f"{png_path.name} is PNG color type {header[25]}, expected RGBA color type 6"
		)


def main() -> None:
	parser = argparse.ArgumentParser()
	parser.add_argument("--svg", type=Path, required=True)
	parser.add_argument("--css", type=Path)
	parser.add_argument(
		"--output",
		type=Path,
		default=Path("Content/SourceArt/UI/Formal/RoundVictory"),
	)
	args = parser.parse_args()
	if not args.svg.is_file():
		raise FileNotFoundError(args.svg)
	if args.css and not args.css.is_file():
		raise FileNotFoundError(args.css)

	output = args.output.resolve()
	output.mkdir(parents=True, exist_ok=True)
	edge = find_edge()
	source_tree = ET.parse(args.svg)
	source_root = source_tree.getroot()
	content = list(source_root)
	if len(content) < 3 or len(list(content[1])) < 43:
		raise RuntimeError("The supplied victory SVG no longer matches the approved Figma export")

	original_svg = output / "RoundVictory_Figma.svg"
	shutil.copy2(args.svg, original_svg)
	manifest_rows: list[dict[str, str | int]] = []
	with tempfile.TemporaryDirectory(prefix="reecho_victory_svg_") as temporary:
		temporary_dir = Path(temporary)
		for export in EXPORTS:
			fragment_svg = temporary_dir / f"{export.name}.svg"
			fragment_png = output / f"{export.name}.png"
			ET.ElementTree(build_fragment(source_root, export)).write(
				fragment_svg, encoding="utf-8", xml_declaration=True
			)
			width = int(export.view_box[2])
			height = int(export.view_box[3])
			render_svg(edge, fragment_svg, fragment_png, width, height)
			manifest_rows.append(
				{
					"asset": export.name,
					"png": fragment_png.name,
					"width": width,
					"height": height,
					"sha256": sha256(fragment_png),
					"description": export.description,
				}
			)

	if args.css:
		shutil.copy2(args.css, output / "RoundVictory_Figma.css")
	with (output / "_SourceManifest.csv").open("w", encoding="utf-8-sig", newline="") as stream:
		writer = csv.DictWriter(
			stream, fieldnames=("asset", "png", "width", "height", "sha256", "description")
		)
		writer.writeheader()
		writer.writerows(manifest_rows)
	print(f"Prepared {len(manifest_rows)} editable victory layers in {output}")


if __name__ == "__main__":
	main()
