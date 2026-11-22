#!/usr/bin/env python3
"""Frame edge patcher for the Caelestia window decoration.

Caelestia's screen frame has a soft inner edge. Its last pixel row is only
partly drawn, and where a window starts right after it (maximized or tiled
windows), a thin line of raw wallpaper shows through between frame and
window. This moves the frame's inner edge half a pixel over the windows, so
that row is fully drawn. Nothing else changes: the rounded corners, the
shadow and the blur stay as they are, and on the empty desktop half a pixel
is not visible.

The plugin API has no hook for the frame, so this makes one clearly marked
edit to modules/drawers/ContentWindow.qml. It is idempotent: the plugin runs
it on every shell start, so a Caelestia update that replaces the file simply
gets patched again.

Usage:
    patch.py apply  <shell dir>   -> prints OK | PATCHED | FAILED: <reason>
    patch.py revert <shell dir>   -> prints REVERTED | NOTHING
    patch.py status <shell dir>   -> prints APPLIED | MISSING
"""

import os
import re
import sys
import tempfile

VERSION = "5"
TAG = "caelestia-window-frames"
BEGIN = f"// {TAG}:begin v{VERSION}"
END = f"// {TAG}:end"
MARK = f"/*{TAG}*/"
BLOCK_RE = re.compile(
    r"^[ \t]*// " + re.escape(TAG) + r":begin[^\n]*\n.*?^[ \t]*// " + re.escape(TAG) + r":end[^\n]*\n",
    re.S | re.M,
)

# Inserted next to an anchor that must appear exactly once: (anchor, "before" | "after", body).
INSERTIONS = []

# Whole-line edits: (original line, new line, how many lines match), compared
# without indentation. A changed line keeps its original in a trailing marker
# comment, so any version of this script can put it back.
LINE_EDITS = [
    # Pull the frame's inner edge half a pixel over the windows.
    ("readonly property real sdfBorderOffset: 2 * fsTransitionProg",
     "readonly property real sdfBorderOffset: 2 * fsTransitionProg - 0.5", 1),
]
WAS_RE = re.compile(r"^([ \t]*).*/\*" + re.escape(TAG) + r" was: (.*)\*/[ \t]*$", re.M)
# Lines written by earlier versions, which did not keep the original.
LEGACY = [
    (f"readonly property real sdfBorderOffset: 2 * fsTransitionProg - 0.5 {MARK}", "readonly property real sdfBorderOffset: 2 * fsTransitionProg"),
    (f"shadowEnabled: false {MARK}", "shadowEnabled: true"),
]


def target(shell_dir: str) -> str:
    return os.path.join(shell_dir, "modules", "drawers", "ContentWindow.qml")


def strip(text: str) -> str:
    out = BLOCK_RE.sub("", text)
    out = WAS_RE.sub(lambda m: m.group(1) + m.group(2), out)
    for patched_form, original in LEGACY:
        out = out.replace(patched_form, original)
    return out


def has_any(text: str) -> bool:
    return BLOCK_RE.search(text) is not None or MARK in text or WAS_RE.search(text) is not None


def patched(text: str) -> str:
    lines = text.split("\n")
    for original, new, expected in LINE_EDITS:
        hits = [i for i, line in enumerate(lines) if line.strip() == original]
        if len(hits) != expected:
            raise ValueError(f"expected {expected} line(s), found {len(hits)}: {original!r} (Caelestia changed; plugin needs updating)")
        for i in hits:
            indent = lines[i][: len(lines[i]) - len(lines[i].lstrip())]
            lines[i] = f"{indent}{new} /*{TAG} was: {original}*/"
    out = "\n".join(lines)
    for anchor, where, body in INSERTIONS:
        count = out.count(anchor)
        if count != 1:
            raise ValueError(f"anchor found {count} times: {anchor.strip()!r} (Caelestia changed; plugin needs updating)")
        indent = anchor[: len(anchor) - len(anchor.lstrip())]
        block = indent + BEGIN + "\n" + body + indent + END + "\n"
        out = out.replace(anchor, anchor + block if where == "after" else block + anchor, 1)
    return out


def write_atomic(path: str, text: str) -> None:
    st = os.stat(path)
    fd, tmp = tempfile.mkstemp(dir=os.path.dirname(path), prefix=".ContentWindow.qml.")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(text)
        os.chmod(tmp, st.st_mode & 0o7777)
        os.replace(tmp, path)
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in ("apply", "revert", "status"):
        print(__doc__.strip())
        return 2
    action, shell_dir = sys.argv[1], sys.argv[2]
    path = target(shell_dir)
    try:
        with open(path, encoding="utf-8") as f:
            text = f.read()
    except OSError as e:
        print(f"FAILED: cannot read {path}: {e}")
        return 1

    clean = strip(text)

    if action == "status":
        current = all(f"{TAG} was: {orig}" in text for orig, _, _ in LINE_EDITS) and BLOCK_RE.search(text) is None
        print("APPLIED" if current else ("OUTDATED" if has_any(text) else "MISSING"))
        return 0

    if action == "revert":
        if not has_any(text):
            print("NOTHING")
            return 0
        write_atomic(path, clean)
        print("REVERTED")
        return 0

    # apply
    try:
        want = patched(clean)
    except ValueError as e:
        # Leave whatever is there untouched rather than half-patch the shell.
        print(f"FAILED: {e}")
        return 1
    if want == text:
        print("OK")
        return 0
    write_atomic(path, want)
    with open(path, encoding="utf-8") as f:
        if f.read() != want:
            print("FAILED: write did not stick")
            return 1
    print("PATCHED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
