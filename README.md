![Tourne'Table - a ghost in your desktop's shell](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/BANNER/tourne-header.png)

# `Tourne'Table` **[Audio Visualizer]**

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/11.gif)

*Oscilloscope shape running live audio - bottom-left placement, blurred panel, single-pixel border.*

> **A real-time audio visualizer that lives on your Windows desktop.**
> Built on the foundation of Salyts' Desktop Audio Visualizer, rebuilt around performance.

Play music. Bars dance on your wallpaper. That's the whole idea.

It listens to **whatever your PC is already playing** - Spotify, YouTube, a game, a call - and draws it behind your desktop icons. No virtual audio cable, no drivers, nothing to configure. It just picks up your system audio.

It is built to be cheap to run. The render thread wakes only at the frame rate you ask for, the wallpaper blur is computed once instead of every frame, and the drawing surface is sized to the widget rather than the whole desktop, so at a 144 FPS target it costs about **a third of one CPU core** and **one degree** of CPU package temperature while it plays, and nothing at all while it doesn't.

## ABOUT THIS PROJECT

Built with an emphasis on lower resource consumption, more efficient rendering, better frame pacing, expanded visualization options, dynamic album-art integration, native Windows media info, deeper customization, and power-conscious idle behavior.

The goal is simple:

> **Make the desktop move with the music - without making the CPU move mountains to do it.**

## Runs as its own process

Tourne'Table doesn't live inside `explorer.exe`. It runs as its own dedicated process (a `windhawk.exe`/`windhawk-mod.exe` instance launched specifically for this mod), using [Windhawk's tool-mod pattern](https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process). You'll see it as its own entry in Task Manager, separate from the shell.

Practically, that means:
- A crash or hang in the visualizer can't take the shell down with it.
- Restarting `explorer.exe` (Task Manager > Restart, or a shell crash) doesn't kill the visualizer - it keeps running and reattaches behind the desktop icons the moment `explorer.exe` is back up.
- Requires a Windhawk build with tool-mod support (1.7.3+, including the 2.0 alpha line). On an older build, this mod simply won't have anywhere to run.

> **Note:** this only changes *where the mod's code runs*, not what it does - it still finds the same `WorkerW` behind your desktop icons and draws there, just by asking for it across a process boundary instead of from inside `explorer.exe`.

---

## ◈ PERFORMANCE AT A GLANCE

Measured on an **Intel Core Ultra 265KF** (8 P-cores plus 12 E-cores), running a 120-bar oscilloscope at a 144 FPS target with background blur on, and with media controls, the peak-frequency readout, peak hold and beat flash all enabled.

Every figure is the **change against an idle baseline**, captured back to back in the same session with the same music playing in both, so what you are reading is the cost of the mod rather than whatever else the machine happened to be doing.

| Metric | Disabled | Running | Cost |
|:--|--:|--:|--:|
| **Total CPU usage** | 2.20 % | 3.80 % | **+1.6 pp** *(about 0.3 of one core)* |
| **Peak single-thread** | 14.85 % | 24.90 % | **+10.1 pp** |
| **CPU package power** | 19.19 W | 26.36 W | **+7.2 W** |
| **CPU package temp** | 35.0 °C | 36.0 °C | **+1.0 °C** |

Medians across 543 samples running and 384 disabled, at 1.25 s intervals. Medians rather than averages because both captures contained brief unrelated background spikes, and a median is not moved by them. Splitting the run by GPU state brackets the cost at +1.3 to +1.7 pp and +6.6 to +7.6 W, so the figures above sit mid-range rather than at the flattering end.

When audio stops, rendering stops - not "slows down," *stops*.

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/10.gif)

*Oscilloscope shape running live audio - bottom-left placement, blurred panel, single-pixel border.*

Full methodology, raw numbers and honest caveats are further down.

---

## ◆ VISUAL STYLES

### 8 Shapes

| Shape | What it does |
|:--|:--|
| **Stereo** | Classic equalizer bars. Low notes left, high notes right. |
| **Mountain** | Peaks in the middle, tapers toward both edges. |
| **Mirror** | The opposite - grows from the outside edges inward. |
| **Wave** | Normal bars with a slow ripple rolling through them. |
| **Breathe** | A gentle swell that rises with the music instead of jumping. |
| **Dots** | Stacked dots instead of solid bars - old LED-meter look. |
| **Radial** | Bars shoot outward from a center point, like a sunburst. |
| **Oscilloscope** | A single line tracing the actual sound wave. |

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/8.gif)

*Closeup of the same setup - the waveform trace drawn as a continuous line.*

### 9 Color Modes

| Mode | What it does |
|:--|:--|
| **Solid** | One flat color. |
| **Gradient** | Fades between two colors across the bars. |
| **Reactive Gradient** | Shifts toward the second color as things get *louder*. |
| **Windows Accent** | Matches your Windows accent color, updating instantly. |
| **Album Art** | Pulls the dominant color from the playing track's cover. |
| **Dynamic Album** | Gradient between the cover art's two strongest colors. |
| **Acrylic** | Grows more opaque the louder it gets - invisible in silence. |
| **Rainbow Cycle** | Continuously cycling hue, adjustable speed. |
| **Tourne** | Built-in teal → red gradient from my personal palette. |

