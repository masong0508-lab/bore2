# The Sims 3 (Nintendo 3DS) Create-A-Sim: what it does, and how the DS port copies it (D67)

Source: a 3 s-per-frame capture of the 3DS game's Create-A-Townie (59 frames, one camera shot of the handheld). Notes written by the DS helper; the PS1 helper's `docs/CAS_SIMS3DS_WORKFLOW.md` was not on main yet when this was written, so check it for anything it saw that is not here.

## The workflow in the video
1. **Welcome pop-up** (top of the bottom screen, a tick button): "Welcome to Create A Townie! Here you can create a Sim to add to your town..." One tick and it is gone.
2. **A tab strip down the left of the touch screen**: six round icon tabs in a fixed order: **Basics, Looks, Hair, Clothing, Personality, Summary** (the last is a tick). The title of the current step is centred at the top of the panel, with an X (close) top right. The tab you are on stays lit. You can jump to any tab at any time; nothing is locked.
3. **Basics**: Name (first box, last box, a dice button that rolls a name), Gender (two big buttons), Fitness (thin / heavy), "Randomize - Create a random Sim", Skin colour (a row of six round swatches).
   - Tapping a name box opens an **on-screen QWERTY keyboard** on the bottom screen ("Enter First Name": a text bar, backspace arrow, number row, Shift, Space, Symbols, a tick to accept). After the first name it goes straight on to "Enter Last Name". The 3D Sim stays on the top screen.
4. **Looks**: a Capture Face button and a small picture of the face (Edit Features); inside, one screen per feature (Ears and Head, Eyebrows, Eyes (with eye colours at the right), Nose, Mouth): a list of presets down the left and a **square 2D pad** with a dot you drag. The top camera **zooms in to the face** and stays there.
5. **Hair**: small tab icons across the top (hair, facial hair, hats / accessories, lock), a **grid of thumbnails** (4 across, scroll), four colour swatches at the right. Picking one changes the Sim at once. Camera on the head.
6. **Clothing**: category icons across the top (everyday, formal, sleepwear, swimwear, athletic), then part icons (top, bottom, full body, shoes), a **scrolling grid of thumbnails** with a scroll bar and arrows, colour options. The top **camera moves to the part**: chest for tops, legs for trousers, feet for shoes.
7. **Personality**: a Random button, **three trait slots** (shown as "?" until filled), an Accept tick, and a **scrolling list of traits with a check box each**; a trait that is the opposite of a chosen one is greyed (Good Sense of Humor / No Sense of Humor); with three chosen no more can be added. A second screen picks the **Lifetime Wish** (a row of five wish icons, the chosen one's big picture and name below, a Change button); pointing at a wish shows its name and description in a box on the **top screen**.
8. **Summary**: the Sim's name, the outfit and look icons, Lifetime Wish, Personality, "Add to Sims Exchange" (a tick box) and a big tick to confirm. Confirming asks "Would you like to save your game?" Yes / No.
9. The top screen is the Sim standing in a room on a round pedestal the whole time, turning slowly; its camera is full body on Basics, Personality and Summary, close on Looks and Hair, and moves to the body part on Clothing.

## What the DS port does (ports/dsi/source/main.c, `cas_*`)
| 3DS | DS (D67) |
|---|---|
| Welcome pop-up with a tick | Pop-up "Create a Sim": 3 short lines and OK (A or B also closes it) |
| Six tabs down the left | The same six, an icon and a short word each: Basics, Looks, Hair, Outfit, Traits, Done. Tap one, or L / R, or Back / Next at the foot ("1 / 6") |
| X (close) | "< Title" on step 1 asks "Leave without making a Sim?" Leave / Stay |
| Name boxes, dice, QWERTY keyboard, first then last | The same: two boxes, a dice, a keyboard (digits, letters, - and ', Shift, Space, Del, Cancel, OK), First name then Last name, up to 9 letters each |
| Gender, Fitness, Randomize, skin swatches | Girl / Boy, Slim / Mid / Big (sets belly, torso and hips), Random Sim (everything), six skin tones and "Mix" (R G B bars) |
| Looks: face picture, one pad per feature | Face (6 face types), Features (Mouth, Eyes, Nose, Brows, Head, Jaw, Ears, Body: a pad per feature, Reset), Sliders (every slider, Face or Body, 6 a page, Max x1-x3) |
| Hair: style grid, facial hair, hats, colours | Style (the PS1's 26 hair styles, 8 a page, 4 pages), Colour (six dyes and R G B bars), Beard (6), Hat (6: none, cap, beanie, top hat, party, crown) |
| Clothing: top, bottom, colour | Top (6 colours as portraits), Bottom (6 trousers), Mix (any colour for the top) |
| Personality: 3 trait slots, list with check boxes, opposites greyed, Random | The same with 12 traits in six opposite pairs (Neat/Slob, Foodie/Light eater, Night owl/Sleepyhead, Friendly/Loner, Playful/Easygoing, Iron bladder/Tiny bladder). Tapping one shows its words on the top screen. They do something: the pair's need falls half as fast, or half as fast again |
| Lifetime Wish (5), description on the top screen | Five wishes (Swimming in Cash, Climb the Ladder, Landlord, Big Family, Fight Champion). Done once in the game: a toast "Wish done! +$2000". Shown on the Life page |
| Summary, tick, "save?" | Done: name, portrait, traits, wish; "Play!" asks "Start playing with this Sim?" Play! / Edit |
| Top screen: Sim on a pedestal, camera by step | The Sim turning slowly, camera full body (Basics, Traits, Done, Body pad / sliders), head (Looks, Hair), chest (Outfit Top / Mix), legs (Outfit Bottom); it glides between them |

Not copied: Capture Face (no camera), outfit categories (everyday / formal / ...: the DS Sim has one outfit), shoes, eye colours as a list (the PS1 hair-colour decides the iris), Add to Sims Exchange, the pedestal and room (the DS draws the sky).

## Saving
A d65 / d66 save has none of this: it loads with no hat, Navy trousers, no traits and no wish. The new numbers are in a new block `'CX'` (8 bytes a Sim) after the Boreagotchi's block in the mini-games' save area (`mg_pack` / `mg_unpack` in ds_games.c). The `Sim` struct is untouched (it is saved whole).
