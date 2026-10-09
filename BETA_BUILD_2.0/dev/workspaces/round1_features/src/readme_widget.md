## ☰ RIGHT-CLICK MENU *(2.0)*

Right-click the visualizer for quick settings, applied the moment you pick them:

- **Audio Source**: the default output (what you hear), the default input, or **any active device by name**, outputs and inputs, virtual ones included. VB-Audio Cable and Matrix, Voicemeeter buses, an Ableton return routed to its own device: if Windows lists it, it's there. Outputs are captured by loopback, inputs recorded directly.
- **Shape**, **Terminal Style**, **Color Mode**, **Analysis Engine**, **Band Layout**, **Weighting**, **Ballistics**, **Readout**, **Workload**, **Renderer**, **Target FPS**.
- Toggles for Peak Hold Caps, Beat Flash, Now Playing, the Track Progress Bar, Media Controls, Pixel-Sharp Text, **Pixel Snap** and **Subpixel Nudges**.
- **Pause Visualizer**, which blanks it and stops the audio stream until you right-click the same spot again (or the media strip) and untick it.
- **Copy Quick Settings** and **Reset Quick Settings**.

Windhawk lets a mod read its settings but not write them, so a menu choice is kept as the mod's own saved value and laid over the settings page. The page keeps showing what's underneath, which is why the menu says how many quick settings are active. **Copy Quick Settings** puts them on the clipboard as `name = value` lines, worded as the menu shows them (for example `Shape = Terminal`, `Peak Hold Caps = On`), so you can make them permanent on the settings page. **Reset Quick Settings** then hands everything back to the page.

The menu only opens where the visualizer is actually showing on the desktop, never through a window that covers it, and a right-click there doesn't open the desktop's own menu. If you'd rather keep a plain right-click for the desktop, set **Right-Click Menu** (Interaction) to **Ctrl + Right-Click**. Right-clicking the media strip opens the same menu.

---

## ◫ SHARP AT EVERY SIZE, OR BETWEEN PIXELS *(2.0)*

**Why it could look soft.** Every size was scaled by your display scaling and every position was a percentage of the screen, both kept as exact fractions. At 125% a 2 px bar is 2.5 px wide, and a Position of 50% can put the visualizer at x = 812.37. A bar that starts or ends partway through a pixel gets an antialiased edge, so some sizes came out crisp and others blurry.

**Pixel Snap** *(Position, on by default)* rounds the position and every scaled size (bar width, gap, max size, panel padding) to whole pixels, so every bar edge, the panel and the text sit exactly on the pixel grid at any size and any scaling. Bar heights stay fractional, which keeps their motion smooth. The process also declares per-monitor DPI awareness on each of its own threads now, so Windows never stretches the picture on a monitor whose scaling differs from your main one.

**Subpixel placement** is Pixel Snap **off**. The picture can then sit between pixels, so it moves by fractions of a pixel, at the cost of slightly soft edges. Physically, a crisp edge and a position between two pixels can't both happen.

- **Keyboard Move Fine Step** *(Interaction)*: 1/2, 1/4, 1/8 or 1/16 px.
- Hold **Keyboard Move Fine Key** with the move modifier, or tick **Subpixel Nudges** in the right-click menu to make every nudge fine.
- Fine steps move the visualizer. The media strip and the text overlays still move whole pixels, since a window and a text box can't sit between them.
- With Pixel Snap on, fine nudges still add up: four quarter-pixel presses move the picture one pixel.

---

## ▣ THE TERMINAL SHAPE *(2.0)*

**Shape = Terminal** draws the visualizer as text: a grid of characters in a monospace font, in three styles.

- **Columns**: every bar a column of characters (`#` by default) that turns the **Hot Color** above **Hot Threshold**, with the peak cap as a character of its own. The cava / btop look.
- **Waterfall**: a spectrogram in characters. Each line is one moment and each column one band, with the character picked from **Waterfall Ramp** by level and the newest line on top.
- **Meters**: text meters, `Bass:    [######     48%]`, for bass, mid, treble and the loudest band.

It runs on the same per-bar levels as every other shape, so the engine, band layout, weighting and ballistics all apply. With the Direct3D 11 renderer the characters come from a glyph atlas baked once, and the whole grid is one instanced draw: a few hundred cells cost the same as a few hundred bars. Pick a pixel font and **Text Rendering = Pixel** for hard-edged characters.

---

## ◰ RECIPES

**A compact media widget** (coral bars on a dark teal panel, the title over the artist on the right, controls at the top left, a progress bar underneath):

| Setting | Value |
|:--|:--|
| Shape / Bar Count / Bar Width / Bar Gap | Stereo / `64` / `2` / `2` |
| Bar Max Size / Idle Size / Corner Radius | `36` / `2` / `0` |
| Color Mode / Color | Solid / `#FFFF8F8F` |
| Background Color / Padding / Corner Radius | `#FF121C21` / `12 12 52 12` / `4` |
| Background Border Size / Color | `1` / `#FF2A3A40` |
| Now Playing | on, Display Seconds `0`, Layout **Two lines**, Placement **Inside the panel, top**, Alignment **Right** |
| Now Playing Color / Artist Color | `#FFFF8F8F` / `#FF64AA89` |
| Font / Text Rendering | a pixel font (e.g. Press Start 2P at `8`) / **Pixel** |
| Track Progress | on, Below the panel, Height `2`, Gap `6`, Color `#FFFF8F8F`, Track `#FF64AA89` |
| Media Controls | on, Icon Size `14`, Spacing `8`, Icon Color `#FF64AA89`, Anchor **Panel, top left**, Insets `10` / `10` |

The two colours are the Red and Pale Green of the palette above, and the panel is its Dark Teal.

**A terminal readout** (green and red characters on black):

| Setting | Value |
|:--|:--|
| Shape | Terminal |
| Terminal Style | Columns, Waterfall or Meters |
| Font / Size | Consolas or Cascadia Mono / `12` |
| Background Color / Corner Radius | `#FF000000` / `0` |
| Peak Hold Caps | on (Columns) |

---