### Plus

- **2 orientations** - bars grow vertically or horizontally
- **3 anchors** - grow from the Bottom, the Top, or **both directions from the Middle**
- **Peak Hold Caps** - thin markers hang at each bar's recent peak and slowly fall *(classic hardware EQ)*
- **Beat Flash** - bars brighten on detected bass hits, on top of any color mode
- **Multiband Oscilloscope** - the waveform tints toward whichever part of the spectrum is loudest

---

## ♪ NOW PLAYING DISPLAY

Shows the current **artist and title** above the visualizer, pulled from the same Windows media session that powers your volume popup. Works with Spotify, browsers, and most media players.

Fades in on track change, fades out after a configurable delay. Custom color, font family and size.

---

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/7.gif)

*Zoomed view of the waveform trace.*

## ◐ THE PALETTE

This project follows my personal theming palette - reflected in the repo screenshots. It can be changed to any hex / RGB / RGBA value you want.

| Name | Hex | RGB |
|:--|:--|:--|
| Green | `#15ffa2` | `21, 255, 162` |
| Red | `#ff8f8f` | `255, 143, 143` |
| Pale Green | `#64aa89` | `100, 170, 137` |
| Alt Pale Green | `#6ba6a0` | `107, 166, 160` |
| Pale Red | `#a95d5d` | `169, 93, 93` |
| Dark Teal *(background)* | `#121c21` | `18, 28, 33` |

The built-in **Tourne** color mode is drawn from these.

---

# ✦ EVERY SETTING, EXPLAINED

**You don't need any of this to use the mod.** The defaults work. This is for when you want to tweak something and you're wondering what a word means.

## Appearance

**Shape** - Which of the 8 styles above to draw.

**Orientation** - *Horizontal* = a row of bars growing up and down. *Vertical* = a column growing left and right.
> The Oscilloscope follows Orientation as of v0.8.1 - in Vertical it's the horizontal layout rotated 90° clockwise, sweeping top to bottom. It still ignores Anchor. Its extent along the sweep axis is governed by Bar Count × Bar Width even though it has no bars, so if you're using it, expect to size it through those two settings rather than Bar Max Size.

**Bar Count** - How many bars. More = finer detail, wider visualizer. Range **1-2048** (enough to span a 4K or ultrawide screen).

**Bar Width** - How fat each bar is, in pixels.

**Bar Gap** - Space between bars, in pixels. Set to `0` and they touch.

**Bar Max Size** - How tall a bar gets at full volume. This is the overall height of the visualizer.

**Bar Idle Size** - How tall bars sit in silence. `0` makes them vanish completely; a few pixels leaves a thin resting line.

**Bar Corner Radius** - How rounded the bar corners are. One number rounds all four equally, or give four numbers separated by spaces for individual control: `top-left top-right bottom-right bottom-left`. Example: `5 5 0 0` rounds only the top.

**Color Mode** - Which of the 9 coloring styles above to use.

**Color** - The color used in Solid mode. Format is `#AARRGGBB` or `#RRGGBB` - that's **A**lpha (transparency), then **R**ed, **G**reen, **B**lue in hex. Lower the first two digits to make it see-through.
> Every color field in the mod also accepts `rgba(r, g, b, a)` (alpha 0-1) or `rgb(r, g, b)`, if hex isn't your thing.

**Gradient Color 1 / 2** - Start and end colors for the gradient modes.

**Sensitivity** - How hard the bars react. Too low and quiet music barely moves them; too high and everything slams to max. Range 0-300. Turn it down for bass-heavy tracks, up for quiet recordings.
> **Heads up:** past a certain point, raising Sensitivity mostly makes quiet passages louder rather than making loud passages hit any harder - once a band's signal is strong enough to reach max bar height, more gain has nothing left to add there. See **Sensitivity Curve** below for how that ceiling is handled.

**Sensitivity Curve** - How the top of the Sensitivity range is handled once a frequency band would otherwise overshoot max bar height.
- **Exponential (Soft)** - smoothest rolloff, but starts softening the loudest parts earliest.
- **Knee (Balanced)**, default - quiet-to-moderate signal is left completely untouched, only the loud end gets compressed. Closest to how the visualizer looked before this setting existed, but with the "everything above X does nothing" problem fixed.
- **Power (Headroom)** - cheapest curve, gives the most headroom before compressing kicks in, at the cost of being the least tunable at the very top.
> **In short:** all three still hit max bar height on loud audio, but none of them silently discard how *far* above max the signal actually is anymore - Sensitivity now visibly does something across its whole 0-300 range instead of flatlining early.

