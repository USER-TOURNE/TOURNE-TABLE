### 8. One wake per frame, for audio and video together *(2.0)*

1.5 woke two threads to get audio onto the screen: a capture thread on every WASAPI packet (about 100 times a second while anything played) and a pacing thread once per frame. 2.0 merges them: once per frame, one thread drains whatever audio has arrived, analyses it and hands the frame over. At 60 FPS that is roughly 100 fewer wake-ups a second.

### 9. Frames that didn't change are never presented *(2.0, Direct3D 11 renderer)*

Every present hands Windows' compositor a new buffer and wakes it to blend that rectangle again, even when the pixels are identical. The renderer now compares a summary of the frame with the last one shown and skips both the draw and the present when nothing moved by a quarter of a pixel. During sustained notes, quiet passages and every idle state, that is most frames, and a compositor with nothing to do goes back to sleep.

### 10. Three steps of idle *(2.0)*

- **Playing**: one wake per frame.
- **Silent** (after Pause When Silent): wakes only when audio arrives or four times a second, and presents nothing unless something changes.
- **Deep Idle** (five seconds later): the loopback stream itself is stopped, and Windows' own peak meter is read four times a second instead. A running capture stream registers an audio power request, which can hold the PC out of sleep (check `powercfg /requests`); a stopped one doesn't. The engine thread also asks Windows for efficiency-core scheduling (EcoQoS) while idle, and opts out of it while playing so frame pacing stays tight.

### 11. Less for the compositor to blend *(2.0, Direct3D 11 renderer)*

The animated surface covers only the panel; the text has a surface of its own that only changes when the text does. And when the panel is opaque (Blur on, or a solid colour), Windows is told so, and copies those pixels instead of blending them.

