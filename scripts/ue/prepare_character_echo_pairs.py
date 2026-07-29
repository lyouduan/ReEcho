from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Content" / "SourceArt" / "Characters" / "NewCast" / "CharacterEchoPairs.png"
OUTPUT = SOURCE.parent / "Sprites"
NAMES = ("Cat", "Heart", "Spade", "Clover", "Diamond")

def remove_edge_background(image: Image.Image, sample_y: int, tolerance: int = 24) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    samples = (pixels[0, 0][:3], pixels[width-1, 0][:3], pixels[0, height-1][:3], pixels[width-1, height-1][:3])
    background = tuple(sum(sample[c] for sample in samples) // 4 for c in range(3))
    eligible = bytearray(width * height)
    for y in range(height):
        for x in range(width):
            rgb = pixels[x, y][:3]
            if max(abs(rgb[c] - background[c]) for c in range(3)) <= tolerance:
                eligible[y * width + x] = 1
    queue = deque()
    seen = bytearray(width * height)
    for x in range(width):
        queue.extend(((x, 0), (x, height - 1)))
    for y in range(height):
        queue.extend(((0, y), (width - 1, y)))
    while queue:
        x, y = queue.popleft()
        index = y * width + x
        if seen[index] or not eligible[index]:
            continue
        seen[index] = 1
        pixels[x, y] = (*pixels[x, y][:3], 0)
        if x: queue.append((x - 1, y))
        if x + 1 < width: queue.append((x + 1, y))
        if y: queue.append((x, y - 1))
        if y + 1 < height: queue.append((x, y + 1))
    return rgba

def trim(image: Image.Image, padding: int = 12) -> Image.Image:
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        raise RuntimeError("Sprite became fully transparent")
    left, top, right, bottom = bounds
    return image.crop((max(0, left-padding), max(0, top-padding), min(image.width, right+padding), min(image.height, bottom+padding)))

def main() -> None:
    sheet = Image.open(SOURCE)
    if sheet.size != (3840, 1952):
        raise RuntimeError(f"Unexpected source size: {sheet.size}")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    column_width = sheet.width // 5
    row_height = sheet.height // 2
    for column, name in enumerate(NAMES):
        x0, x1 = column * column_width, (column + 1) * column_width
        for prefix, y0, y1 in (("Player", 0, row_height - 6), ("Echo", row_height + 6, sheet.height)):
            crop = sheet.crop((x0, y0, x1, y1))
            sample_y = 8 if prefix == "Player" else crop.height - 9
            sprite = trim(remove_edge_background(crop, sample_y))
            destination = OUTPUT / f"{prefix}_{name}.png"
            sprite.save(destination)
            print(f"{destination.relative_to(ROOT)} {sprite.size}")

if __name__ == "__main__":
    main()
