🟢 Step 1 — Core basics (EventRouter)

✅ Files: core/EventRouter.*

Empty EventRouter builds.

Catch SDL keyboard/mouse/touch events.

Print events to console.

By end: App runs, console logs input events.

🟡 Step 2 — Simple UI (Root + GainBar + UIManager)

✅ Files: ui/UIManager.* (stub → real)

Root window shows.

GainBar visible.

UIManager holds and updates them.

By end: App runs, shows GainBar, responds to input.

🟠 Step 3 — Visual polish (Calligraphy + Primitives)

✅ Files: ui/Calligraphy.*

Calligraphy draws strokes with NanoVG.

Primitives provide rectangles/keys.

UIManager manages them.

By end: Touchscreen/mouse draws visible strokes.

🔵 Step 4 — Audio foundation (AudioEngine + Oscillator)

✅ Files: audio/AudioEngine.*, dsp/Oscillator.*

AudioEngine plays one sine.

Oscillator supports sine, square, saw.

Can start/stop audio cleanly.

By end: App produces stable sound.

🟣 Step 5 — Integration (UI ↔ Audio)

✅ Connections: UIManager + EventRouter → AudioEngine

Dropdown chooses waveform.

Key rectangle triggers notes.

GainBar controls volume.

By end: Play notes visually & audibly.

🔴 Step 6 — Advanced features (DSP + App polish)

✅ DSP in dsp/ (filters, FFT, FX).
✅ EventRouter extended (multi-touch, gestures).
✅ UI polish (Gooshe selection, menus).

By end: Full Combi Mode version, clean structure.