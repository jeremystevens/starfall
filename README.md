<div align="center">

# STARFALL

### A retro arcade shooter written in C with SDL2

A small retro space shooter I've been building in **C** while learning
SDL2 and getting more comfortable with lower-level game programming.

![C](https://img.shields.io/badge/C-Programming-00599C?style=for-the-badge&logo=c&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL2-2.30+-111111?style=for-the-badge)
![Linux](https://img.shields.io/badge/Linux-Arch-1793D1?style=for-the-badge&logo=archlinux&logoColor=white)
![Code::Blocks](https://img.shields.io/badge/IDE-Code%3A%3ABlocks-41AD48?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-In_Development-orange?style=for-the-badge)

**No sprites, no game engine — just C, SDL2, procedural audio, and a
suspiciously egg-shaped spaceship.**

</div>

------------------------------------------------------------------------

## About the Project

**STARFALL** is a C/SDL2 port of my original **Pyxel Space Shooter**, a
retro-inspired arcade game built around the look and feel of classic
Atari-era games.

I didn't want to translate the Python version line by line. Instead,
I've been rebuilding the game one system at a time and using each part
as an excuse to learn more C. Along the way I've worked with pointers,
structs, separate header/source files, SDL2 rendering and input, frame
timing, object pools, collision handling, procedural audio, compilation,
and linking.

------------------------------------------------------------------------

## Current Features

| System | Status |
| --- | :---: |
| SDL2 window, renderer & game loop | ✅ |
| Player movement & screen boundaries | ✅ |
| Retro player ship | ✅ |
| Parallax starfield | ✅ |
| Bullet object pool | ✅ |
| Hold-to-fire & cooldown | ✅ |
| Procedural retro laser audio | ✅ |
| Enemy pool & timed spawning | ✅ |
| Scout enemies | ✅ |
| Bomber enemies | ✅ |
| Bullet/enemy collision | ✅ |
| Enemy health & score tracking | ✅ |
| Modular C architecture | ✅ |
| Player damage / lives | ✅ |
| Enemy projectile system | ✅ |
| Scout firing behavior | ✅ |
| Bomber bomb-drop behavior | ✅ |
| Procedural enemy weapon audio | ✅ |
| Enemy bullet/player collision | ✅ |
| Additional enemy types | ✅ |
| Asteroids / environmental hazards | ✅ |
| Asteroid splitting (large → small fragments) | ✅ |
| Bullet/asteroid & player/asteroid collision | ✅ |
| Retro HUD & visible scoring | ✅ |
| Title screen with animated starfield background | ✅ |
| Arcade lifecycle: Title → Play → Game Over → Title | ✅ |
| Persistent high score (save/load to disk) | ✅ |
| New high score detection, save & fanfare | ✅ |
| Custom 5×7 bitmap font | ✅ |
| Player, Scout & Bomber destruction effects | ✅ |
| Procedural destruction audio | ✅ |
| Power-up pickups (Rapid Fire, Shield, Spread Shot) | ✅ |
| Power-up HUD countdown bars & shield indicator | ✅ |
| Random power-up drops from destroyed enemies | ✅ |
| Time-based wave/difficulty system (Wave Director) | ✅ |
| Hand-tuned introductory waves (1-5) with named announcements | ✅ |
| Mathematical difficulty scaling (Wave 6+) with safe minimums | ✅ |
| Randomized spawn timing variance | ✅ |
| Minimum spawn-position spacing (enemies & asteroids) | ✅ |
| Boss encounters (three-phase Dreadnought) | ✅ |
| Extra lives at score thresholds | ✅ |
| Screen shake (player hits, boss defeat) | ✅ |
| Hit-flash feedback (Bomber, large asteroids, boss) | ✅ |
| Floating score popups | ✅ |
| Dreadnought warning sequence (visual + klaxon) | ✅ |
| Wave announcement slide transition | ✅ |
| Power-up collection burst effect | ✅ |
| Extra life on-screen notification | ✅ |
| Dreadnought progressive damage effects | ✅ |
| Dramatic final-death pause before Game Over | ✅ |
| Procedural title/boss/Game Over soundtrack | ✅ |
| Adaptive boss-theme tempo (three intensity tiers) | ✅ |
| Pause (P), freezing every gameplay timer | ✅ |

> **Active development:** the game is a working prototype while systems
> from the original are ported and expanded.

------------------------------------------------------------------------

## The White Egg Ship

What began as a simple sprite-free player design somehow evolved into
the **White Egg Ship**: a compact retro spacecraft with a light-blue
cockpit, orange exhaust, and yellow projectiles.

Its primary weapon appears to fire high-velocity yolks.

**This was not in the original design document.**

------------------------------------------------------------------------

## Procedural 1983-Style Audio

The laser effect is generated **entirely in C** rather than loaded from
an audio file. SDL2's audio callback receives generated waveform samples
while the frequency slides downward to create a crunchy arcade-style
**PEW**.

The system uses `SDL_OpenAudioDevice()`, `SDL_AudioSpec`, audio
callbacks, generated sample buffers, and waveform synthesis.

Player and Scout weapon sounds are generated independently and mixed in
the audio callback, allowing both effects to play at the same time. The
Egg Ship retains its higher-pitched descending **PEW**, while Scout
projectiles use a shorter, lower-frequency, bassier retro effect. The
Bomber's payload release adds a third, even lower **thunk** — pitched
below both lasers so a bomb drop never gets mistaken for a shot being
fired.

The same callback now also mixes in the two destruction sounds described
below (see Destruction Effects), the power-up pickup chime — the only
sound that slides *up* in pitch instead of down, so a pickup never reads
as a threat — and two multi-note fanfares built from the same
technique: the extra life jingle (see Extra Lives) and a longer, more
celebratory new high score fanfare (see Persistent High Score). Weapon
fire, engine sounds, explosions, pickups, and both fanfares can all
overlap without stepping on each other.

**A genuinely new pattern - the Dreadnought warning klaxon:** every
sound above is a fixed-duration one-shot that counts samples down to
silence. The boss's warning klaxon (see Boss Encounters) is the first
sound in the project that isn't - `audio_set_boss_warning()` just flips
a switch, and the callback free-runs a continuous LOW → HIGH → LOW sweep
(110Hz-240Hz, deeper than every other sound in the game, including the
bomb-drop thunk) for as long as that switch stays on, snapping back to
LOW every 500ms for a repeating "whoop...whoop...whoop" cadence. Being
the first sound that doesn't stop on its own meant it also needed to be
explicitly turned off on every path out of the warning - not just the
happy path - so it could never loop forever into a run where it no
longer belonged.

No laser WAV required.

------------------------------------------------------------------------

## Procedural Soundtrack

Beyond one-shot sound effects, STARFALL now plays three full musical
compositions — a title theme, a boss theme, and a Game Over theme — none
of them loaded from an audio file. Every note is sequenced sample by
sample by a small generic engine (`music.h`/`music.c`) and mixed into
the exact same SDL audio callback as every laser and explosion.

**A generic sequencer, not three hardcoded songs:** `music.c` only knows
how to step a fixed number of independent voices through whatever note
data (frequency + duration, in samples) it's handed, looping if asked,
scaled by a shared playback rate — it has no idea what a "title screen"
or "boss fight" is. The actual compositions and the decision of *what*
plays *when* live in a separate module, `soundtrack.h`/`soundtrack.c`,
which exposes a small intent-level API — `soundtrack_play_title()`,
`soundtrack_play_boss()`, `soundtrack_set_boss_intensity()`,
`soundtrack_play_game_over()`, `soundtrack_stop()` — so `main.c` never
touches a raw note array or a voice index; it just says *which* theme
should be playing right now.

**The three themes:**

| Theme | Source | Plays during |
| --- | --- | --- |
| Title | Johann Strauss II, *The Blue Danube* (Mutopia Project arrangement, CC BY-SA 4.0) | `GAME_TITLE`, looping |
| Boss | Edvard Grieg, *In the Hall of the Mountain King* (Mutopia Project, public domain) | The Dreadnought fight, looping |
| Game Over | Frédéric Chopin, *Marche funèbre* | Once, after death |

Each theme is two independently-verified voices (a melody plus an
accompaniment part), transcribed note-by-note from public-domain
sources and started together under one locked operation so both voices
stay sample-aligned from their very first callback.

**Adaptive boss tempo:** the Dreadnought fight reuses the same
GUNSHIP/BARRAGE/CRITICAL health-percentage thresholds that already drive
the boss's move speed and fire rate (see Boss Encounters) to also drive
the music: the boss theme speeds up from 1.00x to 1.15x to 1.30x as the
fight escalates. Only the playback rate changes on a tier shift — the
melody and accompaniment are never restarted or reset, so the
performance simply continues faster from wherever it already was,
staying perfectly in sync with itself.

**Sequencing, not overlap:** the title theme cuts out the instant
gameplay starts and restarts from the beginning (sample-aligned, back to
normal speed) the moment a run returns to the title screen. The boss
theme is silenced immediately on defeat, on a mid-fight player death, or
via the debug boss-skip key — it never keeps looping into a wave it no
longer belongs to. The Game Over theme is deliberately sequenced to wait
for the new-high-score fanfare (see Persistent High Score) to finish
playing before it starts, so the two are never heard at once.

------------------------------------------------------------------------

## Scout Weapons

Scouts now fight back with a dedicated enemy projectile system. Enemy
shots use their own fixed object pool, travel from right to left, render
as red-orange projectiles, and automatically return their slots to the
pool after leaving the screen.

Each Scout maintains its own firing timer rather than relying on one
global weapon timer. Successful shots are reported back to the game loop
so weapon audio is triggered only when a projectile is actually created.

The current development firing delay is **1000 ms**, making Scout fire
frequent enough to keep combat lively now that enemy bullets can
actually hit the player.

------------------------------------------------------------------------

## Bomber Enemies

The second enemy type is here, and it isn't just a reskinned Scout —
the Bomber introduces its own movement, its own weapon, and its own
risk/reward profile.

|  | Scout | Bomber |
| --- | --- | --- |
| Size | Small | Large |
| Speed | Fast | Slow |
| Health | 1 hit | 3 hits |
| Movement | Straight line | Bobs up and down |
| Weapon | Fires straight at the player | Drops a bomb from underneath |
| Points | 10 | 30 |
| Explosion | Standard burst | Bigger, darker burst |

**Movement:** rather than flying a straight horizontal line like a
Scout, a Bomber drifts slowly left while bobbing up and down in a smooth
sine-wave pattern, making it read as a heavier, less predictable target.

**Weapon:** Bombers don't aim at the player at all. Instead, each one
drops a bomb straight down from its belly on its own independent timer.
The bomb starts off falling slowly and picks up speed under a simple
gravity simulation, so timing a dodge means watching the bomb's fall,
not just its starting position. Bombs share the same enemy-projectile
object pool as Scout bolts but carry their own type, movement, collision
size, and rendering.

**Durability & reward:** a Bomber takes three hits to bring down instead
of one, and pays out three times the score. Landing the kill also
triggers a noticeably bigger explosion than a Scout's, using more
particles and a deeper orange palette to make the bigger kill feel like
a bigger event.

------------------------------------------------------------------------

## Asteroids / Environmental Hazards

Alongside Scouts and Bombers, the playfield now has a purely
environmental hazard: asteroids drifting in from the right edge of the
screen, independent of the enemy spawn system.

Asteroids use the same fixed-size object-pool pattern as every other
system in the game — no dynamic allocation, no linked lists, just a
`MAX_ASTEROIDS`-sized array with an `active` flag per slot.

|  | Large Asteroid | Small Fragment |
| --- | --- | --- |
| Health | 3 hits | 1 hit |
| Size | Larger | Smaller |
| Spawns naturally | Yes | No — only from splitting |
| Splits when shot down | Into 2 small fragments | No |
| Points (shot down) | 50 | 20 |
| Points (rams player) | None | None |

**Spawning:** large asteroids spawn from the right edge on their own
timer, at a randomized height that respects the HUD boundary, with
randomized speed and a small amount of vertical drift so they don't all
follow the same line across the screen.

**Splitting:** destroying a large asteroid with a bullet breaks it into
two small fragments, each inheriting the parent's momentum but kicked in
slightly different directions — one drifting up, one drifting down —
so they visibly spread apart. Fragments reuse free slots from the same
asteroid pool (never allocated separately), and small fragments do
**not** split again, which rules out runaway asteroid multiplication.

**Collision:** bullet/asteroid and player/asteroid collision are handled
in `collision.c` alongside the enemy collision logic, following the same
shape. Getting shot reduces an asteroid's health until it's destroyed;
colliding with the player always destroys the asteroid immediately, but
— matching how ramming an enemy already worked — awards no score and
doesn't trigger a split, so the player isn't punished with a fresh pair
of hazards right after taking a hit.

**Reused systems, not new ones:** asteroid destruction spawns a burst
from the *same* shared `ExplosionParticle` pool used by every other
explosion in the game (a bigger gray-orange burst for large asteroids, a
smaller one for fragments), and currently reuses the existing
Scout/Bomber destruction sound rather than introducing a new audio path.
A dedicated rock-breaking sound is a planned follow-up, not a missing
dependency.

------------------------------------------------------------------------

## Power-Ups

Destroyed Scouts and Bombers occasionally drop a collectible pickup —
a small colored capsule with a letter identifying it, drawn with the
same 5×7 bitmap font used everywhere else in the HUD rather than
hand-drawn icon art. Three effects exist so far:

|  | Rapid Fire (`R`) | Spread Shot (`3`) | Shield (`S`) |
| --- | --- | --- | --- |
| Type | Timed (10s) | Timed (10s) | One-charge |
| Effect | Shorter fire cooldown | Fires 3 bullets in a fan | Absorbs one hit |
| Drop source | Scouts (5%), Bombers (10%) | Scouts (5%), Bombers (10%) | Scouts (5%), Bombers (10%) |
| Re-collecting | Refreshes the timer | Refreshes the timer | No effect (still one charge) |

**Drops:** the drop chance and effect type are decided in the same
shared `enemy_destroyed()` helper collision.c already used for
explosions and scoring, so drops happen identically whether the kill was
a bullet hit or a ram — no duplicated logic. Asteroids have no
equivalent hook and never drop anything.

**State tracking:** rather than scattering loose timer variables through
`main.c`, every active effect lives in one `PowerUpState` struct (in
`powerup.c`) that `main.c` updates and reads each frame — the same
"plain struct plus free functions" style used by `Player`, `Enemy`, and
every other system in the project.

**Rapid Fire & Spread Shot:** both reuse the existing bullet pool and
firing path rather than introducing new bullet types. `bullets_fire()`
now takes an explicit cooldown parameter instead of a hardcoded
constant, so Rapid Fire is just a shorter number passed in for as long
as it's active. Spread Shot adds a second firing function,
`bullets_fire_spread()`, following the same "two spawn functions sharing
one pool" pattern already used for the Bomber's bombs versus the Scout's
bolts — it fires three bullets (straight, angled up, angled down) from
the same pool, and if the pool doesn't have three free slots it simply
fires fewer rather than overflowing. Both effects are independent and
can be active (and shown in the HUD) at the same time.

**Shield:** integrates directly into `player_take_damage()` — the
single function every hazard (enemy ramming, enemy bullets, asteroids)
already calls to apply damage. An active shield is consumed there and
the hit is absorbed before a life is ever lost, so no collision handler
needed its own shield-checking logic. A small SDL-primitive energy-field
outline renders around the ship whenever it's active.

**HUD:** rather than growing the reserved HUD strip (which would shrink
the playfield), Rapid Fire and Spread Shot each get a compact
letter-plus-shrinking-bar indicator squeezed into the dead horizontal
space that already existed between the score and lives text. The bar
blinks once a timed effect drops below ~25% duration remaining. Shield,
being charge-based rather than timed, just shows its letter with no bar.

**Collection feedback:** picking up any power-up now spawns a small
colored burst centered on the player, reusing `explosions_spawn()`
exactly as-is rather than a new particle system — just a smaller count
than any destruction effect, so it reads as "gained something" rather
than "something died." The color comes straight from the same
`powerup_color()` function that already colors the HUD bar for that
type, so a burst and its HUD indicator can never show mismatched colors.
This is purely additive: Shield's persistent energy-field outline
(driven by `powerup_state.shield_active` for the effect's whole
duration) is untouched — the burst is a one-time "you just picked this
up" beat, not a replacement for the ongoing indicator.

------------------------------------------------------------------------

## The Wave Director

Rather than every enemy/asteroid system spawning at its own fixed,
unchanging rate forever, a dedicated **Wave Director**
(`wave.h`/`wave.c`) now organizes the existing mechanics into a
structured, continuously escalating arcade progression — a classic
"director decides difficulty, existing systems execute it" split.
`wave.c` never spawns, moves, or renders an enemy or asteroid itself; it
only decides *when* and *how aggressively* the systems that already know
how to do those things should run.

**Time-based, not kill-based:** a wave lasts a fixed duration
(`WAVE_DURATION_MS`, currently 2 minutes — aiming for the pacing of a
classic NES-era shmup stage) rather than requiring the player to clear a
quota of enemies. There's no hard stop between waves: existing enemies,
asteroids, bullets, and power-ups all keep going exactly as they were,
and the *next* wave's spawn behavior simply phases in underneath them.

|  | Wave 1 | Wave 2 | Wave 3 | Wave 4 | Wave 5 | Wave 6+ |
| --- | --- | --- | --- | --- | --- | --- |
| Name | FIRST CONTACT | ASTEROID BELT | HEAVY CONTACT | CROSS FIRE | DREADNOUGHT | THREAT LEVEL *N* |
| Scouts | ✅ (forgiving) | ✅ | ✅ | ✅ | — | ✅ |
| Asteroids | — | ✅ (eased in) | ✅ | ✅ | — | ✅ |
| Bombers | — | — | ✅ (eased in) | ✅ | — | ✅ |
| Boss | — | — | — | — | ✅ (Dreadnought) | — |
| Difficulty | Hand-tuned | Hand-tuned | Hand-tuned | Hand-tuned | Fixed boss fight | Formula-scaled |

**Hand-tuned introduction, then math takes over:** Waves 1-5 are
individually hand-tuned so each one introduces exactly one new idea
(Scouts alone, then asteroids, then Bombers, then rising pressure). From
Wave 6 onward, every spawn delay and firing delay scales down linearly
with `difficulty_level = current_wave - 5` instead of requiring a
hand-written entry for every wave number forever.

**Safety first:** every scaled value is clamped to a hard-coded minimum
(e.g. a Scout can never spawn faster than every 300ms, or fire faster
than every 400ms) so the game can never reach an absurd wave number and
ask the CPU to spam bullets or enemies into oblivion. This was verified
with a standalone test harness at Waves 10, 25, 50, 100, 1000, and even
100,000 — difficulty climbs smoothly, then plateaus cleanly at the
floor, with no negative, zero, or integer-overflow behavior. The
clamping matters more than it might look: doing the scaling math
directly in unsigned arithmetic would let a high enough wave number
*wrap around* to a huge delay instead of a tiny one — the opposite
bug, silently making spawns stop instead of intensify.

**Not perfectly robotic:** each threat type spawns on its own
independent timer with its own small random jitter around its target
delay (rolled once per spawn, not every frame, so it doesn't
statistically bias toward the shortest possible interval), so Scouts,
Bombers, and asteroids drift out of sync with each other instead of
arriving on a predictable metronome.

**Existing systems, extended, not duplicated:** integrating this
required `enemies_spawn()` (previously one function handling both Scout
and Bomber spawning with a single shared timer and a random type roll)
to split into `enemies_spawn_scout()`/`enemies_spawn_bomber()`, each
with its own timer and an externally-supplied delay — mirroring the
two-function-sharing-one-pool pattern already used by the Bomber's
bomb-drop versus the Scout's laser bolt. `asteroids_spawn()` got the
same treatment. No new object pools, no new collision logic, no new game
state — the Wave Director only ever adjusts parameters those systems
already accept.

**Announcement overlay, not a game state:** a centered "WAVE X" /
wave-name announcement (reusing the existing 5×7 font, with a new
`text_width()` helper added to it for centering strings of varying
length) appears for ~1.8 seconds at the start of every wave. It's drawn
on top of gameplay, not a separate frozen state — the player, enemies,
bullets, and asteroids all keep moving underneath it. Wave progression
itself pauses cleanly during `GAME_OVER` the same way every other timer
in the project already does, and resets to Wave 1 on restart.

**Slide transition:** rather than a hard cut, each line slides in from
the right, holds centered, then continues off to the left — carved
entirely out of the *existing* ~1.8-second window (300ms in, 1200ms
holding, 300ms out) rather than extending it, so wave timing itself is
untouched. The centered destination is still computed first via
`text_width()` exactly as before; a small `announcement_slide_x()`
helper just animates toward it on the way in and away from it on the way
out, so a short name like "WAVE 4" and a long one like "THREAT LEVEL 47"
both still land correctly centered once the hold phase begins.

**Boss waves don't run on the clock:** `WaveDifficulty` carries a
`boss_wave` flag (currently set only for Wave 5). A boss-flagged wave
suppresses all normal Scout/Bomber/asteroid spawning and never
auto-advances on `WAVE_DURATION_MS` — `wave_update()` simply skips its
timer check entirely while one is active. The only way out is
`wave_advance_after_boss()`, called by `main.c` once the boss encounter
itself reports the fight is over. See Boss Encounters below.

**Minimum spawn spacing:** Scouts, Bombers, and large asteroids each now
re-roll their spawn Y position (up to a few attempts) if it lands too
close to another of the same type already on screen, so two that spawn
back-to-back read as two separate threats instead of a stacked blob
riding together.

**Wave 3/4 rebalance:** Wave 3 originally introduced Bombers *and*
slashed the Scout/asteroid spawn delays most of the way to their Wave 4
values in the same step — stacking a brand-new enemy type on top of the
single biggest spawn-rate jump in the whole progression, which made it
the wall most runs actually died on. Wave 3's delays were eased back so
Bombers arrive against an already-survivable baseline; Wave 4 now
absorbs more of that ramp instead.

------------------------------------------------------------------------

## Boss Encounters — The Dreadnought

Wave 5 is no longer a normal wave — it's a dedicated boss encounter
(`boss.h`/`boss.c`) against the **Dreadnought**, a large armored warship
built entirely from filled SDL2 rectangles rather than the hand-drawn
outlines used for Scouts and Bombers.

**Lifecycle:** the boss moves through six states — `INACTIVE` →
`WARNING` (building tension before it appears) → `ENTERING` (slides in
from the right edge) → `ACTIVE` (in combat) → `DYING` (destruction
sequence) → `DEFEATED`. `main.c` triggers `boss_begin_warning()` the
moment it sees Wave 5 begin, and hands control back to the Wave Director
once the fight is fully resolved — `wave.c` itself never inspects boss
health or state directly.

**Warning sequence:** rather than the boss simply appearing, Wave 5
opens with a ~1.8-second `BOSS_STATE_WARNING` beat — a blinking red
"WARNING" over a static white "DREADNOUGHT," backed by a continuous deep
klaxon (see Procedural 1983-Style Audio). This lives as a sub-state on
`Boss` itself rather than a new top-level game state, matching how
`ENTERING`/`ACTIVE`/`DYING` already model distinct lifecycle stages —
every function that already treats "not `BOSS_STATE_ACTIVE`" as inert
(`boss_fire()`, both boss collision handlers, `boss_spawn_support()`)
needed zero changes to also treat `WARNING` correctly. The existing
"WAVE 5 / DREADNOUGHT" announcement is suppressed specifically during
this state (both start at the same instant and last the same ~1.8
seconds, so showing both would overlap into unreadable stacked text) —
the boss-specific warning wins that moment since it's the more dramatic
and more specific of the two. `boss_update()` advances `WARNING` into a
normal `boss_spawn()` once the timer elapses, returning a
`BOSS_WARNING_ENDED` bit so `main.c` knows to stop the klaxon - which
also has to be stopped explicitly on every other path out of `WARNING`
(the `B` debug-skip key, or a restart happening mid-warning), since a
continuous sound - unlike this project's other one-shot effects - never
stops on its own.

| Phase | Trigger | Move Speed | Fire Rate | Attack Pattern |
| --- | --- | --- | --- | --- |
| GUNSHIP (1) | Fight start | Normal | Slowest | Straight nose-cannon shot only |
| BARRAGE (2) | ≤66% health | Faster | Faster | Alternates straight shot with a 3-bolt spread |
| CRITICAL (3) | ≤33% health | Fastest | Fastest | Mostly spread fire, periodically calls in a support Scout |

**Combat:** the boss holds a fixed horizontal combat position and
bounces vertically off the health-bar overlay and the bottom of the play
area. Its nose cannon reuses the exact same enemy-bullet pool and
collision path as a Scout's laser, so Shield and invulnerability both
apply automatically. The 3-bolt spread attack introduced a new
`enemy_bullets_fire_angled()`, letting a bolt travel along a custom
trajectory instead of the fixed straight-left path every other enemy
shot uses. CRITICAL's support Scout is a completely ordinary Scout,
spawned through the existing `enemies_spawn_scout()` — there's no such
thing as a "boss-flagged" enemy once it exists.

**Damage:** only player bullets can hurt the boss
(`collisions_bullets_boss()`), each hit knocking off one health point
and spawning a small spark at the impact point. Ramming the boss
(`collisions_player_boss()`) damages the player only, through the same
Shield/invulnerability-aware path every other hazard uses — colliding
with the Dreadnought can never be used to defeat it.

**Hit flash:** every successful hit briefly flashes the hull white,
reusing the exact same `set_boss_draw_color()` helper the existing
phase-transition flash already uses — a routine bullet impact just
combines into the same `flashing` boolean rather than needing its own
drawing logic. The one deliberate tuning choice: this flash is
noticeably shorter than the phase-transition flash (45ms vs. 400ms),
specifically because Rapid Fire's cooldown is only 70ms — anything
close to that length would keep the boss looking permanently white under
sustained fire instead of visibly flickering with each individual hit.

**Progressive damage (Phase 2+):** as BARRAGE and CRITICAL take over,
the Dreadnought visibly deteriorates using the exact same phase
thresholds that already drive its speed and fire rate — no separate
damage-state system was added. From BARRAGE onward, small spark bursts
periodically land at a random spot on the hull (reusing
`explosions_spawn()` again), on their own independent timer rather than
a per-frame random roll, matching how every other timed event in this
project already works. CRITICAL fires these more often and shifts them
to a smokier gray. Purely cosmetic, render-only additions layer on top:
the engine glow periodically cuts out (faster and more aggressively at
CRITICAL), and a couple of permanent scorch-mark pixels appear on the
hull at each phase — none of it touches health, speed, or fire rate,
which remain entirely the phase system's job.

**Feedback:** every phase transition triggers a brief white damage-flash
across the hull plus an explosion burst, so a phase change is impossible
to miss mid-fight. Defeat plays a staggered destruction sequence —
several small explosions scattered across the hull, then one large
finale — all reusing the same shared `ExplosionParticle` pool as every
other explosion in the game, followed by a short "DREADNOUGHT /
DESTROYED" message before the Wave Director advances to Wave 6.

**Presentation:** a dedicated health bar and "DREADNOUGHT" label overlay
draws near the top of the screen for the duration of the fight —
separate from the permanent HUD strip, since it only matters during a
boss encounter. The player's own top movement boundary is temporarily
raised so it can't fly up behind the bar.

Defeating the Dreadnought awards **500 points** — by far the largest
single reward in the game.

------------------------------------------------------------------------

## Destruction Effects

Enemies no longer simply vanish, and neither does the player. Every kill
— a Scout or Bomber destroyed by a player bullet, an enemy destroyed
by ramming the player, or the player getting hit — now triggers a
small retro particle explosion from a dedicated, reusable
`ExplosionParticle` object pool (the same fixed-array pool pattern used
for bullets and enemies).

Each burst throws a handful of pixel particles outward from the impact
point, fading from a bright white-yellow flash through the blast color
to a dark ember before the particle returns to the pool. Each ship type
gets a visually distinct burst: Scouts break apart in orange/red,
Bombers go out in a bigger, deeper-orange fireball to match their size,
and the player's ship bursts in blue-white — all using particle counts
scaled to how big the explosion should read.

The explosions are matched with two new **procedurally generated**
destruction sounds, built the same way as the laser effects: filtered
white noise mixed with a decaying low-frequency rumble tone, synthesized
live in the SDL audio callback. The player's explosion is a longer,
lower-pitched boom; the enemy explosion (shared by Scouts and Bombers)
is a shorter, higher-pitched pop — easy to tell apart by ear, no audio
files involved.

------------------------------------------------------------------------

## Hit Flash Feedback

Explosions already communicate destruction; this covers damage that
*doesn't* destroy the target — a Bomber or large asteroid taking a hit
without dying, which previously had no feedback at all beyond the health
bar-less pool internally ticking down.

**One small helper per module, not a shared framework:** `enemy.c` and
`asteroid.c` each gained their own tiny
`set_enemy_draw_color()`/`set_asteroid_draw_color()` function — flash
white if a `hit_flash_until` timestamp hasn't elapsed, draw the normal
color otherwise — mirroring the exact pattern `boss.c`'s own
`set_boss_draw_color()` already used for its phase-transition flash.
`collision.c` sets that timestamp in an `else` branch right next to the
existing health-decrement code, only when the target *survives* the hit
(a lethal hit already gets a full explosion, which is feedback enough on
its own).

**Deliberately excluded:** Scouts and small asteroid fragments both have
exactly 1 HP, so every hit destroys them outright — there's never a
"survived a hit" moment to flash for, so neither got this treatment.

Health, score, and destruction are completely unaffected — every
trigger point is a rendering-only side effect sitting beside code that
was already there.

------------------------------------------------------------------------

## Screen Shake & Flash

A dedicated `screen_effects.h`/`screen_effects.c` module owns two
render-only presentation effects layered on top of the simulation —
nothing in it ever receives a pointer to `Player`, `Enemy`, `Boss`, or
any collision data; its only inputs are timestamps and tunable
constants, and its only outputs are a render offset and a color.

**Screen shake:** triggered by two events so far — a small, brief jolt
(2px / 150ms) on every player hit, and a noticeably stronger one (4px /
400ms) reserved for the Dreadnought's own destruction, so it reads as
clearly bigger than an ordinary hit. A weaker shake arriving while a
stronger one is still playing is ignored rather than cutting the bigger
one short. The offset re-rolls a fresh random jitter every frame and
decays linearly to zero, so it visibly rattles rather than holding one
static displacement.

**Render-only, guaranteed:** `SDL_RenderSetViewport()` interprets its
rect in the same logical 160×120 coordinate space
`SDL_RenderSetLogicalSize()` already established at startup, so
offsetting it shifts everything drawn for the rest of that frame by
exactly the shake amount — no gameplay struct's `x`/`y` is ever
touched, and collision code has no way to even know shake exists. The
viewport is recomputed and set fresh every single frame, even when idle
(where it's just `(0, 0)`), so there's no stale-viewport state that
could carry into a frame where nothing is shaking.

**Flash:** a brief, translucent full-screen color overlay, currently
used for a subtle red flash on the player's final death (see Final Death
Sequence below). This is the one place the renderer's blend mode is ever
changed away from the default — `screen_effects_render_flash()`
switches to `SDL_BLENDMODE_BLEND` just long enough to draw the flash
rect, then explicitly restores whatever blend mode was active before, so
nothing else in the renderer is ever left in a different state than it
started in.

------------------------------------------------------------------------

## Floating Score Popups

A `popup.h`/`popup.c` module shows the player exactly what a kill was
worth — "+10" for a Scout, "+30" for a Bomber, "+500" for the
Dreadnought — using a fixed-size `ScorePopup` pool, the same "plain
struct plus free functions, no allocation" pattern as every other pool
in the project.

**Visualizes score, never awards it:** every popup call sits *after* the
matching `score +=` line in `collision.c`, reading the exact value that
call just added (straight from
`enemy_destroyed()`/`asteroid_destroyed()`'s own return value, so no
score constant is ever duplicated). The module has no reference to the
`score` variable anywhere in it — it is structurally incapable of
awarding points itself, only echoing a value the caller already
credited.

**Ramming correctly gets no popup:**
`enemy_destroyed()`/`asteroid_destroyed()` are shared by both the
bullet-kill path and the ramming path, but ramming awards zero score
(its return value is simply discarded by `main.c`). Rather than spawn a
popup inside those shared helpers — which would incorrectly show "+30"
for a kill that earned nothing — the popup call lives specifically at
the bullet-kill call sites, where score is actually credited.

**Fade and motion:** each popup drifts upward at a fixed per-frame rate
and fades through three fixed color tiers by remaining-life fraction
(bright → gold → dim ember) — the same technique `explosion.c` already
uses for its own fade-out, rather than true alpha blending the rest of
the renderer doesn't otherwise use. A missed spawn when the pool is full
is silently skipped, never treated as an error.

**The Dreadnought's payout feels bigger:** its +500 popup uses a second
spawn function, `popups_spawn_emphasized()`, sharing the exact same pool
and render path but with a longer duration and larger scale — reused,
not duplicated.

------------------------------------------------------------------------

## Extra Lives

Classic NES-era shmups almost always reward the player with a bonus life
at a score milestone, and the game now does the same: reach **1,000
points** and you earn an extra life, then another **every 2,000 points**
after that (3,000 / 5,000 / 7,000 / ...).

**Why those numbers:** the thresholds are tuned against the game's own
scoring economy, not picked arbitrarily — a Scout is worth only 10
points and even the Dreadnought boss is 500, so a flat "every 20,000
points" convention borrowed straight from a bigger-scoring NES game
would almost never trigger. The first threshold is deliberately close so
an average run earns one quickly (typically partway through Wave 2),
while the repeating interval is wider so lives taper off relative to
score rather than piling up once Wave 6+'s difficulty scaling makes
points easier to earn.

**Implementation:** the next threshold lives directly on `Player`
(`next_extra_life_score`) rather than as a separate list of milestones
— `player_check_extra_life()` compares it against the current score
once per frame and, using a `while` loop rather than `if`, can award
more than one life in the same frame if a big score jump (like the
boss's 500-point payout) happens to cross more than one threshold at
once. It advances by a fixed interval each time it's awarded, so there's
no hardcoded table of "every possible" milestone to run out of.

**Fanfare:** rather than reusing the pickup sound, an extra life plays
its own short 4-note ascending arpeggio (C5 → E5 → G5 → C6) — a
classic "1-up jingle" shape built the same procedural way as every other
sound in the game (see Procedural 1983-Style Audio), just stepping
through a fixed note sequence instead of sliding one tone.

**On-screen notification:** the fanfare is now matched with a brief
"EXTRA LIFE" readout, armed as a side effect of
`player_check_extra_life()` awarding at least one life — the
function's actual job (incrementing `lives`) is completely unchanged,
this just piggybacks on it. If a single big score jump crosses more than
one threshold in the same frame (the boss's 500-point payout is the
likeliest way), the count is captured too, so it shows "EXTRA LIFE X2"
as one message rather than two overlapping ones. (The font only covers
A–Z and 0–9, so it reads "EXTRA LIFE" rather than "EXTRA LIFE!" —
there's no exclamation-mark glyph to fall back on.)

Resets to the first threshold on restart, alongside every other piece of
run state.

------------------------------------------------------------------------

## Title Screen, High Score & Arcade Game Over Flow

Launching the game no longer drops straight into Wave 1. It now follows
the same shape as a real arcade cabinet:

``` text
GAME_TITLE  --(SPACE)-->  GAME_PLAYING  --(death)-->  GAME_PLAYER_DEATH  --(delay)-->  GAME_OVER  --(ENTER)-->  GAME_TITLE  --(SPACE)-->  ...
```

Four states in total. `GAME_PLAYER_DEATH` is the newest — a short
dramatic pause between losing the final life and the Game Over screen
actually appearing (see Final Death Sequence below).
`GAME_TITLE`/`GAME_PLAYING`/`GAME_OVER` still make up the core loop; no
separate state was added just for the "new high score" message, which
remains a single `new_high_score` boolean tied to the `GAME_OVER`
screen, per the same design principle of not multiplying states for what
a flag can express.

**Title screen:** reuses the existing animated starfield as its
background rather than a second decorative star system, and the same 5×7
bitmap font/`text_width()` centering used everywhere else in the game.
Shows "STARFALL," the persisted high score, "PRESS SPACE," and the
controls. `GAME_TITLE` gates the entire gameplay-simulation block in
`main.c` — no enemies, asteroids, wave progression, power-up spawning,
or boss behavior run while sitting on the title screen; only the
starfield keeps moving.

| Key | Action |
| --- | --- |
| Arrow keys | Move the ship |
| SPACE | Hold to fire during gameplay · press to start a new game from the title screen |
| ENTER | Return to the title screen from Game Over |
| P | Pause / resume during gameplay |

**Starting a game without a same-press double-fire:** leaving the title
screen is driven by a discrete `SDL_KEYDOWN` event (filtered to ignore
OS auto-repeat) rather than the continuous keyboard state used for
firing, and the actual transition into `GAME_PLAYING` is applied only
*after* that frame's gameplay logic has already run — so the very
press that leaves the title screen can never also register as frame
one's shot. A `suppress_fire_until_space_released` flag then covers
every subsequent frame that same physical press is still held (a human
tap easily spans several frames at 60 FPS), only letting SPACE mean
"fire" again once the key is actually released. Leaving `GAME_OVER`
deliberately uses a *different* key (ENTER, not SPACE) for the same
reason: dying while holding SPACE — the fire button, a very likely
moment to be holding it — must not let one held key walk straight
through `GAME_OVER → GAME_TITLE → a brand new game` without the player
ever seeing the title screen in between.

**Centralized reset — `game_start_new()`:** every piece of run state
(player, bullets, enemies, asteroids, power-ups, explosions, wave, boss,
spawn timers, the extra-life threshold, the new-high-score flag, score)
resets through one function, called from the single place a run is
allowed to begin — restarting after Game Over goes through the title
screen first rather than having its own separate reset path.
Centralizing this also fixed a real bug: `wave_init()` used to only run
once, at program startup, so Wave 1's "how long has this announcement
been showing" timer started counting from the moment the executable
launched rather than from when the player actually pressed SPACE —
lingering on the title screen for a couple of seconds could silently
expire it before Wave 1 ever became visible.

**Game Over:** shows the run's final score, the current high score (or
"NEW HIGH SCORE!" in its place — see below), and "PRESS ENTER," all
centered the same way as the title screen. The frozen battlefield stays
visible behind the text (only the starfield keeps animating) and nothing
advances again until the player acts — there's no timer that returns
to the title screen automatically.

**New high score detection, exactly once:** the comparison against the
stored record happens at the `GAME_PLAYER_DEATH -> GAME_OVER` handoff,
not at the moment of the fatal hit. By the time that handoff happens,
`score` can no longer change at all — every scoring source for the
death frame already ran before `game_state` left `GAME_PLAYING`, and
nothing touches it during the `GAME_PLAYER_DEATH` pause — so there's
no risk of comparing against a value that's still about to go up. The
comparison is strictly greater-than, so tying the existing record
doesn't count as beating it. A new record updates the in-memory high
score, saves it to disk, and triggers a fanfare — all exactly once, at
that single handoff point.

------------------------------------------------------------------------

## Final Death Sequence

Losing the final life used to jump straight to `GAME_OVER`. It now
pauses for about a second first, giving the player's own destruction
effect room to play out before the screen changes.

**A new `GameState`, not a flag on `GAME_OVER`:** `GAME_PLAYER_DEATH`
sits between `GAME_PLAYING` and `GAME_OVER`. The alternative — reusing
`GAME_OVER` with an internal "is the pause still running" flag — would
have needed that flag threaded into every place that already gates on
`game_state == GAME_OVER` specifically: the Game Over text block, the
ENTER-key input handler, and the wave-announcement suppression. A
distinct state value gets all of that for free, with zero changes to any
of those existing gates - they simply keep working as
`GAME_PLAYER_DEATH` and `GAME_OVER` remain different values.

**Freezing came for free:** by the time this milestone began, leaving
`GAME_PLAYING` already stopped movement, firing, collisions, scoring,
and wave/boss updates entirely, while the starfield, the shared
explosion pool, the score-popup pool, and the screen-shake viewport
offset all already updated unconditionally every frame regardless of
state. `GAME_PLAYER_DEATH` needed none of that rebuilt - it's the exact
same "everything real stops, a few purely cosmetic things keep
animating" behavior `GAME_OVER` already had, just under its own state
value so the Game Over screen doesn't appear yet.

**No `SDL_Delay()`:** the pause is a plain elapsed-time check
(`SDL_GetTicks() - player_death_started_at >= PLAYER_DEATH_DELAY_MS`)
evaluated once per frame like every other timer in the project. The
event loop, and every purely cosmetic system, keeps running normally
throughout - nothing freezes execution itself.

**Flash, finally implemented:** `screen_effects.c`'s flash mechanism
(see Screen Shake & Flash) was deliberately left unbuilt until a real
trigger existed - this is that trigger. A brief, translucent red flash
accompanies the existing player-hit shake on the fatal blow, kept
translucent specifically so it doesn't hide the destruction effect
playing out underneath it.

**A simplification fell out of the redesign:** the earlier `just_died`
same-frame-deferral flag (from resolving a score-timing bug discovered
during the Phase 9 input audit) is gone entirely. Since no gameplay
logic ever runs again once `GAME_PLAYING` is left, and several real
frames now pass during the pause before the high-score comparison
happens, `score` is unquestionably final long before that comparison
ever runs - there's nothing left to defer within a single frame.

------------------------------------------------------------------------

## Pause

Pressing **P** during `GAME_PLAYING` freezes the run in a new
`GAME_PAUSED` state, with a "PAUSED" overlay drawn over the frozen
battlefield the same way Game Over's text sits over it. Pressing P again
resumes exactly where the run left off.

**One offset, every timer frozen at once:** rather than pausing each
system individually, every gameplay timer in `main.c` — spawn delays,
wave duration, invulnerability, boss timers, popups, screen effects, all
of it — already reads "now" through a single `game_ticks()` function
instead of calling `SDL_GetTicks()` directly. While paused, `game_ticks()`
simply returns the frozen instant pausing began; on resume, however long
the pause lasted is folded into a running offset so every timer picks up
again from exactly where it stopped, with no sudden jump. Without this,
resuming would make every "time since X" check see a burst equal to the
pause's real-world duration — read as enemies spawning all at once, a
wave ending early, or a power-up expiring instantly.

Screen shake and flash are the one deliberate exception: they already
run every frame regardless of game state (so a hit's shake can finish
animating even into Game Over), so pausing freezes their *effective*
input via the same `game_ticks()` call rather than needing a separate
pause check of their own.

------------------------------------------------------------------------

## Persistent High Score

A dedicated module (`highscore.h`/`highscore.c`) owns all save-file I/O
— `main.c` calls `highscore_load()`/`highscore_save()` and never
touches `fopen()` itself.

**File format:** a small human-readable text file, `highscore.dat`,
containing just the number (e.g. `12450`) — no JSON, no binary format,
no leaderboard yet. It's created relative to whatever directory the
executable is launched from; that's a known simplification for this
milestone rather than an oversight, and is worth revisiting with a
proper per-user save location if the game is ever packaged for
distribution.

**Safe parsing:** `fgets()` into a fixed buffer, then `strtol()` with
its `endptr` checked against the buffer's start — a clean way to
detect "found zero digits at all" (garbage text) without hand-rolled
string scanning. A missing file, an empty file, unparsable text, a
negative number, and an absurdly large value (clamped to `INT_MAX`) are
all treated identically: not an error, just "no valid high score on
record yet," resolving to `0`.

**When it's read and written:** loaded exactly once, at program startup,
before the title screen ever appears — never re-read from disk
anywhere else, and never rewritten every frame while it's just being
displayed. It's saved to disk exactly once per run, and only when that
run's final score beats the existing record.

**Failure is not fatal:** if the file can't be written (e.g. a read-only
working directory), `highscore_save()` prints a warning to stderr and
returns `0`. The in-memory value still updates for the rest of the
session either way, and the game keeps running rather than treating a
failed save as a reason to stop.

**Fanfare:** a new high score plays a longer, more triumphant sound than
the extra-life jingle — a full 8-note ascending C major scale (C5
through C6, every scale degree rather than the extra-life arpeggio's
1-3-5-8 skip) with the final note held roughly four times longer than
the rest, so it lands and settles rather than just stopping. Built with
the same "step through a fixed note table" technique as the extra-life
fanfare (see Extra Lives), just with one extra wrinkle: the final note's
duration is chosen conditionally instead of being the same fixed length
as every other note.

------------------------------------------------------------------------

## Retro HUD

The game includes a reusable **5×7 bitmap font renderer written in C**,
with support for **A–Z and 0–9**. It powers the live score/lives HUD,
the title screen, and the Game Over interface without SDL_ttf or
external font assets.

The top of the 160×120 logical screen is reserved as dedicated HUD space
(still just 10px tall — adding power-ups didn't grow it; their compact
indicators fit in existing dead space instead). Player movement and
enemy spawning respect this boundary, with a divider separating the
interface from the playfield. The HUD itself — along with the ship,
enemies, and every other gameplay entity — is hidden entirely while
`GAME_TITLE` is active (see Title Screen, High Score & Arcade Game Over
Flow above); it reappears the moment a run actually begins.

------------------------------------------------------------------------

## Project Architecture

The project began as a single `main.c` approaching **600 lines**. As
systems became functional, they were extracted into dedicated modules.
`main.c` is now primarily the game's orchestrator.

An eight-phase, behavior-preserving refactoring pass (v0.8.0) later
went through `main.c` specifically to make that orchestrator role
explicit: presentation moved into `game_render.c`, the scattered
run/state-machine bookkeeping consolidated into one `GameRuntime`
struct, input handling and state transitions became named functions,
and the entire `GAME_PLAYING` per-frame update became seven ordered,
semantically-named stages instead of one long inline block. See Main.c
Code-Quality Refactor in the roadmap below for the full phase list.

``` text
starfall/
│
├── include/
│   ├── asteroid.h
│   ├── audio.h
│   ├── boss.h
│   ├── bullet.h
│   ├── collision.h
│   ├── enemy.h
│   ├── enemy_bullet.h
│   ├── explosion.h
│   ├── game_config.h
│   ├── game_render.h
│   ├── highscore.h
│   ├── music.h
│   ├── player.h
│   ├── popup.h
│   ├── powerup.h
│   ├── screen_effects.h
│   ├── soundtrack.h
│   ├── starfield.h
│   ├── text.h
│   └── wave.h
│
├── src/
│   ├── asteroid.c
│   ├── audio.c
│   ├── boss.c
│   ├── bullet.c
│   ├── collision.c
│   ├── enemy.c
│   ├── enemy_bullet.c
│   ├── explosion.c
│   ├── game_render.c
│   ├── highscore.c
│   ├── music.c
│   ├── player.c
│   ├── popup.c
│   ├── powerup.c
│   ├── screen_effects.c
│   ├── soundtrack.c
│   ├── starfield.c
│   ├── text.c
│   └── wave.c
│
├── main.c
└── space_shooter.cbp
```

| Module | Responsibility |
| --- | --- |
| `player.c` | Player initialization, movement, boundaries, damage, extra-life score thresholds, the extra-life notification & rendering |
| `starfield.c` | Parallax stars, recycling & rendering |
| `bullet.c` | Bullet pool, firing, cooldown, movement & rendering |
| `enemy.c` | Enemy pool, Scout & Bomber spawning, movement (straight-line or bobbing), rendering (including Bomber hit-flash) & timed firing behavior |
| `enemy_bullet.c` | Enemy projectile pool covering both Scout laser bolts and Bomber bombs, firing, movement (including bomb gravity), rendering & cleanup |
| `asteroid.c` | Asteroid pool, large-asteroid spawning, movement & off-screen cleanup, large → small fragment splitting & rendering (including large-asteroid hit-flash) |
| `boss.c` | The Dreadnought boss: lifecycle state machine (including the warning sub-state), three-phase combat, fire patterns, support-Scout calls, destruction sequence, progressive damage effects & health bar/defeat/warning overlays |
| `powerup.c` | Power-up pickup pool, spawning/movement/rendering, the `PowerUpState` effect tracker, HUD indicator rendering & shield outline |
| `explosion.c` | Reusable particle-pool explosion effect, spawning, physics & fade-out rendering |
| `highscore.c` | Persistent high-score file I/O - safe load/save, isolating all `fopen`/parsing from `main.c` |
| `screen_effects.c` | Render-only screen shake & full-screen flash - a viewport/color offset only, never touches gameplay coordinates |
| `popup.c` | Floating "+value" score popup pool - visualizes score already awarded elsewhere, never awards it itself |
| `collision.c` | Cross-system collision handling, score results, hit-flash timers & spawning destruction effects |
| `audio.c` | SDL2 audio device, procedural weapon/explosion/fanfare/klaxon synthesis & sound mixing |
| `music.c` | Generic multi-voice music sequencer - voice/note advancement, sample-accurate looping & playback-rate scaling, no game-specific knowledge |
| `soundtrack.c` | STARFALL's own compositions (title/boss/Game Over themes) & the intent-level API `main.c` calls to trigger them |
| `game_render.c` | Renders one complete frame (title, HUD, entities, overlays) from a read-only `RenderContext` - presentation only, never mutates gameplay state, awards score, or starts audio |
| `text.c` | Custom scalable 5×7 bitmap text renderer (A–Z, 0–9) & string-width measurement |
| `wave.c` | The Wave Director - wave timing, hand-tuned/formula-scaled difficulty, spawn-jitter floors, and the wave announcement overlay |
| `game_config.h` | Shared screen, HUD & gameplay-area dimensions |
| `main.c` | SDL setup, game loop, game states, HUD & system coordination |

------------------------------------------------------------------------

## Tech Stack

| | |
| --- | --- |
| **Language** | C |
| **Graphics / Input / Audio** | SDL2 |
| **Audio** | Procedural waveform synthesis |
| **Development OS** | Arch Linux (Omarchy) |
| **IDE** | Code::Blocks |
| **Compiler** | GCC |
| **Architecture** | Multi-file native C |

The current game uses **no sprite assets**. Ships, stars, enemies, and
projectiles are drawn with SDL2 primitives.

------------------------------------------------------------------------

## Building on Linux

For Arch Linux / Omarchy:

``` bash
sudo pacman -S --needed base-devel sdl2
```

For Debian / Ubuntu:

``` bash
sudo apt update
sudo apt install build-essential libsdl2-dev
```

Verify SDL2:

``` bash
pkg-config --modversion sdl2
pkg-config --cflags --libs sdl2
```

Typical compiler configuration:

``` text
-Wall -g -Iinclude -I/usr/include/SDL2
```

Typical linker libraries:

``` text
-lSDL2 -lm
```

The included `space_shooter.cbp` contains the Code::Blocks project
configuration.

### Building on Windows with MinGW

When compiling the project on Windows with MinGW/MSYS2, a couple of
source-level differences may be required depending on the SDL2
installation.

#### SDL2 Header Path

The Linux build uses:

``` c
#include <SDL.h>
```

For the Windows/MinGW SDL2 setup, use:

``` c
#include <SDL2/SDL.h>
```

This applies to project files that directly include the SDL2 header.

#### `main()` Entry Point

For the Windows/MinGW build, use the argument-based `main()` signature:

``` c
int main(int argc, char *argv[])
{
    /* game initialization */
}
```

instead of:

``` c
int main(void)
```

If `argc` and `argv` are not otherwise used by the game, they can remain
unused.

#### Cross-Platform Source Differences

When switching between the Linux and Windows/MinGW builds, these are the
two source changes to check first if SDL2 header or program entry-point
errors occur:

| Linux | Windows / MinGW |
| --- | --- |
| `#include <SDL.h>` | `#include <SDL2/SDL.h>` |
| `int main(void)` | `int main(int argc, char *argv[])` |

The Makefile uses the same source list for both build targets, including
`src/explosion.c`, `src/asteroid.c`, `src/powerup.c`, `src/wave.c`,
`src/boss.c`, `src/highscore.c`, `src/screen_effects.c`, and
`src/popup.c`.

------------------------------------------------------------------------

## Why Port It to C?

The original game was written in **Python + Pyxel**. This port explores
how those same systems work at a lower level.

| Python / Pyxel | C / SDL2 |
| --- | --- |
| Python objects | C structs |
| Dynamic collections | Fixed object pools |
| Methods | Functions + pointers |
| Python modules | `.c` + `.h` modules |
| Automatic references | Explicit pointers |
| Framework rendering | SDL2 renderer calls |
| Framework audio | SDL audio + generated samples |

> **The objective isn't simply to make the game work. The objective is
> to understand why it works.**

------------------------------------------------------------------------

## Development Roadmap

### Foundation

-   [x] SDL2 initialization
-   [x] Window and renderer
-   [x] Game loop
-   [x] Keyboard input
-   [x] Frame timing

### Player & Environment

-   [x] Player movement
-   [x] Screen boundaries
-   [x] Retro ship rendering
-   [x] Cockpit and exhaust
-   [x] Scrolling parallax starfield

### Combat

-   [x] Bullet object pool
-   [x] Hold-to-fire
-   [x] Fire cooldown
-   [x] Procedural laser audio
-   [x] Bullet/enemy collision
-   [x] Dedicated enemy projectile object pool
-   [x] Enemy projectile movement, rendering and cleanup
-   [x] Procedural Scout weapon audio
-   [x] Procedural Bomber bomb-drop audio
-   [x] Simultaneous player/enemy audio mixing
-   [x] Reusable explosion particle pool
-   [x] Player and enemy destruction effects
-   [x] Procedural destruction audio (player + enemy)

### Enemies

-   [x] Enemy pool
-   [x] Timed spawning
-   [x] Scout enemy
-   [x] Enemy health
-   [x] Retro Scout graphics
-   [x] Player/enemy collision
-   [x] Player lives and damage
-   [x] Timed invulnerability
-   [x] Invulnerability blink feedback
-   [x] Timed Scout firing
-   [x] Individual Scout firing timers
-   [x] Scout projectile attacks
-   [x] Enemy bullet/player collision
-   [x] Bomber enemy
-   [x] Retro Bomber graphics
-   [x] Bomber bobbing movement
-   [x] Multi-hit Bomber health
-   [x] Individual Bomber bomb-drop timers
-   [x] Bomb gravity and falling collision
-   [x] Bomber-specific score value and explosion
-   [x] Additional enemy types

### Hazards

-   [x] Asteroid object pool
-   [x] Large asteroid spawning (right-edge, randomized position &
    speed)
-   [x] Asteroid movement, drift and off-screen cleanup
-   [x] Procedural rocky asteroid rendering (large & small)
-   [x] Bullet/asteroid collision and multi-hit health
-   [x] Large → two small asteroid fragment splitting
-   [x] Small fragments (no further splitting)
-   [x] Player/asteroid collision using existing damage &
    invulnerability system
-   [x] Asteroid destruction effects (reusing the shared explosion pool)
-   [x] Asteroid scoring
-   [x] Asteroid pool reset on restart

### Power-Ups

-   [x] Power-up pickup pool
-   [x] Rapid Fire, Shield, and Spread Shot types
-   [x] Pickup spawning, movement & off-screen cleanup
-   [x] Retro pickup icon rendering (reusing the 5×7 font)
-   [x] Player/pickup collision & collection
-   [x] Centralized `PowerUpState` effect tracking (no loose timer
    variables)
-   [x] Rapid Fire cooldown integration (auto-reverts on expiry)
-   [x] Spread Shot 3-way bullet pattern (reusing the existing bullet
    pool)
-   [x] Shield integrated into the common player damage path
-   [x] Shield energy-field visual effect
-   [x] HUD countdown bars for timed effects, with low-time blink
    warning
-   [x] HUD shield indicator
-   [x] Re-collecting a timed effect refreshes its duration
-   [x] Random power-up drops from destroyed Scouts/Bombers (asteroids
    excluded)
-   [x] Procedural pickup sound (rising tone, distinct from every laser)
-   [x] Full power-up state reset on restart

### Wave Director

-   [x] Time-based wave progression (no hard stop between waves)
-   [x] `WaveState`/`WaveDifficulty` data model
-   [x] Hand-tuned Waves 1-5 with introductory names (FIRST CONTACT ->
    DREADNOUGHT)
-   [x] Wave 6+ mathematical difficulty scaling from a shared formula
-   [x] Hard-clamped minimum spawn/fire delays (verified safe up to Wave
    100,000)
-   [x] Centered wave announcement overlay (gameplay keeps running
    underneath it)
-   [x] Randomized spawn timing variance per threat type
-   [x] Scout/Bomber firing-rate difficulty scaling with its own safe
    floor
-   [x] Wave progression frozen during GAME_OVER
-   [x] Full wave state reset on restart
-   [x] `enemies_spawn()` split into independently-timed Scout/Bomber
    spawn functions
-   [x] `boss_wave` flag in `WaveDifficulty`, suppressing normal spawns
    and the auto-advance timer
-   [x] `wave_advance_after_boss()` to hand off to the next wave once a
    boss is defeated
-   [x] Minimum spawn-position spacing for Scouts, Bombers & large
    asteroids

### Boss Encounters

-   [x] Dedicated boss module (`boss.h`/`boss.c`) with its own lifecycle
    state machine
-   [x] Wave 5 converted into a fixed Dreadnought boss encounter
-   [x] Entrance sequence sliding in from the right edge
-   [x] Three-phase combat (GUNSHIP → BARRAGE → CRITICAL) driven by
    health percentage
-   [x] Phase-transition damage flash & explosion feedback
-   [x] 3-bolt spread attack via a new `enemy_bullets_fire_angled()`
-   [x] CRITICAL-phase support Scout calls (reusing
    `enemies_spawn_scout()`)
-   [x] Bullet/boss and player/boss collision, reusing the
    Shield/invulnerability path
-   [x] Staggered multi-explosion destruction sequence
-   [x] Boss health bar & label overlay
-   [x] "DREADNOUGHT / DESTROYED" completion message
-   [x] Player top-boundary raised during a boss fight to clear the
    health bar overlay
-   [x] Dreadnought defeat score value (500 points)

### Extra Lives

-   [x] Score-threshold extra life system (first at 1,000 points, then
    every 2,000 after)
-   [x] `next_extra_life_score` tracked on `Player`, advancing per award
    instead of a fixed milestone table
-   [x] Multi-threshold awards in a single frame handled correctly
    (e.g. a boss-kill score jump)
-   [x] Dedicated 4-note ascending arpeggio fanfare, distinct from the
    pickup sound
-   [x] Extra life state reset on restart

### Interface & Game States

-   [x] Custom 5×7 bitmap font with A–Z and 0–9
-   [x] Live score and lives HUD
-   [x] Reserved HUD gameplay boundary and divider
-   [x] GAME_TITLE / GAME_PLAYING / GAME_PLAYER_DEATH / GAME_OVER states
-   [x] Retro GAME OVER screen with final score and high score
-   [x] Full gameplay reset when a new game starts

### Arcade Front End & High Score

-   [x] `GAME_TITLE` as the initial state, gating all gameplay
    simulation (spawning, wave progression, boss behavior) until a game
    actually starts
-   [x] Title screen reusing the existing starfield and 5×7
    font/`text_width()` centering - no second decorative star system
-   [x] Title screen shows the game title, persisted high score, "PRESS
    SPACE," and controls
-   [x] Event-driven (not polled) SPACE-to-start, with same-frame and
    held-key double-fire both prevented
-   [x] Centralized `game_start_new()` reset, used by the only path a
    run is allowed to begin from
-   [x] Wave 1's announcement timer anchored to the actual start of a
    run, not program launch
-   [x] Dedicated `highscore.h`/`highscore.c` module owning all
    save-file I/O
-   [x] Plain-text `highscore.dat` save file, documented format and
    location
-   [x] Safe parsing via `fgets()` + `strtol()` - missing file, empty
    file, garbage text, negative values, and absurdly large values all
    resolve to `0` without crashing
-   [x] High score loaded once at startup, saved only on a genuine new
    record
-   [x] Failed save handled gracefully (warning to stderr, in-memory
    value still updates, game continues)
-   [x] New-record comparison deferred to the
    `GAME_PLAYER_DEATH -> GAME_OVER` handoff (see Arcade Polish & Game
    Feel below), so late-landing points from the death frame are never
    missed
-   [x] Strictly-greater-than comparison - tying the record doesn't
    count as beating it
-   [x] "NEW HIGH SCORE!" replaces the normal high-score line on Game
    Over when a record is set
-   [x] Dedicated 8-note ascending-scale fanfare for a new high score,
    distinct from the extra-life jingle
-   [x] `GAME_OVER -> GAME_TITLE` on ENTER (a different key from the
    title's SPACE, preventing a held-SPACE double-skip through both
    screens)
-   [x] Game Over requires explicit input - no auto-return-to-title
    timer
-   [x] Verified across a full multi-run arcade loop and a persistence
    test (missing file, corrupted file, quit/relaunch) with no state
    leakage or crashes

### Arcade Polish & Game Feel

-   [x] Dedicated `screen_effects.h`/`screen_effects.c` module -
    render-only, no gameplay coordinates ever touched
-   [x] Screen shake via `SDL_RenderSetViewport()` offset, recomputed
    fresh every frame (idle = `(0, 0)`, never stale)
-   [x] Player-hit shake (2px/150ms) and a stronger Dreadnought-defeat
    shake (4px/400ms); a weaker shake can't cut a stronger one short
-   [x] Full-screen translucent flash support, with explicit blend-mode
    restoration after every draw
-   [x] Per-module hit-flash helpers (`enemy.c`, `asteroid.c`) matching
    `boss.c`'s existing phase-flash pattern, for damage that doesn't
    destroy the target
-   [x] Boss hit-flash tuned shorter than Rapid Fire's cooldown, so
    sustained fire flickers instead of reading as solid white
-   [x] Dedicated `popup.h`/`popup.c` floating score-popup pool, spawned
    only where score is actually credited (never on a zero-score ramming
    kill)
-   [x] Emphasized (bigger, longer) popup variant reserved for the
    Dreadnought's +500
-   [x] `BOSS_STATE_WARNING` sub-state: blinking "WARNING / DREADNOUGHT"
    announcement before Wave 5's boss appears, suppressing the redundant
    normal wave announcement for that window
-   [x] Continuous procedural warning klaxon
    (`audio_set_boss_warning()`) - the project's first non-one-shot
    sound, explicitly stopped on every path out of the warning (normal
    completion, debug skip, or a mid-warning restart)
-   [x] Wave announcement slide-in/hold/slide-out transition, carved out
    of the existing announcement duration rather than extending it
-   [x] Power-up collection burst, colored via the same
    `powerup_color()` the HUD bar already uses, additive to (not
    replacing) Shield's persistent field
-   [x] "EXTRA LIFE" / "EXTRA LIFE X{n}" on-screen notification, armed
    as a side effect of `player_check_extra_life()` without changing
    what it awards
-   [x] Dreadnought progressive damage: phase-timed spark/smoke bursts,
    engine flicker, and permanent scorch marks, all keyed off the
    existing phase thresholds
-   [x] `GAME_PLAYER_DEATH` state: ~1 second dramatic pause after the
    final hit before `GAME_OVER` appears, with no `SDL_Delay()` anywhere
-   [x] High-score comparison moved to the
    `GAME_PLAYER_DEATH -> GAME_OVER` handoff, replacing the earlier
    same-frame `just_died` deferral entirely
-   [x] `screen_effects` explicitly reset on `GAME_OVER -> GAME_TITLE`
    (not just on `game_start_new()`), so a still-fading shake/flash can
    never bleed into the title screen
-   [x] Verified via a full code-level safety audit: no dynamic
    allocation anywhere in the project, every new pool/timer field
    traced to an init path, `-Wall -Wextra -Wpedantic` clean

### Procedural Soundtrack & Pause (v0.8.0)

-   [x] Generic reusable multi-voice music sequencer (`music.h`/
    `music.c`) - voices, note advancement, sample-accurate looping &
    playback-rate scaling, with no idea what a "title screen" or "boss
    fight" is
-   [x] Three fully-transcribed public-domain compositions: the Blue
    Danube Waltz (title theme), In the Hall of the Mountain King (boss
    theme), and Chopin's Marche funèbre (Game Over theme)
-   [x] Title theme loops on `GAME_TITLE`, stops instantly the moment
    gameplay starts, and restarts sample-aligned at normal speed on
    every return to the title screen
-   [x] Boss theme starts once at `BOSS_WARNING_ENDED` and accelerates
    through three intensity tiers (1.00x/1.15x/1.30x) tied to the
    existing GUNSHIP/BARRAGE/CRITICAL phase thresholds, without ever
    restarting or desyncing its two voices
-   [x] Boss theme stopped immediately on defeat, on a mid-fight player
    death, and via the debug boss-skip key
-   [x] Game Over theme sequenced to wait for the new-high-score
    fanfare to finish before starting, so the two are never heard
    overlapping
-   [x] `GAME_PAUSED` state (P key), freezing every gameplay timer
    through a single `game_ticks()` offset - including screen shake and
    flash, which run outside the normal gameplay-update gate
-   [x] Wave 3/4 difficulty rebalance - Wave 3's spawn-rate jump eased
    so introducing Bombers no longer coincides with the steepest ramp
    in the whole progression
-   [x] Soundtrack module refactor: composition data and an
    intent-level API (`soundtrack_play_title()`, `soundtrack_play_boss()`,
    `soundtrack_set_boss_intensity()`, `soundtrack_play_game_over()`,
    `soundtrack_stop()`) extracted from `main.c` into dedicated
    `soundtrack.h`/`soundtrack.c` - `main.c` no longer contains any raw
    composition note arrays
-   [x] Verified via a normal build and a strict
    `-Wall -Wextra -Wpedantic` build, both clean

### Main.c Code-Quality Refactor (v0.8.0)

An eight-phase, behavior-preserving refactoring pass through `main.c` -
no gameplay, timing, collision, spawning, scoring, boss, power-up,
control, pause, or soundtrack behavior changed at any point; every
phase had its own compile-and-playtest checkpoint before the next one
began.

-   [x] Phase 1 - baseline audit: recorded the pre-refactor line count
    (1,525) and build results, classified every raw `SDL_GetTicks()`
    call, and fixed a real bug found along the way - `audio_shutdown()`
    was missing from three of four program-exit paths (including
    normal shutdown)
-   [x] Phase 2 - all rendering (title screen, HUD, every entity pool,
    every overlay) extracted into `game_render.c`/`game_render.h`
    behind a single `game_render_frame()` entry point and a read-only
    `RenderContext`, with draw order provably unchanged
-   [x] Phase 3 - the run/state-machine bookkeeping that used to be
    scattered across file-scope statics and loose `main()` locals
    (pause tracking, death timing, fire suppression, boss music tier,
    high-score/fanfare flags, score) consolidated into one explicitly-
    initialized `GameRuntime` struct - deliberately not a "God struct":
    entity pools and per-subsystem spawn timers stayed outside it
-   [x] Phase 4 - SDL event polling, the pause toggle, and the
    `GAME_OVER -> GAME_TITLE`/`GAME_TITLE -> GAME_PLAYING` transitions
    extracted into named functions, so every legal state transition is
    easy to find and trace to the key that causes it
-   [x] Phase 5 - the entire `GAME_PLAYING` per-frame update split into
    seven ordered, semantically-named stages (player movement/death,
    firing, projectiles & collisions, threat spawning, power-ups, boss
    events & extra life, wave progression & debug controls) with the
    original update order preserved exactly
-   [x] Phase 6 - duplication cleanup: the repeated "explosion SFX +
    standard shake" player-damage response consolidated behind one
    helper, a `draw_centered_text()` helper replaced eleven duplicated
    centering calculations in `game_render.c`, and every stage
    function's repeated `game_ticks()` calls collapsed to one
    `now` snapshot per frame/stage
-   [x] Phase 7 - release-hygiene audit: fixed a missing `fclose()`
    error check in `highscore_save()`, removed two now-unused includes
    from `main.c`, corrected stale `space_shooter` branding in
    `space_shooter.cbp`'s project title and build output paths (now
    `starfall`, matching the Makefile), and ran a one-off
    AddressSanitizer/UndefinedBehaviorSanitizer build (clean - the only
    finding was an SDL-internal allocation, not attributable to this
    project's code)
-   [x] Phase 8 - final architecture review confirming `GameRuntime`
    stayed focused, rendering stayed isolated in `game_render`,
    soundtrack responsibilities stayed in `soundtrack`, and `main()`
    reads as orchestration rather than implementation detail; one
    stale comment corrected
-   [x] The `B` boss-skip and `5` Wave-5-jump development shortcuts
    preserved, untouched, through all eight phases - by explicit
    decision, not oversight
-   [x] `main.c`: 1,525 -> 1,602 lines - a slight increase, not a
    reduction, since responsibility moved out (`game_render.c`,
    `soundtrack.c`) rather than being deleted; every phase treated line
    count as information, never a target

### Future

-   [ ] Further enemy variety beyond Scouts and Bombers
-   [ ] Dedicated asteroid impact/destruction sound (currently reuses
    the Scout/Bomber destruction sound)
-   [ ] Dedicated boss weapon/hit audio (currently reuses the Scout
    laser and enemy explosion sounds)
-   [ ] Additional power-up types
-   [ ] Persistent wave counter in the HUD (currently shown only via the
    per-wave announcement - the single HUD row is already tightly packed
    with score/lives/power-up indicators)
-   [ ] Additional boss types beyond the Dreadnought, and boss
    encounters at waves beyond 5
-   [ ] Player initials and a top-ten leaderboard (current high score is
    a single value, by design for this milestone)
-   [ ] Per-user save location for `highscore.dat` instead of the
    working directory, if the game is ever packaged for distribution
-   [ ] Remove the temporary debug keys (skip boss fight / jump to
    Wave 5) added for faster iteration - kept deliberately through
    v0.7.0 since they're what makes the Wave 5 warning/boss sequence
    fast to iterate on
-   [ ] Screen shake/flash triggers for additional events (a large
    asteroid or Bomber destruction were suggested but deliberately not
    added yet - starting conservative so shake still reads as
    meaningful)
-   [ ] Further graphics polish
-   [ ] Fades/crossfades between soundtrack tracks (deliberately excluded
    from the v0.8.0 soundtrack work - every transition is currently an
    instant cut)
-   [ ] Additional soundtrack tracks (e.g. per-wave ambient themes)

------------------------------------------------------------------------

## Development Philosophy

Systems are deliberately built incrementally:

``` text
Design the data
      ↓
Build the simplest working version
      ↓
Compile & test
      ↓
Refactor into a module
      ↓
Compile & test again
      ↓
Improve behavior and presentation
```

That rhythm has worked well for me: make one thing work, test it, clean
it up, and only then move on to the next system.

------------------------------------------------------------------------

## Original Python Version

This project is a ground-up C/SDL2 port of **Pyxel Space Shooter**, the
original Python/Pyxel implementation that established the game's retro
arcade direction.

The two implementations are maintained separately so the Python original
and C port can exist side-by-side.

------------------------------------------------------------------------

## Screenshots

Gameplay screenshots and captures will be added as development
progresses.

<!--
<div align="center">
  <img src="docs/screenshots/gameplay.png" alt="STARFALL gameplay" width="700">
</div>
-->

------------------------------------------------------------------------

## Contributing

Suggestions, bug reports, and ideas are welcome. The project favors
straightforward readable C, understandable module boundaries, minimal
unnecessary dependencies, retro arcade presentation, and clarity over
cleverness.

------------------------------------------------------------------------

<div align="center">

### Built the old-school way

**C · SDL2 · GCC · Linux**

*Rebuilding a tiny arcade universe one `.c` file at a time.*

**A C learning project that turned into a real little arcade game.**

</div>
