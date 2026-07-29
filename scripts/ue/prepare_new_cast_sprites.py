from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Content" / "SourceArt" / "Characters" / "NewCast"
OUTPUT = SOURCE / "Sprites"
SHEETS = {
    "EnemySheet.png": {
        "Enemy_Slime": (45, 55, 345, 345),
        "Enemy_ThornSlime": (345, 35, 695, 350),
        "Enemy_RabbitDoll": (750, 50, 1055, 350),
        "Enemy_RabbitBeast": (1060, 0, 1355, 375),
        "Enemy_GoatPriest": (55, 320, 370, 755),
        "Enemy_DarkPriest": (370, 330, 700, 760),
    },
    "MiniBossSheet.png": {
        "Merchant_ClockKeeper": (85, 15, 485, 750),
    },
    "PlayerSheet.png": {
        "Player_Heart": (60, 20, 1000, 1365),
        "Player_Spade": (1020, 20, 2045, 1365),
        "Player_Clover": (2040, 20, 3075, 1365),
        "Player_Diamond": (3040, 20, 4065, 1365),
    },
}

def remove_connected_white(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    eligible = bytearray(width * height)
    for y in range(height):
        for x in range(width):
            r, g, b, _ = pixels[x, y]
            if min(r, g, b) >= 232 and max(r, g, b) - min(r, g, b) <= 12:
                eligible[y * width + x] = 1
    queue = deque()
    seen = bytearray(width * height)
    for x in range(width):
        queue.append((x, 0)); queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y)); queue.append((width - 1, y))
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
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise RuntimeError("Sprite became fully transparent")
    left, top, right, bottom = bounds
    return image.crop((max(0, left-padding), max(0, top-padding), min(image.width, right+padding), min(image.height, bottom+padding)))

def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for sheet_name, sprites in SHEETS.items():
        sheet = Image.open(SOURCE / sheet_name)
        for name, box in sprites.items():
            sprite = trim(remove_connected_white(sheet.crop(box)))
            destination = OUTPUT / f"{name}.png"
            sprite.save(destination)
            print(f"{destination.relative_to(ROOT)} {sprite.size}")

if __name__ == "__main__":
    main()