**Motion Smoothing** - Slows down how quickly bars rise and fall toward their target level. `0` is the original snappy response; raising it trades a bit of responsiveness for steadier, easier-to-read motion. Useful when the bars are rendered small on the desktop, where fast per-frame jitter is hard to track visually.

**EQ Preset** - Which frequencies get emphasized *visually*. Doesn't touch your actual audio.
`Default` no adjustment · `Bass` boosts lows · `Rock` boosts mids and highs · `Pop` heavy on highs · `Jazz` warmer, gentler highs · `Electronic` boosts bass and treble, scoops the middle.

**FFT Size** - How finely sound gets analyzed. An FFT is the math that splits audio into separate frequencies - think of it as sorting sound into buckets by pitch. More buckets = finer detail, slightly more CPU.
`1024` fastest, plenty for most · `2048` / `4096` noticeably crisper · `8192` maximum detail.

> **Note - this is an admittedly 'beta' implementation for now.** Everything up to `8192` runs near-flawlessly with almost no overhead, *with the caveat that you're using anything other than the Oscilloscope shape.*
>
> **One more note:** the higher the FFT Size, the more accurate the Sensitivity slider becomes for your specific audio setup. The correlation generally runs: **as FFT Size goes up, your Sensitivity will need to go up too.** For now that means fine-tuning Sensitivity per EQ Preset *and* per FFT Size, for your particular placement, size and personal adjustments.

**Frequency Scale** - How the frequency range spreads across the bars. This matters more than it sounds.
- **Log** - the natural-feeling default. Gives bass and treble roughly equal visual space, matching how humans hear pitch.
- **Linear** - spreads by raw Hz. Since most musical energy lives low, this crams all the action into a sliver on the left and leaves the right mostly dead. Technically accurate, visually dull.
- **Mel** - uses the *mel scale*, built from research on how people actually perceive pitch. Like Log, tuned to human hearing.

**Peak Frequency Readout** - Shows the loudest note as a live number, e.g. `1.2 kHz`.

**Peak Readout Position** - Where that number sits. Horizontal: Left / Center / Right. Vertical: Above / Top / Middle / Bottom / Below.
> **Above** and **Below** place it fully *outside* the bars so it never overlaps the visualization.

**Peak Readout Offset X / Y** - Fine placement in pixels, on top of the alignment above. Negative is left/up, positive is right/down. The alignment gets it into the right general area; this puts it exactly where you want it - including clear of the background panel, if you'd rather it didn't sit on the dark plate.
> You can also nudge it live with the keyboard instead of typing numbers - see **Interaction**.

**Peak Readout Background** + **Padding / Corner Radius / Border Size / Border Color** - Same idea as the Now Playing panel, sized to the readout. **Fully transparent by default.**
> Worth more here than anywhere else: on the *Top*, *Middle* and *Bottom* alignments the readout sits directly over the bars, where a moving, recolouring background is about the worst thing you can ask text to be legible against. A panel fixes that without having to move the readout outside the visualization.

**Multiband Oscilloscope Coloring** - Only affects the Oscilloscope shape. The line tints toward whatever part of the spectrum is loudest - warm for bass, green for mids, blue for treble. Overrides Color Mode for that shape.

**Anchor** - Which edge bars grow from. `Bottom` rise upward *(classic)* · `Top` hang downward · `Middle` grow **both directions** from a center line.

**Peak Hold Caps** - Leaves a thin marker floating at each bar's recent peak, which slowly drifts down.

**Peak Hold Cap Color** - Color of the caps, in `#AARRGGBB`. Independent of Color Mode - the caps stay this color even with Rainbow, Album Art, Accent, or Acrylic.

**Beat Flash** - Flashes bars toward **Beat Flash Color** on detected bass hits.

**Beat Flash Color** - The color bars flash toward. Accepts `#AARRGGBB`, `#RRGGBB`, `rgba(r, g, b, a)` (alpha 0-1), or `rgb(r, g, b)`. Its alpha caps how strong the flash can get - drop it for a subtle tint instead of a full color swap on every beat. Defaults to white, matching the old behavior.
> Used to just multiply the existing bar color's channels up toward 255, which is why it always looked washed-out white regardless of Color Mode - any color pushed hard enough that way clips toward white. It now blends toward this color instead, so it can flash any hue you want.

**Beat Flash Intensity** - How quickly a beat reaches full Beat Flash Color.

**Rainbow Cycle Speed** - How fast the rainbow rotates. Rainbow mode only.

**Now Playing Text** - Toggles the artist/title display.

**Now Playing Color / Font / Font Size** - Styling for that text. The font must be **installed on your system** - type the exact family name. Windows silently falls back to a default on a typo rather than erroring, so double-check spelling if nothing changes.
> The Peak Frequency Readout shares these same font settings.

**Now Playing Display Seconds** - How long the text stays up after a track change before fading.

**Now Playing Offset X / Y** - Shifts the artist/title text from where it normally sits, in pixels. Negative is left/up, positive is right/down. It still travels with the visualizer - this only changes where it sits *relative* to it.
> By default the text sits just above the bars, which on a padded background panel means part of it lands on the panel. Nudging it up a little clears it. This is also nudgeable live with the keyboard - see **Interaction**.

