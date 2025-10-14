UI Side

KeyRect → draw key, handle touch/mouse events

Calligraphy → draw strokes (bamboo/fountain styles)

UIManager → layout (keys, menus, dropdowns), manage active Gooshe/waveform

EventRouter → dispatch input → UI + Audio

🎵 Audio Side

AudioEngine → main class, owns stream + sample loop

Oscillator → generates waveforms from tables (sine, cello, etc.)

DSP/FX → optional effects (filters, reverb, FFT with KissFFT)

🗂 Master

App → owns UIManager + AudioEngine, runs main loop

👉 Short, modular, easy to expand.



              ┌──────────────┐
              │     App      │
              │ (Main loop)  │
              └─────┬────────┘
                    │
      ┌─────────────┼─────────────┐
      │                           │
┌─────────────┐            ┌─────────────┐
│  UIManager  │            │ AudioEngine │
│ (layout, UI)│            │ (RtAudio)   │
└─────┬───────┘            └─────┬───────┘
      │                           │
  ┌───┴─────┐             ┌───────┴────────┐
  │ KeyRect │             │   Oscillator   │
  │(keys)   │             │ (waveforms)    │
  └─────────┘             └───────┬────────┘
  ┌─────────┐                     │
  │Calligrap│                     │
  │(strokes)│             ┌───────┴───────┐
  └─────────┘             │   DSP / FX    │
                          │ (filters, FFT)│
                          └───────────────┘

        ┌───────────────────────────────┐
        │         EventRouter           │
        │ maps input → UI + Audio       │
        └───────────────────────────────┘






****



              ┌──────────────┐
              │     App      │
              │ main control │
              └─────┬────────┘
                    │
      ┌─────────────┼─────────────┐
      │                           │
┌─────────────┐            ┌─────────────┐
│  UIManager  │            │ AudioEngine │
│ draw & layout│           │ audio stream │
└─────┬───────┘            └─────┬───────┘
      │                           │
  ┌───┴─────┐             ┌───────┴────────┐
  │ KeyRect │             │   Oscillator   │
  │ key UI  │             │ waveforms gen  │
  └─────────┘             └───────┬────────┘
  ┌─────────┐                     │
  │Calligrap│                     │
  │ strokes │             ┌───────┴───────┐
  └─────────┘             │   DSP / FX    │
                          │ filters, FFT  │
                          └───────────────┘

        ┌───────────────────────────────┐
        │         EventRouter           │
        │ input → UI+Audio              │
        └───────────────────────────────┘







📂 app/

main.cpp → entry point

(future: App.h / App.cpp → main control)

📂 audio/

AudioEngine.h / AudioEngine.cpp → audio stream manager

📂 dsp/

Oscillator.h / Oscillator.cpp → waveform generator

Parameters.h → shared constants

📂 ui/

Root.h / Root.cpp → UI container

GainBar.h / GainBar.cpp → gain widget

Primitives.h / Primitives.cpp → NanoVG shapes

nanovg_backend.cpp → NanoVG GL3 backend

Calligraphy.h / Calligraphy.cpp → stroke visuals

UIManager.h / UIManager.cpp → layout + menus

📂 core/

EventRouter.h / EventRouter.cpp → input dispatcher





🥇 Step 1 — Core basics

EventRouter → catch SDL events, just print them at first.

🥈 Step 2 — Simple UI

GainBar (already exists) → connect to EventRouter for testing.

UIManager → manage Root + GainBar layout.

🥉 Step 3 — Visual polish

Calligraphy → implement touch strokes with NanoVG.

Primitives (already exist) → expand as needed.

🥇🥈 Step 4 — Audio foundation

AudioEngine → keep sine wave running (already baseline).

Oscillator → add multiple waveforms.

🥇🥈🥉 Step 5 — Integration

UIManager dropdown → select Gooshe/waveform.

EventRouter → send choice → AudioEngine.

🏆 Step 6 — Advanced features

DSP/FX → filters, tremolo, reverb, FFT.

App (if added) → central loop & lifecycle.




