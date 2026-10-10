import fitz
import os

src = r"D:\WeChatData\xwechat_files\wxid_5awje0wilx0w12_68c2\temp\RWTemp\2026-10\9e20f478899dc29eb19741386f9343c8\Drv.pdf"
out_dir = r"C:\Users\a2025625\Desktop\Foc\.workbuddy\schematics"

doc = fitz.open(src)
page = doc[0]
r = page.rect
clip = fitz.Rect(r.width * 0.03, r.height * 0.70, r.width * 0.24, r.height * 0.97)
pix = page.get_pixmap(matrix=fitz.Matrix(12, 12), clip=clip)
out = os.path.join(out_dir, "crop_temp_detail.png")
pix.save(out)
print("saved", out, pix.width, "x", pix.height)
doc.close()
