"""ReEcho 武器符文图标映射权威管线（Plan67-shop-drop 工作树内版本）

来源纪律（2026-08-23 用户明确）：
- miniGame/ 根下的 命名图标/、rename_map.csv、_icon_src/ 等均未获用户强调，不可依赖。
- 权威来源只有两处（用户强调过的工作树内）：
    1) 策划源表(在线开普勒)的 武器符文C sheet —— 其 icon 列图片与附件下载一一对应；
    2) 本地工作簿 策划数据源/【开普勒】回响数值与构筑体系.xlsx（内嵌了与附件下载同源的图片）；
    3) 附件下载_【开普勒】回响数值与构筑体系/ —— 图片原始文件名是无意义哈希（如 0a22f4a7….png）。

机制（用户给定的权威方法，非网页抓取，腾讯文档需登录无法 scrape）：
  步骤A 锚点定位名称：解析 xlsx 内 xl/drawings/，每张内嵌图的锚点记录「工作表 + 行号」。
        读取该行「宝石/配件名称」列（武器符文C sheet，col C）即得图标对应的符文/配件名。
        即：媒体路径 -> 行 -> 名称。
  步骤B 字节匹配哈希文件：把 93 张下载 PNG 与 xlsx 内嵌图按 SHA-256 逐字节比对，确认同源，
        从而把「下载哈希名」对应到「xlsx 媒体路径」。
  两步合起来：下载哈希名 ->(SHA256)-> xlsx 媒体路径 ->(锚点行)-> 符文名称。
  rename_map.csv 完整记录 旧哈希名 -> 新名 -> 显示名 -> xlsx_media 的可追溯映射。

本脚本实现：解析 策划数据源.xlsx 武器符文C 的 drawing5.xml 锚点 -> 读行名 ->
            SHA-256 匹配 当前 附件下载 文件夹的 PNG -> 输出 DisplayName,AttachmentFile。

输出：_icon_map_raw.csv（DisplayName, AttachmentFile），供复制高清原图到 _icon_src/Icons 使用。

铁律提醒：读取 xlsx 时注意 hidden 行列——openpyxl 默认返回所有行（含隐藏），
        武器体系W 的匕首是隐藏行，若按隐藏过滤会误判"匕首可见/不可见"。
        本脚本只做图标锚点匹配，不改 xlsx；是否过滤 hidden 由调用方按业务定。

用法：python scripts/data/extract_weapon_rune_icon_map.py
"""
import zipfile, os, hashlib, csv, warnings
import xml.etree.ElementTree as ET
import openpyxl
warnings.filterwarnings('ignore')

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PLAN_XLSX = os.path.join(ROOT, '策划数据源', '【开普勒】回响数值与构筑体系.xlsx')
ATTACH = os.path.join(ROOT, '策划数据源', '附件下载_【开普勒】回响数值与构筑体系')
SHEET_NAME = '武器符文C'
NAME_COL = 3
OUT = os.path.join(ROOT, '_icon_map_raw.csv')

with zipfile.ZipFile(PLAN_XLSX) as z:
    wb_root = ET.fromstring(z.read('xl/workbook.xml'))
    ns_main = 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'
    ns_rel = 'http://schemas.openxmlformats.org/officeDocument/2006/relationships'
    sheets = {s.get('name'): s.get(f'{{{ns_rel}}}id') for s in wb_root.find(f'{{{ns_main}}}sheets').findall(f'{{{ns_main}}}sheet')}
    wb_rels = ET.fromstring(z.read('xl/_rels/workbook.xml.rels'))
    rid_target = {r.get('Id'): r.get('Target') for r in wb_rels}
    sheet_to_drawing = {}
    for sname, rid in sheets.items():
        sh_file = rid_target[rid]
        base = os.path.dirname(sh_file); fname = os.path.basename(sh_file)
        try:
            rels_root = ET.fromstring(z.read(f'xl/{base}/_rels/{fname}.rels'))
        except KeyError:
            continue
        for r in rels_root:
            if 'drawing' in r.get('Type', ''):
                sheet_to_drawing[sname] = r.get('Target'); break

    draw_rel = sheet_to_drawing.get(SHEET_NAME)
    draw_path = 'xl/' + draw_rel.replace('../', '')
    ddir = os.path.dirname(draw_path); dname = os.path.basename(draw_path)
    draw_root = ET.fromstring(z.read(draw_path))
    rels_root = ET.fromstring(z.read(f'{ddir}/_rels/{dname}.rels'))
    rid_media = {r.get('Id'): r.get('Target') for r in rels_root}

    ns_d = 'http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing'
    ns_a = 'http://schemas.openxmlformats.org/drawingml/2006/main'
    ns_r = 'http://schemas.openxmlformats.org/officeDocument/2006/relationships'
    wb = openpyxl.load_workbook(PLAN_XLSX)
    ws = wb[SHEET_NAME]

    media_to_name = {}
    for anc_type in ['oneCellAnchor', 'twoCellAnchor', 'absoluteAnchor']:
        for anc in draw_root.findall(f'{{{ns_d}}}{anc_type}'):
            frm = anc.find(f'{{{ns_d}}}from')
            if frm is None: continue
            row_el = frm.find(f'{{{ns_d}}}row'); col_el = frm.find(f'{{{ns_d}}}col')
            if row_el is None or col_el is None: continue
            row = int(row_el.text) + 1; col = int(col_el.text) + 1
            pic = anc.find(f'{{{ns_d}}}pic')
            if pic is None: continue
            blip = pic.find(f'.//{{{ns_a}}}blip')
            if blip is None: continue
            rid = blip.get(f'{{{ns_r}}}embed')
            media = rid_media.get(rid, '')
            media_file = os.path.basename(media)
            name = ws.cell(row=row, column=NAME_COL).value
            if name and str(name).strip():
                media_to_name[media_file] = (str(name).strip(), row)

    embedded_sha = {}
    for m in media_to_name:
        data = None
        for p in [f'xl/drawings/media/{m}', f'xl/media/{m}']:
            try: data = z.read(p); break
            except KeyError: continue
        if data: embedded_sha[m] = hashlib.sha256(data).hexdigest()

attach_sha = {}
for f in os.listdir(ATTACH):
    if f.endswith('.png'):
        attach_sha[hashlib.sha256(open(os.path.join(ATTACH, f), 'rb').read()).hexdigest()] = f

result = {}
for m, (name, row) in media_to_name.items():
    if m in embedded_sha and embedded_sha[m] in attach_sha:
        result[name] = attach_sha[embedded_sha[m]]

with open(OUT, 'w', encoding='utf-8', newline='') as f:
    w = csv.writer(f); w.writerow(['DisplayName', 'AttachmentFile'])
    for name, ah in result.items(): w.writerow([name, ah])
print(f'Matched {len(result)} icons. Wrote {OUT}')
