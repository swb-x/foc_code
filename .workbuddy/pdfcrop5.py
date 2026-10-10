import fitz
import os

src = r"D:\WeChatData\xwechat_files\wxid_5awje0wilx0w12_68c2\temp\RWTemp\2026-10\9e20f478899dc29eb19741386f9343c8\Drv.pdf"
out_dir = r"C:\Users\a2025625\Desktop\Foc\.workbuddy\schematics"

doc = fitz.open(src)
page = doc[0]
r = page.rect
clip = fitz.Rect(r.width * 0.55, r.height * 0.55, r.width * 0.72, r.height * 0.68)
pix = page.get_pixmap(matrix=fitz.Matrix(14, 14), clip=clip)
out = os.path.join(out_dir, "crop_mode_trace.png")
pix.save(out)
print("saved", out, pix.width, "x", pix.height)
doc.close()
