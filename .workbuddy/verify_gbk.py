# -*- coding: utf-8 -*-
import os

ROOTS = [r"C:\Users\a2025625\Desktop\Foc\Core", r"C:\Users\a2025625\Desktop\Foc\motor"]

ok, bad = [], []
for root in ROOTS:
    for dirpath, _, files in os.walk(root):
        for f in sorted(files):
            if not f.lower().endswith((".c", ".h")):
                continue
            p = os.path.join(dirpath, f)
            raw = open(p, "rb").read()
            if not any(b > 0x7F for b in raw):
                continue
            try:
                txt = raw.decode("gbk")
            except UnicodeDecodeError as e:
                bad.append((p, "gbk decode fail: %s" % e))
                continue
            if "\ufffd" in txt:
                bad.append((p, "contains U+FFFD"))
            if "?" in txt and "?" not in raw.decode("latin1"):
                pass
            ok.append((p, txt))

print("GBK-decodable files: %d" % len(ok))
for p, _ in bad:
    print("BAD:", p)
print("---- sample lines ----")
for p, txt in ok:
    for line in txt.splitlines():
        s = line.strip()
        if s and any("\u4e00" <= c <= "\u9fff" for c in s):
            print("%-28s | %s" % (os.path.basename(p), s[:70]))
            break
print("done")
