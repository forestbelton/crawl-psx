#include "AppHdr.h"
#include "lev-pand.h"

#include "externs.h"
#include "monplace.h"
#include "mon-pick.h"
#include "stuff.h"

void init_pandemonium() {
    for (const auto &monster : menv) {
        // Looks for unique demons and sets appropriate lists of demons.
        // NB - also sets the level colours.
        if (monster.type == MONS_MNOLEG) {
            env.mons_alloc[0] = MONS_ABOMINATION_SMALL;
            env.mons_alloc[1] = MONS_ABOMINATION_SMALL;
            env.mons_alloc[2] = MONS_ABOMINATION_SMALL;
            env.mons_alloc[3] = MONS_ABOMINATION_LARGE;
            env.mons_alloc[4] = MONS_NEQOXEC;
            env.mons_alloc[5] = MONS_MIDGE;
            env.mons_alloc[6] = MONS_NEQOXEC;
            env.mons_alloc[7] = MONS_BLUE_DEATH;
            env.mons_alloc[8] = MONS_BALRUG;
            env.mons_alloc[9] = MONS_LEMURE;
            return;
        }

        if (monster.type == MONS_LOM_LOBON) {
            env.mons_alloc[0] = MONS_HELLWING;
            env.mons_alloc[1] = MONS_SMOKE_DEMON;
            env.mons_alloc[2] = MONS_SMOKE_DEMON;
            env.mons_alloc[3] = MONS_YNOXINUL;
            env.mons_alloc[4] = MONS_GREEN_DEATH;
            env.mons_alloc[5] = MONS_BLUE_DEATH;
            env.mons_alloc[6] = MONS_SMOKE_DEMON;
            env.mons_alloc[7] = MONS_HELLWING;
            env.mons_alloc[8] = MONS_WHITE_IMP;
            env.mons_alloc[9] = MONS_HELLWING;
            return;
        }

        if (monster.type == MONS_CEREBOV) {
            env.mons_alloc[0] = MONS_EFREET;
            env.mons_alloc[1] = MONS_ABOMINATION_SMALL;
            env.mons_alloc[2] = MONS_ORANGE_DEMON;
            env.mons_alloc[3] = MONS_ORANGE_DEMON;
            env.mons_alloc[4] = MONS_NEQOXEC;
            env.mons_alloc[5] = MONS_LEMURE;
            env.mons_alloc[6] = MONS_ORANGE_DEMON;
            env.mons_alloc[7] = MONS_YNOXINUL;
            env.mons_alloc[8] = MONS_BALRUG;
            env.mons_alloc[9] = MONS_BALRUG;
            return;
        }

        if (monster.type == MONS_GLOORX_VLOQ) {
            env.mons_alloc[0] = MONS_SKELETON_SMALL;
            env.mons_alloc[1] = MONS_SKELETON_SMALL;
            env.mons_alloc[2] = MONS_SKELETON_LARGE;
            env.mons_alloc[3] = MONS_WHITE_IMP;
            env.mons_alloc[4] = MONS_CACODEMON;
            env.mons_alloc[5] = MONS_HELLWING;
            env.mons_alloc[6] = MONS_SMOKE_DEMON;
            env.mons_alloc[7] = MONS_EXECUTIONER;
            env.mons_alloc[8] = MONS_EXECUTIONER;
            env.mons_alloc[9] = MONS_EXECUTIONER;
            return;
        }
    }

    // colour of monster 9 is colour of floor, 8 is colour of rock
    // IIRC, BLACK is set to LIGHTGRAY
    for (auto pc = 0; pc < 10; pc++) {
        switch (random2(17)) {
            case 0: case 10: env.mons_alloc[pc] = MONS_WHITE_IMP;         break;
            case 1: case 11: env.mons_alloc[pc] = MONS_LEMURE;            break;
            case 2: case 12: env.mons_alloc[pc] = MONS_UFETUBUS;          break;
            case 3: case 13: env.mons_alloc[pc] = MONS_MANES;             break;
            case 4: case 14: env.mons_alloc[pc] = MONS_MIDGE;             break;
            case 5:          env.mons_alloc[pc] = MONS_NEQOXEC;           break;
            case 6:          env.mons_alloc[pc] = MONS_ORANGE_DEMON;      break;
            case 7:          env.mons_alloc[pc] = MONS_HELLWING;          break;
            case 8:          env.mons_alloc[pc] = MONS_SMOKE_DEMON;       break;
            case 9:          env.mons_alloc[pc] = MONS_YNOXINUL;          break;
            case 15:         env.mons_alloc[pc] = MONS_ABOMINATION_SMALL; break;
            case 16:         env.mons_alloc[pc] = MONS_ABOMINATION_LARGE; break;
            default: ;
        }

        if (one_chance_in(10))
            env.mons_alloc[pc] = MONS_HELLION + random2(10);

        if (one_chance_in(30))
            env.mons_alloc[pc] = MONS_RED_DEVIL;

        if (one_chance_in(30))
            env.mons_alloc[pc] = MONS_IMP;

        if (one_chance_in(20))
            env.mons_alloc[pc] = MONS_DEMONIC_CRAWLER + random2(5);
    }

    if (one_chance_in(8))
        env.mons_alloc[7] = MONS_EXECUTIONER + random2(5);

    if (one_chance_in(5))
        env.mons_alloc[8] = MONS_EXECUTIONER + random2(5);

    if (one_chance_in(3))
        env.mons_alloc[9] = MONS_EXECUTIONER + random2(5);

    if (one_chance_in(10))
        env.mons_alloc[7 + random2(3)] = MONS_FIEND;

    if (one_chance_in(10))
        env.mons_alloc[7 + random2(3)] = MONS_ICE_FIEND;

    if (one_chance_in(10))
        env.mons_alloc[7 + random2(3)] = MONS_SHADOW_FIEND;

    if (one_chance_in(10))
        env.mons_alloc[7 + random2(3)] = MONS_PIT_FIEND;

    // set at least some specific monsters for the special levels - this
    // can also be used to set some colours
}

void pandemonium_mons() {
    // must leave allowance for monsters rare on pandemonium (eg wizards etc)
    auto pan_mons = env.mons_alloc[random2(10)];

    if (one_chance_in(40)) {
        do {
            pan_mons = random2(NUM_MONSTERS);   // was random2(400) {dlb}
        } while (!mons_pan(pan_mons));
    }
    mons_place(pan_mons, BEH_HOSTILE, MHITNOT, false, 50,50, LEVEL_PANDEMONIUM);
}