**Now Playing Background** + **Padding / Corner Radius / Border Size / Border Color** - A panel of its own behind the text, **fully transparent by default**. Sized to the *text*, not to the visualizer, so it hugs whatever is currently showing and re-fits itself on every track change.
> This is the other way to solve "the text is hard to read" - instead of moving it clear of the main panel, give it its own. It fades in and out with the text rather than popping, and the border draws whether or not the fill has any alpha, so an outline on its own works.

## Position

**Horizontal Position** - Left-to-right placement as a percentage. `0` hard left, `50` centered, `100` hard right. Decimals are allowed (`50.25`) for fine-tuning - a whole-percent step can be a big pixel jump on a large monitor since it's a percentage of the full work area width.

**Vertical Position** - Top-to-bottom, same idea. `0` top, `100` bottom. Decimals allowed here too.

> ⚠️ Once you've moved the visualizer with the keyboard (or by dragging), that saved position **replaces** both of these fields, and editing them will look like it does nothing. `Modifier + Home` clears it and hands control back. See **Interaction**.

**Monitor** - Which screen to draw on. `1` is your first monitor.

## Interaction

The visualizer is click-through by design - it never steals clicks meant for your desktop icons. This section is how you move it anyway, without going back to the percentage fields and guessing.

### Keyboard Move (recommended)

Hold a modifier and tap a direction key. The box steps by an exact number of pixels per press, the way a window manager nudges a tiled window - you hold the combo, tap until it's where you want it, let go.

**Enable Keyboard Move** - On by default.

**Keyboard Move Modifier** - Held while tapping a direction. Defaults to **Ctrl + Alt**.

> ⚠️ **Use a two-key combo.** This mod *swallows* the keypress - the app underneath doesn't fall back to its own behaviour, it never hears about the key at all. With a one-key modifier that means real losses:
>
> | Modifier | Direction Keys | What you give up |
> |:--|:--|:--|
> | `Ctrl` | WASD | `Ctrl+A` (select all) and **`Ctrl+S` - a save you think you made silently doesn't happen** |
> | `Ctrl` | Arrows | `Ctrl+←/→` word-by-word cursor movement |
> | `Alt` | Arrows | Browser back/forward |
> | *(any)* | *(any)* | `Modifier + 1-4` and `Modifier + Home` as well |
>
> Pick a one-key modifier and the mod will tell you on load exactly which shortcuts you've given up, rather than leaving you to discover it. Nothing is blocked - it's your call - but `Ctrl+S` is the one worth thinking about twice.

**Keyboard Move Direction Keys** - Arrow keys, WASD, or both *(default)*.

**Keyboard Move Step** - Pixels per press. `1` gives true pixel-by-pixel placement.

**Keyboard Move Fast Step / Fast Key** - Hold the Fast Key *(default Shift)* as well to jump by the larger step *(default 10px)* instead. The Fast Key can't be one of the keys already in the modifier - if it is, the mod tells you rather than silently never firing.

#### Moving everything else with the same keys

The same combo with a number switches what the direction keys are steering:

| Combo | Steers |
|:--|:--|
| `Modifier + 1` | The visualizer *(default)* |
| `Modifier + 2` | The Now Playing text |
| `Modifier + 3` | The frequency readout |
| `Modifier + 4` | The Media Controls strip |

The choice sticks until you change it, so you pick a target once and then nudge as long as you like. This is the fast way to place any of them - move it while you're looking at it, instead of guessing at numbers.

> **Modifier + Home** resets whichever target is selected. The visualizer and the Media Controls strip throw away their saved position and snap back to their Position percentages; a text overlay drops back to whatever its Offset settings say.

#### Things worth knowing before you start nudging

**The two kinds of placement persist differently, on purpose.**

| Target | What gets stored | Effect on its settings fields |
|:--|:--|:--|
| Visualizer, Media Controls strip | A percentage of the work area | **Replaces** the Position fields |
| Now Playing text, frequency readout | A pixel delta | **Adds to** the Offset fields |

The reason is what each thing is positioned *against*. The visualizer and the strip are placed against the **monitor**, so a nudge is a new absolute position. The text overlays are placed against the **visualizer** - "just above the panel" is a statement about the box, not about the screen - so a nudge is a relative tweak that has to survive you editing the offsets later.

> ⚠️ **The practical consequence:** once you've moved the visualizer or the strip by keyboard (or by dragging), **editing their Position fields will look like it does nothing.** The saved position wins. Clear it with `Modifier + Home` on that target and the fields take over again. The text Offset fields never have this problem - they always count, because a nudge is added to them rather than replacing them.

**If Hide When Covered has the strip parked, you won't see it move.** The nudge still applies - it's genuinely hidden, not just behind something - and it'll be in the new spot when it comes back. Coverage is re-checked once a second, so hiding and reappearing aren't instant either.

