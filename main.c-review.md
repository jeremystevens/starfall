# main.c Review Notes

Code review of `main.c` (1602 lines, SDL2 port of a Pyxel/Python space shooter).
Overall verdict: clean, disciplined C. Strong refactor discipline and unusually
thorough "why" comments. These are the softer points — polish items and things
worth double-checking, not blockers.

## Nitpicks

1. **Over-explained trivial code.** `main.c:332-333` explains what `&event`
   does (`&event passes the address of our event variable so that SDL_PollEvent()
   can write event information into it.`). That's textbook-level SDL — anyone
   reading this file already knows. The comment is noise at this point.

2. **Repeated "one snapshot" boilerplate.** Every stage function
   (`update_player_movement`, `update_player_firing`, `update_projectiles_and_collisions`,
   `update_threats`, `update_powerups`, `update_boss_and_extra_life`,
   `update_wave_and_debug`) opens with the same ~8-line comment explaining why a
   single `Uint32 now = game_ticks(runtime)` snapshot is safe. It's correct each
   time, but it's the same reasoning copied six-plus times. A single shared note
   (or one explanation at the first occurrence, with one-liners after) would
   carry the same weight with less repetition.

3. **`update_threats()` returns `WaveDifficulty` only for a debug key.**
   The return value exits `update_threats()` solely so `update_wave_and_debug()`
   can read `difficulty.boss_wave` for the `B`-skip-boss shortcut. That's a mild
   coupling smell — presentation/dev functionality reaching back up the call
   chain for a value the gameplay stage computed. Fine as-is, but worth revisiting
   if the debug shortcuts ever get properly replaced (they're already flagged
   "Remove before release").

4. **Long comment blocks on config macros.** `main.c:29-54` spends ~26 lines
   explaining `PLAYER_HIT_SHAKE_*`, `BOSS_DEFEATED_SHAKE_*`, and the death
   flash constants. The rationale is useful, but it reads like an essay for
   four `#define`s. Trim-worthy on a future pass.

## Worries worth checking

1. **`suppress_fire_until_space_released` reset path.** `main.c:454` sets this
   flag when a game starts; it's only cleared on keyboard release
   (`update_player_firing`, `main.c:555-559`). Consider: window loses focus
   while SPACE is held, or focus is restored — does the key state/event behavior
   leave the flag stuck, silently swallowing the first volley of the run?
   Likely benign, but the flag has exactly one reset site tied to a physical
   keypress, which is the fragile part.

2. **Repeated `respond_to_player_hit` timing.** The four player-damage sites in
   `update_projectiles_and_collisions()` all fire hit response using the same
   snapshot `now`. If two collisions damage the player in the same frame, the
   shake/flash just restarts (fine), but the intent — "one hit this frame"
   worth of response — is what each call site believes it's producing, while the
   code allows more. Worth confirming the audio side (player explosion SFX on
   every `respond_to_player_hit` call) isn't double-triggering in practice.

3. **Death-frame ordering is preserved, but fragile.** By design, a hit that
   takes lives to 0 is only *detected* next frame (see `update_player_movement`
   comment), and the same frame death is detected, the rest of the GAME_PLAYING
   pipeline still runs (firing, collisions, spawns). That matches the original
   inline code, but it means a player can squeeze out kills/spawns they
   "shouldn't" get on the death frame. Behavior is intentional — just be aware
   it's load-bearing.

4. **High-score compare excludes ties deliberately** (`main.c:1452`:
   strictly `>`, matching isn't beating). This is a design decision, but a
   player who *ties* the record gets no acknowledgement at all on GAME_OVER
   (no "matched your high score" state). Minor UX gap if the record is tight.

5. **`game_ticks()` freeze covers render-side effects but not the title screen's
   own timers.** Screen shake/flash are correctly frozen during pause; worth
   double-checking that the title screen's looping-animation timers (if any use
   `SDL_GetTicks()` directly rather than `game_ticks()`) can't run from a stale
   source during a pause-initiated state change. Everything gameplay-adjacent
   looked consistently on `game_ticks()`.

---

Verdict: strong. All listed items are polish or verification candidates, not
bugs. The heavy refactor (Phase 3 / 6-style extraction) has produced the clearest
version of this file to date — `main()` itself now reads like a spec of the
frame pipeline.