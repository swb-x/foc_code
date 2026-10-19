import os

ROOTS = [r"C:\Users\a2025625\Desktop\Foc\Core", r"C:\Users\a2025625\Desktop\Foc\motor"]

for root in ROOTS:
    for dirpath, _, files in os.walk(root):
        for f in sorted(files):
            if not f.lower().endswith((".c", ".h")):
                continue
            p = os.path.join(dirpath, f)
            raw = open(p, "rb").read()
            bom_utf8 = raw.startswith(b"\xef\xbb\xbf")
            try:
                raw.decode("utf-8")
                enc = "UTF-8"
            except UnicodeDecodeError:
                enc = "NOT-UTF8"
            has_gbk = False
            try:
                raw.decode("gbk")
                has_gbk = True
            except UnicodeDecodeError:
                pass
            # 是否含高位字节（即含非 ASCII 文本）
            hi = any(b > 0x7F for b in raw)
            print("%-10s bom=%-5s gbk=%-5s nonascii=%-5s %s" % (enc, bom_utf8, has_gbk, hi, p))
print("done")