**Direction keys are swallowed while the modifier is held.** That's what makes them reliable, but it means the app underneath never sees them - see the warning under **Keyboard Move Modifier** above. This is why the default is a two-key combo.

> Everything is saved together in `%LOCALAPPDATA%\TourneTable\position_override.txt`. Deleting that file by hand resets all four targets at once.

### Drag-to-Move

**Off by default as of v0.6.0.** Hold the modifier + mouse button anywhere over the visualizer and drag. While dragging, bar rendering pauses and only the background/border box moves (if Background is off, there's nothing to see moving until you let go - it's still repositioning correctly underneath).

> This rides a global mouse hook and has to repaint the whole visualizer to keep up with the cursor. On a heavy shape or a high bar count that repaint is the bottleneck, and dragging feels like pulling through mud - the position is always correct, it just can't redraw fast enough to show you. The keyboard move above has no such problem, because one keypress is one discrete jump instead of a stream of positions to chase. Drag is still here if you prefer it; it's no longer the default way in.

**Drag Modifier Key / Drag Mouse Button** - Which modifier (or None) and which mouse button start a drag. Defaults to **Ctrl + Middle Click**. Double-click the same combo without dragging to clear a dragged position.

> **Persistence note:** Windhawk mods can only *read* settings, not write them back - so a moved position can't appear in the Horizontal/Vertical Position fields above. **This is the root of every "why is that field being ignored?" caveat on this page.** It's saved instead to a small file of its own (`%LOCALAPPDATA%\TourneTable\position_override.txt`), which holds all four move targets and takes priority on every future launch until you clear it. Deleting that file by hand resets all four at once.

## Media Controls

A small Previous / Play-Pause / Next strip you can click, talking to whatever app is currently playing media (Spotify, a browser tab, anything that reports itself to Windows) through the same native media session API the Now Playing text reads from. No per-app setup - if Windows' own media overlay (Win+Z, or the flyout on the volume slider) can control it, so can this.

> **Note:** unlike the visualizer itself, this strip sits *on top of* other windows instead of behind the desktop icons. The visualizer is deliberately click-through so it never steals input from your desktop; a control you're meant to click has to be the opposite of that.

**Enabled** - Turns the strip on.

**Icon Color** - Color for the built-in Previous/Play/Pause/Next glyphs, used for any button whose icon path below is left blank. Format is `#AARRGGBB`, `#RRGGBB`, `rgba(r, g, b, a)`, or `rgb(r, g, b)`.

**Previous / Play / Pause / Next Icon Path** - Point any of these at a local image file (PNG, JPG, BMP, or ICO - transparency respected) to replace that button's icon with your own. Leave a path blank to keep the built-in glyph for that button. Play and Pause are separate icons since the button swaps between them depending on whether something is currently playing.

**Icon Size** - Size of each button, in pixels (square).

**Icon Spacing** - Gap between buttons, in pixels.

**Backing Plate Color / Corner Radius** - A panel drawn behind the whole strip. **Fully transparent by default**, so icons with transparency sit directly on the wallpaper with nothing behind them.
> v0.5.2 through v0.6.0 drew this plate unconditionally, as a fix for pale icons disappearing against a pale wallpaper. That made it impossible to have transparent icons actually look transparent. It's a setting now - if you do hit the contrast problem, something like `#8C141414` gives you the old soft dark plate back.

**Backing Plate Padding** - Breathing room between the icons and the edge of the plate, in pixels. `0` for none.
> This **grows the strip** rather than shrinking the icons, so turning it up never makes the buttons smaller or harder to click. It applies whether or not the plate is visible, which means it very slightly shifts where the Horizontal / Vertical Position percentages land - the position is a percentage of the *remaining* screen space, and a wider strip leaves slightly less of it.

**Backing Plate Border Size / Color** - An outline around the plate, matching its corner radius. `0` for none.
> The border draws **whether or not the plate has any fill**, so a hairline outline with nothing behind the icons is a valid look - leave Backing Plate Color transparent and just set a border.

**Hide When Covered** + **Threshold** - The strip is topmost, so by default it stays on screen over whatever else you have open. Turn this on and it gets out of the way while a real application window sits underneath it, coming back when that window moves or closes. The threshold sets how much of the strip has to be underneath a window first, from 5% to 100% in steps of 5 *(default 50%)*.
> Same detection as the visualizer's **Pause When Covered**, aimed at the strip's own rectangle. Off by default, since a strip that vanishes unexpectedly is worse than one that's occasionally in the way.
> Coverage is re-checked **once a second**, so hiding and reappearing aren't instant. While it's parked it is genuinely hidden - keyboard nudges still apply, you just won't see them land until it comes back.

**Horizontal Position / Vertical Position** - Same 0-100 percentage-of-work-area idea as the visualizer's own Position settings, decimals included, but tracked completely separately - so the control strip can sit somewhere else on screen entirely (e.g. bottom-right, while the visualizer sits bottom-center).
> The strip is also **move target 4** for the keyboard move - hold the modifier, press `4`, then nudge it a pixel at a time with the arrows. See **Interaction**.
> ⚠️ Same caveat as the visualizer's Position fields: once nudged, the saved position **replaces** these two, and editing them will look like it does nothing until you clear it with `Modifier + 4` then `Home`.

