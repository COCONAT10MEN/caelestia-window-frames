#!/usr/bin/env python3
"""Frame edge patcher for the Caelestia window decoration.

Caelestia's screen frame has a soft inner edge. Its last pixel row is only
partly drawn, and where a window starts right after it (maximized or tiled
windows), a thin line of raw wallpaper shows through between frame and
window. This moves the frame's inner edge half a pixel over the windows, so
that row is fully drawn. Nothing else changes: the rounded corners, the
shadow and the blur stay as they are, and on the empty desktop half a pixel
is not visible.

It also draws the frame's shadow on its own layer, masked off windows: the
shadow falls on the wallpaper exactly as before, but no longer darkens the
top of a window that touches the frame (which read as a dark band laid over
its titlebar).

The plugin API has no hook for the frame, so this makes a few clearly marked
edits to modules/drawers/ContentWindow.qml. It is idempotent: the plugin runs
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

VERSION = "4"
TAG = "caelestia-window-frames"
BEGIN = f"// {TAG}:begin v{VERSION}"
END = f"// {TAG}:end"
MARK = f"/*{TAG}*/"
BLOCK_RE = re.compile(
    r"^[ \t]*// " + re.escape(TAG) + r":begin[^\n]*\n.*?^[ \t]*// " + re.escape(TAG) + r":end[^\n]*\n",
    re.S | re.M,
)

# Inserted next to an anchor that must appear exactly once: (anchor, "before" | "after", body).
INSERTIONS = [
    (
        "    Item {\n        id: layoutContainer\n",
        "before",
        """    // The frame's shadow on its own layer, under the frame: the frame's
    // own shadow effect (same blur, same strength), masked by the frame and
    // by every window, so it falls on the wallpaper only. The built-in one is
    // turned off below.
    readonly property var wfShadowWindows: (Kwin.activeWsId, Kwin.windowList.filter(w => root.wfCastsOnWindow(w)))
    function wfCastsOnWindow(w: var): bool {
        if (!w || w.minimized === true || !(w.width > 0) || !(w.height > 0))
            return false;
        const name = root.screen?.name ?? "";
        if (name && w.output && w.output !== name)
            return false;
        const ws = w.workspace?.id ?? -1;
        const active = Kwin.activeWorkspaceFor(name);
        return ws <= 0 || active <= 0 || ws === active;
    }
    Item {
        id: wfShadow

        anchors.fill: parent
        opacity: layoutContainer.opacity
        visible: root.shadowOpacity > 0

        ShaderEffectSource {
            id: wfShadowShape

            anchors.fill: parent
            sourceItem: layoutContainer
            hideSource: false
            visible: false
        }
        Item {
            id: wfShadowMask

            anchors.fill: parent
            visible: false
            layer.enabled: true

            ShaderEffectSource {
                anchors.fill: parent
                sourceItem: layoutContainer
                hideSource: false
            }
            Repeater {
                model: root.wfShadowWindows

                Rectangle {
                    required property var modelData

                    x: modelData.x - (root.screen?.x ?? 0)
                    y: modelData.y - (root.screen?.y ?? 0)
                    width: modelData.width
                    height: modelData.height
                    color: "white"
                }
            }
        }
        MultiEffect {
            anchors.fill: parent
            source: wfShadowShape
            // The source already covers the whole screen; padding would also
            // stretch the mask out of line.
            autoPaddingEnabled: false
            shadowEnabled: true
            blurMax: 15
            shadowColor: Qt.alpha(Colours.palette.m3shadow, Math.max(0, root.shadowOpacity))
            maskEnabled: true
            maskInverted: true
            maskSource: wfShadowMask
            // Shown = 1 - smoothstep(0, 1, mask alpha): full on the wallpaper,
            // none under the frame or a window, smooth along their edges.
            maskThresholdMin: 0.5
            maskSpreadAtMin: 1.0
            maskThresholdMax: 1.0
            maskSpreadAtMax: 1.0
        }
    }
""",
    ),
]

# Whole-line edits: (original line, new line, how many lines match), compared
# without indentation. A changed line keeps its original in a trailing marker
# comment, so any version of this script can put it back.
LINE_EDITS = [
    # Pull the frame's inner edge half a pixel over the windows.
    ("readonly property real sdfBorderOffset: 2 * fsTransitionProg",
     "readonly property real sdfBorderOffset: 2 * fsTransitionProg - 0.5", 1),
    # The frame's built-in shadow: replaced by the masked layer above.
    ("shadowEnabled: true", "shadowEnabled: false", 1),
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
        print("APPLIED" if BEGIN in text else ("OUTDATED" if has_any(text) else "MISSING"))
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
