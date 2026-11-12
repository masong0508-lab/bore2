#!/usr/bin/env python3
"""make_traits.py: the BORE 2 trait / wish / earned-trait / cheat-trait data, made from one table of EFFECT HOOKS.

    python3 tools/make_traits.py            writes src/traits_data.h and docs/traits.json

A trait is a short list (1 to 3) of effects; an effect is (hook, magnitude). A hook is one real gameplay number the game reads
through src/traits.h (a need's fall rate, a skill's gain, the pay of a career, the speed of a vehicle ...). The magnitude is in tiers:
percent (or points) = magnitude * the hook's unit.  Nothing is random at run time: this script is deterministic (same output every run).
See docs/TRAITS.md.
"""
import json, os, random, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# ---------------------------------------------------------------- styles: how a hook counts
# mode: 0 PCT (scale a number), 1 ADD (flat points / levels), 2 CHN (chance, percentage points), 3 FLR (mood floor)
M_PCT, M_ADD, M_CHN, M_FLR = 0, 1, 2, 3
#            mode   unit good  regular tiers T1..T4   cheat tiers C1..C3
STYLES = {
    'DEC': (M_PCT,  5, -1, (2, 3, 4, 6),   (-10, -18, -20)),   # a fall rate: less is better; -20 = nothing falls
    'GAIN':(M_PCT,  5, +1, (2, 3, 4, 6),   (20, 60, 100)),
    'SKL': (M_PCT, 10, +1, (1, 2, 3, 4),   (10, 40, 120)),
    'INC': (M_PCT, 10, +1, (1, 2, 3, 4),   (10, 40, 90)),
    'PAY': (M_PCT,  5, +1, (2, 3, 4, 6),   (20, 60, 100)),
    'CUT': (M_PCT,  5, -1, (2, 3, 4, 6),   (-10, -18, -20)),   # a cost / damage taken: less is better; -20 = free
    'SPD': (M_PCT,  5, +1, (2, 3, 4, 6),   (20, 40, 60)),
    'CHN': (M_CHN,  3, +1, (2, 3, 4, 6),   (10, 20, 34)),
    'ADD': (M_ADD,  2, +1, (1, 2, 3, 4),   (8, 20, 50)),
    'LVL': (M_ADD,  1, +1, (1, 1, 2, 3),   (4, 7, 10)),
    'FLR': (M_FLR, 10, +1, (1, 2, 3, 4),   (10, 15, 20)),
}

CATS = ['BODY', 'MIND', 'SOCIAL', 'ROMANCE', 'WORK', 'MONEY', 'FOOD', 'HOME', 'OUTDOORS', 'CREATIVE', 'FUN', 'NIGHT AND DAY', 'PETS', 'LUCK', 'FIGHTING', 'EARNED']
CAT_SHORT = ['BODY', 'MIND', 'SOCIAL', 'ROMANCE', 'WORK', 'MONEY', 'FOOD', 'HOME', 'OUTDOORS', 'CREATIVE', 'FUN', 'NIGHT/DAY', 'PETS', 'LUCK', 'FIGHT', 'EARNED']
CAT_ICON = ['IC_BOLT', 'IC_BOOK', 'IC_CHAT', 'IC_HEART', 'IC_CASE', 'IC_MONEY', 'IC_FOOD', 'IC_HOUSE', 'IC_SUN', 'IC_BRUSH', 'IC_PARTY', 'IC_BED', 'IC_SMILE', 'IC_DICE', 'IC_HAMMER', 'IC_STAR']
CHEAT_SUBS = ['NEEDS', 'SKILLS', 'MONEY', 'WORK', 'SOCIAL', 'MOOD', 'TRAVEL', 'GAMES', 'LIFE', 'MIXES']

