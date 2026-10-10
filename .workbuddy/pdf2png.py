import fitz
import os

src_dir = r"D:\WeChatData\xwechat_files\wxid_5awje0wilx0w12_68c2\temp\RWTemp\2026-10\9e20f478899dc29eb19741386f9343c8"
out_dir = r"C:\Users\a2025625\Desktop\Foc\.workbuddy\schematics"
os.makedirs(out_dir, exist_ok=True)

for name in ["Ctrl.pdf", "Drv.pdf"]:
    doc = fitz.open(os.path.join(src_dir, name))
    print(name, "pages:", doc.page_count)
    for i, page in enumerate(doc):
        # render at high zoom for readability
        zoom = 3.0
        mat = fitz.Matrix(zoom, zoom)
        pix = page.get_pixmap(matrix=mat)
        out = os.path.join(out_dir, f"{name[:-4]}_p{i+1}.png")
        pix.save(out)
        print("saved", out, pix.width, "x", pix.height)
    doc.close()
