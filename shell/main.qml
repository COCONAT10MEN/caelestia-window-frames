import QtQuick
import Quickshell
import Quickshell.Io
import Caelestia
import qs.services

// The shell half of the Caelestia window decoration: keeps the screen frame's
// inner edge half a pixel over the windows (see patch.py), so no thin line of
// wallpaper shows between frame and window. Runs on every shell start; after
// a Caelestia update replaces the file, it patches it again and restarts the
// shell once.
Scope {
    id: root

    readonly property string shellDir: decodeURIComponent(Qt.resolvedUrl(".").toString().replace(/^file:\/\//, "")).replace(/\/$/, "")
    // The plugin root (this file lives in its shell/ folder).
    readonly property string pluginDir: shellDir.replace(/\/shell$/, "")

    // Tells the window decoration the frame's edge has moved, so it matches
    // its edge pixels to the new overlap. Present only while the patch is in
    // place and the plugin is running.
    readonly property string flagPath: (Quickshell.env("XDG_STATE_HOME") || (Quickshell.env("HOME") + "/.local/state")) + "/caelestia/window-frames-edge"

    // Restart the shell process (via its systemd service) instead of an
    // in-process reload, which can deadlock and does not restart plugins.
    function restartShell(): void {
        Quickshell.execDetached(["sh", root.shellDir + "/restart-shell.sh", Quickshell.shellPath("shell.qml")]);
    }

    function handle(output: string): void {
        const result = output.trim();
        if (result === "PATCHED") {
            console.info("[caelestia-window-frames] patched the frame edge, restarting the shell");
            root.restartShell();
        } else if (result === "OK") {
            console.info("[caelestia-window-frames] patch already in place");
            flagFile.setText("1\n");
        } else {
            console.warn("[caelestia-window-frames] " + result);
            Quickshell.execDetached(["rm", "-f", root.flagPath]);
            Toaster.toast(qsTr("Caelestia window frames"), qsTr("Could not patch the screen frame's edge after an update: %1").arg(result), "warning");
        }
    }

    Process {
        running: true
        command: ["python3", "-I", root.shellDir + "/patch.py", "apply", Quickshell.shellDir]
        stdout: StdioCollector {
            onStreamFinished: root.handle(text)
        }
    }

    FileView {
        id: flagFile

        path: root.flagPath
        printErrors: false
    }

    // Plugin icon: the Material Symbols "toolbar" glyph (fill 1, grade 25,
    // medium weight, as Settings uses), centred by geometry and coloured from
    // the live palette. Removed while the plugin is not running so Settings
    // falls back to its own font icon instead of stale colours.
    readonly property string glyphPath: "M202.23919677734375 108.46722412109375Q163.31314392089843 108.46722412109375 135.8901840209961 135.8901840209961Q108.46722412109375 163.31314392089843 108.46722412109375 202.23919677734375V544.0405731201172H851.5327758789062V202.23919677734375Q851.5327758789062 163.31314392089843 824.1098159790039 135.8901840209961Q796.6868560791015 108.46722412109375 757.7608032226562 108.46722412109375ZM108.46722412109375 637.8125457763672V757.7578726375805Q108.46722412109375 796.7092742919922 135.8901840209961 824.1210250854492Q163.31314392089843 851.5327758789062 202.23919677734375 851.5327758789062H757.7608032226562Q796.6868560791015 851.5327758789062 824.1098159790039 824.1098159790039Q851.5327758789062 796.6868560791015 851.5327758789062 757.7608032226562V637.8125457763672Z"

    function hex(c: color): string {
        const h = v => Math.round(v * 255).toString(16).padStart(2, "0");
        return "#" + h(c.r) + h(c.g) + h(c.b);
    }

    readonly property string iconSvg: {
        const fg = Colours.palette.m3onSecondaryContainer;
        return '<svg xmlns="http://www.w3.org/2000/svg" width="192" height="192" viewBox="0 0 48 48">'
            + '<path transform="translate(12.3725 35.6275) scale(0.024224 -0.024224)" fill="' + hex(fg) + '" d="' + glyphPath + '"/>'
            + '</svg>\n';
    }

    onIconSvgChanged: iconFile.setText(iconSvg)
    Component.onCompleted: iconFile.setText(iconSvg)
    // Disabled, removed or shell quitting: drop the icon and the flag.
    Component.onDestruction: Quickshell.execDetached(["rm", "-f", root.pluginDir + "/icon.svg", root.flagPath])

    FileView {
        id: iconFile

        path: root.pluginDir + "/icon.svg"
        printErrors: false
    }
}