# ---------------------------------------------------------------- the hooks
# CODE|category|sub-group|style|text when up ({n} = the number)|text when down|good nouns (3)|bad nouns (2)|flaw adjectives (2)|cheat noun
HOOKS_TXT = """
DEC_FOOD|FOOD|HUNGER|DEC|GETS HUNGRY {n}% FASTER|GETS HUNGRY {n}% SLOWER|Camel,Grazer,Iron Gut|Glutton,Bottomless|Greedy,Peckish|Stomach
DEC_WC|BODY|BLADDER|DEC|BLADDER FILLS {n}% FASTER|BLADDER FILLS {n}% SLOWER|Cast Iron,Dry Rock,Big Tank|Leaky,Tiny Tank|Leaky,Jumpy|Bladder
DEC_REST|BODY|SLEEP|DEC|GETS TIRED {n}% FASTER|GETS TIRED {n}% SLOWER|Marathon,Fresh Start,Energiser|Yawner,Drowsy|Sleepy,Dozy|Energy
DEC_CLEAN|BODY|HYGIENE|DEC|GETS DIRTY {n}% FASTER|GETS DIRTY {n}% SLOWER|Spotless,Squeaky,Polished|Mucky,Dust Magnet|Grubby,Smelly|Hygiene
DEC_COMFY|HOME|COMFORT|DEC|COMFORT FADES {n}% FASTER|COMFORT FADES {n}% SLOWER|Cosy,Snug,Cushion|Fidget,Wriggler|Fidgety,Itchy|Comfort
DEC_FUN|FUN|PLAY|DEC|GETS BORED {n}% FASTER|GETS BORED {n}% SLOWER|Easy Smile,Playful,Merry|Boredom,Yawn|Bored,Dull|Fun Meter
DEC_SOCIAL|SOCIAL|CHAT|DEC|GETS LONELY {n}% FASTER|GETS LONELY {n}% SLOWER|Hermit,Content One,Solo|Clinger,Needy One|Clingy,Needy|Company
GAIN_FOOD|FOOD|MEALS|GAIN|FOOD FILLS {n}% MORE|FOOD FILLS {n}% LESS|Hearty,Big Eater,Full Plate|Picky,Nibbler|Picky,Fussy|Meals
GAIN_WC|BODY|BLADDER|GAIN|RELIEF IS {n}% BIGGER|RELIEF IS {n}% SMALLER|Quick Flush,Easy Going,Relief|Shy Bladder,Slow Flow|Shy,Hesitant|Relief
GAIN_REST|BODY|SLEEP|GAIN|RESTS {n}% BETTER|RESTS {n}% WORSE|Deep Sleeper,Power Napper,Dreamer|Light Sleeper,Tosser|Restless,Twitchy|Rest
GAIN_CLEAN|BODY|HYGIENE|GAIN|WASHES {n}% BETTER|WASHES {n}% WORSE|Scrubber,Sparkle,Soap Hero|Lazy Washer,Splasher|Splashy,Hasty|Scrub
GAIN_COMFY|HOME|COMFORT|GAIN|COMFORT GROWS {n}% MORE|COMFORT GROWS {n}% LESS|Sofa Sage,Lounger,Nester|Stiff Back,Prickle|Stiff,Prickly|Lounge
GAIN_FUN|FUN|PLAY|GAIN|FUN GROWS {n}% MORE|FUN GROWS {n}% LESS|Fun Seeker,Joy Hunter,Giggler|Killjoy,Straight Face|Dour,Grim|Joy
GAIN_SOCIAL|SOCIAL|CHAT|GAIN|CHATS FILL {n}% MORE|CHATS FILL {n}% LESS|Chatterbox,Good Company,Warmth|Cold Fish,Flat Tone|Cold,Flat|Warmth
ACT_EAT|FOOD|MEALS|GAIN|MEALS GIVE {n}% MORE|MEALS GIVE {n}% LESS|Foodie,Gourmet,Big Plate|Skimper,Fast Food|Rushed,Bland|Appetite
ACT_SLEEP|BODY|SLEEP|GAIN|SLEEP GIVES {n}% MORE|SLEEP GIVES {n}% LESS|Sleepyhead,Log,Hibernator|Fitful,Insomniac|Wakeful,Jittery|Slumber
ACT_SHOWER|BODY|HYGIENE|GAIN|SHOWERS GIVE {n}% MORE|SHOWERS GIVE {n}% LESS|Shower Singer,Steamer,Rinse|Drip Dry,Quick Dunk|Dripping,Rushed|Shower
ACT_SIT|HOME|COMFORT|GAIN|SITTING GIVES {n}% MORE|SITTING GIVES {n}% LESS|Couch Potato,Cushion Lord,Sitter|Fidgeter,Pacer|Edgy,Twitchy|Couch
ACT_FUN|FUN|PLAY|GAIN|FUN THINGS GIVE {n}% MORE|FUN THINGS GIVE {n}% LESS|Hobbyist,Gamer,Player|Spoilsport,Sourpuss|Sour,Gloomed|Hobby
ACT_READ|MIND|LEARNING|GAIN|READING GIVES {n}% MORE|READING GIVES {n}% LESS|Bookworm,Page Turner,Scholar|Skimmer,Dozer|Dim,Vague|Reader
ACT_RUN|BODY|FITNESS|GAIN|EXERCISE GIVES {n}% MORE|EXERCISE GIVES {n}% LESS|Runner,Sprinter,Gym Rat|Slacker,Puffer|Puffy,Lazy|Workout
ACT_SWIM|OUTDOORS|WATER|GAIN|SWIMS GIVE {n}% MORE|SWIMS GIVE {n}% LESS|Fish,Paddler,Diver|Sinker,Dry Lander|Soggy,Sinking|Swimmer
ACT_TALK|SOCIAL|CHAT|GAIN|TALKING GIVES {n}% MORE|TALKING GIVES {n}% LESS|Talker,Natterer,Gossip|Mumbler,Nodder|Mumbly,Dry|Talker
ACT_PHONE|SOCIAL|FRIENDS|GAIN|PHONE CALLS GIVE {n}% MORE|PHONE CALLS GIVE {n}% LESS|Phone Pal,Dialler,Texter|Hang Up,Bad Line|Crackly,Dropped|Phone
ACT_COFFEE|FOOD|DRINKS|GAIN|COFFEE WAKES {n}% MORE|COFFEE WAKES {n}% LESS|Bean Lover,Barista,Caffeine Pro|Decaf,Cold Cup|Weak,Decaf|Coffee
SK_COOK|FOOD|COOKING|SKL|COOKING SKILL GROWS {n}% FASTER|COOKING SKILL GROWS {n}% SLOWER|Chef,Saucier,Pot Master|Burner,Kitchen Klutz|Burnt,Clumsy|Cook
SK_BODY|BODY|FITNESS|SKL|BODY SKILL GROWS {n}% FASTER|BODY SKILL GROWS {n}% SLOWER|Athlete,Trainer,Iron Frame|Couch Bound,Weakling|Weedy,Frail|Muscle
SK_FUN|FUN|PLAY|SKL|FUN SKILL GROWS {n}% FASTER|FUN SKILL GROWS {n}% SLOWER|Showman,Jester,Entertainer|Wallflower,Stiff One|Awkward,Wooden|Showtime
SK_LOGIC|MIND|LEARNING|SKL|LOGIC SKILL GROWS {n}% FASTER|LOGIC SKILL GROWS {n}% SLOWER|Brain,Thinker,Puzzler|Muddler,Fuzzy One|Fuzzy,Muddled|Genius
SK_CHAR|SOCIAL|CHARM|SKL|CHARISMA GROWS {n}% FASTER|CHARISMA GROWS {n}% SLOWER|Charmer,Smooth Talker,Silver Tongue|Blunt One,Bluffer|Blunt,Tactless|Charisma
SK_CREAT|CREATIVE|MAKING|SKL|CREATIVITY GROWS {n}% FASTER|CREATIVITY GROWS {n}% SLOWER|Artist,Maker,Muse|Copycat,Doodler|Blank,Copied|Creator
SK_MECH|HOME|FIXING|SKL|MECHANICAL SKILL GROWS {n}% FASTER|MECHANICAL SKILL GROWS {n}% SLOWER|Tinkerer,Fixer,Gearhead|Butterfingers,Thumbs|Fumbling,Greasy|Mechanic
SK_FARM|OUTDOORS|FARMING|SKL|FARMING SKILL GROWS {n}% FASTER|FARMING SKILL GROWS {n}% SLOWER|Green Thumb,Farmhand,Tiller|Brown Thumb,Weed Grower|Muddy,Wilted|Harvest
SK_MUSIC|CREATIVE|MUSIC|SKL|MUSIC SKILLS GROW {n}% FASTER|MUSIC SKILLS GROW {n}% SLOWER|Maestro,Strummer,Songbird|Tin Ear,Off Key|Tuneless,Flat|Music
MOOD_BASE|MIND|MOOD|ADD|MOOD IS {n} POINTS HIGHER|MOOD IS {n} POINTS LOWER|Sunny,Optimist,Bright Spark|Grouch,Gloom Cloud|Moody,Glum|Spirit
MOOD_PULL|MIND|MOOD|GAIN|MOOD SETTLES {n}% FASTER|MOOD SETTLES {n}% SLOWER|Bouncy,Resilient,Quick Mend|Brooder,Sulker|Brooding,Sulky|Bounce
ML_GOOD|MIND|MOOD|GAIN|GOOD MOODLETS ARE {n}% BIGGER|GOOD MOODLETS ARE {n}% SMALLER|Joy Sponge,Happy Soul,Cheerer|Numb One,Dull Heart|Numb,Blase|Glow
ML_BAD|MIND|NERVES|CUT|BAD MOODLETS ARE {n}% BIGGER|BAD MOODLETS ARE {n}% SMALLER|Thick Skin,Tough Nut,Rock|Sensitive,Soft Heart|Touchy,Tender|Armour
MOOD_MIN|MIND|NERVES|FLR|MOOD NEVER SINKS BELOW {m}||Steady,Anchor,Even Keel|||Floor
REL_GAIN|SOCIAL|FRIENDS|GAIN|FRIENDSHIPS GROW {n}% FASTER|FRIENDSHIPS GROW {n}% SLOWER|Friend Maker,Warm Heart,Buddy|Standoff,Distant One|Aloof,Distant|Bond
REL_LOSS|SOCIAL|FRIENDS|CUT|FALLING OUTS HURT {n}% MORE|FALLING OUTS HURT {n}% LESS|Forgiver,Peacemaker,Teflon|Grudge,Hothead|Prickly,Hot|Peace
CHAT_OK|SOCIAL|CHAT|CHN|CHATS WORK {n}% MORE OFTEN|CHATS WORK {n}% LESS OFTEN|Easy Talker,Good Listener,Ice Breaker|Blurter,Foot Mouth|Tactless,Rambling|Chat Power
JOKE_OK|SOCIAL|CHARM|CHN|JOKES LAND {n}% MORE OFTEN|JOKES LAND {n}% LESS OFTEN|Joker,Comedian,Punster|Groaner,Dad Joke|Corny,Cheesy|Humour
CHARM|SOCIAL|CHARM|LVL|CHARISMA ACTS {n} LEVELS HIGHER|CHARISMA ACTS {n} LEVELS LOWER|Magnetic,Natural,Star Quality|Dull One,Plain One|Plain,Dowdy|Aura
FLIRT_OK|ROMANCE|FLIRT|CHN|FLIRTS WORK {n}% MORE OFTEN|FLIRTS WORK {n}% LESS OFTEN|Flirt,Smooth Move,Heartbreaker|Clumsy Wink,Wink Fail|Bashful,Awkward|Flirt
ROMANCE|ROMANCE|LOVE|GAIN|ROMANCE GROWS {n}% FASTER|ROMANCE GROWS {n}% SLOWER|Romantic,Hopeful,Sweet Talker|Cold Heart,Frosty One|Frosty,Icy|Romance
HUG_OK|ROMANCE|LOVE|CHN|HUGS WORK {n}% MORE OFTEN|HUGS WORK {n}% LESS OFTEN|Cuddler,Hugger,Teddy|Stiff Hug,Prickle Hug|Stiff?,Starched|Hugs
LOVE_MOOD|ROMANCE|LOVE|ADD|MOOD IS {n} HIGHER WITH A PARTNER|MOOD IS {n} LOWER WITH A PARTNER|Smitten,Devoted,Soulmate|Needs Space,Jealous One|Jealous,Possessive|Devotion
REL_DECAY|ROMANCE|BONDS|CUT|FRIENDSHIPS FADE {n}% FASTER|FRIENDSHIPS FADE {n}% SLOWER|Loyal,True Blue,Steadfast|Drifter,Forgetful One|Fickle,Forgetful|Loyalty
ENGAGE|ROMANCE|BONDS|CHN|PROPOSALS WORK {n}% MORE OFTEN|PROPOSALS WORK {n}% LESS OFTEN|Proposer,Ring Bearer,Settler|Commitment Shy,Runner|Wary,Skittish|Vows
JOB_PAY|WORK|PAY|PAY|PAY IS {n}% HIGHER|PAY IS {n}% LOWER|Earner,Breadwinner,Bonus Hunter|Underpaid,Cheap Hire|Underpaid,Cheap|Wages
JOB_PERF|WORK|SHIFT|GAIN|WORKS {n}% HARDER|WORKS {n}% LESS|Grafter,Workhorse,Go Getter|Clock Watcher,Slacker|Slack,Idle|Effort
JOB_PROMO|WORK|CAREER|GAIN|PROMOTIONS COME {n}% SOONER|PROMOTIONS COME {n}% LATER|Climber,Ladder Star,Fast Tracker|Stuck One,Dead End|Stuck,Passed Over|Ladder
JOB_STRIKE|WORK|CAREER|CUT|BAD WORK DAYS COUNT {n}% MORE|BAD WORK DAYS COUNT {n}% LESS|Reliable,Punctual,Safe Hands|Late Bird,Shirker|Tardy,Sloppy|Job Shield
PAY_FACTORY|WORK|CAREERS|PAY|FACTORY PAY IS {n}% HIGHER|FACTORY PAY IS {n}% LOWER|Line Worker,Foreman,Riveter|Wrench Dropper,Part Timer|Rusty,Oily|Factory Boss
PAY_FOOD|WORK|CAREERS|PAY|KITCHEN PAY IS {n}% HIGHER|KITCHEN PAY IS {n}% LOWER|Cafe Star,Server,Short Order|Tray Dropper,Spiller|Spilly,Slow|Kitchen Boss
PAY_RETAIL|WORK|CAREERS|PAY|SHOP PAY IS {n}% HIGHER|SHOP PAY IS {n}% LOWER|Shopkeep,Till Pro,Stock Hero|Slow Till,Stock Faller|Short,Pricey|Shop Boss
PAY_FIT|WORK|CAREERS|PAY|GYM PAY IS {n}% HIGHER|GYM PAY IS {n}% LOWER|Gym Pro,Spotter,Pep Coach|Flab Coach,Sweaty Coach|Flabby,Sweaty|Gym Boss
PAY_SWIM|WORK|CAREERS|PAY|POOL PAY IS {n}% HIGHER|POOL PAY IS {n}% LOWER|Lifeguard,Pool Pro,Whistle Blower|Puddle Watch,Damp Duty|Soggy,Damp|Pool Boss
PAY_HOTEL|WORK|CAREERS|PAY|HOTEL PAY IS {n}% HIGHER|HOTEL PAY IS {n}% LOWER|Concierge,Bellhop,Host|Key Loser,Lost Bag|Rude,Lost|Hotel Boss
PAY_GAME|WORK|CAREERS|PAY|ARCADE JOB PAY IS {n}% HIGHER|ARCADE JOB PAY IS {n}% LOWER|Arcade Ace,Token Boss,Game Host|Glitcher,Lag Maker|Glitchy,Laggy|Arcade Boss
PAY_BIZ|WORK|CAREERS|PAY|OFFICE PAY IS {n}% HIGHER|OFFICE PAY IS {n}% LOWER|Suit,Exec,Dealmaker|Paper Jam,Memo Loser|Jammed,Buried|Office Boss
PAY_TEACH|WORK|CAREERS|PAY|TEACHING PAY IS {n}% HIGHER|TEACHING PAY IS {n}% LOWER|Teacher,Mentor,Tutor|Chalk Dust,Detention Fan|Dusty,Strict|Class Boss
PAY_NIGHT|WORK|CAREERS|PAY|NIGHT JOB PAY IS {n}% HIGHER|NIGHT JOB PAY IS {n}% LOWER|Night Shifter,Bouncer,Moon Worker|Day Sleeper,Zombie Worker|Groggy,Bleary|Night Boss
PAY_MED|WORK|CAREERS|PAY|MEDIC PAY IS {n}% HIGHER|MEDIC PAY IS {n}% LOWER|Medic,Healer,Nurse|Faint Heart,Squeamish One|Queasy,Squeamish|Medic Boss
PAY_FILM|WORK|CAREERS|PAY|FILM JOB PAY IS {n}% HIGHER|FILM JOB PAY IS {n}% LOWER|Film Star,Stuntman,Director|Blooper,Flubber|Flubbing,Muffed|Movie Boss
PAY_BEAUTY|WORK|CAREERS|PAY|SALON PAY IS {n}% HIGHER|SALON PAY IS {n}% LOWER|Stylist,Barber,Glam Pro|Bad Cut,Frizz Maker|Frizzy,Messy|Salon Boss
PAY_ART|WORK|CAREERS|PAY|STUDIO PAY IS {n}% HIGHER|STUDIO PAY IS {n}% LOWER|Painter,Curator,Gallery Pro|Smudge,Blot Maker|Smudgy,Blotchy|Studio Boss
PAY_FARM|WORK|CAREERS|PAY|FARM PAY IS {n}% HIGHER|FARM PAY IS {n}% LOWER|Farmer,Harvester,Rancher|Scarecrow,Weed Patch|Weedy,Dusty|Farm Boss
INCOME|MONEY|INCOME|INC|ALL EARNINGS ARE {n}% HIGHER|ALL EARNINGS ARE {n}% LOWER|Money Magnet,Hustler,Rainmaker|Leaky Purse,Pocket Hole|Broke,Leaky|Money
SPEND|MONEY|SPENDING|CUT|EVERYTHING COSTS {n}% MORE|EVERYTHING COSTS {n}% LESS|Skinflint,Frugal,Thrifty|Spendthrift,Big Spender|Splashy,Lavish|Thrift
SHOP_DISC|MONEY|SHOPPING|CUT|SHOP PRICES ARE {n}% HIGHER|SHOP PRICES ARE {n}% LOWER|Haggler,Bargain Hunter,Coupon Clipper|Easy Mark,Sucker|Gullible,Soft Touch|Discount
FARE|MONEY|SPENDING|CUT|TRIPS COST {n}% MORE|TRIPS COST {n}% LESS|Bus Hero,Pass Holder,Cheap Rider|Taxi Addict,Fare Payer|Posh,Fancy|Ticket
WALK_SPEED|OUTDOORS|WALKING|SPD|WALKS {n}% FASTER|WALKS {n}% SLOWER|Brisk Walker,Hiker,Trekker|Dawdler,Slowpoke|Slow,Plodding|Stride
VEH_SPEED|OUTDOORS|VEHICLES|SPD|RIDES {n}% FASTER|RIDES {n}% SLOWER|Speedster,Road Racer,Pedal Pusher|Crawler,Granny Gear|Sluggish,Creaky|Turbo
FUEL_USE|OUTDOORS|VEHICLES|CUT|BURNS {n}% MORE FUEL|BURNS {n}% LESS FUEL|Eco Driver,Fuel Sipper,Smooth Cruiser|Gas Guzzler,Lead Foot|Thirsty,Heavy|Fuel Tank
NIGHT_REST|NIGHT AND DAY|NIGHT OWL|DEC|GETS TIRED {n}% FASTER AT NIGHT|GETS TIRED {n}% SLOWER AT NIGHT|Night Owl,Moon Walker,Star Gazer|Night Nodder,Dusk Dozer|Dozy,Droopy|Moonlight
DAY_REST|NIGHT AND DAY|EARLY BIRD|DEC|GETS TIRED {n}% FASTER BY DAY|GETS TIRED {n}% SLOWER BY DAY|Early Bird,Sun Chaser,Lark|Noon Napper,Day Dozer|Sluggish,Foggy|Sunrise
NIGHT_MOOD|NIGHT AND DAY|NIGHT OWL|ADD|MOOD IS {n} HIGHER AT NIGHT|MOOD IS {n} LOWER AT NIGHT|Night Lover,Dusk Dancer,Twilight|Moon Moper,Dark Brooder|Spooked,Wary|Nightfall
DAY_MOOD|NIGHT AND DAY|EARLY BIRD|ADD|MOOD IS {n} HIGHER BY DAY|MOOD IS {n} LOWER BY DAY|Day Lover,Sun Seeker,Noon Bright|Day Hater,Sun Sulker|Pale,Shaded|Daylight
COLD_TOL|OUTDOORS|WEATHER|ADD|MOOD IS {n} HIGHER IN WINTER COLD|MOOD IS {n} LOWER IN WINTER COLD|Frost Fan,Snow Bunny,Polar|Shiverer,Cold Feet|Chilly,Shivery|Frost
HEAT_TOL|OUTDOORS|WEATHER|ADD|MOOD IS {n} HIGHER IN SUMMER HEAT|MOOD IS {n} LOWER IN SUMMER HEAT|Sun Worshipper,Beach Bum,Heat Lover|Melter,Sweater|Sweaty,Wilting|Sunshine
RAIN_TOL|OUTDOORS|WEATHER|ADD|MOOD IS {n} HIGHER IN RAIN OR SNOW|MOOD IS {n} LOWER IN RAIN OR SNOW|Puddle Jumper,Storm Fan,Rain Dancer|Drizzle Hater,Wet Cat|Drenched,Damp|Storm
CROWD|SOCIAL|CROWDS|ADD|MOOD IS {n} HIGHER IN A CROWD|MOOD IS {n} LOWER IN A CROWD|Crowd Lover,Party Animal,Life Of Party|Crowd Hater,Shrinker|Jumpy,Edgy|Crowd
ALONE|SOCIAL|CROWDS|ADD|MOOD IS {n} HIGHER WHEN ALONE|MOOD IS {n} LOWER WHEN ALONE|Lone Wolf,Quiet One,Peace Seeker|Lonesome,Third Wheel|Wistful,Hollow|Solitude
FEAR_WATER|OUTDOORS|WATER|ADD|MOOD IS {n} HIGHER NEAR WATER|MOOD IS {n} LOWER NEAR WATER|Water Baby,Wave Rider,Sea Lover|Water Fear,Puddle Fear|Soaked,Waterlogged|Splash
FEAR_DARK|NIGHT AND DAY|DARKNESS|ADD|MOOD IS {n} HIGHER OUTSIDE AT NIGHT|MOOD IS {n} LOWER OUTSIDE AT NIGHT|Fearless,Dark Walker,Bat|Dark Fear,Nightlight|Jumpy?,Timid|Darkness
FEAR_HEIGHT|HOME|STAIRS|ADD|MOOD IS {n} HIGHER UPSTAIRS|MOOD IS {n} LOWER UPSTAIRS|Sky Lover,Loft Dweller,Tower Fan|Height Fear,Ground Hugger|Dizzy,Giddy|Heights
INDOORS|HOME|ROOMS|ADD|MOOD IS {n} HIGHER INDOORS|MOOD IS {n} LOWER INDOORS|Homebody,Hearth Keeper,Nest Lover|Cabin Fever,Wall Climber|Cooped,Stuffy|Hearth
OUTDOORS|OUTDOORS|WALKING|ADD|MOOD IS {n} HIGHER OUTDOORS|MOOD IS {n} LOWER OUTDOORS|Outdoorsy,Explorer,Rambler|Indoor Cat,Cave Dweller|Pasty,Pale Skin|Wilds
PET_FOOD|PETS|PETS|CUT|PET GETS HUNGRY {n}% FASTER|PET GETS HUNGRY {n}% SLOWER|Pet Feeder,Dog Whisperer,Treat Giver|Forgetful Owner,Empty Bowl|Forgetful,Absent|Kibble
PET_JOY|PETS|PETS|ADD|MOOD IS {n} HIGHER WITH A PET|MOOD IS {n} LOWER WITH A PET|Animal Friend,Pet Lover,Fur Fan|Allergic One,Sneezer|Sneezy,Itchy|Furry
GOTCHI_XP|PETS|BOREAGOTCHI|GAIN|BOREAGOTCHI CARE GIVES {n}% MORE XP|BOREAGOTCHI CARE GIVES {n}% LESS XP|Egg Whisperer,Pixel Parent,Toy Tamer|Button Masher,Forgetter|Fumbling,Forgetful|Hatcher
GOTCHI_GIFT|PETS|BOREAGOTCHI|GAIN|BOREAGOTCHI GIFTS PAY {n}% MORE|BOREAGOTCHI GIFTS PAY {n}% LESS|Gift Magnet,Treasure Pet,Lucky Hatch|Stingy Pet,Thrifty Pet|Stingy,Stubborn|Treasure
ARC_SCORE|FUN|ARCADE|GAIN|ARCADE SCORES ARE {n}% HIGHER|ARCADE SCORES ARE {n}% LOWER|High Scorer,Arcade Ace,Combo King|Button Fumbler,Lag Victim|Laggy,Fumbly|Score
ARC_TIX|FUN|ARCADE|GAIN|ARCADE WINS PAY {n}% MORE TICKETS|ARCADE WINS PAY {n}% FEWER TICKETS|Ticket Taker,Prize Hunter,Token Shark|Ticket Loser,Short Change|Unlucky,Short|Tickets
FIGHT_DMG|FIGHTING|STRIKE|GAIN|HITS HURT {n}% MORE|HITS HURT {n}% LESS|Brawler,Puncher,Heavy Hand|Wimp Hit,Soft Fist|Soft,Slow|Fist
FIGHT_DEF|FIGHTING|GUARD|CUT|HITS ON YOU HURT {n}% MORE|HITS ON YOU HURT {n}% LESS|Tough Guy,Iron Chin,Stone Wall|Glass Jaw,Soft Belly|Glassy,Squishy|Shield
FIGHT_REGEN|FIGHTING|GRIT|GAIN|HEALS {n}% FASTER|HEALS {n}% SLOWER|Quick Mender,Regenerator,Bounce Back|Slow Healer,Bruiser|Bruised,Sore|Recovery
FIGHT_BELT|FIGHTING|STRIKE|LVL|FIGHTS {n} BELTS HIGHER|FIGHTS {n} BELTS LOWER|Black Belt,Dojo Star,Sensei|White Belt,Beginner|Green,Raw|Belt
LUCK|LUCK|LUCK|CHN|LUCKY BREAKS COME {n}% MORE OFTEN|LUCKY BREAKS COME {n}% LESS OFTEN|Lucky Charm,Four Leaf,Fortunate|Jinx,Black Cat|Jinxed,Unlucky|Fortune
LOOT|LUCK|FINDS|GAIN|FOUND MONEY IS {n}% BIGGER|FOUND MONEY IS {n}% SMALLER|Finder,Coin Spotter,Gold Eye|Hole In Pocket,Dropper|Careless,Holey|Treasure Eye
SPARK|LUCK|FINDS|CHN|INSPIRATION STRIKES {n}% MORE OFTEN|INSPIRATION STRIKES {n}% LESS OFTEN|Inspired,Muse Touched,Spark|Blank Page,Dry Well|Blank,Dry|Spark
AGE_SPEED|BODY|AGING|CUT|GROWS OLD {n}% FASTER|GROWS OLD {n}% SLOWER|Slow Ager,Young At Heart,Evergreen|Fast Ager,Wrinkler|Worn,Weathered|Youth
GRADE|MIND|SCHOOL|GAIN|SCHOOL GRADES RISE {n}% FASTER|SCHOOL GRADES RISE {n}% SLOWER|Teacher Pet,Straight A,Swot|Daydreamer,Doodle King|Dreamy,Distracted|Grades
ASP_GAIN|MIND|FOCUS|GAIN|WANTS PAY {n}% MORE|WANTS PAY {n}% LESS|Driven,Ambitious,Go For It|Aimless One,Drifter|Aimless,Drifting|Drive
ASP_KEEP|MIND|FOCUS|CUT|ASPIRATION FADES {n}% FASTER|ASPIRATION FADES {n}% SLOWER|Steady Flame,Pilot Light,Eternal Spark|Fizzle,Flash In Pan|Fading,Flickery|Flame
"""

