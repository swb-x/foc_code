import os

FILES = [
    r"C:\Users\a2025625\Desktop\Foc\Core\Src\app_adc.c",
    r"C:\Users\a2025625\Desktop\Foc\motor\mcpwm_foc.c",
]

for p in FILES:
    raw = open(p, "rb").read()
    txt = raw.decode("gbk", errors="replace")
    print("=== %s" % p)
    for i, line in enumerate(txt.splitlines(), 1):
        if "?" in line or "\ufffd" in line:
            print("  %4d | %s" % (i, line.strip()))
print("done")
