import os, sys

ROOTS = [r"C:\Users\a2025625\Desktop\Foc\Core", r"C:\Users\a2025625\Desktop\Foc\motor"]
EXTS = (".c", ".h")

# 典型 UTF-8 被按 GBK 解读后出现的乱码字符
MOJIBAKE_CHARS = set("ÃÂÂ©ÂµÂ®Â°â‚¬â€šâ€žÂ Â£Â¤Â¥Â¦Â§Â¨ÂªÂ«Â¬Â­Â®Â¯Â°Â±Â²Â³Â´ÂµÂ¶Â·Â¸Â¹ÂºÂ»Â¼Â½Â¾Â¿")

def classify(path):
    raw = open(path, "rb").read()
    info = {"path": path, "size": len(raw)}
    try:
        raw.decode("utf-8")
        info["utf8"] = True
    except UnicodeDecodeError as e:
        info["utf8"] = False
        info["utf8_err"] = str(e)
    try:
        txt = raw.decode("gbk")
        info["gbk"] = True
    except UnicodeDecodeError:
        info["gbk"] = False

    # 乱码探测：双字节高位字符成片出现且包含替换符
    text = None
    try:
        text = raw.decode("utf-8", errors="replace")
    except Exception:
        pass
    if text is not None:
        bad = sum(1 for ch in text if ch == "\ufffd")
        info["replacement_chars"] = bad
        # 找出可疑片段
        sus = []
        for line in text.splitlines():
            if "?" in line or "\ufffd" in line or "Ã" in line or "Â" in line:
                sus.append(line.strip())
        info["suspect_lines"] = sus[:10]
    return info

for root in ROOTS:
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.lower().endswith(EXTS):
                p = os.path.join(dirpath, f)
                info = classify(p)
                flags = []
                if not info.get("utf8"):
                    flags.append("NOT_UTF8")
                if info.get("replacement_chars", 0) > 0:
                    flags.append("REPL_CHARS=%d" % info["replacement_chars"])
                if flags:
                    print("[%s] %s" % (",".join(flags), p))
                    for l in info.get("suspect_lines", [])[:6]:
                        print("     |", l)
print("done")