SUBS = {}      # category -> ordered sub-group names
HOOKS = []     # dicts
def parse_hooks():
    for ln in HOOKS_TXT.strip().splitlines():
        f = ln.split('|')
        assert len(f) == 10, ln
        code, cat, sub, style, up, dn, gn, bn, fa, cn = f
        cat = cat.strip()
        assert cat in CATS, cat
        mode, unit, good, reg, ch = STYLES[style]
        h = dict(code=code, cat=CATS.index(cat), sub=sub, style=style, mode=mode, unit=unit, good=good, reg=reg, cheat=ch,
                 up=up, dn=dn, gn=[x.strip() for x in gn.split(',') if x.strip()], bn=[x.strip() for x in bn.split(',') if x.strip()],
                 fa=[x.strip().replace('?', '') for x in fa.split(',') if x.strip()], cn=cn.strip(), id=len(HOOKS) + 1)
        for k in range(len(h['gn'])): h['gn'][k] = h['gn'][k].replace('?', '')
        HOOKS.append(h)
        SUBS.setdefault(h['cat'], [])
        if sub not in SUBS[h['cat']]: SUBS[h['cat']].append(sub)
parse_hooks()
HBY = {h['code']: h for h in HOOKS}
NH = len(HOOKS) + 1   # E_N (0 is E_NONE)

