# Music for Level 1 - pick one version per cue

Generated 2026-11-14 with AI tools on fal (both models are marked safe for commercial use). Every cue was made twice, once with each model. **Pick one version per cue** and tell Yair (or Claude); the picked files get imported into `/Game/DirectorsCut/Audio/Music` (that folder is already in the packaged build's always-cook list).

**Listen:** open `Art/Audio/Listen.html` in a browser (all music and sound-effect takes side by side), or play the files in `Art/Audio/Music_fal/` directly.

| Cue | Where it plays | Length | Version A: Sonilo Music | Version B: Stable Audio 3 |
|---|---|---|---|---|
| Heroic loop | Beats 1-2 (the hunt, the bacon quest) | 60 s | `M_Heroic_sonilo.m4a` | `M_Heroic_sa3.wav` |
| Frantic loop | Beat 3 (hyperspawn swarm) | 60 s | `M_Frantic_sonilo.m4a` | `M_Frantic_sa3.wav` |
| Pastel romance loop | Beat 4 (Princess / King Boar) | 60 s | `M_Romance_sonilo.m4a` | `M_Romance_sa3.wav` |
| Boss battle | Beat 5 (the Director) | 75 s | `M_Boss_sonilo.m4a` | `M_Boss_sa3.wav` |
| Victory sting | Wrap / level complete | 8 s | `M_Victory_sonilo.m4a` | `M_Victory_sa3.wav` |

All files are in `Art/Audio/Music_fal/`. Stable Audio files are 44.1 kHz stereo WAV; Sonilo files are AAC (`.m4a`), which Unreal can't import directly, so a picked Sonilo version gets converted to WAV first.

## Prompts used (same for both models)

| Cue | Prompt |
|---|---|
| Heroic | Heroic orchestral fantasy adventure theme for a comedy video game, playful and a little over-the-top, brass fanfare, bouncy strings, snare drum, medieval movie soundtrack, instrumental, seamless loop |
| Frantic | Frantic fast action chase music for a comedy video game, driving drums, staccato strings, electric guitar riffs, chaotic and silly, high energy, instrumental, seamless loop |
| Romance | Cheesy pastel fairytale romance music, soft piano, harp glissandos, sugary strings, over-the-top sweet love theme for a comedy video game, instrumental, seamless loop |
| Boss | Dramatic epic boss battle music against a movie director, choir, pounding taiko drums, brass stabs, dark orchestral, theatrical and slightly campy, instrumental, seamless loop |
| Victory | Short triumphant victory fanfare sting, orchestral brass and timpani, ends cleanly on a big final chord |

## Notes for wiring it up (level flow = lead's code)

- The loops were asked to be "seamless", but AI music rarely loops perfectly: check the loop point, and if it clicks, set a short fade or loop only a section in the Sound Wave / MetaSound.
- Lower the music while the Director (VO) talks (ducking), so lines stay readable.
- Tools to list in the submission: **fal** - Sonilo V1.1 Text to Music, Stable Audio 3 Medium, Sonilo V1.1 Text to Sound Effects.
