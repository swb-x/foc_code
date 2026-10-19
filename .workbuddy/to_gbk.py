# -*- coding: utf-8 -*-
"""把 Core/ 与 motor/ 下所有含中文的源文件统一为 GBK 编码（MDK 中文环境原生编码）。
   同时修复已知被 '?' 吞掉的字符。"""
import os

ROOTS = [r"C:\Users\a2025625\Desktop\Foc\Core", r"C:\Users\a2025625\Desktop\Foc\motor"]
EXTS = (".c", ".h")

# 已知损坏文本修复（写文件时 µ 无法进 GBK 被替换成了 ?）
FIXES = [
    ("每 50?s 自动触发", "每 50us 自动触发"),
]

# GBK 无法表示的字符 → 安全替代
CHAR_MAP = {
    "\u00b5": "\u03bc",   # µ micro sign -> Greek mu (GBK has μ, not µ)
}

def detect_and_decode(raw):
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return raw.decode("gbk")

changed = []
for root in ROOTS:
    for dirpath, _, files in os.walk(root):
        for f in files:
            if not f.lower().endswith(EXTS):
                continue
            p = os.path.join(dirpath, f)
            raw = open(p, "rb").read()
            if not any(b > 0x7F for b in raw):
                continue                      # 纯 ASCII，不动

            txt = detect_and_decode(raw)
            for a, b in FIXES:
                txt = txt.replace(a, b)
            for a, b in CHAR_MAP.items():
                txt = txt.replace(a, b)

            # 找出仍然无法编进 GBK 的字符
            bad = set()
            for ch in txt:
                try:
                    ch.encode("gbk")
                except UnicodeEncodeError:
                    bad.add(ch)

            if bad:
                print("!! %s 仍有 GBK 不支持的字符: %s" % (p, [hex(ord(c)) for c in bad]))
                continue

            new = txt.encode("gbk")
            if new != raw:
                open(p, "wb").write(new)
                changed.append(p)

print("---- converted to GBK ----")
for p in changed:
    print("  ", p)
print("total %d files" % len(changed))
