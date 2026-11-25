# Everyday goals — BORE 2

* PS1/Dreamcast: Get a Job, Feed the Pet and Play with the Pet connect existing activities to the wants system. Unemployed Sims no longer roll work/pay/promotion goals; babies do not roll wants or fears. Social wants remap when household slots move. Goal/fear awards are limited to one per event kind; actual skill level, not level 10, is used for skill checks.
* DS/DSi: Feed the Dog, Harvest a Crop, Sell Farm Food, Care for Livestock and Find an Island Secret are new goals wired to real interactions. Friendship wants require a new friendship; aspiration cash awards reflect newly crossed stars instead of the entire accumulated star count. The jukebox handles an empty song list. Invalid saved goal IDs are not used to index name tables.
* Both: fallback selection avoids duplicate goals. New IDs are appended; existing save record sizes and enum positions are unchanged. No new graphics, heap allocations or framebuffers.

QA: `python3 test/wants_regression.py` compiles production wants code with host stubs. Native compilation, existing emulator suites, memory and real console performance remain to be verified. This branch does not include the outstanding house rendering, expansion progression or title cleanup PRs.
