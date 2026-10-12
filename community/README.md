# Community setups

Setups people share for Tourne'Table. Open the Style Editor (right-click the visualizer) and press **Community** to try one: it plays at once, and you choose Keep or No.

## Sharing yours

In the Style Editor, tick the parts you want to share (Look, Motion, Extras, Place), type a name in Saved style, press **Community**, then **Share Mine to the Community**. That opens a GitHub issue with your setup already in it; add a screenshot and send it. Your audio device, hotkeys, file paths and hardware choices never go in a setup.

You can also trade setups without GitHub: **Copy code** puts a one-line `TT2:` code on the clipboard for any chat, and **Save file** writes a `.ttstyle` file.

## Adding one (for the maintainer)

1. Copy the setup text from the issue into `setups/<short-name>.ttstyle` (letters, digits, `-`, `_` and `.` only).
2. Add a line to `setups/index.txt`: `file|name|author|about`.
3. Commit to `main`. The list is read from `main`, so it shows up for everyone right away.

A setup is plain text: its first line is `Tourne'Table setup 1`, then `name=`, `author=`, `parts=`, then `setting=value` lines (only the ones that differ from the defaults) and `quick.<key>=value` for quick settings.
