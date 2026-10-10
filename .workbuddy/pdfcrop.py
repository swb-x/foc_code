import fitz
import os

src_dir = r"D:\WeChatData\xwechat_files\wxid_5awje0wilx0w12_68c2\temp\RWTemp\2026-10\9e20f478899dc29eb19741386f9343c8"
out_dir = r"C:\Users\a2025625\Desktop\Foc\.workbuddy\schematics"

# page is 842 x 595 pt (A4 landscape); use fractional rects
crops = [
    ("Ctrl.pdf", "ctrl_mcu",      (0.00, 0.05, 0.40, 0.70)),
    ("Ctrl.pdf", "ctrl_xtal",     (0.30, 0.65, 0.62, 0.97)),
    ("Ctrl.pdf", "ctrl_reset",    (0.00, 0.05, 0.22, 0.35)),
    ("Ctrl.pdf", "ctrl_ldo",      (0.62, 0.05, 0.95, 0.40)),
    ("Ctrl.pdf", "ctrl_led",      (0.62, 0.55, 0.95, 0.80)),
    ("Drv.pdf",  "drv_predrv",    (0.52, 0.33, 0.80, 0.75)),
    ("Drv.pdf",  "drv_table",     (0.36, 0.33, 0.55, 0.62)),
    ("Drv.pdf",  "drv_inverter",  (0.03, 0.35, 0.38, 0.72)),
    ("Drv.pdf",  "drv_dcdc",      (0.05, 0.05, 0.40, 0.30)),
    ("Drv.pdf",  "drv_protect",   (0.45, 0.03, 0.78, 0.33)),
    ("Drv.pdf",  "drv_sense",     (0.03, 0.72, 0.40, 0.97)),
    ("Drv.pdf",  "drv_amp",       (0.75, 0.33, 1.00, 0.62)),
]

for fname, tag, (x0, y0, x1, y1) in crops:
    doc = fitz.open(os.path.join(src_dir, fname))
    page = doc[0]
    r = page.rect
    clip = fitz.Rect(r.width * x0, r.height * y0, r.width * x1, r.height * y1)
    mat = fitz.Matrix(8, 8)
    pix = page.get_pixmap(matrix=mat, clip=clip)
    out = os.path.join(out_dir, f"crop_{tag}.png")
    pix.save(out)
    print("saved", out, pix.width, "x", pix.height)
    doc.close()