# ---------------------------------------------------------------- game events (counters) the wishes and the earned traits count
# code, text. 'ev' = counted where it happens (end_action and friends); 'poll' = read off the game's own numbers now and then (traits.c tr_poll)
EVENTS = [
 ('COOKED', 'MEALS COOKED', 'ev'), ('ATEOUT', 'MEALS EATEN OUT', 'ev'), ('BAKED', 'CAKES BAKED', 'ev'), ('SNACKS', 'SNACKS EATEN', 'ev'),
 ('SLEPT', 'GOOD SLEEPS', 'ev'), ('SHOWERS', 'SHOWERS TAKEN', 'ev'), ('FUNS', 'FUN ACTIVITIES', 'ev'), ('FILMS', 'FILMS SEEN', 'ev'),
 ('BOOKS', 'BOOKS READ', 'ev'), ('COFFEES', 'COFFEES DRUNK', 'ev'), ('RUNS', 'WORKOUTS', 'ev'), ('SWIMS', 'SWIMS', 'ev'),
 ('CALLS', 'PHONE CALLS', 'ev'), ('PIZZAS', 'PIZZAS ORDERED', 'ev'), ('GATHERS', 'THINGS GATHERED', 'ev'), ('CHATS', 'CHATS', 'ev'),
 ('CARES', 'BABIES LOOKED AFTER', 'ev'), ('JOKES', 'JOKES TOLD', 'ev'), ('HUGS', 'HUGS GIVEN', 'ev'), ('FLIRTS', 'FLIRTS THAT WORKED', 'ev'),
 ('KIND', 'KIND WORDS', 'ev'), ('PAYDAYS', 'PAYDAYS', 'poll'), ('PROMOS', 'PROMOTIONS', 'ev'), ('HOMEWORK', 'HOMEWORK DONE', 'ev'),
 ('CAKES', 'BIRTHDAY CAKES EATEN', 'ev'), ('LUCKY', 'LUCKY BREAKS', 'ev'),
 ('DAYS', 'DAYS LIVED (5 A STEP)', 'poll'), ('EARNED', 'MONEY EARNED ($500 A STEP)', 'poll'), ('CASH', 'MONEY IN HAND ($1000 A STEP)', 'poll'),
 ('JOBLV', 'JOB LEVEL', 'poll'), ('FRIENDS', 'FRIENDS', 'poll'), ('FAMILY', 'PEOPLE IN THE FAMILY', 'poll'), ('DISTRICTS', 'PLACES VISITED', 'poll'),
 ('FIGHTS', 'ARCADE FIGHTS WON', 'poll'), ('CHAMP', 'FIGHT CHAMPION', 'poll'), ('ARCPLAYS', 'ARCADE GAMES PLAYED', 'poll'),
 ('TIX', 'ARCADE TICKETS (10 A STEP)', 'poll'), ('PARTIES', 'PARTIES THROWN', 'poll'), ('HOME', 'HOMES OWNED', 'poll'),
 ('COLLEGE', 'COLLEGE LEVEL', 'poll'), ('SPACE', 'SPACE MISSIONS', 'poll'), ('VEHICLES', 'VEHICLES OWNED', 'poll'),
 ('PARTNER', 'HAS A PARTNER', 'poll'), ('HATCHED', 'BOREAGOTCHI HATCHED', 'poll'), ('GELDER', 'BOREAGOTCHI GROWN OLD', 'poll'),
 ('PET', 'HAS A PET', 'poll'), ('BANDLV', 'BEST MUSIC SKILL', 'poll'),
 ('SK_COOK', 'COOKING LEVEL', 'poll'), ('SK_BODY', 'BODY LEVEL', 'poll'), ('SK_FUN', 'FUN LEVEL', 'poll'), ('SK_LOGIC', 'LOGIC LEVEL', 'poll'),
 ('SK_CHAR', 'CHARISMA LEVEL', 'poll'), ('SK_CREAT', 'CREATIVITY LEVEL', 'poll'), ('SK_MECH', 'MECHANICAL LEVEL', 'poll'), ('SK_FARM', 'FARMING LEVEL', 'poll'),
]
EVN = {e[0]: i + 1 for i, e in enumerate(EVENTS)}   # EV_x = index (0 is EV_NIL)

THEMES = "PICKONE NOTHINGON PIRATE ASTRONAUT CHEF SKATER ROCKER WIZARD KNIGHT COWBOY DIVER FAIRY CLOWN SUPERHERO DETECTIVE SAILOR ATHLETE VIKING BEACHDAY SNOWDAY PARTYANIMAL SPOOKY SANTA EXPLORER ROYALTY GAMER ROCKSTAR GARDENER BUILDER DOCTOR POLICE SCHOLAR HIPSTER SAMURAI DJ BAKER CAMPER MAGICIAN ORANGESWIRL BLUESWIRL ARCADEKID JUMPPACKHERO".split()
def theme(n): return THEMES.index(n)

# ---------------------------------------------------------------- EARNED traits: name | event | count | effects (HOOK+tiers)
EARNED_TXT = """
Home Cook|COOKED|25|SK_COOK+2,ACT_EAT+2
Kitchen Hero|COOKED|100|SK_COOK+4,GAIN_FOOD+3
Regular Diner|ATEOUT|25|ACT_EAT+2,ML_GOOD+2
Food Critic|ATEOUT|80|SHOP_DISC-3,ACT_EAT+3
Cake Boss|BAKED|10|SPARK+3,ACT_EAT+2
Snack Lord|SNACKS|40|DEC_FOOD-3,GAIN_FOOD+2
Sound Sleeper|SLEPT|30|GAIN_REST+3,DEC_REST-2
Dream Weaver|SLEPT|90|ACT_SLEEP+4,MOOD_BASE+1
Squeaky Clean|SHOWERS|40|GAIN_CLEAN+3,DEC_CLEAN-2
Fun Machine|FUNS|60|GAIN_FUN+3,DEC_FUN-2
Film Buff|FILMS|15|ACT_SIT+3,MOOD_BASE+1
Bookworm Prime|BOOKS|40|ACT_READ+4,SK_LOGIC+2
Caffeine Crew|COFFEES|30|ACT_COFFEE+4,DEC_REST-2
Gym Regular|RUNS|40|ACT_RUN+3,SK_BODY+2
Iron Lungs|RUNS|120|SK_BODY+3,DEC_REST-3,FIGHT_REGEN+2
Water Rat|SWIMS|25|ACT_SWIM+4,FEAR_WATER+2
Phone Addict|CALLS|40|ACT_PHONE+4,REL_GAIN+1
Pizza Pro|PIZZAS|8|DEC_FOOD-2,ML_GOOD+2
Forager|GATHERS|60|LOOT+3,SK_FARM+2
Chatty Cathy|CHATS|60|ACT_TALK+3,CHAT_OK+2
Social Star|CHATS|200|REL_GAIN+3,CHARM+1
Baby Whisperer|CARES|30|ML_GOOD+2,MOOD_PULL+2
Class Clown|JOKES|25|JOKE_OK+4,SK_FUN+2
Hug Machine|HUGS|25|HUG_OK+4,ROMANCE+2
Smooth Mover|FLIRTS|20|FLIRT_OK+4,ROMANCE+2
Sweet Talker|KIND|40|CHAT_OK+3,REL_LOSS-2
Steady Earner|PAYDAYS|30|JOB_PAY+2,JOB_STRIKE-3
Career Climber|PROMOS|3|JOB_PROMO+4,JOB_PERF+2
Honour Roll|HOMEWORK|20|GRADE+4,SK_LOGIC+2
Cake Lover|CAKES|5|ML_GOOD+2,MOOD_BASE+1
Lucky Streak|LUCKY|25|LUCK+3,LOOT+2
Long Haul|DAYS|20|AGE_SPEED-2,MOOD_PULL+2
Veteran|DAYS|60|AGE_SPEED-3,ML_BAD-3
Money Maker|EARNED|20|INCOME+2,SPEND-2
Tycoon|EARNED|100|INCOME+3,SHOP_DISC-3
Nest Egg|CASH|20|SPEND-3,MOOD_BASE+1
Well Liked|FRIENDS|5|REL_GAIN+3,CROWD+1
Big Family|FAMILY|5|LOVE_MOOD+2,DEC_SOCIAL-3
Globe Trotter|DISTRICTS|4|WALK_SPEED+3,FARE-3
Ring Master|FIGHTS|8|FIGHT_DMG+3,FIGHT_DEF-2
Arcade Regular|ARCPLAYS|30|ARC_SCORE+3,ARC_TIX+3
Party Host|PARTIES|3|CROWD+2,GAIN_SOCIAL+2
Home Owner|HOME|1|DEC_COMFY-3,INDOORS+2
Scholar Mind|COLLEGE|2|SK_LOGIC+3,GRADE+3
Space Cadet|SPACE|3|SK_MECH+3,MOOD_BASE+1
Road Warrior|VEHICLES|3|VEH_SPEED+3,FUEL_USE-3
Soulmate Found|PARTNER|1|LOVE_MOOD+3,MOOD_BASE+1
Egg Keeper|HATCHED|3|GOTCHI_XP+4,PET_JOY+2
Elder Keeper|GELDER|1|GOTCHI_GIFT+4,GOTCHI_XP+2
Pet Parent|PET|1|PET_JOY+3,PET_FOOD-2
Band Veteran|BANDLV|6|SK_MUSIC+3,SPARK+2
"""
# REWARD traits (wish rewards): name | effects.  Used by wishes with reward kind TRAIT.
REWARD_TXT = """
Cash King|INCOME+3,SPEND-2
Ladder Legend|JOB_PROMO+5,JOB_PAY+3
Landlord Pride|DEC_COMFY-4,SHOP_DISC-3
Family Glow|DEC_SOCIAL-4,LOVE_MOOD+3
Champ Spirit|FIGHT_DMG+5,FIGHT_DEF-3
Master Chef|SK_COOK+4,ACT_EAT+4
Iron Body|SK_BODY+4,DEC_REST-4
Big Brain|SK_LOGIC+4,GRADE+4
Social Glue|REL_GAIN+4,CHAT_OK+3
Maker Mind|SK_CREAT+4,SPARK+4
Green Fingers|SK_FARM+4,LOOT+3
Wheel Wizard|VEH_SPEED+5,FUEL_USE-4
Game Legend|ARC_SCORE+5,ARC_TIX+4
Pet Hero|PET_JOY+4,GOTCHI_XP+4
World Walker|WALK_SPEED+4,OUTDOORS+3
Heart Winner|ROMANCE+5,FLIRT_OK+4
Party Soul|CROWD+4,ML_GOOD+3
Rich Life|INCOME+4,SHOP_DISC-4
Lucky Soul|LUCK+5,LOOT+4
Calm Mind|MOOD_MIN+2,ML_BAD-4
Star Power|SK_FUN+4,JOKE_OK+4
Sound Body|DEC_FOOD-4,DEC_CLEAN-4
Night Hero|NIGHT_REST-4,NIGHT_MOOD+3
Wise Elder|AGE_SPEED-4,MOOD_BASE+2
"""