## Settings Validation

**Warn About Invalid Settings** - On by default.

Several settings on this page are free-text boxes: colors, positions, padding, corner radii, icon paths, font name. Windhawk saves whatever you type into them, valid or not. Up to v0.5.3 the mod then quietly fell back to a default on anything it couldn't parse - so a typo looked exactly like a setting that had saved fine and simply did nothing.

With this on, a summary window appears listing every field that couldn't be read, what you typed, what format was expected, and what it's using instead. It appears when the mod loads and whenever you save settings - never when everything parses.

The same list goes to the Windhawk mod log either way, so turning this off makes it silent, not blind.

## Background

**Enabled** - Draws a panel behind the bars. Turn off for bars floating directly on the wallpaper.

**Color** - Panel color in `#AARRGGBB`. The alpha controls transparency.

**Padding** - Breathing room between the bars and the panel edge. One value for all four sides, or four space-separated values for left, right, top, bottom - so you can grow the panel more on one side without moving the bars themselves or touching any other setting.
> Negative values are allowed - they shrink that side inward instead of growing it, right up to (and past) the bars if pushed far enough. Pushed too far, that side just holds at a 1px sliver rather than doing anything more destructive.

**Corner Radius** - How rounded the panel corners are. Same one-or-four-value rules as bar radius.

**Blur** - Frosted-glass blur of your wallpaper behind the panel. `0` disables.
> This used to be the single most expensive setting in the mod. It's now computed once and cached, so it's essentially free per frame.

**Border Size / Border Color** - A thin outline around the panel. `0` for none.

## Performance

**Target FPS** - How many times per second it redraws. Higher = smoother, more CPU. Little point exceeding your monitor's refresh rate.

**Pause On Fullscreen** - Stops completely when a fullscreen app runs. Detects both true fullscreen *(games)* and borderless windows. Since the visualizer lives on the desktop it's invisible anyway - this just stops it burning power. Resumes automatically.

**Pause When Silent (seconds)** - After this long without audio, drops to a trickle instead of full speed. `0` disables.

**Auto-Hide When Idle** + **Delay** - Fades out entirely after prolonged silence. Once fully faded it **stops rendering completely** - not just invisible, genuinely doing nothing until audio returns.

**Pause When Covered** - Stops rendering *and* audio capture while hidden behind another window.
> **Off by default.** Reliably detecting "am I covered?" on Windows 11 is genuinely tricky - the shell is full of invisible windows that report themselves as visible. The check only counts real, uncloaked, titled application windows, but if the visualizer ever stops when it shouldn't, this is the switch to flip.

**Pause When Covered - Threshold** - How much of the visualizer has to be covered before that kicks in, from 5% to 100% in steps of 5. Default `100%`, meaning it only pauses when completely hidden.
> Coverage is measured as **real area across every covering window combined**, not per window - two windows each hiding half of it add up to fully covered, which is what you actually see on screen. Lower values pause sooner and save more power; the trade is that it can stop while a sliver is still peeking out.

---

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/4.gif)

*Zoomed view at a different scale.*

# ▲ WHERE THE EFFICIENCY COMES FROM

In rough order of measured impact.

### 1. Precision frame pacing - the biggest single win

The obvious way to pace a desktop widget is `DwmFlush()`, which blocks until the monitor's next refresh. That wakes the render thread **on every vertical blank, forever** - 60, 144, 240+ times a second - regardless of target FPS, whether anything needs redrawing, or whether the visualizer is even visible.

This barely registers as CPU% in Task Manager, because the thread is blocked, not spinning. But every wake-up drags a core out of deep idle. Do that continuously and the core never settles into its efficient sleep states - which reads as a small, permanent bump in package power and temperature. The classic "low usage, still runs warm" signature.

> **Note:** this becomes exponentially more noticeable on AMD architecture.
>
> **Note:** also exponentially more noticeable if you have **C-States disabled** in your BIOS or elsewhere. Shoutout to Process Lasso, Core Director, Park Control and HWiNFO64 for helping me work out why all my E-cores were sitting at 65-70 °C when they were supposed to be idle.

**Instead:** a high-resolution waitable timer firing only at the configured rate. Plain `Sleep()` isn't good enough - it's quantized to ~15.6 ms, which would turn a 60 FPS target into stuttery 30-40 FPS.

### 2. Pre-rendered background blur

A Gaussian blur is a full-image convolution - the most expensive thing Direct2D does in this scene. Recomputing it every frame is pure waste, because its input (your wallpaper) never changes.

**Instead:** it is computed exactly once into a cached bitmap, and each frame just copies that. The cache covers only the widget's bounding box, which is **tens of KB of video memory rather than several MB**. It re-bakes automatically if the widget moves or resizes.

