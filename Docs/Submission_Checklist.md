# Submission checklist: 2026 Epic AI MegaJam

Checked against https://itch.io/jam/2026-epic-ai-megajam on 2026-11-14 (evening, Israel time).
**Deadline: Nov 15, 5:59 PM PST = Nov 16, 03:59 Israel time. Late entries are disqualified.**

The 19-page Official Rules PDF linked from the jam page could not be read by the assistant (the viewer exposes no text).
Someone on the team must read it, especially the eligibility section (age 18+, active Epic Games account, country).

## Rules on the jam page

| Rule | Status | Evidence / what is left |
|---|---|---|
| Built in Unreal Engine or UEFN, not both | OK | Unreal Engine project only |
| Team of up to five people | **Confirm** | Git shows 3 committers (Yair, Saar, Taisho); count everyone who contributed, including the lead and the artists |
| Not started before Nov 12 | OK | First commit: 2026-11-12 15:09 (+02:00), an empty repo; the project itself was added at 20:19 |
| Declare anything made earlier | **Confirm** | Nothing in the repo predates the jam. Ask each artist whether any drawing or prompt existed before Nov 12 |
| Custom gameplay beyond Epic's starter templates | OK | Sword, boar AI, King Boar, Director boss, 5 story beats, quests, narrator; built on the First Person / Shooter template |
| Assets you didn't make need a licence and must be listed | OK, **list them** | No Fab or marketplace packs. Only Epic template content (mannequin, template weapon mesh and blast sound). AI-generated content is listed under Tools |
| No other people's IP, no inappropriate content | OK, see notes | See "Content notes" below |
| Submit a packaged build, not the project | **NOT DONE** | No packaged build has been made yet |
| Theme: DIRECTOR'S CUT | OK | A meddling Director narrates and re-cuts the game beat by beat |

## What to submit (Unreal Engine teams, on itch.io)

- [ ] Packaged build. Windows 10/11, development build recommended (Shipping also accepted)
- [ ] Zip named `TeamName_GameName.zip`, under 1 GB (project content is about 280 MB, so size is not a problem)
- [ ] Uploaded to the team's own itch.io account, then submitted to the jam
- [ ] Team name, each member's name and the email linked to their Epic Games account
- [ ] Game name (the UI art says "Rated PG for Pig"; the project and docs still say "Director's Cut": pick one)
- [ ] Gameplay video link, 30 to 60 seconds, gameplay in the first five seconds
- [ ] Tools used (optional, needed for partner prizes): list below
- [ ] Dev log link (optional, required only for the Glass Box prize)

## Tools used (draft for the submission form)

- **Claude Code** (Anthropic; Claude Opus 5.5 and Claude Fable 5.2): C++ gameplay, level scripts, Blender scripts, docs
- **Cursor**: used by Saar (gameplay flow, HUD, death slate, ammo counter) and Taisho (asset commits)
- **Unreal Engine MCP** (editor automation from the coding agent)
- **Meshy**: 3D models (film camera, boom mic, woodland slide, first stage floor, cobblestone tile) and text-to-image pictures (ground, planks, stall, signs, wanted poster, stone bricks; model nano-banana-2)
- **fal**: music (Stable Audio 3, Sonilo Music), sound effect takes (Sonilo SFX), Director voice lines (`fal-ai/elevenlabs/tts/eleven-v3`, voice Callum)
- **Blender 5.2**: clean-up and building of all props
- Hand-drawn art by the team: sword and shotgun frames, boar frames, Princess, bacon, house, grass, hay bale, apple tree, UI (logo, buttons, pause panel, HP bar, loading icon)

Confirm with Saar and Taisho whether any other tool was used (for example an OpenAI model, which has its own prize question).

## Content notes (other people's IP)

- **Art style:** the look was described in prompts and docs as "Forgive Me Father 2 style". A style is not a protected work, and no art, name or logo from that game is in the build. Do not name it in the store text or video.
- **Wanted poster:** a generic "kill ten boars" MMO joke (quest "!" and "REWARD: 10 XP"); no game's name, logo or art is used.
- **Cine camera:** the modern camera model from `taisho/3d-cameras` carried a real brand name and was removed; it is not on `main`. Do not merge that branch.
- **Title "Rated PG for Pig":** fine as a parody title; do not use the film-rating board's boxed rating symbol anywhere.
- **Audio:** music, voice and sound effects are AI-generated through fal (Saar imported the 16 fal sound effects on Nov 14, replacing the code-generated placeholders).

## Known risks before the deadline

1. **No packaged build exists.** The start map and packaging settings changed on Nov 14 (main menu map, `MapsToCook`, always-cook folders). Package and play it end to end.
2. **Menus and loading screen are untested with real input** and in a packaged build.
3. **Level 1 has never been played from start to finish** in one run.
4. Project name in `Config/DefaultGame.ini` is still "First Person Template" (shows as the window title).