# ---------------------------------------------------------------- LIFETIME WISHES: id, name, description, category, event, count, reward
# reward kinds: CASH n | TRAIT (the next reward trait) | OUTFIT theme | FOOD n | BUS days | TOKENS n | ASP n (aspiration points)
WISH_TXT = """
SWIMMING IN CASH|HAVE $10000 IN HAND|MONEY|CASH|10|CASH 2000
CLIMB THE LADDER|REACH JOB LEVEL 3|WORK|JOBLV|3|CASH 2000
LANDLORD|OWN A HOME OF YOUR OWN|HOME|HOME|1|CASH 2000
BIG FAMILY|HAVE 6 PEOPLE IN THE FAMILY|SOCIAL|FAMILY|6|CASH 2000
FIGHT CHAMPION|BECOME THE FIGHT CHAMPION|FIGHTING|CHAMP|1|CASH 2000
HOME COOKING|COOK 10 MEALS|FOOD|COOKED|10|FOOD 6
MASTER CHEF|REACH COOKING LEVEL 8|FOOD|SK_COOK|8|TRAIT
DINING OUT|EAT OUT 15 TIMES|FOOD|ATEOUT|15|CASH 600
SWEET TOOTH|BAKE 8 CAKES|FOOD|BAKED|8|OUTFIT BAKER
PIZZA NIGHTS|ORDER 6 PIZZAS|FOOD|PIZZAS|6|FOOD 4
COFFEE LOVER|DRINK 25 COFFEES|FOOD|COFFEES|25|CASH 500
GYM RAT|WORK OUT 30 TIMES|BODY|RUNS|30|OUTFIT ATHLETE
IRON BODY|REACH BODY LEVEL 8|BODY|SK_BODY|8|TRAIT
POOL PARTY|GO SWIMMING 20 TIMES|OUTDOORS|SWIMS|20|OUTFIT DIVER
SOUND SLEEPER|HAVE 30 GOOD SLEEPS|BODY|SLEPT|30|CASH 500
FRESH AS A DAISY|TAKE 40 SHOWERS|BODY|SHOWERS|40|CASH 500
BOOKWORM|READ 30 BOOKS|MIND|BOOKS|30|OUTFIT SCHOLAR
BIG BRAIN|REACH LOGIC LEVEL 8|MIND|SK_LOGIC|8|TRAIT
HONOUR ROLL|DO 20 HOMEWORKS|MIND|HOMEWORK|20|CASH 800
COLLEGE GRAD|REACH COLLEGE LEVEL 2|MIND|COLLEGE|2|CASH 3000
SPACE CADET|FINISH 3 SPACE MISSIONS|MIND|SPACE|3|OUTFIT ASTRONAUT
FRIENDLY FACE|MAKE 5 FRIENDS|SOCIAL|FRIENDS|5|CASH 1000
CHATTERBOX|HAVE 60 CHATS|SOCIAL|CHATS|60|ASP 1500
PHONE PAL|MAKE 25 PHONE CALLS|SOCIAL|CALLS|25|BUS 5
COMEDY NIGHT|TELL 20 JOKES|SOCIAL|JOKES|20|TRAIT
HUG LIFE|GIVE 20 HUGS|SOCIAL|HUGS|20|ASP 1500
KIND SOUL|SAY 40 KIND WORDS|SOCIAL|KIND|40|CASH 600
CHARM SCHOOL|REACH CHARISMA LEVEL 8|SOCIAL|SK_CHAR|8|TRAIT
PARTY STARTER|THROW 3 PARTIES|FUN|PARTIES|3|OUTFIT PARTYANIMAL
FIRST LOVE|HAVE A PARTNER|ROMANCE|PARTNER|1|CASH 1500
SMOOTH OPERATOR|LAND 15 FLIRTS|ROMANCE|FLIRTS|15|TRAIT
FIRST PAYDAY|GET PAID 1 TIME|WORK|PAYDAYS|1|CASH 300
WORKING LIFE|GET PAID 25 TIMES|WORK|PAYDAYS|25|CASH 1500
PROMOTED|WIN 2 PROMOTIONS|WORK|PROMOS|2|CASH 1000
TOP OF THE TREE|REACH JOB LEVEL 5|WORK|JOBLV|5|TRAIT
PAYCHECK PRO|EARN $20000 IN ALL|MONEY|EARNED|40|CASH 1000
FIFTY GRAND|EARN $50000 IN ALL|MONEY|EARNED|100|TRAIT
DEEP POCKETS|HAVE $50000 IN HAND|MONEY|CASH|50|OUTFIT ROYALTY
LANDED GENTRY|OWN A HOME AND ONE MORE|HOME|HOME|2|CASH 2500
FAMILY MAN|HAVE 4 PEOPLE IN THE FAMILY|HOME|FAMILY|4|CASH 800
BIRTHDAY BASH|EAT 6 BIRTHDAY CAKES|HOME|CAKES|6|FOOD 5
FIXER UPPER|REACH MECHANICAL LEVEL 8|HOME|SK_MECH|8|CASH 1200
ON THE ROAD|OWN 2 VEHICLES|OUTDOORS|VEHICLES|2|BUS 10
SEE IT ALL|VISIT EVERY DISTRICT|OUTDOORS|DISTRICTS|4|TRAIT
FORAGER|GATHER 40 THINGS|OUTDOORS|GATHERS|40|CASH 700
GREEN THUMB|REACH FARMING LEVEL 8|OUTDOORS|SK_FARM|8|OUTFIT GARDENER
ARTIST AT HEART|REACH CREATIVITY LEVEL 8|CREATIVE|SK_CREAT|8|TRAIT
ROCK ON|REACH MUSIC LEVEL 6|CREATIVE|BANDLV|6|OUTFIT ROCKSTAR
ARCADE ACE|PLAY 30 ARCADE GAMES|FUN|ARCPLAYS|30|TOKENS 20
TICKET TYCOON|COLLECT 500 ARCADE TICKETS|FUN|TIX|50|TRAIT
MOVIE BUFF|SEE 12 FILMS|FUN|FILMS|12|CASH 600
FUN TIMES|DO 50 FUN THINGS|FUN|FUNS|50|TRAIT
SHOWTIME|REACH FUN LEVEL 8|FUN|SK_FUN|8|OUTFIT CLOWN
PET LOVER|HAVE A PET|PETS|PET|1|FOOD 4
EGG HEAD|HATCH 3 BOREAGOTCHI|PETS|HATCHED|3|TRAIT
ELDER CARE|GROW A BOREAGOTCHI TO ELDER|PETS|GELDER|1|TRAIT
BABY CARER|LOOK AFTER BABIES 25 TIMES|PETS|CARES|25|CASH 800
SURVIVOR|LIVE 50 DAYS|NIGHT AND DAY|DAYS|10|CASH 800
OLD TIMER|LIVE 150 DAYS|NIGHT AND DAY|DAYS|30|TRAIT
ARENA WINNER|WIN 10 ARCADE FIGHTS|FIGHTING|FIGHTS|10|TRAIT
LUCKY DAYS|HAVE 30 LUCKY BREAKS|LUCK|LUCKY|30|TRAIT
"""

# ---------------------------------------------------------------- composing the regular traits
TIERW = [2, 3, 4, 6]
ADJ_T1 = ['Mild', 'Light', 'Gentle', 'Quiet', 'Soft', 'Small', 'Slight', 'Easy']
ADJ_T2 = ['Steady', 'Solid', 'Sound', 'Trusty', 'Fair', 'Sturdy', 'Proper', 'Able']
ADJ_T3 = ['Strong', 'Bold', 'Keen', 'Prime', 'Deft', 'Fierce', 'Brave', 'Sharp']
ADJ_T4 = ['Mighty', 'Grand', 'Peak', 'Epic', 'Born', 'Master', 'Royal', 'Noble']
ADJ_SYN = ['Total', 'Whole', 'Full', 'Prime', 'Rounded', 'Twin', 'Dual', 'Blend']
ADJ_QUIRK = ['Happy', 'Proud', 'Cheery', 'Jolly', 'Merry', 'Daft', 'Loud', 'Brave', 'Wild', 'Lucky', 'Sly', 'Bold']
ADJ_C = [['Super', 'Turbo', 'Hyper'], ['Mega', 'Ultra', 'Grand'], ['God', 'Infinite', 'Absolute']]
NAME_MAX = 14
NEUTRAL = ['Red', 'Blue', 'Green', 'Pink', 'Gold', 'Silver', 'Odd', 'Old', 'New', 'Big', 'Tiny', 'Busy', 'Sunny', 'Rainy', 'Windy', 'Snowy', 'Dusty', 'Salty', 'Sweet', 'Spicy', 'Crisp', 'Plain', 'Fancy', 'Round', 'Flat', 'Tall', 'Short', 'Calm', 'Cosy', 'Cool', 'Warm', 'Hot', 'Wet', 'Dry', 'Lone', 'Wee', 'Neat', 'Proud', 'Humble', 'Silent', 'Noisy', 'Mellow', 'Spry', 'Zesty', 'Lively', 'Nifty', 'Handy', 'Jazzy', 'Peppy', 'Breezy']

def mag(h, tier, sign=+1):
    """tier 1..4 -> magnitude, sign +1 boost (the good direction), -1 cost (the bad direction)"""
    m = h['reg'][tier - 1]
    return m * h['good'] * sign

def eff_text(h, m):
    n = abs(m) * h['unit']
    if h['mode'] == M_FLR:
        return h['up'].replace('{m}', str(-100 + 10 * m))
    t = h['up'] if m > 0 else h['dn']
    return t.replace('{n}', str(n))

def is_good(h, m):
    return (m > 0) == (h['good'] > 0)

class Gen:
    def __init__(self):
        self.names = set()
        self.traits = []     # dicts: id, name, cat, sub, kind, eff[(hook_id, mag)]
    def uniq(self, cands):
        for c in cands:
            if len(c) <= NAME_MAX and c.upper() not in self.names:
                self.names.add(c.upper()); return c
        # last resort: shorten + index
        base = cands[0][:NAME_MAX - 2]
        for i in range(2, 90):
            c = '%s %d' % (base, i)
            if c.upper() not in self.names:
                self.names.add(c.upper()); return c
        raise SystemExit('no name for ' + cands[0])

G = Gen()