### 3. Widget-sized render surface

The visualizer occupies a thin strip, so a desktop-spanning render surface would clear and present millions of untouched pixels every frame, and park two full-desktop buffers in VRAM.

**Instead:** the surface is sized to the widget's bounding box, and the composition layer is offset to position it. That is roughly an order of magnitude less per-frame pixel work and VRAM.

### 4. Cached geometry

Building the background panel and border means allocating a path geometry and constructing four lines and four arcs by hand. It is rebuilt only when size, padding, radii or border width actually change - in normal use, almost never - rather than every frame.

### 5. Cached monitor lookup

Working out which monitor to draw on means `EnumDisplayMonitors()`, a real round-trip through the display driver stack. It is resolved once and cached, refreshed on display change, rather than called per frame.

### 6. Reduced frame latency

DXGI queues up to three frames ahead by default. For a passive widget that's pure latency and power draw with no upside. Capped at **1**.

### 7. Genuine idle shutdown

**Auto-Hide** now stops rendering completely once faded - presents one blank frame, then exits the render path entirely. **Pause When Covered** stops rendering and capture while hidden, checked once per second rather than per frame.

### Also fixed - torn cross-thread reads

Not part of the measured list above since it's a correctness fix, not a perf win with a benchmark attached to it. The per-band and waveform data handed from the audio capture thread to the render thread used to be a plain array of individual atomics - meaning the render thread could read some elements from *this* frame's update and the rest from the *previous* one, if it happened to read mid-write. Now published and read as a single unit via a seqlock (a sequence counter the reader checks before and after copying, retrying if a write landed in between), so every frame the render thread sees is either fully old or fully new - never a mix.

---

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/2.gif)

*Oscilloscope shape running live audio - bottom-left placement, blurred panel, single-pixel border.*

# ▦ THE FULL BENCHMARK DATA

All figures measured on an **Intel Core Ultra 265KF** (8 P-cores plus 12 E-cores) running Windows 11 on a 144 Hz display.

## Method

Two HWiNFO64 captures taken back to back in a single session, 45 seconds apart, at 1.25 s sampling:

| | Samples | Span |
|:--|--:|--:|
| Mod running | 543 | 678 s |
| Mod disabled | 384 retained of 442 | 480 s |

**Music played continuously through both.** This is the part that makes the comparison mean anything. An idle baseline with the audio stopped would fold the music player's own cost into the mod's, and earlier attempts at this did exactly that: they left a 2 GB gap in resident memory between the two captures, which is a clear sign the two environments were not the same machine doing the same thing minus one mod.

Settings under test: oscilloscope, horizontal, 120 bars at 2 px wide with a 1 px gap, middle anchor, background blur 33, 1 px border, media controls on, peak-frequency readout on, peak hold on, beat flash on, Now Playing off, 144 FPS target.

Validation before any comparison was drawn:

- Both files carry a byte-identical 480-column header, so no sensor was added, removed or reordered between them.
- Resident memory came out at 9,339 MB disabled against 9,077 MB running, a 262 MB difference in the *opposite* direction to the mod. The environments match.
- The disabled capture's final 60 s shows a rising tail from an unrelated process and is excluded. Everything before it is flat to within 0.3 pp.

## Results

| Metric | Disabled | Running | Cost |
|:--|--:|--:|--:|
| Total CPU usage | 2.20 % | 3.80 % | **+1.6 pp** |
| Total CPU utility | 2.00 % | 4.20 % | +2.2 pp |
| Peak single-thread | 14.85 % | 24.90 % | +10.1 pp |
| CPU package power | 19.19 W | 26.36 W | **+7.2 W** |
| IA cores power | 12.73 W | 19.40 W | +6.7 W |
| CPU package temperature | 35.0 °C | 36.0 °C | **+1.0 °C** |
| GPU core load | 0.00 % | 4.00 % | +4.0 pp |
| GPU D3D usage | 0.60 % | 13.50 % | +12.9 pp |
| GPU power | 4.78 W | 9.14 W | +4.4 W |

On a 20-core part, +1.6 points of total CPU is roughly **0.3 of one core**.

### Why these are medians and not averages

Both captures contain brief excursions that have nothing to do with the mod. The running capture spikes to 11.1 % total CPU around the 300 s mark while GPU load simultaneously *falls*, which is the signature of background work, not of a visualizer drawing harder.

Dropping those windows because their CPU is high would be selecting on the outcome and would bias the result downward. A median is not moved by them and requires no such judgement call. As a check, a 10 % trimmed mean agrees with every median above to within 0.07 pp.

### Bracketing the estimate

The running capture contains two distinct GPU states, load around 5.1 % and around 2.2 %, only one of which the disabled capture ever shows. Something else was using the GPU for part of the run. Splitting on that boundary and comparing each state separately against the disabled capture gives:

| | Low GPU state | High GPU state |
|:--|--:|--:|
| Total CPU | +1.3 pp | +1.7 pp |
| CPU package power | +6.6 W | +7.6 W |
| CPU package temperature | +0.0 °C | +1.0 °C |

The headline figures sit mid-range rather than at the flattering end of that bracket.

## Frame pacing

`v1.1.0` fixed a bug that clamped any Target FPS above about 64 down to exactly 64. Measured against a 144 FPS target:

| | Frame rate | SD | Of target |
|:--|--:|--:|--:|
| Before | 63.97 FPS | 0.156 | 44.4 % |
| After | **141.71 FPS** | 0.104 | **98.4 %** |

`1 / 15.625 ms = 64.0`, where 15.625 ms is the default Windows system timer tick. That the measured rate lands on it to three significant figures is what identified the cause. The remaining 1.6 % after the fix is timer wake latency, about 0.11 ms of overshoot per frame.

## Time spent in `Present`

Instrumented directly with `QueryPerformanceCounter` around the call, reported every 5 seconds, with the sync interval switched live inside one session rather than compared across separate runs. 219 windows, roughly 155,000 frames:

| | Sync interval 1 | Sync interval 0 |
|:--|--:|--:|
| Time in `Present`, average | 0.358 ms | **0.236 ms** |
| Render thread blocked | 5.07 % of wall time | **3.34 %** |
| Windows blocked more than 8 % | 18 % | **4 %** |

Swap chain buffer count was tested over the same windows and made no measurable difference at all (p of 0.85 and above), so it stays at 2.

## Honest caveats

1. **Whole-machine, not per-process.** HWiNFO measures the entire system. A figure quoted as the mod's cost therefore includes work the mod *causes* elsewhere, in the desktop compositor and the graphics driver, and not only time spent inside its own threads. That makes it a larger number than a per-process profiler would report. It is still the right number to quote, because it is what the machine actually pays, but it is not a clean attribution to this process.
2. **One session, two captures.** Back to back and with the environment verified as matched, but not repeated across days or machines. Run-to-run variance beyond this session is unknown.
3. **Unrelated background activity** appears in both captures, handled as described above rather than removed by hand.
4. **No fan data.** This board exposes no CPU or chassis fan sensor, so nothing here says whether the thermal cost is audible. It is one degree, so probably not, but that is inference and not measurement.
5. **Single hardware configuration.** One CPU, one GPU, one display. Nothing here predicts behaviour on other hardware, and the frame-pacing bug in particular would have presented differently on a machine with a different timer resolution.

---

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/3.gif)

*Oscilloscope shape running live audio - bottom-left placement, blurred panel, single-pixel border.*

## ♥ CREDITS

**[USER-TOURNE](https://github.com/USER-TOURNE)** - Author and maintainer: performance work, new features, benchmarking and documentation.

**[Salyts](https://github.com/Salyts)** - Original author of Desktop Audio Visualizer. This project exists because the foundation was good enough to be worth optimizing. Author of his own mod and repo; and a base for this one.

**[GR0UD](https://github.com/GR0UD)** - Audio visualizer code the original was adapted from. Author of his own work; and the base for Salyts.

### A coincidence worth acknowledging

While I was midway through my initial testing, **NeiZ** (author/maintainer, with **SuperSmile123** contributing) released [Desktop Audio Visualizer Plus](https://github.com/ramensoftware/windhawk-mods/commit/e01d0d0dbd6204804235831fd7f68821e4614028) (`neiz-supersmile-audio-visualizer`). I had planned to publish my own commit that same night - holy coincidence.

His release spurred another round of testing on my end, and I've since gone through roughly twelve more iterations. I didn't want to ship something that essentially achieved what I was already going for, especially if they'd figuratively led me out to pasture to put a bullet in me - aha.

To be explicit about attribution: **I did not borrow from or reference NeiZ or SuperSmile123's work at any point**, other than benchmarking theirs for efficiency to decide whether continuing development was worth it. Thanks to them and their contributors regardless.

---

## ◘ LICENSE

Released under the **MIT License**.

```
MIT License

Copyright (c) 2026 USER-TOURNE

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### In the spirit of the WTFPL

> **0. You just do what the fuck you want to.**
>
> Take it. Fork it. Gut it. Rewrite the parts I got wrong. Ship it. Sell it.
> Learn something from it and never speak to me again.
>
> The only thing MIT actually asks is that the copyright line rides along -
> keep the notice, and we're square.
>
> *(I went with MIT over the real WTFPL for one boring reason: this thing
> injects a DLL into `explorer.exe`, and MIT comes with a warranty disclaimer.
> The WTFPL does not. Same energy, fewer ways for my life to get complicated.)*

### Third-party notices

This project builds on upstream work by **Salyts** and **GR0UD**. Their original
code carries its own license terms - if you redistribute this, carry their notices
along with mine.

---

## ❖ SUPPORT

If this saved you some degrees, some watts, or just made your desktop nicer to
look at, you can throw something in the hat. Entirely optional, genuinely appreciated.

**[ko-fi.com/tourne](https://ko-fi.com/tourne)**