def make_regular(target=600):
    rng = random.Random(20241108)
    hooks = HOOKS
    nh = len(hooks)
    per = [target // nh + (1 if i < target % nh else 0) for i in range(nh)]
    # (a few hooks get 5, most 6, until the total is 600)
    out = []
    def cost_for(h, tier):
        # a cost: the bad direction of some other hook that has a flaw word, outside this hook's category if we can
        cands = [c for c in hooks if c['fa'] and c['code'] != h['code'] and c['cat'] != h['cat'] and c['code'] not in ('MOOD_MIN',)]
        c = rng.choice(cands)
        return c, mag(c, tier, -1)
    def side_for(h, tier):
        cands = [c for c in hooks if c['cat'] == h['cat'] and c['code'] != h['code']]
        if not cands: cands = [c for c in hooks if c['code'] != h['code']]
        c = rng.choice(cands)
        return c, mag(c, tier, +1)
    for i, h in enumerate(hooks):
        variants = ['p1', 'c2', 'c3', 'q', 'd', 'x'][:]
        # hooks without a bad direction (a floor) cannot be a cost or a quirk
        k = per[i]
        order = ['p1', 'c2', 'd', 'c3', 'q', 'x']
        if not h['bn']: order = ['p1', 'c2', 'd', 'c3', 'x', 'p2']
        for v in order[:k]:
            ef = []
            if v == 'p1':
                ef = [(h, mag(h, 1))]; nm = [a + ' ' + h['gn'][0] for a in rng.sample(ADJ_T1, 4)]
            elif v == 'p2':
                ef = [(h, mag(h, 3))]; nm = [a + ' ' + h['gn'][0] for a in rng.sample(ADJ_T3, 4)]
            elif v == 'c2':
                c, cm = cost_for(h, 1); ef = [(h, mag(h, 2)), (c, cm)]
                nm = [a + ' ' + h['gn'][1 % len(h['gn'])] for a in c['fa']] + [a + ' ' + h['gn'][1 % len(h['gn'])] for a in rng.sample(ADJ_T2, 3)]
            elif v == 'c3':
                c, cm = cost_for(h, 2); ef = [(h, mag(h, 3)), (c, cm)]
                nm = [a + ' ' + h['gn'][2 % len(h['gn'])] for a in c['fa']] + [a + ' ' + h['gn'][2 % len(h['gn'])] for a in rng.sample(ADJ_T3, 3)]
            elif v == 'x':
                c, cm = cost_for(h, 3); ef = [(h, mag(h, 4)), (c, cm)]
                nm = [a + ' ' + h['gn'][0] for a in reversed(c['fa'])] + [a + ' ' + h['gn'][0] for a in rng.sample(ADJ_T4, 4)]
            elif v == 'd':
                s, sm = side_for(h, 1); ef = [(h, mag(h, 2)), (s, sm)]
                nm = [a + ' ' + h['gn'][0] for a in rng.sample(ADJ_SYN, 4)] + [a + ' ' + h['gn'][1 % len(h['gn'])] for a in rng.sample(ADJ_SYN, 4)]
            elif v == 'q':
                s, sm = side_for(h, 1); ef = [(h, mag(h, 2, -1)), (s, sm)]
                nm = [a + ' ' + h['bn'][0] for a in rng.sample(ADJ_QUIRK, 6)] + [a + ' ' + h['bn'][1 % len(h['bn'])] for a in rng.sample(ADJ_QUIRK, 6)]
            nm = nm + [a + ' ' + n for n in (h['gn'] + h['bn'] + [w for x in h['gn'] + h['bn'] for w in x.split()]) for a in rng.sample(NEUTRAL, 14)]   # (more to try when the first ones are taken or too long)
            out.append(dict(kind='reg', cat=h['cat'], sub=SUBS[h['cat']].index(h['sub']), eff=[(e[0]['id'], e[1]) for e in ef], names=nm))
    return out

def hook_by_id(i): return HOOKS[i - 1]

# ---------------------------------------------------------------- build everything
def build():
    regs = make_regular(600)
    assert len(regs) == 600, len(regs)
    traits = []   # index = id - 1 -> but ids are sparse: use dict id -> trait
    T = {}
    nid = 1
    for r in regs:
        r['name'] = G.uniq(r['names']); r['id'] = nid; T[nid] = r; nid += 1
    FIRST_EARN = 601
    # earned
    earned = []
    for ln in EARNED_TXT.strip().splitlines():
        nm, ev, thr, eff = ln.split('|')
        ef = []
        for e in eff.split(','):
            code = e[:max(e.rfind('+'), e.rfind('-'))]; m = int(e[len(code):])
            h = HBY[code]
            ef.append((h['id'], m))
        nm = G.uniq([nm]); earned.append(dict(kind='earn', id=FIRST_EARN + len(earned), name=nm, ev=ev, thr=int(thr), eff=ef, cat=15, sub=0))
    assert len(earned) >= 40 and len(earned) <= 56, len(earned)
    NEARN = 56
    FIRST_RWD = FIRST_EARN + NEARN       # 657
    rewards = []
    for ln in REWARD_TXT.strip().splitlines():
        nm, eff = ln.split('|')
        ef = []
        for e in eff.split(','):
            code = e[:max(e.rfind('+'), e.rfind('-'))]; m = int(e[len(code):])
            ef.append((HBY[code]['id'], m))
        nm = G.uniq([nm]); rewards.append(dict(kind='rwd', id=FIRST_RWD + len(rewards), name=nm, eff=ef, cat=15, sub=1))
    NRWD = 24
    assert len(rewards) <= NRWD
    FIRST_CHEAT = 701
    # earned sub-groups
    SRC = {'COOKED': 0, 'ATEOUT': 0, 'BAKED': 0, 'SNACKS': 0}
    esub = ['HOME LIFE', 'WORK LIFE', 'SOCIAL LIFE', 'OUT AND ABOUT', 'SKILLS', 'PLAY', 'REWARDS']
    def earned_sub(ev):
        if ev in ('COOKED', 'ATEOUT', 'BAKED', 'SNACKS', 'SLEPT', 'SHOWERS', 'COFFEES', 'PIZZAS', 'CAKES', 'HOME', 'FAMILY', 'DAYS'): return 0
        if ev in ('PAYDAYS', 'PROMOS', 'HOMEWORK', 'EARNED', 'CASH', 'COLLEGE'): return 1
        if ev in ('CHATS', 'JOKES', 'HUGS', 'FLIRTS', 'KIND', 'CALLS', 'FRIENDS', 'PARTIES', 'PARTNER', 'CARES'): return 2
        if ev in ('GATHERS', 'SWIMS', 'DISTRICTS', 'VEHICLES', 'SPACE', 'LUCKY'): return 3
        if ev in ('RUNS', 'BOOKS', 'BANDLV'): return 4
        return 5
    for e in earned: e['sub'] = earned_sub(e['ev'])
    for r in rewards: r['sub'] = 6
    # cheats
    cheats = []
    def csub_of(h):
        c = h['code']
        if c.startswith('DEC_') or c.startswith('GAIN_') or c.startswith('ACT_'): return 0
        if c.startswith('SK_'): return 1
        if c in ('INCOME', 'SPEND', 'SHOP_DISC', 'FARE', 'LOOT'): return 2
        if c.startswith('PAY_') or c.startswith('JOB_'): return 3
        if h['cat'] in (2, 3) or c in ('REL_GAIN',): return 4
        if c in ('MOOD_BASE', 'MOOD_PULL', 'ML_GOOD', 'ML_BAD', 'MOOD_MIN', 'NIGHT_MOOD', 'DAY_MOOD', 'COLD_TOL', 'HEAT_TOL', 'RAIN_TOL', 'FEAR_WATER', 'FEAR_DARK', 'FEAR_HEIGHT', 'INDOORS', 'OUTDOORS', 'CROWD', 'ALONE', 'LOVE_MOOD', 'PET_JOY'): return 5
        if c in ('WALK_SPEED', 'VEH_SPEED', 'FUEL_USE'): return 6
        if c in ('ARC_SCORE', 'ARC_TIX', 'FIGHT_DMG', 'FIGHT_DEF', 'FIGHT_REGEN', 'FIGHT_BELT'): return 7
        return 8
    cid = FIRST_CHEAT
    for h in HOOKS:
        for t in range(3):
            adj = ADJ_C[t]
            nouns = [h['cn']] + h['cn'].split()[:1] + h['cn'].split()[-1:]
            EXTRA_C = [['Plus', 'Prime', 'Pro', 'Max', 'Neo', 'Ace', 'Elite', 'Boost', 'Power', 'Rapid', 'Slick', 'Sweet'], ['Giga', 'Titan', 'Cosmic', 'Royal', 'Epic', 'Alpha', 'Atomic', 'Rocket', 'Laser', 'Neon', 'Jumbo', 'Mighty'], ['Omega', 'Divine', 'Eternal', 'Legend', 'Hack', 'Boss', 'Cheat', 'Wizard', 'Genie', 'Mythic', 'Cosmic God', 'Final']]
            nm = [a + ' ' + n for n in nouns for a in adj + ADJ_C[(t + 1) % 3] + EXTRA_C[t]]
            cheats.append(dict(kind='cheat', id=cid, name=G.uniq(nm), cat=csub_of(h), sub=0, eff=[(h['id'], h['cheat'][t] * (1 if h['cheat'][t] > 0 else 1))], hook=h['code']))
            cid += 1
    MIXES = [
     ('God Fighter', 'FIGHT_DMG+80,FIGHT_DEF-20,FIGHT_REGEN+100'), ('Immortal', 'AGE_SPEED-20,DEC_REST-20,DEC_CLEAN-20'),
     ('Never Hungry', 'DEC_FOOD-20,DEC_WC-20,DEC_CLEAN-20'), ('Never Tired', 'DEC_REST-20,DEC_FUN-20,DEC_COMFY-20'),
     ('Perfect Needs', 'DEC_SOCIAL-20,DEC_FUN-20,DEC_COMFY-20'), ('Midas Touch', 'INCOME+90,SPEND-20,SHOP_DISC-20'),
     ('Free Stuff', 'SPEND-20,SHOP_DISC-20,FARE-20'), ('The Big Boss', 'JOB_PAY+100,JOB_PROMO+100,JOB_STRIKE-20'),
     ('Speed Demon', 'WALK_SPEED+40,VEH_SPEED+60,FUEL_USE-20'), ('Instant Genius', 'SK_LOGIC+120,SK_CREAT+120,SK_CHAR+120'),
     ('Sprint Star', 'SK_BODY+120,ACT_RUN+60,FIGHT_BELT+10'), ('Polymath', 'SK_COOK+120,SK_MECH+120,SK_FARM+120'),
     ('Rock God', 'SK_MUSIC+120,SK_FUN+120,SK_CHAR+120'), ('Arcade King', 'ARC_SCORE+100,ARC_TIX+100,LUCK+34'),
     ('Lucky Star', 'LUCK+34,LOOT+100,SPARK+34'), ('Beloved By All', 'REL_GAIN+100,CHARM+10,CHAT_OK+34'),
     ('Heartthrob', 'ROMANCE+100,FLIRT_OK+34,HUG_OK+34'), ('Pure Sunshine', 'MOOD_BASE+50,MOOD_MIN+20,ML_BAD-20'),
     ('Zen Master', 'MOOD_MIN+20,ML_BAD-20,MOOD_PULL+100'), ('Weatherproof', 'COLD_TOL+50,HEAT_TOL+50,RAIN_TOL+50'),
     ('Fearless', 'FEAR_WATER+50,FEAR_DARK+50,FEAR_HEIGHT+50'), ('Beastmaster', 'PET_FOOD-20,PET_JOY+50,GOTCHI_XP+100'),
     ('Star Pupil', 'GRADE+100,ASP_GAIN+100,ASP_KEEP-20'), ('Dream Sleeper', 'ACT_SLEEP+100,GAIN_REST+100,DEC_REST-20'),
     ('Gourmet God', 'ACT_EAT+100,GAIN_FOOD+100,SK_COOK+120'), ('Party Machine', 'CROWD+50,GAIN_SOCIAL+100,DEC_SOCIAL-20'),
     ('Sweet Home', 'DEC_COMFY-20,GAIN_COMFY+100,INDOORS+50'), ('Explorer King', 'WALK_SPEED+40,OUTDOORS+50,COLD_TOL+50'),
     ('Cash And Carry', 'INCOME+90,LOOT+100,SPEND-20'), ('Total Cheat', 'INCOME+90,DEC_FOOD-20,SK_LOGIC+120'),
    ]
    for nm, eff in MIXES:
        ef = []
        for e in eff.split(','):
            code = e[:max(e.rfind('+'), e.rfind('-'))]; m = int(e[len(code):])
            ef.append((HBY[code]['id'], m))
        cheats.append(dict(kind='cheat', id=cid, name=G.uniq([nm]), cat=9, sub=0, eff=ef, hook='MIX')); cid += 1
    assert len(cheats) >= 300, len(cheats)
    # wishes
    wishes = []
    rwd_i = 0
    for ln in WISH_TXT.strip().splitlines():
        nm, desc, cat, ev, thr, rw = ln.split('|')
        assert cat in CATS, cat
        rk = rw.split()
        kind = rk[0]
        arg = 0
        if kind == 'TRAIT':
            arg = FIRST_RWD + rwd_i; rwd_i += 1
        elif kind == 'OUTFIT': arg = theme(rk[1])
        else: arg = int(rk[1])
        assert ev in EVN, ev
        assert int(thr) <= 255
        wishes.append(dict(id=len(wishes) + 1, name=nm, desc=desc, cat=CATS.index(cat), ev=ev, thr=int(thr), rk=kind, ra=arg))
    assert len(wishes) >= 55, len(wishes)
    assert rwd_i <= len(rewards), (rwd_i, len(rewards))
    return T, earned, rewards, cheats, wishes, dict(FIRST_EARN=FIRST_EARN, NEARN=NEARN, FIRST_RWD=FIRST_RWD, NRWD=NRWD, FIRST_CHEAT=FIRST_CHEAT, MAXID=cid - 1, esub=esub)

RK = {'CASH': 1, 'TRAIT': 2, 'OUTFIT': 3, 'FOOD': 4, 'BUS': 5, 'TOKENS': 6, 'ASP': 7}

def c_str(s): return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

def describe(t):
    parts = []
    for hid, m in t['eff']:
        h = hook_by_id(hid)
        parts.append(eff_text(h, m))
    return '. '.join(parts)

# ------------------------------------------------ slots, unlocks and the personality -> traits mapping (docs/TRAITS.md, "Slots" and "Family")
BASE_SLOTS = 6                      # slots every grown-up has: 1..6
STAGE_SLOTS = [1, 3, 4, 6, 6]       # normal slots by life stage: BABY CHILD TEEN ADULT ELDER (AG_*)
UNLOCK_STAGE = 2                    # the unlockable slots 7..10 are for TEEN and up
UNLOCKS = [('WISHES', 1, 'FINISH 1 WISH'), ('WISHES', 3, 'FINISH 3 WISHES'), ('JOB', 5, 'REACH JOB LEVEL 5'), ('WISHES', 10, 'FINISH 10 WISHES')]   # slot 7, 8, 9, 10, in this order
BARS = ['NEAT', 'OUTGOING', 'ACTIVE', 'PLAYFUL', 'NICE']
# the pull of each personality bar (0..10, 5 is neutral) on each trait category: weight * (bar - 5)
_PK = {'BODY': (1, 0, 4, 0, 0), 'MIND': (1, -2, -1, 0, 0), 'SOCIAL': (0, 3, 0, 1, 2), 'ROMANCE': (0, 2, 0, 1, 2), 'WORK': (2, 0, 1, -2, 0), 'MONEY': (1, 0, 0, -1, -2),
       'FOOD': (0, 0, -1, 1, 1), 'HOME': (3, -2, -1, 0, 1), 'OUTDOORS': (-1, 1, 3, 1, 0), 'CREATIVE': (-1, 0, -1, 2, 0), 'FUN': (-1, 1, 0, 3, 0), 'NIGHT AND DAY': (-1, 2, 0, 1, 0),
       'PETS': (0, 0, 0, 1, 3), 'LUCK': (-1, 0, 0, 2, 1), 'FIGHTING': (0, 0, 3, 0, -3)}
PK = [_PK[c] for c in CATS[:15]]
# the aspiration (AS_FORTUNE, AS_KNOW, AS_POP, AS_PLEAS, AS_HOME, AS_GROW): a flat bonus (x 4) to some categories
_PA = [{'MONEY': 3, 'WORK': 2, 'LUCK': 1}, {'MIND': 3, 'WORK': 2, 'CREATIVE': 1}, {'SOCIAL': 3, 'FUN': 2, 'NIGHT AND DAY': 2}, {'FUN': 2, 'FOOD': 3, 'ROMANCE': 2, 'NIGHT AND DAY': 1},
       {'HOME': 3, 'FOOD': 2, 'PETS': 2, 'ROMANCE': 1}, {'MIND': 2, 'BODY': 1, 'FUN': 1}]
PASP = [[a.get(c, 0) for c in CATS[:15]] for a in _PA]
# the star sign's element (sign index % 4: fire ARIES LEO SAGITTARIUS, earth TAURUS VIRGO CAPRICORN, air GEMINI LIBRA AQUARIUS, water CANCER SCORPIO PISCES): a flat bonus (x 3)
_PS = [{'FIGHTING': 2, 'OUTDOORS': 2, 'BODY': 1}, {'MONEY': 2, 'HOME': 2, 'WORK': 1}, {'SOCIAL': 2, 'MIND': 1, 'CREATIVE': 2}, {'ROMANCE': 2, 'PETS': 1, 'CREATIVE': 1, 'LUCK': 1}]
PSIGN = [[a.get(c, 0) for c in CATS[:15]] for a in _PS]

def main():
    T, earned, rewards, cheats, wishes, K = build()
    all_traits = {}
    for i, t in T.items(): all_traits[i] = t
    for t in earned + rewards + cheats: all_traits[t['id']] = t
    maxid = K['MAXID']
    # sanity: unique names, non-empty effects, magnitudes in s8
    seen = set()
    for i, t in sorted(all_traits.items()):
        assert t['eff'], t
        assert len(t['eff']) <= 3, t
        assert t['name'].upper() not in seen, t['name']; seen.add(t['name'].upper())
        for hid, m in t['eff']: assert -127 <= m <= 127 and m != 0, (t, m)
    # ------------------------------------------------ the header
    L = []
    A = L.append
    A('// GENERATED by tools/make_traits.py - do not edit. See docs/TRAITS.md.')
    A('#pragma once')
    A('enum {')
    A('    E_NONE,')
    for h in HOOKS: A('    E_%s,' % h['code'])
    A('    E_N };')
    A('// families (one call site in the game handles each, by index)')
    A('#define E_DEC0 E_DEC_FOOD      // + need 0..6 (N_FOOD..N_SOCIAL)')
    A('#define E_GAIN0 E_GAIN_FOOD    // + need 0..6')
    A('#define E_SK0 E_SK_COOK        // + skill 0..8 (cook body fun logic char creat mech farm music)')
    A('#define E_PAY0 E_PAY_FACTORY   // + career 0..14 (places.h CAR_)')
    A('enum {')
    A('    EV_NIL,')
    for e in EVENTS: A('    EV_%s,' % e[0])
    A('    EV_N };')
    A('#define TR_NREG 600')
    A('#define TR_FIRST_EARN %d' % K['FIRST_EARN'])
    A('#define TR_NEARN %d' % K['NEARN'])
    A('#define TR_FIRST_RWD %d' % K['FIRST_RWD'])
    A('#define TR_NRWD %d' % K['NRWD'])
    A('#define TR_FIRST_CHEAT %d' % K['FIRST_CHEAT'])
    A('#define TR_NCHEAT %d' % len(cheats))
    A('#define TR_MAXID %d' % maxid)
    A('#define TR_NCAT %d' % len(CATS))
    A('#define TR_NHOOK %d' % len(HOOKS))
    A('#define WISH_N %d' % len(wishes))
    A('#define TR_BASE %d' % BASE_SLOTS)
    A('#define TR_NUNL %d' % len(UNLOCKS))
    A('#define TR_UNL_STAGE %d' % UNLOCK_STAGE)
    A('enum { UK_WISHES, UK_JOB };')
    A('static const u8 tr_stage_slots[5] = { %s };   // normal slots by life stage' % ', '.join(str(x) for x in STAGE_SLOTS))
    A('static const u8 tr_unl_kind[TR_NUNL] = { %s };   // slot 7..10: what counts' % ', '.join('UK_' + u[0] for u in UNLOCKS))
    A('static const u8 tr_unl_thr[TR_NUNL] = { %s };    // ... and how many' % ', '.join(str(u[1]) for u in UNLOCKS))
    A('static const char *const tr_unl_text[TR_NUNL] = { %s };' % ', '.join(c_str(u[2]) for u in UNLOCKS))
    A('enum { M_PCT, M_ADD, M_CHN, M_FLR };')
    A('enum { RK_NONE, RK_CASH, RK_TRAIT, RK_OUTFIT, RK_FOOD, RK_BUS, RK_TOKENS, RK_ASP };')
    # resident small tables
    A('// ---- resident: each hook\'s unit (percent or points per tier) and mode')
    A('extern const u8 tr_unit[E_N];   // (one copy, in traits.c)')
    A('#ifdef TR_DEFS')
    A('const u8 tr_unit[E_N] = { 0, %s };' % ', '.join(str(h['unit']) for h in HOOKS))
    A('#endif')
    A('static const u8 tr_mode[E_N] = { 0, %s };' % ', '.join(str(h['mode']) for h in HOOKS))
    A('typedef struct { u8 ev, thr, rk; u16 ra; } WishDef;')
    A('static const WishDef tr_wish[WISH_N + 1] = { { 0, 0, 0, 0 },')
    for w in wishes: A('    { EV_%s, %d, RK_%s, %d },   // %d %s' % (w['ev'], w['thr'], w['rk'], w['ra'], w['id'], w['name']))
    A('};')
    A('typedef struct { u8 ev, thr; } EarnDef;   // earned trait TR_FIRST_EARN + index')
    A('static const EarnDef tr_earn[TR_NEARN] = {')
    for i in range(K['NEARN']):
        if i < len(earned): A('    { EV_%s, %d },   // %s' % (earned[i]['ev'], earned[i]['thr'], earned[i]['name']))
        else: A('    { 0, 0 },')
    A('};')
    A('static const u8 tr_ev_abs[EV_N] = { 0, %s };   // 1: the counter is a level or a state read off the game (progress = its value), 0: a count of events' % ', '.join('1' if e[2] == 'poll' else '0' for e in EVENTS))
    A('// the earned and reward traits\' effects (hook, tier) x3: resident, so a trait can be won in the middle of the game')
    A('static const u8 tr_small[TR_NEARN + TR_NRWD][6] = {')
    sm = earned + [None] * (K['NEARN'] - len(earned)) + rewards + [None] * (K['NRWD'] - len(rewards))
    for t in sm:
        if t is None: A('    { 0, 0, 0, 0, 0, 0 },')
        else:
            ef = t['eff'] + [(0, 0)] * (3 - len(t['eff']))
            A('    { %s, %s },   // %s' % (', '.join(str(e[0]) for e in ef), ', '.join(str(e[1] & 255) for e in ef), t['name']))
    A('};')
    # resident core for the personality mapping (traits_auto.c): the regular traits' effects and categories, so a member's traits are assigned without CREATE.OVL
    A('static const u8 tr_core[TR_FIRST_EARN][6] = {   // regular trait effects: hook x3, tier x3 (s8)')
    for i in range(0, K['FIRST_EARN']):
        t = all_traits.get(i)
        if not t: A('    { 0, 0, 0, 0, 0, 0 },'); continue
        ef = t['eff'] + [(0, 0)] * (3 - len(t['eff']))
        A('    { %s, %s },' % (', '.join(str(e[0]) for e in ef), ', '.join(str(e[1] & 255) for e in ef)))
    A('};')
    _cc = [all_traits[i]['cat'] if i in all_traits else 0 for i in range(0, K['FIRST_EARN'])]
    if len(_cc) % 2: _cc.append(0)
    A('static const u8 tr_corecat[(TR_FIRST_EARN + 1) / 2] = { %s };   // regular trait category, a nibble each (id even: low)' % ', '.join(str(_cc[i] | (_cc[i + 1] << 4)) for i in range(0, len(_cc), 2)))
    A('static const s8 tr_pk[15][5] = {   // personality bars (NEAT OUTGOING ACTIVE PLAYFUL NICE) -> category pull, x (bar - 5)')
    for ci in range(15): A('    { %s },   // %s' % (', '.join(str(x) for x in PK[ci]), CATS[ci]))
    A('};')
    A('static const s8 tr_pasp[6][15] = {   // aspiration -> category bonus (x 4)')
    for r in PASP: A('    { %s },' % ', '.join(str(x) for x in r))
    A('};')
    A('static const s8 tr_psign[4][15] = {   // star sign element (sign % 4) -> category bonus (x 3)')
    for r in PSIGN: A('    { %s },' % ', '.join(str(x) for x in r))
    A('};')
    A('static const u8 tr_hook_cat[E_N] = { 0, %s };   // the hook\'s category (the filter by kind of effect)' % ', '.join(str(h['cat']) for h in HOOKS))
    A('static const u8 tr_wish_cat[WISH_N + 1] = { 0, %s };' % ', '.join(str(w['cat']) for w in wishes))
    # the big tables (overlay, tests, ports)
    A('#ifdef TR_BIG')
    A('#define tr_sname(i) (tr_names + tr_nameoff[(i) < TR_NEARN ? TR_FIRST_EARN + (i) : TR_FIRST_RWD + (i) - TR_NEARN])   // earned and reward names, from the big table')
    A('// ---- big (TRAITS.OVL, the host test): every trait\'s effects, names, category')
    A('static const u8 tr_eff[TR_MAXID + 1][6] = {   // hook x3, tier x3 (s8)')
    for i in range(0, maxid + 1):
        t = all_traits.get(i)
        if not t: A('    { 0, 0, 0, 0, 0, 0 },'); continue
        ef = t['eff'] + [(0, 0)] * (3 - len(t['eff']))
        A('    { %s, %s },' % (', '.join(str(e[0]) for e in ef), ', '.join(str(e[1] & 255) for e in ef)))
    A('};')
    A('static const u8 tr_cat[TR_MAXID + 1] = {   // category | sub-group << 4   (cheats: cheat sub-group)')
    row = []
    for i in range(0, maxid + 1):
        t = all_traits.get(i)
        row.append('%d' % ((t['cat'] | (t['sub'] << 4)) if t else 0))
    for i in range(0, len(row), 30): A('    ' + ', '.join(row[i:i + 30]) + ',')
    A('};')
    # names
    pool = ''
    offs = []
    for i in range(0, maxid + 1):
        t = all_traits.get(i)
        offs.append(len(pool))
        pool += (t['name'].upper() if t else '') + '\0'
    A('static const u16 tr_nameoff[TR_MAXID + 2] = {')
    offs.append(len(pool))
    for i in range(0, len(offs), 24): A('    ' + ', '.join(str(o) for o in offs[i:i + 24]) + ',')
    A('};')
    A('static const char tr_names[] =')
    for i in range(0, maxid + 1):
        t = all_traits.get(i)
        A('    %s "\\0"' % c_str(t['name'].upper() if t else ''))
    A('    ;')
    reg = sorted([i for i in range(1, K['FIRST_EARN']) if i in all_traits], key=lambda i: all_traits[i]['name'].upper())
    chs = sorted([t['id'] for t in cheats], key=lambda i: all_traits[i]['name'].upper())
    A('static const u16 tr_sorted[TR_NREG] = { %s };   // the regular traits by name' % ', '.join(str(i) for i in reg))
    A('static const u16 tr_csorted[TR_NCHEAT] = { %s };   // the cheats by name' % ', '.join(str(i) for i in chs))
    A('static const char *const tr_catname[TR_NCAT] = { %s };' % ', '.join(c_str(c) for c in CAT_SHORT))
    A('static const char *const tr_catlong[TR_NCAT] = { %s };' % ', '.join(c_str(c) for c in CATS))
    A('#define TR_ICONS %s   // (ui.h icons, by category)' % ', '.join(CAT_ICON))
    A('static const char *const tr_subname[TR_NCAT][8] = {')
    for ci in range(len(CATS)):
        subs = SUBS.get(ci, []) if ci != 15 else K['esub']
        A('    { %s },' % ', '.join(c_str(s) for s in subs + [''] * (8 - len(subs))))
    A('};')
    A('static const char *const tr_cheatsub[%d] = { %s };' % (len(CHEAT_SUBS), ', '.join(c_str(c) for c in CHEAT_SUBS)))
    A('static const char *const tr_hook_name[E_N] = { "", %s };' % ', '.join(c_str(h['code']) for h in HOOKS))
    A('static const char *const tr_hook_up[E_N] = { "", %s };' % ', '.join(c_str(h['up']) for h in HOOKS))
    A('static const char *const tr_hook_dn[E_N] = { "", %s };' % ', '.join(c_str(h['dn'] or h['up']) for h in HOOKS))
    A('static const u8 tr_hook_good[E_N] = { 0, %s };  // 1: a positive tier is good, 0: a negative tier is good' % ', '.join('1' if h['good'] > 0 else '0' for h in HOOKS))
    # effect-type filters: kinds
    KINDS = ['NEEDS', 'SKILLS', 'MOOD', 'SOCIAL', 'LOVE', 'WORK', 'MONEY', 'TRAVEL', 'WEATHER', 'FEARS', 'PETS', 'GAMES', 'FIGHT', 'LUCK', 'AGE']
    def kind_of(c):
        if c.startswith('DEC_') or c.startswith('GAIN_') or c.startswith('ACT_'): return 0
        if c.startswith('SK_'): return 1
        if c in ('MOOD_BASE', 'MOOD_PULL', 'ML_GOOD', 'ML_BAD', 'MOOD_MIN', 'ASP_GAIN', 'ASP_KEEP', 'NIGHT_MOOD', 'DAY_MOOD', 'INDOORS', 'OUTDOORS', 'PET_JOY', 'LOVE_MOOD'): return 2
        if c in ('REL_GAIN', 'REL_LOSS', 'CHAT_OK', 'JOKE_OK', 'CHARM', 'CROWD', 'ALONE'): return 3
        if c in ('FLIRT_OK', 'ROMANCE', 'HUG_OK', 'REL_DECAY', 'ENGAGE'): return 4
        if c.startswith('PAY_') or c.startswith('JOB_') or c == 'GRADE': return 5
        if c in ('INCOME', 'SPEND', 'SHOP_DISC', 'FARE'): return 6
        if c in ('WALK_SPEED', 'VEH_SPEED', 'FUEL_USE'): return 7
        if c in ('NIGHT_REST', 'DAY_REST', 'COLD_TOL', 'HEAT_TOL', 'RAIN_TOL'): return 8
        if c in ('FEAR_WATER', 'FEAR_DARK', 'FEAR_HEIGHT'): return 9
        if c in ('PET_FOOD', 'GOTCHI_XP', 'GOTCHI_GIFT'): return 10
        if c in ('ARC_SCORE', 'ARC_TIX'): return 11
        if c.startswith('FIGHT_'): return 12
        if c in ('LUCK', 'LOOT', 'SPARK'): return 13
        return 14
    A('#define TR_NKIND %d' % len(KINDS))
    A('static const char *const tr_kindname[TR_NKIND] = { %s };' % ', '.join(c_str(k) for k in KINDS))
    A('static const u8 tr_hook_kind[E_N] = { 0, %s };' % ', '.join(str(kind_of(h['code'])) for h in HOOKS))
    # wishes
    A('static const char *const tr_wish_name[WISH_N + 1] = { "",')
    for w in wishes: A('    %s,' % c_str(w['name']))
    A('};')
    A('static const char *const tr_wish_desc[WISH_N + 1] = { "",')
    for w in wishes: A('    %s,' % c_str(w['desc']))
    A('};')
    A('static const char *const tr_ev_name[EV_N] = { "", %s };' % ', '.join(c_str(e[1]) for e in EVENTS))
    A('#endif')
    open(os.path.join(ROOT, 'src', 'traits_data.h'), 'w').write('\n'.join(L) + '\n')
    # ------------------------------------------------ the JSON (the DS port reads this)
    def tj(t):
        d = dict(id=t['id'], name=t['name'].upper(), kind=t['kind'], category=(CATS[t['cat']] if t['kind'] != 'cheat' else 'CHEATS'),
                 group=((SUBS[t['cat']][t['sub']] if t['kind'] == 'reg' else (K['esub'][t['sub']] if t['kind'] in ('earn', 'rwd') else CHEAT_SUBS[t['cat']]))),
                 effects=[dict(hook=hook_by_id(h)['code'], tier=m, amount=m * hook_by_id(h)['unit'], unit=('points' if hook_by_id(h)['mode'] == M_ADD else 'percent')) for h, m in t['eff']],
                 text=describe(t))
        if t['kind'] == 'earn': d['event'] = t['ev']; d['count'] = t['thr']
        return d
    js = dict(
        slots=dict(base=BASE_SLOTS, by_stage=dict(zip(['BABY', 'CHILD', 'TEEN', 'ADULT', 'ELDER'], STAGE_SLOTS)), unlock_from_stage='TEEN',
                   unlocks=[dict(slot=BASE_SLOTS + 1 + i, counts=u[0], at=u[1], text=u[2]) for i, u in enumerate(UNLOCKS)],
                   cheat_traits='only in unlocked slots 7..10, and only with OPTIONS > CHEAT TRAITS on'),
        personality=dict(bars=BARS, category_pull_per_bar_point=dict((CATS[i], PK[i]) for i in range(15)),
                         aspiration_bonus_x4=dict((['FORTUNE', 'KNOW', 'POP', 'PLEASURE', 'HOME', 'GROW'][i], dict((CATS[c], PASP[i][c]) for c in range(15) if PASP[i][c])) for i in range(6)),
                         sign_element_bonus_x3=dict((['FIRE', 'EARTH', 'AIR', 'WATER'][i], dict((CATS[c], PSIGN[i][c]) for c in range(15) if PSIGN[i][c])) for i in range(4)),
                         score='category_score = sum(pull * (bar - 5)) + 4 * aspiration + 3 * sign; trait_score = 4 * category_score + jitter(seed, trait) 0..23, + 40 if a parent has it; earned and prize traits are in the pool at half the category score and need 60; at most 3 per category and 2 inherited; no clashes'),
        about='BORE 2 traits, generated by tools/make_traits.py. amount = tier * unit. See docs/TRAITS.md.',
        hooks=[dict(id=h['id'], code=h['code'], category=CATS[h['cat']], group=h['sub'], mode=['percent', 'points', 'chance', 'floor'][h['mode']], unit=h['unit'],
                    positive_is_good=h['good'] > 0, up=h['up'], down=h['dn'], regular_tiers=list(h['reg']), cheat_tiers=list(h['cheat'])) for h in HOOKS],
        categories=[dict(id=i, name=c, groups=(SUBS.get(i, []) if i != 15 else K['esub'])) for i, c in enumerate(CATS)],
        cheat_groups=CHEAT_SUBS,
        events=[dict(id=i + 1, code=e[0], text=e[1], how=e[2]) for i, e in enumerate(EVENTS)],
        traits=[tj(t) for t in T.values()],
        earned=[tj(t) for t in earned],
        rewards=[tj(t) for t in rewards],
        cheats=[tj(t) for t in cheats],
        wishes=[dict(id=w['id'], name=w['name'], text=w['desc'], category=CATS[w['cat']], event=w['ev'], count=w['thr'], reward=w['rk'], reward_arg=(THEMES[w['ra']] if w['rk'] == 'OUTFIT' else (all_traits[w['ra']]['name'].upper() if w['rk'] == 'TRAIT' else w['ra']))) for w in wishes],
    )
    json.dump(js, open(os.path.join(ROOT, 'docs', 'traits.json'), 'w'), indent=1)
    print('hooks %d  regular %d  earned %d  rewards %d  cheats %d  wishes %d  events %d  max id %d' % (len(HOOKS), len(T), len(earned), len(rewards), len(cheats), len(wishes), len(EVENTS), maxid))
    from collections import Counter
    c = Counter(t['cat'] for t in T.values())
    print('per category:', ', '.join('%s %d' % (CAT_SHORT[k], c[k]) for k in sorted(c)))

if __name__ == '__main__':
    main()
