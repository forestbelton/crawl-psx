/*
 *  File:       items.cc
 *  Summary:    Misc (mostly) inventory related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 * <9> 7/08/01   MV   Added messages for chunks/corpses rotting
 * <8> 8/07/99   BWR  Added Rune stacking
 * <7> 6/13/99   BWR  Added auto staff detection
 * <6> 6/12/99   BWR  Fixed time system.
 * <5> 6/9/99    DML  Autopickup
 * <4> 5/26/99   JDJ  Drop will attempt to take off armour.
 * <3> 5/21/99   BWR  Upped armour skill learning slightly.
 * <2> 5/20/99   BWR  Added assurance that against inventory count being wrong.
 * <1> -/--/--   LRH  Created
 */

#include "AppHdr.h"
#include "items.h"

#include <string.h>

#include "externs.h"

#include "beam.h"
#include "cloud.h"
#include "debug.h"
#include "delay.h"
#include "effects.h"
#include "invent.h"
#include "it_use2.h"
#include "item_use.h"
#include "itemname.h"
#include "monplace.h"
#include "monstuff.h"
#include "mstuff2.h"
#include "mon-util.h"
#include "mutation.h"
#include "player.h"
#include "randart.h"
#include "religion.h"
#include "shopping.h"
#include "skills.h"
#include "spl-cast.h"
#include "stuff.h"

class item_list {
    int obj;

    explicit item_list() : obj(NON_ITEM) {}

public:
    item_list(const int x, const int y) : obj(igrd[x][y]) {}

    item_list& operator++() {
        if (obj != NON_ITEM) {
            obj = mitm[obj].link;
        }
        return *this;
    }

    auto& operator*() const {
        return mitm[obj];
    }

    bool operator!=(const item_list& other) const {
        return obj != other.obj;
    }

    [[nodiscard]] item_list begin() const {
        return *this;
    }

    static item_list end() {
        return item_list();
    }
};

static void autopickup();

/**
 * @brief Get the first unused inventory item slot
 * @return An unused inventory item index, or -1 if inventory is full
 */
static int get_inv_item_slot();

void fix_item_coordinates() {
    // Nails all items to the ground (i.e. sets x,y)
    for (short x = 0; x < GXM; x++) {
        for (short y = 0; y < GYM; y++) {
            for (auto &item : item_list(x, y)) {
                item.x = x;
                item.y = y;
            }
        }
    }
}

void link_items() {
    // first, initailize igrd array
    for (auto x = 0; x < GXM; x++) {
        for (auto y = 0; y < GYM; y++) {
            igrd[x][y] = NON_ITEM;
        }
    }

    // link all items on the grid, plus shop inventory,
    // DON'T link the huge pile of monster items at (0,0)
    for (int i = 0; i < mitm.size(); i++) {
        if (!mitm[i].valid() || (mitm[i].x == 0 && mitm[i].y == 0)) {
            // item is not assigned,  or is monster item.  ignore.
            mitm[i].link = NON_ITEM;
            continue;
        }

        // link to top
        mitm[i].link = igrd[ mitm[i].x ][ mitm[i].y ];
        igrd[ mitm[i].x ][ mitm[i].y ] = i;
    }
}

int cull_items() {
    // XXX: Not the prettiest of messages, but the player
    // deserves to know whenever this kicks in. -- bwr
    mpr("Too many items on level, removing some.", MSGCH_WARN);

    /* Rules:
       1. Don't clean up anything nearby the player
       2. Don't clean up shops
       3. Don't clean up monster inventory
       4. Clean 15% of items (NB: Not checked? -forest)
       5. never remove food, orbs, runes
       7. uniques weapons are moved to the abyss
       8. randarts are simply lost
       9. unrandarts are 'destroyed', but may be generated again
    */

    int next;
    int first_cleaned = NON_ITEM;

    // 2. avoid shops by avoiding (0,5..9)
    // 3. avoid monster inventory by iterating over the dungeon grid
    for (auto x = 5; x < GXM; x++) {
        for (auto y = 5; y < GYM; y++) {
            // 1. not near player!
            if (x > you.x_pos - 9 && x < you.x_pos + 9
                && y > you.y_pos - 9 && y < you.y_pos + 9) {
                continue;
            }

            // iterate through the grids list of items:
            for (auto item = igrd[x][y]; item != NON_ITEM; item = next) {
                next = mitm[item].link; // in case we can't get it later.

                if (mitm[item].can_clean() && random2(100) < 15) {
                    if (mitm[item].is_fixed_artefact()) {
                        // 7. move uniques to abyss
                        set_unique_item_status(OBJ_WEAPONS, mitm[item].special, UNIQ_LOST_IN_ABYSS);
                    } else if (mitm[item].is_unrandom_artefact()) {
                        // 9. unmark unrandart
                        const auto idx = find_unrandart_index(item);
                        if (x >= 0) {
                            set_unrandart_exist(idx, 0);
                        }
                    }

                    // POOF!
                    destroy_item(item);
                    if (first_cleaned == NON_ITEM) {
                        first_cleaned = item;
                    }
                }
            }
        }
    }

    return first_cleaned;
}

bool dec_inv_item_quantity(const int obj, const int amount) {
    bool ret = false;

    if (you.equip[EQ_WEAPON] == obj) {
        you.wield_change = true;
    }

    if (you.inv[obj].quantity <= amount) {
        for (int i = 0; i < NUM_EQUIP; i++) {
            if (you.equip[i] == obj) {
                you.equip[i] = -1;
                if (i == EQ_WEAPON) {
                    unwield_item(obj);
                    canned_msg(MSG_EMPTY_HANDED);
                }
            }
        }
        you.inv[obj].base_type = OBJ_UNASSIGNED;
        you.inv[obj].quantity = 0;
        ret = true;
    } else {
        you.inv[obj].quantity -= amount;
    }

    burden_change();

    return ret;
}

bool dec_mitm_item_quantity(const int obj, const int amount) {
    auto stack_exists = false;
    if (mitm[obj].quantity <= amount) {
        destroy_item(obj);
        stack_exists = true;
    }
    mitm[obj].quantity -= amount;
    return stack_exists;
}

void inc_inv_item_quantity(const int obj, const int amount) {
    if (you.equip[EQ_WEAPON] == obj) {
        you.wield_change = true;
    }
    you.inv[obj].quantity += amount;
    burden_change();
}

void inc_mitm_item_quantity(const int obj, const int amount) {
    mitm[obj].quantity += amount;
}

void init_item(const int item) {
    if (item == NON_ITEM) {
        return;
    }

    mitm[item] = {
        OBJ_UNASSIGNED,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        NON_ITEM,
    };
}

int get_item_slot(const int reserve) {
    assert(reserve >= 0);

    auto item = NON_ITEM;
    for (item = 0; item < MAX_ITEMS - reserve; item++) {
        if (!mitm[item].valid()) {
            break;
        }
    }

    if (item >= MAX_ITEMS - reserve) {
        item = reserve <= 10 ? cull_items() : NON_ITEM;
    }

    if (item != NON_ITEM) {
        init_item(item);
    }

    return item;
}

void unlink_item(int dest) {
    // Don't destroy non-items, may be called after an item has been
    // reduced to zero quantity, however.
    if (dest == NON_ITEM || !mitm[dest].valid()) {
        return;
    }

    if (mitm[dest].x == 0 && mitm[dest].y == 0) {
        // (0,0) is where the monster items are (and they're unlinked by igrd),
        // although it also contains items that are not linked in yet.
        //
        // Check if a monster has it:
        for (auto &monster : menv) {
            if (monster.type == -1) {
                continue;
            }

            for (auto &monster_item : monster.inv) {
                if (monster_item != dest) {
                    continue;
                }

                monster_item = NON_ITEM;
                mitm[dest].x = 0;
                mitm[dest].y = 0;
                mitm[dest].link = NON_ITEM;

                // This causes problems when changing levels. -- bwr
                // if (monster->type == MONS_DANCING_WEAPON)
                //     monster_die(monster, KILL_RESET, 0);
                return;
            }
        }

        // Always return because this item might just be temporary.
        return;
    }

    // Linked item on map:
    //
    // Use the items (x,y) to access the list (igrd[x][y]) where
    // the item should be linked.

    // First check the top:
    if (igrd[mitm[dest].x][mitm[dest].y] == dest) {
        // Link igrd to the second item
        igrd[mitm[dest].x][mitm[dest].y] = mitm[dest].link;

        mitm[dest].x = 0;
        mitm[dest].y = 0;
        mitm[dest].link = NON_ITEM;
        return;
    }

    // Okay, item is buried, find item that's on top of it:
    for (auto &item : item_list(mitm[dest].x, mitm[dest].y)) {
        if (item.valid() && item.link == dest) {
            // Unlink dest
            item.link = mitm[dest].link;

            mitm[dest].x = 0;
            mitm[dest].y = 0;
            mitm[dest].link = NON_ITEM;
            return;
        }
    }

#if DEBUG
    // Okay, the sane ways are gone... let's warn the player:
    mpr( "BUG WARNING: Problems unlinking item!!!", MSGCH_DANGER );

    // Okay, first we scan all items to see if we have something
    // linked to this item.  We're not going to return if we find
    // such a case... instead, since things are already out of
    // alignment, let's assume there might be multiple links as well.
    bool  linked = false;
    int   old_link = mitm[dest].link; // used to try linking the first

    // clean the relevant parts of the object:
    mitm[dest].base_type = OBJ_UNASSIGNED;
    mitm[dest].quantity = 0;
    mitm[dest].x = 0;
    mitm[dest].y = 0;
    mitm[dest].link = NON_ITEM;

    // Look through all items for links to this item.
    for (c = 0; c < MAX_ITEMS; c++)
    {
        if (is_valid_item( mitm[c] ) && mitm[c].link == dest)
        {
            // unlink item
            mitm[c].link = old_link;

            if (!linked)
            {
                old_link = NON_ITEM;
                linked = true;
            }
        }
    }

    // Now check the grids to see if it's linked as a list top.
    for (c = 2; c < (GXM - 1); c++)
    {
        for (cy = 2; cy < (GYM - 1); cy++)
        {
            if (igrd[c][cy] == dest)
            {
                igrd[c][cy] = old_link;

                if (!linked)
                {
                    old_link = NON_ITEM;  // cleaned after the first
                    linked = true;
                }
            }
        }
    }


    // Okay, finally warn player if we didn't do anything.
    if (!linked)
        mpr("BUG WARNING: Item didn't seem to be linked at all.", MSGCH_DANGER);
#endif
}

void destroy_item(const int dest) {
    // Don't destroy non-items, but this function may be called upon
    // to remove items reduced to zero quantity, so we allow "invalid"
    // objects in.
    if (dest != NON_ITEM && mitm[dest].valid()) {
        unlink_item(dest);

        mitm[dest].base_type = OBJ_UNASSIGNED;
        mitm[dest].quantity = 0;
    }
}

void destroy_item_stack(const int x, const int y) {
    int o = igrd[x][y];
    igrd[x][y] = NON_ITEM;
    while (o != NON_ITEM) {
        const int next = mitm[o].link;
        if (mitm[o].valid()) {
            if (mitm[o].base_type == OBJ_ORBS) {
                set_unique_item_status(OBJ_ORBS, mitm[o].sub_type, UNIQ_LOST_IN_ABYSS);
            } else if (mitm[o].is_fixed_artefact()) {
                set_unique_item_status(OBJ_WEAPONS, mitm[o].special, UNIQ_LOST_IN_ABYSS);
            }
            mitm[o].base_type = OBJ_UNASSIGNED;
            mitm[o].quantity = 0;
        }
        o = next;
    }
}

void item_check(const char keyin) {
    char item_show[50][50];

    int counter = 0;

    if (const int grid = grd[you.x_pos][you.y_pos]; grid >= DNGN_ENTER_HELL && grid <= DNGN_PERMADRY_FOUNTAIN) {
        if (grid >= DNGN_STONE_STAIRS_DOWN_I && grid <= DNGN_ROCK_STAIRS_DOWN) {
            mprf("There is a %s staircase leading down here.", grid == DNGN_ROCK_STAIRS_DOWN ? "rock" : "stone");
        } else if (grid >= DNGN_STONE_STAIRS_UP_I && grid <= DNGN_ROCK_STAIRS_UP) {
            mprf("There is a %s staircase leading upwards here.", grid == DNGN_ROCK_STAIRS_UP ? "rock" : "stone");
        } else {
            switch (grid) {
                case DNGN_ENTER_HELL:
                    mpr("There is a gateway to Hell here.");
                    break;
                case DNGN_ENTER_GEHENNA:
                    mpr("There is a gateway to Gehenna here.");
                    break;
                case DNGN_ENTER_COCYTUS:
                    mpr("There is a gateway to the frozen wastes of Cocytus here.");
                    break;
                case DNGN_ENTER_TARTARUS:
                    mpr("There is a gateway to Tartarus here.");
                    break;
                case DNGN_ENTER_DIS:
                    mpr("There is a gateway to the Iron City of Dis here.");
                    break;
                case DNGN_ENTER_SHOP:
                    snprintf(info, INFO_SIZE, "There is an entrance to %s here.", shop_name(you.x_pos, you.y_pos));
                    mpr(info);
                    break;
                case DNGN_ENTER_LABYRINTH:
                    mpr("There is an entrance to a labyrinth here.");
                    mpr("Beware, for starvation awaits!");
                    break;
                case DNGN_ENTER_ABYSS:
                    mpr("There is a one-way gate to the infinite horrors of the Abyss here.");
                    break;
                case DNGN_STONE_ARCH:
                    mpr("There is an empty stone archway here.");
                    break;
                case DNGN_EXIT_ABYSS:
                    mpr("There is a gateway leading out of the Abyss here.");
                    break;
                case DNGN_ENTER_PANDEMONIUM:
                    mpr("There is a gate leading to the halls of Pandemonium here.");
                    break;
                case DNGN_EXIT_PANDEMONIUM:
                    mpr("There is a gate leading out of Pandemonium here.");
                    break;
                case DNGN_TRANSIT_PANDEMONIUM:
                    mpr("There is a gate leading to another region of Pandemonium here.");
                    break;
                case DNGN_ENTER_ORCISH_MINES:
                    mpr("There is a staircase to the Orcish Mines here.");
                    break;
                case DNGN_ENTER_HIVE:
                    mpr("There is a staircase to the Hive here.");
                    break;
                case DNGN_ENTER_LAIR:
                    mpr("There is a staircase to the Lair here.");
                    break;
                case DNGN_ENTER_SLIME_PITS:
                    mpr("There is a staircase to the Slime Pits here.");
                    break;
                case DNGN_ENTER_VAULTS:
                    mpr("There is a staircase to the Vaults here.");
                    break;
                case DNGN_ENTER_CRYPT:
                    mpr("There is a staircase to the Crypt here.");
                    break;
                case DNGN_ENTER_HALL_OF_BLADES:
                    mpr("There is a staircase to the Hall of Blades here.");
                    break;
                case DNGN_ENTER_ZOT:
                    mpr("There is a gate to the Realm of Zot here.");
                    break;
                case DNGN_ENTER_TEMPLE:
                    mpr("There is a staircase to the Ecumenical Temple here.");
                    break;
                case DNGN_ENTER_SNAKE_PIT:
                    mpr("There is a staircase to the Snake Pit here.");
                    break;
                case DNGN_ENTER_ELVEN_HALLS:
                    mpr("There is a staircase to the Elven Halls here.");
                    break;
                case DNGN_ENTER_TOMB:
                    mpr("There is a staircase to the Tomb here.");
                    break;
                case DNGN_ENTER_SWAMP:
                    mpr("There is a staircase to the Swamp here.");
                    break;
                case DNGN_RETURN_FROM_ORCISH_MINES:
                case DNGN_RETURN_FROM_HIVE:
                case DNGN_RETURN_FROM_LAIR:
                case DNGN_RETURN_FROM_VAULTS:
                case DNGN_RETURN_FROM_TEMPLE:
                    mpr("There is a staircase back to the Dungeon here.");
                    break;
                case DNGN_RETURN_FROM_SLIME_PITS:
                case DNGN_RETURN_FROM_SNAKE_PIT:
                case DNGN_RETURN_FROM_SWAMP:
                    mpr("There is a staircase back to the Lair here.");
                    break;
                case DNGN_RETURN_FROM_CRYPT:
                case DNGN_RETURN_FROM_HALL_OF_BLADES:
                    mpr("There is a staircase back to the Vaults here.");
                    break;
                case DNGN_RETURN_FROM_TOMB:
                    mpr("There is a staircase back to the Crypt here.");
                    break;
                case DNGN_RETURN_FROM_ELVEN_HALLS:
                    mpr("There is a staircase back to the Mines here.");
                    break;
                case DNGN_RETURN_FROM_ZOT:
                    mpr("There is a gate leading back out of this place here.");
                    break;
                case DNGN_ALTAR_ZIN:
                    mpr("There is a glowing white marble altar of Zin here.");
                    break;
                case DNGN_ALTAR_SHINING_ONE:
                    mpr("There is a glowing golden altar of the Shining One here.");
                    break;
                case DNGN_ALTAR_KIKUBAAQUDGHA:
                    mpr("There is an ancient bone altar of Kikubaaqudgha here.");
                    break;
                case DNGN_ALTAR_YREDELEMNUL:
                    mpr("There is a basalt altar of Yredelemnul here.");
                    break;
                case DNGN_ALTAR_XOM:
                    mpr("There is a shimmering altar of Xom here.");
                    break;
                case DNGN_ALTAR_VEHUMET:
                    mpr("There is a shining altar of Vehumet here.");
                    break;
                case DNGN_ALTAR_OKAWARU:
                    mpr("There is an iron altar of Okawaru here.");
                    break;
                case DNGN_ALTAR_MAKHLEB:
                    mpr("There is a burning altar of Makhleb here.");
                    break;
                case DNGN_ALTAR_SIF_MUNA:
                    mpr("There is a deep blue altar of Sif Muna here.");
                    break;
                case DNGN_ALTAR_TROG:
                    mpr("There is a bloodstained altar of Trog here.");
                    break;
                case DNGN_ALTAR_NEMELEX_XOBEH:
                    mpr("There is a sparkling altar of Nemelex Xobeh here.");
                    break;
                case DNGN_ALTAR_ELYVILON:
                    mpr("There is a silver altar of Elyvilon here.");
                    break;
                case DNGN_BLUE_FOUNTAIN:
                    mpr("There is a fountain here (q to drink).");
                    break;
                case DNGN_SPARKLING_FOUNTAIN:
                    mpr("There is a sparkling fountain here (q to drink).");
                    break;
                case DNGN_DRY_FOUNTAIN_I:
                case DNGN_DRY_FOUNTAIN_II:
                case DNGN_DRY_FOUNTAIN_IV:
                case DNGN_DRY_FOUNTAIN_VI:
                case DNGN_DRY_FOUNTAIN_VIII:
                case DNGN_PERMADRY_FOUNTAIN:
                    mpr("There is a dry fountain here.");
                    break;
                default: ;
            }
        }
    }

    if (igrd[you.x_pos][you.y_pos] == NON_ITEM && keyin == ';') {
        mpr("There are no items here.");
        return;
    }

    autopickup();

    for (const auto &item : item_list(you.x_pos, you.y_pos)) {
        counter++;

        if (counter > 45) {
            strcpy(item_show[counter], "Too many items.");
            break;
        }

        if (item.base_type == OBJ_GOLD) {
            sprintf(item_show[counter], "%d gold piece%s", item.quantity, item.quantity > 1 ? "s" : "");
        } else {
            char str_pass[ITEMNAME_SIZE];
            strcpy(item_show[counter], item_name(item, DESC_NOCAP_A, str_pass));
        }
    }

    if (counter == 1) {
        mprf("You see here %s.", item_show[counter]); // remember 'an'.
    } else if (counter < 6 || keyin == ';') {
        mpr("Things that are here:");
        for (auto i = 1; i <= counter; ++i) {
            mpr(item_show[i]);
        }
    } else {
        mpr("There are several objects here.");
    }
}

#ifndef PSX
#define PICKUP_ACTIONS "(y,n,a,q)"
#define PICKUP_KEY_YES 'y'
#define PICKUP_KEY_NO 'n'
#define PICKUP_KEY_ALL 'a'
#define PICKUP_KEY_QUIT 'q'
#else
#define PICKUP_ACTIONS "Yes(" S_CIRCLE "),No(" S_CROSS "),All(" S_SQUARE "),Quit(" S_TRIANGLE ")"
#define PICKUP_KEY_YES PAD_CROSS
#define PICKUP_KEY_NO PAD_CIRCLE
#define PICKUP_KEY_ALL PAD_SQUARE
#define PICKUP_KEY_QUIT PAD_TRIANGLE
#endif

static int get_inv_item_slot() {
    auto item_index = -1;
    for (auto m = 0; m < you.inv.size(); m++) {
        if (!you.inv[m].valid()) {
            item_index = m;
            break;
        }
    }
    return item_index;
}

void pickup() {
    int o = 0;
    char str_pass[ITEMNAME_SIZE];

    if (you.attribute[ATTR_TRANSFORMATION] == TRAN_AIR && you.duration[DUR_TRANSFORMATION] > 0) {
        mpr("You can't pick up anything in this form!");
        return;
    }

    if (player_is_levitating() && !wearing_amulet(AMU_CONTROLLED_FLIGHT)) {
        mpr("You can't reach the floor from up here.");
        return;
    }

    // Fortunately, the player is prevented from testing their
    // portable altar in the Ecumenical Temple. -- bwr
    if (grd[you.x_pos][you.y_pos] == DNGN_ALTAR_NEMELEX_XOBEH && !player_in_branch(BRANCH_ECUMENICAL_TEMPLE)) {
        if (inv_count() >= ENDOFPACK) {
            mpr("There is a portable altar here, but you can't carry anything else.");
            return;
        }

        if (yesno("There is a portable altar here. Pick it up?")) {
            for (int m = 0; m < you.inv.size(); m++) {
                if (you.inv[m].valid()) {
                    continue;
                }
                you.inv[m].base_type = OBJ_MISCELLANY;
                you.inv[m].sub_type = MISC_PORTABLE_ALTAR_OF_NEMELEX;
                you.inv[m].plus = 0;
                you.inv[m].plus2 = 0;
                you.inv[m].special = 0;
                you.inv[m].colour = LIGHTMAGENTA;
                you.inv[m].quantity = 1;
                set_ident_flags(you.inv[m], ISFLAG_IDENT_MASK);

                you.inv[m].x = -1;
                you.inv[m].y = -1;
                you.inv[m].link = static_cast<short>(m);

                burden_change();

                in_name(m, DESC_INVENTORY_EQUIP, str_pass);
                mpr(str_pass);
                break;
            }
            grd[you.x_pos][you.y_pos] = DNGN_FLOOR;
        }
    }

    o = igrd[you.x_pos][you.y_pos];
    if (o == NON_ITEM) {
        mpr("There are no items here.");

        // Just a single item
    } else if (mitm[o].link == NON_ITEM) {
        if (const int num = move_item_to_player(o, mitm[o].quantity); num == -1) {
            mpr("You can't carry that many items.");
        } else if (num == 0) {
            mpr("You can't carry that much weight.");
        }
    } else {
        input_key_t keyin = 0;
        mpr("There are several objects here.");

        while (o != NON_ITEM) {
            const auto next = mitm[o].link;

            if (keyin != PAD_SQUARE) {
                if (mitm[o].base_type == OBJ_GOLD) {
                    sprintf(
                        info,
                        "Pick up %d gold piece%s? " PICKUP_ACTIONS,
                        mitm[o].quantity,
                        mitm[o].quantity > 1 ? "s" : ""
                    );
                } else {
                    it_name(o, DESC_NOCAP_A, str_pass);
                    sprintf(info, "Pick up %s?" PICKUP_ACTIONS, str_pass);
                }
                mpr(info, MSGCH_PROMPT);
#ifndef PSX
                keyin = get_ch();
#else
                keyin = getpad();
#endif
            }

            if (keyin == PICKUP_KEY_QUIT) {
                break;
            }

            if (keyin == PICKUP_KEY_YES || keyin == PICKUP_KEY_ALL) {
                if (const auto result = move_item_to_player(o, mitm[o].quantity); result == 0) {
                    mpr("You can't carry that much weight.");
                    keyin = PICKUP_KEY_QUIT; // resets from PICKUP_KEY_ALL
                } else if (result == -1) {
                    mpr("You can't carry that many items.");
                    break;
                }
            }

            o = next;
        }
    }
}

bool items_stack(const item_def &item1, const item_def &item2) {
    // Both items must be stackable
    if (!item1.is_stackable() || !item2.is_stackable()) {
        return false;
    }

    // Base and subtypes must always be the same to stack
    if (item1.base_type != item2.base_type || item1.sub_type != item2.sub_type)
        return false;

    // These classes also require pluses and special
    if (item1.base_type == OBJ_MISSILES
        || item1.base_type == OBJ_MISCELLANY) // only runes
    {
        if (item1.plus != item2.plus
            || item1.plus2 != item2.plus2
            || item1.special != item2.special) {
            return false;
        }
    }

    // Check the flags, food/scrolls/potions don't care about the item's
    // ident status (scrolls and potions are known by identifying any
    // one of them, the individual status might not be the same).
    if (item1.base_type == OBJ_FOOD
        || item1.base_type == OBJ_SCROLLS
        || item1.base_type == OBJ_POTIONS) {
        if ((item1.flags & ~ISFLAG_IDENT_MASK)
            != (item2.flags & ~ISFLAG_IDENT_MASK)) {
            return false;
        }

        // Thanks to mummy cursing, we can have potions of decay
        // that don't look alike... so we don't stack potions
        // if either isn't identified and they look different.  -- bwr
        if (item1.base_type == OBJ_POTIONS
            && item1.special != item2.special
            && (item_not_ident(item1, ISFLAG_KNOW_TYPE)
                || item_not_ident(item2, ISFLAG_KNOW_TYPE))) {
            return false;
        }
    } else if (item1.flags != item2.flags) {
        return false;
    }

    return true;
}

int move_item_to_player(const int obj, int quant_got, const bool quiet) {
    int retval = quant_got;
    bool partialPickup = false;

    // Gold has no mass, so we handle it first.
    if (mitm[obj].base_type == OBJ_GOLD) {
        dec_mitm_item_quantity(obj, quant_got);
        you.gold += quant_got;
        you.redraw_gold = 1;
        you.turn_is_over = 1;
        if (!quiet) {
            mprf("You pick up %d gold piece%s.", quant_got, quant_got > 1 ? "s" : "");
        }
        return retval;
    }

    const auto unit_mass = mass_item(mitm[obj]);
    const auto item_mass = unit_mass * mitm[obj].quantity;
    quant_got = mitm[obj].quantity;

    // multiply both constants * 10

    if (you.burden + item_mass > carrying_capacity()) {
        // Calculate quantity we can actually pick up
        const auto part = (carrying_capacity() - you.burden) / unit_mass;

        if (part < 1) {
            return 0;
        }

        // only pickup 'part' items
        quant_got = part;
        partialPickup = true;
        retval = part;
    }

    if (mitm[obj].is_stackable()) {
        for (auto m = 0; m < you.inv.size(); ++m) {
            if (items_stack( you.inv[m], mitm[obj] )) {
                if (!quiet && partialPickup)
                    mpr("You can only carry some of what is here.");

                inc_inv_item_quantity(m, quant_got);
                dec_mitm_item_quantity(obj, quant_got);
                burden_change();

                if (!quiet) {
                    in_name(m, DESC_INVENTORY, info);
                    mpr(info);
                }

                you.turn_is_over = 1;
                return retval;
            }
        }
    }

    if (inv_count() >= ENDOFPACK) {
        return -1;
    }

    if (!quiet && partialPickup) {
        mpr("You can only carry some of what is here.");
    }

    if (const auto m = get_inv_item_slot(); m != -1) {
        // copy item
        you.inv[m] = mitm[obj];
        you.inv[m].x = -1;
        you.inv[m].y = -1;
        you.inv[m].link = static_cast<short>(m);
        you.inv[m].quantity = static_cast<short>(quant_got);
        dec_mitm_item_quantity(obj, quant_got);
        burden_change();

        if (!quiet) {
            in_name(m, DESC_INVENTORY, info);
            mpr(info);
        }

        if (you.inv[m].base_type == OBJ_ORBS && you.char_direction == DIR_DESCENDING) {
            if (!quiet) {
                mpr("Now all you have to do is get back out of the dungeon!");
            }
            you.char_direction = DIR_ASCENDING;
        }
    }

    you.turn_is_over = 1;

    return retval;
}

void move_item_to_grid(int *const obj, const int x, const int y) {
    // must be a valid reference to a valid object
    if (*obj == NON_ITEM || !mitm[*obj].valid()) {
        return;
    }

    // If it's a stackable type...
    if (mitm[*obj].is_stackable()) {
        // Look for similar item to stack:
        for (int i = igrd[x][y]; i != NON_ITEM; i = mitm[i].link) {
            // check if item already linked here -- don't want to unlink it
            if (*obj == i) {
                return;
            }

            if (items_stack(mitm[*obj], mitm[i])) {
                // Add quantity to item already here, and dispose
                // of obj, while returning the found item. -- bwr
                inc_mitm_item_quantity(i, mitm[*obj].quantity);
                destroy_item(*obj);
                *obj = i;
                return;
            }
        }
    }

    assert(*obj != NON_ITEM);

    // Need to actually move object, so first unlink from old position.
    unlink_item(*obj);

    // move item to coord:
    mitm[*obj].x = x;
    mitm[*obj].y = y;

    // link item to top of list.
    mitm[*obj].link = igrd[x][y];
    igrd[x][y] = *obj;
}

void move_item_stack_to_grid(const int x, const int y, const int targ_x, const int targ_y) {
    // Tell all items in stack what the new coordinate is.
    for (auto &item : item_list(x, y)) {
        item.x = static_cast<short>(targ_x);
        item.y = static_cast<short>(targ_y);
    }
    igrd[targ_x][targ_y] = igrd[x][y];
    igrd[x][y] = NON_ITEM;
}

bool copy_item_to_grid(const item_def &item, const int x_plos, const int y_plos, int quant_drop) {
    if (quant_drop == 0) {
        return false;
    }

    // If quant_drop == -1, copy the entire quantity
    if (quant_drop < 0) {
        quant_drop = item.quantity;
    }

    // Try to stack with any existing item
    if (item.is_stackable()) {
        for (auto &item1 : item_list(x_plos, y_plos)) {
            if (items_stack(item, item1)) {
                item1.quantity += quant_drop;
                return true;
            }
        }
    }

    // If item was not found in the current stack, add it to the top
    int new_item = get_item_slot(10);
    if (new_item != NON_ITEM) {
        // copy item
        mitm[new_item] = item;

        // set quantity, and set the item as unlinked
        mitm[new_item].quantity = static_cast<short>(quant_drop);
        mitm[new_item].x = 0;
        mitm[new_item].y = 0;
        mitm[new_item].link = NON_ITEM;

        move_item_to_grid(&new_item, x_plos, y_plos);
    }

    return new_item != NON_ITEM;
}

bool move_top_item(const int src_x, const int src_y, const int dest_x, const int dest_y) {
    auto item = igrd[src_x][src_y];
    if (item != NON_ITEM) {
        move_item_to_grid(&item, dest_x, dest_y);
    }
    return item != NON_ITEM;
}


/**
 * @brief Drop player gold onto the ground.
 * @param amount The amount of gold to drop
 */
static void drop_gold(unsigned int amount) {
    if (you.gold == 0) {
        mpr("You don't have any money.");
        return;
    }

    amount = MINIMUM(amount, you.gold);
    mprf("You drop %d gold piece%s.", amount, amount > 1 ? "s" : "");

    for (auto &item : item_list(you.x_pos, you.y_pos)) {
        if (item.base_type == OBJ_GOLD) {
            item.quantity += amount;
            you.gold -= amount;
            you.redraw_gold = 1;
            return;
        }
    }

    // Place on top of stack.
    auto i = get_item_slot(10);
    if (i == NON_ITEM) {
        mpr("Too many items on this level, not dropping the gold.");
        return;
    }

    mitm[i].base_type = OBJ_GOLD;
    mitm[i].quantity = amount;
    mitm[i].flags = 0;

    move_item_to_grid(&i, you.x_pos, you.y_pos);

    you.gold -= amount;
    you.redraw_gold = 1;
}

void drop() {
    if (inv_count() < 1 && you.gold == 0) {
        canned_msg(MSG_NOTHING_CARRIED);
        return;
    }

    // XXX: Need to handle quantities:
    int quant_drop = -1;
    const int item_dropped = prompt_invent_item("Drop which item?", -1, true, true,
                                          true, '$', &quant_drop);

    if (item_dropped == PROMPT_ABORT || quant_drop == 0) {
        canned_msg( MSG_OK );
    } else if (item_dropped == PROMPT_GOT_SPECIAL) {
        // drop gold
        if (quant_drop < 0 || quant_drop > you.gold) {
            quant_drop = you.gold;
        }

        drop_gold(quant_drop);
        return;
    }

    drop_item(item_dropped, quant_drop);
}

void drop_item(const int item_dropped, int quant_drop) {
    char str_pass[ITEMNAME_SIZE];

    if (quant_drop < 0 || quant_drop > you.inv[item_dropped].quantity) {
        quant_drop = you.inv[item_dropped].quantity;
    }

    if (item_dropped == you.equip[EQ_LEFT_RING]
        || item_dropped == you.equip[EQ_RIGHT_RING]
        || item_dropped == you.equip[EQ_AMULET]) {
        mpr("You will have to take that off first.");
        return;
    }

    if (item_dropped == you.equip[EQ_WEAPON]
        && you.inv[item_dropped].base_type == OBJ_WEAPONS
        && item_cursed(you.inv[item_dropped])) {
        mpr("That object is stuck to you!");
        return;
    }

    for (int i = EQ_CLOAK; i <= EQ_BODY_ARMOUR; i++) {
        if (item_dropped == you.equip[i] && you.equip[i] != -1) {
            if (!Options.easy_armour) {
                mpr("You will have to take that off first.");
            }
            else {
                // If we take off the item, queue up the item being dropped
                if (takeoff_armour(item_dropped)) {
                    start_delay( DELAY_DROP_ITEM, 1, item_dropped, 1 );
                    you.turn_is_over = 0; // turn happens later
                }
            }

            // Regardless, we want to return here because either we're
            // aborting the drop, or the drop is delayed until after
            // the armour is removed. -- bwr
            return;
        }
    }

    // Unwield needs to be done before copy in order to clear things
    // like temporary brands. -- bwr
    if (item_dropped == you.equip[EQ_WEAPON]) {
        unwield_item(item_dropped);
        you.equip[EQ_WEAPON] = -1;
        canned_msg( MSG_EMPTY_HANDED );
    }

    if (!copy_item_to_grid(you.inv[item_dropped], you.x_pos, you.y_pos, quant_drop)) {
        mpr("Too many items on this level, not dropping the item.");
        return;
    }

    quant_name(you.inv[item_dropped], quant_drop, DESC_NOCAP_A, str_pass);
    mprf("You drop %s.", str_pass);

    dec_inv_item_quantity(item_dropped, quant_drop);
    you.turn_is_over = 1;
}

/**
 * @brief Move a monster to an approximate location.
 * @param mon Monster to move
 * @param x X-coordinate to move to
 * @param y Y-coordinate to move to
 * @return True if the monster was moved
 */
static bool shift_monster(monsters *mon, const int x, const int y) {
    bool found_move = false;

    int tx, ty;
    int nx = 0, ny = 0;

    int count = 0;

    if (x == 0 && y == 0) {
        int i;

        // Try and find a random floor space some distance away
        for (i = 0; i < 50; i++) {
            tx = 5 + random2( GXM - 10 );
            ty = 5 + random2( GYM - 10 );

            if (grd[tx][ty] == DNGN_FLOOR && grid_distance(x, y, tx, ty) > 10) {
                break;
            }
        }

        if (i == 50) {
            return false;
        }
    }

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            tx = x + i;
            ty = y + j;

            if (tx < 5 || tx > GXM - 5 || ty < 5 || ty > GXM - 5) {
                continue;
            }

            // Won't drop on anything but vanilla floor right now
            if (grd[tx][ty] != DNGN_FLOOR || mgrd[tx][ty] != NON_MONSTER || (tx == you.x_pos && ty == you.y_pos)) {
                continue;
            }

            count++;
            if (one_chance_in(count)) {
                nx = tx;
                ny = ty;
                found_move = true;
            }
        }
    }

    if (found_move) {
        const int mon_index = mgrd[mon->x][mon->y];
        mgrd[mon->x][mon->y] = NON_MONSTER;
        mgrd[nx][ny] = mon_index;
        mon->x = nx;
        mon->y = ny;
    }

    return found_move;
}

void update_corpses(const double elapsedTime) {
    if (elapsedTime <= 0.0) {
        return;
    }

    const long rot_time = static_cast<long>(elapsedTime / 20.0);
    for (int c = 0; c < MAX_ITEMS; c++) {
        if (mitm[c].quantity < 1) {
            continue;
        }

        if (mitm[c].base_type != OBJ_CORPSES && mitm[c].base_type != OBJ_FOOD) {
            continue;
        }

        if (mitm[c].base_type == OBJ_CORPSES && mitm[c].sub_type > CORPSE_SKELETON) {
            continue;
        }

        if (mitm[c].base_type == OBJ_FOOD && mitm[c].sub_type != FOOD_CHUNK) {
            continue;
        }

        if (rot_time >= mitm[c].special) {
            if (mitm[c].base_type == OBJ_FOOD) {
                destroy_item(c);
            } else {
                if (mitm[c].sub_type == CORPSE_SKELETON || !mons_skeleton( mitm[c].plus )) {
                    destroy_item(c);
                } else {
                    mitm[c].sub_type = CORPSE_SKELETON;
                    mitm[c].special = 200;
                    mitm[c].colour = LIGHTGREY;
                }
            }
        } else {
            ASSERT(rot_time < 256);
            mitm[c].special -= rot_time;
        }
    }

    int fountain_checks = (int)(elapsedTime / 1000.0);
    if (random2(1000) < (int)(elapsedTime) % 1000)
        fountain_checks += 1;

    // Dry fountains may start flowing again
    if (fountain_checks > 0) {
        for (auto cx = 0; cx < GXM; cx++) {
            for (auto cy = 0; cy < GYM; cy++) {
                if (grd[cx][cy] > DNGN_SPARKLING_FOUNTAIN && grd[cx][cy] < DNGN_PERMADRY_FOUNTAIN) {
                    for (int i=0; i<fountain_checks; i++) {
                        if (one_chance_in(100)) {
                            if (grd[cx][cy] > DNGN_SPARKLING_FOUNTAIN) {
                                grd[cx][cy]--;
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool remove_enchant_levels(monsters *mon, const int slot, const int min, const int levels) {
    const auto new_level = mon->enchantment[slot] - levels;
    if (new_level < min) {
        mons_del_ench(mon, mon->enchantment[slot], mon->enchantment[slot], true);
        return true;
    }
    mon->enchantment[slot] = new_level;
    return false;
}

//---------------------------------------------------------------
//
// update_enchantments
//
// Update a monster's enchantments when the player returns
// to the level.
//
// Management for enchantments... problems with this are the oddities
// (monster dying from poison several thousands of turns later), and
// game balance.
//
// Consider: Poison/Sticky Flame a monster at range and leave, monster
// dies but can't leave level to get to player (implied game balance of
// the delayed damage is that the monster could be a danger before
// it dies).  This could be fixed by keeping some monsters active
// off level and allowing them to take stairs (a very serious change).
//
// Compare this to the current abuse where the player gets
// effectively extended duration of these effects (although only
// the actual effects only occur on level, the player can leave
// and heal up without having the effect disappear).
//
// This is a simple compromise between the two... the enchantments
// go away, but the effects don't happen off level.  -- bwr
//
//---------------------------------------------------------------
static void update_enchantments(monsters *mon, const int levels) {
    for (auto i = 0; i < mon->enchantment.size(); i++) {
        switch (mon->enchantment[i]) {
            case ENCH_YOUR_POISON_I:
            case ENCH_YOUR_POISON_II:
            case ENCH_YOUR_POISON_III:
            case ENCH_YOUR_POISON_IV:
                remove_enchant_levels(mon, i, ENCH_YOUR_POISON_I, levels);
                break;

            case ENCH_YOUR_SHUGGOTH_I:
            case ENCH_YOUR_SHUGGOTH_II:
            case ENCH_YOUR_SHUGGOTH_III:
            case ENCH_YOUR_SHUGGOTH_IV:
                remove_enchant_levels(mon, i, ENCH_YOUR_SHUGGOTH_I, levels);
                break;

            case ENCH_YOUR_ROT_I:
            case ENCH_YOUR_ROT_II:
            case ENCH_YOUR_ROT_III:
            case ENCH_YOUR_ROT_IV:
                remove_enchant_levels(mon, i, ENCH_YOUR_ROT_I, levels);
                break;

            case ENCH_BACKLIGHT_I:
            case ENCH_BACKLIGHT_II:
            case ENCH_BACKLIGHT_III:
            case ENCH_BACKLIGHT_IV:
                remove_enchant_levels(mon, i, ENCH_BACKLIGHT_I, levels);
                break;

            case ENCH_YOUR_STICKY_FLAME_I:
            case ENCH_YOUR_STICKY_FLAME_II:
            case ENCH_YOUR_STICKY_FLAME_III:
            case ENCH_YOUR_STICKY_FLAME_IV:
                remove_enchant_levels(mon, i, ENCH_YOUR_STICKY_FLAME_I, levels);
                break;

            case ENCH_POISON_I:
            case ENCH_POISON_II:
            case ENCH_POISON_III:
            case ENCH_POISON_IV:
                remove_enchant_levels(mon, i, ENCH_POISON_I, levels);
                break;

            case ENCH_STICKY_FLAME_I:
            case ENCH_STICKY_FLAME_II:
            case ENCH_STICKY_FLAME_III:
            case ENCH_STICKY_FLAME_IV:
                remove_enchant_levels(mon, i, ENCH_STICKY_FLAME_I, levels);
                break;

            case ENCH_FRIEND_ABJ_I:
            case ENCH_FRIEND_ABJ_II:
            case ENCH_FRIEND_ABJ_III:
            case ENCH_FRIEND_ABJ_IV:
            case ENCH_FRIEND_ABJ_V:
            case ENCH_FRIEND_ABJ_VI:
                if (remove_enchant_levels(mon, i, ENCH_FRIEND_ABJ_I, levels)) {
                    monster_die(mon, KILL_RESET, 0);
                }
                break;

            case ENCH_ABJ_I:
            case ENCH_ABJ_II:
            case ENCH_ABJ_III:
            case ENCH_ABJ_IV:
            case ENCH_ABJ_V:
            case ENCH_ABJ_VI:
                if (remove_enchant_levels(mon, i, ENCH_ABJ_I, levels)) {
                    monster_die(mon, KILL_RESET, 0);
                }
                break;


            case ENCH_SHORT_LIVED:
                monster_die(mon, KILL_RESET, 0);
                break;

            case ENCH_TP_I:
            case ENCH_TP_II:
            case ENCH_TP_III:
            case ENCH_TP_IV:
                monster_teleport(mon, true);
                break;

            case ENCH_CONFUSION:
                monster_blink(mon);
                break;

            case ENCH_GLOWING_SHAPESHIFTER:
            case ENCH_SHAPESHIFTER:
            case ENCH_CREATED_FRIENDLY:
            case ENCH_SUBMERGED:
            default:
                // don't touch these
                break;

            case ENCH_SLOW:
            case ENCH_HASTE:
            case ENCH_FEAR:
            case ENCH_INVIS:
            case ENCH_CHARM:
            case ENCH_SLEEP_WARY:
                // delete enchantment (using function to get this done cleanly)
                mons_del_ench(mon, mon->enchantment[i], mon->enchantment[i], true);
                break;
        }
    }
}

void update_level(const double elapsedTime) {
    int turns = (int) (elapsedTime / 10.0);

#if DEBUG_DIAGNOSTICS
    int mons_total = 0;

    snprintf( info, INFO_SIZE, "turns: %d", turns );
    mpr( info, MSGCH_DIAGNOSTICS );
#endif

    update_corpses( elapsedTime );

    for (int m = 0; m < MAX_MONSTERS; m++) {
        monsters *mon = &menv[m];

        if (mon->type == -1)
            continue;

#if DEBUG_DIAGNOSTICS
        mons_total++;
#endif

        // following monsters don't get movement
        if (mon->flags & MF_JUST_SUMMONED)
            continue;

        // XXX: Allow some spellcasting (like Healing and Teleport)? -- bwr
        // const bool healthy = (mon->hit_points * 2 > mon->max_hit_points);

        // This is the monster healing code, moved here from tag.cc:
        if (monster_descriptor( mon->type, MDSC_REGENERATES )
            || mon->type == MONS_PLAYER_GHOST)
        {
            heal_monster( mon, turns, false );
        }
        else
        {
            heal_monster( mon, (turns / 10), false );
        }

        if (turns >= 10)
            update_enchantments( mon, turns / 10 );

        // Don't move water or lava monsters around
        if (monster_habitat( mon->type ) != DNGN_FLOOR)
            continue;

        // Let sleeping monsters lie
        if (mon->behaviour == BEH_SLEEP)
            continue;


        const int range = (turns * mon->speed) / 10;
        const int moves = (range > 50) ? 50 : range;

        // const bool short_time = (range >= 5 + random2(10));
        const bool long_time  = (range >= (500 + roll_dice( 2, 500 )));

        const bool ranged_attack = (mons_has_ranged_spell( mon )
                                    || mons_has_ranged_attack( mon ));

#if DEBUG_DIAGNOSTICS
        // probably too annoying even for DEBUG_DIAGNOSTICS
        snprintf( info, INFO_SIZE,
                  "mon #%d: range %d; long %d; pos (%d,%d); targ %d(%d,%d); flags %d",
                  m, range, long_time, mon->x, mon->y,
                  mon->foe, mon->target_x, mon->target_y, mon->flags );

        mpr( info, MSGCH_DIAGNOSTICS );
#endif

        if (range <= 0)
            continue;

        if (long_time
            && (mon->behaviour == BEH_FLEE
                || mon->behaviour == BEH_CORNERED
                || testbits( mon->flags, MF_BATTY )
                || ranged_attack
                || coinflip()))
        {
            if (mon->behaviour != BEH_WANDER)
            {
                mon->behaviour = BEH_WANDER;
                mon->foe = MHITNOT;
                mon->target_x = 10 + random2( GXM - 10 );
                mon->target_y = 10 + random2( GYM - 10 );
            }
            else
            {
                // monster will be sleeping after we move it
                mon->behaviour = BEH_SLEEP;
            }
        }
        else if (ranged_attack)
        {
            // if we're doing short time movement and the monster has a
            // ranged attack (missile or spell), then the monster will
            // flee to gain distance if its "too close", else it will
            // just shift its position rather than charge the player. -- bwr
            if (grid_distance(mon->x, mon->y, mon->target_x, mon->target_y) < 3)
            {
                mon->behaviour = BEH_FLEE;

                // if the monster is on the target square, fleeing won't work
                if (mon->x == mon->target_x && mon->y == mon->target_y)
                {
                    if (you.x_pos != mon->x || you.y_pos != mon->y)
                    {
                        // flee from player's position if different
                        mon->target_x = you.x_pos;
                        mon->target_y = you.y_pos;
                    }
                    else
                    {
                        // randomize the target so we have a direction to flee
                        mon->target_x += (random2(3) - 1);
                        mon->target_y += (random2(3) - 1);
                    }
                }

#if DEBUG_DIAGNOSTICS
                mpr( "backing off...", MSGCH_DIAGNOSTICS );
#endif
            }
            else
            {
                shift_monster( mon, mon->x, mon->y );

#if DEBUG_DIAGNOSTICS
                snprintf(info, INFO_SIZE, "shifted to (%d,%d)", mon->x, mon->y);
                mpr( info, MSGCH_DIAGNOSTICS );
#endif
                continue;
            }
        }

        int pos_x = mon->x, pos_y = mon->y;

        // dirt simple movement:
        for (int i = 0; i < moves; i++) {
            int mx = (pos_x > mon->target_x) ? -1 :
                     (pos_x < mon->target_x) ?  1
                                             :  0;

            int my = (pos_y > mon->target_y) ? -1 :
                     (pos_y < mon->target_y) ?  1
                                             :  0;

            if (mon->behaviour == BEH_FLEE)
            {
                mx *= -1;
                my *= -1;
            }

            if (pos_x + mx < 0 || pos_x + mx >= GXM)
                mx = 0;

            if (pos_y + my < 0 || pos_y + my >= GXM)
                my = 0;

            if (mx == 0 && my == 0)
                break;

            if (grd[pos_x + mx][pos_y + my] < DNGN_FLOOR)
                break;

            pos_x += mx;
            pos_y += my;
        }

        if (!shift_monster( mon, pos_x, pos_y ))
            shift_monster( mon, mon->x, mon->y );

#if DEBUG_DIAGNOSTICS
        snprintf( info, INFO_SIZE, "moved to (%d,%d)", mon->x, mon->y );
        mpr( info, MSGCH_DIAGNOSTICS );
#endif
    }

#if DEBUG_DIAGNOSTICS
    snprintf( info, INFO_SIZE, "total monsters on level = %d", mons_total );
    mpr( info, MSGCH_DIAGNOSTICS );
#endif

    for (int i = 0; i < MAX_CLOUDS; i++)
        delete_cloud(i);
}

constexpr const char *HELL_MESSAGES[] = {
    "\"You will not leave this place.\"",
    "\"Die, mortal!\"",
    "\"We do not forgive those who trespass against us!\"",
    "\"Trespassers are not welcome here!\"",
    "\"You do not belong in this place!\"",
    "\"Leave now, before it is too late!\"",
    "\"We have you now!\"",
    "You feel a terrible foreboding...",
    "You hear words spoken in a strange and terrible language...",
    nullptr,
    "Something frightening happens.",
    "You sense an ancient evil watching you...",
    "You feel lost and a long, long way from home...",
    "You suddenly feel all small and vulnerable.",
    "A gut-wrenching scream fills the air!",
    "You shiver with fear.",
    "You sense a hostile presence.",
    "You hear diabolical laughter!",
};

void handle_time(long time_delta) {
    int temp_rand; // probability determination {dlb}

    // so as not to reduplicate f(x) calls {dlb}
    unsigned char which_miscast = SPTYP_RANDOM;

    int which_beastie = MONS_PROGRAM_BUG; // error trapping {dlb}
    unsigned char i; // loop variable {dlb}
    bool new_rotting_item = false; //mv: becomes true when some new item becomes rotting

    // BEGIN - Nasty things happen to people who spend too long in Hell:
    if (player_in_hell() && coinflip()) {
        temp_rand = random2(array_size(HELL_MESSAGES));
        auto msg = HELL_MESSAGES[temp_rand];
        if (msg == nullptr) {
            msg = you.species != SP_MUMMY ? "You smell brimstone." : "Brimstone rains from above.";
        }
        mprf2(MSGCH_TALK, 0, msg);

        temp_rand = random2(27);

        if (temp_rand > 17) // 9 in 27 odds {dlb}
        {
            temp_rand = random2(8);

            if (temp_rand > 3) // 4 in 8 odds {dlb}
                which_miscast = SPTYP_NECROMANCY;
            else if (temp_rand > 1) // 2 in 8 odds {dlb}
                which_miscast = SPTYP_SUMMONING;
            else if (temp_rand > 0) // 1 in 8 odds {dlb}
                which_miscast = SPTYP_CONJURATION;
            else // 1 in 8 odds {dlb}
                which_miscast = SPTYP_ENCHANTMENT;

            miscast_effect(which_miscast, 4 + random2(6), random2avg(97, 3),
                           100, "the effects of Hell");
        } else if (temp_rand > 7) // 10 in 27 odds {dlb}
        {
            // 60:40 miscast:summon split {dlb}
            auto summon_instead = random2(5) > 2;

            switch (you.where_are_you) {
                case BRANCH_DIS:
                    if (summon_instead)
                        which_beastie = summon_any_demon(DEMON_GREATER);
                    else
                        which_miscast = SPTYP_EARTH;
                    break;
                case BRANCH_GEHENNA:
                    if (summon_instead)
                        which_beastie = MONS_FIEND;
                    else
                        which_miscast = SPTYP_FIRE;
                    break;
                case BRANCH_COCYTUS:
                    if (summon_instead)
                        which_beastie = MONS_ICE_FIEND;
                    else
                        which_miscast = SPTYP_ICE;
                    break;
                case BRANCH_TARTARUS:
                    if (summon_instead)
                        which_beastie = MONS_SHADOW_FIEND;
                    else
                        which_miscast = SPTYP_NECROMANCY;
                    break;
                default: // this is to silence gcc compiler warnings {dlb}
                    if (summon_instead)
                        which_beastie = MONS_FIEND;
                    else
                        which_miscast = SPTYP_NECROMANCY;
                    break;
            }

            if (summon_instead) {
                create_monster(which_beastie, 0, BEH_HOSTILE, you.x_pos,
                               you.y_pos, MHITYOU, 250);
            } else {
                miscast_effect(which_miscast, 4 + random2(6),
                               random2avg(97, 3), 100, "the effects of Hell");
            }
        }

        // NB: no "else" - 8 in 27 odds that nothing happens through
        // first chain {dlb}
        // also note that the following is distinct from and in
        // addition to the above chain:

        // try to summon at least one and up to five random monsters {dlb}
        if (one_chance_in(3)) {
            create_monster(RANDOM_MONSTER, 0, BEH_HOSTILE,
                           you.x_pos, you.y_pos, MHITYOU, 250);

            for (i = 0; i < 4; i++) {
                if (one_chance_in(3)) {
                    create_monster(RANDOM_MONSTER, 0, BEH_HOSTILE,
                                   you.x_pos, you.y_pos, MHITYOU, 250);
                }
            }
        }
    }
    // END - special Hellish things...

    // Adjust the player's stats if s/he's diseased (or recovering).
    if (!you.disease) {
        if (you.strength < you.max_strength && one_chance_in(100)) {
            mpr("You feel your strength returning.", MSGCH_RECOVERY);
            you.strength++;
            you.redraw_strength = 1;
        }

        if (you.dex < you.max_dex && one_chance_in(100)) {
            mpr("You feel your dexterity returning.", MSGCH_RECOVERY);
            you.dex++;
            you.redraw_dexterity = 1;
        }

        if (you.intel < you.max_intel && one_chance_in(100)) {
            mpr("You feel your intelligence returning.", MSGCH_RECOVERY);
            you.intel++;
            you.redraw_intelligence = 1;
        }
    } else {
        if (one_chance_in(30)) {
            mpr("Your disease is taking its toll.", MSGCH_WARN);
            lose_stat(STAT_RANDOM, 1);
        }
    }

    // Adjust the player's stats if s/he has the deterioration mutation
    if (you.mutation[MUT_DETERIORATION]
        && random2(200) <= you.mutation[MUT_DETERIORATION] * 5 - 2) {
        lose_stat(STAT_RANDOM, 1);
    }

    int added_contamination = 0;

    // Account for mutagenic radiation.  Invis and haste will give the
    // player about .1 points per turn,  mutagenic randarts will give
    // about 1.5 points on average,  so they can corrupt the player
    // quite quickly.  Wielding one for a short battle is OK,  which is
    // as things should be.   -- GDL
    if (you.invis && random2(10) < 6)
        added_contamination++;

    if (you.haste && !you.berserker && random2(10) < 6)
        added_contamination++;

    // randarts are usually about 20x worse than running around invisible
    // or hasted.. this seems OK.
    added_contamination += random2(1 + scan_randarts(RAP_MUTAGENIC));

    // we take off about .5 points per turn
    if (!you.invis && !you.haste && coinflip())
        added_contamination -= 1;

    contaminate_player(added_contamination);

    // only check for badness once every other turn
    if (coinflip()) {
        if (you.magic_contamination >= 5
            /* && random2(150) <= you.magic_contamination */) {
            mpr("Your body shudders with the violent release of wild energies!", MSGCH_WARN);

            // for particularly violent releases,  make a little boom
            if (you.magic_contamination > 25 && one_chance_in(3)) {
                struct bolt boom;
                boom.type = SYM_BURST;
                boom.colour = BLACK;
                boom.flavour = BEAM_RANDOM;
                boom.target_x = you.x_pos;
                boom.target_y = you.y_pos;
                boom.damage = dice_def(3, (you.magic_contamination / 2));
                boom.thrower = KILL_MISC;
                boom.aux_source = "a magical explosion";
                boom.beam_source = NON_MONSTER;
                boom.isBeam = false;
                boom.isTracer = false;
                strcpy(boom.beam_name, "magical storm");

                boom.ench_power = (you.magic_contamination * 5);
                boom.ex_size = (you.magic_contamination / 15);
                if (boom.ex_size > 9)
                    boom.ex_size = 9;

                explosion(boom);
            }

            // we want to warp the player,  not do good stuff!
            if (one_chance_in(5))
                mutate(100);
            else
                give_bad_mutation(coinflip());

            // we're meaner now,  what with explosions and whatnot,  but
            // we dial down the contamination a little faster if its actually
            // mutating you.  -- GDL
            contaminate_player(-(random2(you.magic_contamination / 4) + 1));
        }
    }

    // Random chance to identify staff in hand based off of Spellcasting
    // and an appropriate other spell skill... is 1/20 too fast?
    if (you.equip[EQ_WEAPON] != -1
        && you.inv[you.equip[EQ_WEAPON]].base_type == OBJ_STAVES
        && item_not_ident(you.inv[you.equip[EQ_WEAPON]], ISFLAG_KNOW_TYPE)
        && one_chance_in(20)) {
        int total_skill = you.skills[SK_SPELLCASTING];

        switch (you.inv[you.equip[EQ_WEAPON]].sub_type) {
            case STAFF_WIZARDRY:
            case STAFF_ENERGY:
                total_skill += you.skills[SK_SPELLCASTING];
                break;
            case STAFF_FIRE:
                if (you.skills[SK_FIRE_MAGIC] > you.skills[SK_ICE_MAGIC])
                    total_skill += you.skills[SK_FIRE_MAGIC];
                else
                    total_skill += you.skills[SK_ICE_MAGIC];
                break;
            case STAFF_COLD:
                if (you.skills[SK_ICE_MAGIC] > you.skills[SK_FIRE_MAGIC])
                    total_skill += you.skills[SK_ICE_MAGIC];
                else
                    total_skill += you.skills[SK_FIRE_MAGIC];
                break;
            case STAFF_AIR:
                if (you.skills[SK_AIR_MAGIC] > you.skills[SK_EARTH_MAGIC])
                    total_skill += you.skills[SK_AIR_MAGIC];
                else
                    total_skill += you.skills[SK_EARTH_MAGIC];
                break;
            case STAFF_EARTH:
                if (you.skills[SK_EARTH_MAGIC] > you.skills[SK_AIR_MAGIC])
                    total_skill += you.skills[SK_EARTH_MAGIC];
                else
                    total_skill += you.skills[SK_AIR_MAGIC];
                break;
            case STAFF_POISON:
                total_skill += you.skills[SK_POISON_MAGIC];
                break;
            case STAFF_DEATH:
                total_skill += you.skills[SK_NECROMANCY];
                break;
            case STAFF_CONJURATION:
                total_skill += you.skills[SK_CONJURATIONS];
                break;
            case STAFF_ENCHANTMENT:
                total_skill += you.skills[SK_ENCHANTMENTS];
                break;
            case STAFF_SUMMONING:
                total_skill += you.skills[SK_SUMMONINGS];
                break;
            default: ;
        }

        if (random2(100) < total_skill) {
            set_ident_flags(you.inv[you.equip[EQ_WEAPON]], ISFLAG_IDENT_MASK);

            char str_pass[ITEMNAME_SIZE];
            in_name(you.equip[EQ_WEAPON], DESC_NOCAP_A, str_pass);
            snprintf(info, INFO_SIZE, "You are wielding %s.", str_pass);
            mpr(info);
            more();

            you.wield_change = true;
        }
    }

    // Check to see if an upset god wants to do something to the player
    // jmf: moved huge thing to religion.cc
    handle_god_time();

    // If the player has the lost mutation forget portions of the map
    if (you.mutation[MUT_LOST]) {
        if (random2(100) <= you.mutation[MUT_LOST] * 5)
            forget_map(5 + random2(you.mutation[MUT_LOST] * 10));
    }

    // Update all of the corpses and food chunks on the floor
    update_corpses(time_delta);

    // Update all of the corpses and food chunks in the player's
    // inventory {should be moved elsewhere - dlb}


    for (i = 0; i < ENDOFPACK; i++) {
        if (you.inv[i].quantity < 1)
            continue;

        if (you.inv[i].base_type != OBJ_CORPSES && you.inv[i].base_type != OBJ_FOOD)
            continue;

        if (you.inv[i].base_type == OBJ_CORPSES
            && you.inv[i].sub_type > CORPSE_SKELETON) {
            continue;
        }

        if (you.inv[i].base_type == OBJ_FOOD && you.inv[i].sub_type != FOOD_CHUNK)
            continue;

        if ((time_delta / 20) >= you.inv[i].special) {
            if (you.inv[i].base_type == OBJ_FOOD) {
                if (you.equip[EQ_WEAPON] == i) {
                    unwield_item(you.equip[EQ_WEAPON]);
                    you.equip[EQ_WEAPON] = -1;
                    you.wield_change = true;
                }

                mpr("Your equipment suddenly weighs less.", MSGCH_ROTTEN_MEAT);
                you.inv[i].quantity = 0;
                burden_change();
                continue;
            }

            if (you.inv[i].sub_type == CORPSE_SKELETON)
                continue; // carried skeletons are not destroyed

            if (!mons_skeleton(you.inv[i].plus)) {
                if (you.equip[EQ_WEAPON] == i) {
                    unwield_item(you.equip[EQ_WEAPON]);
                    you.equip[EQ_WEAPON] = -1;
                }

                you.inv[i].quantity = 0;
                burden_change();
                continue;
            }

            you.inv[i].sub_type = 1;
            you.inv[i].special = 0;
            you.inv[i].colour = LIGHTGREY;
            you.wield_change = true;
            continue;
        }

        you.inv[i].special -= (time_delta / 20);

        if (you.inv[i].special < 100 && (you.inv[i].special + (time_delta / 20) >= 100)) {
            new_rotting_item = true;
        }
    }

    //mv: messages when chunks/corpses become rotten
    if (new_rotting_item) {
        switch (you.species) {
            // XXX: should probably still notice?
            case SP_MUMMY: // no smell
            case SP_TROLL: // stupid, living in mess - doesn't care about it
                break;

            case SP_GHOUL: //likes it
                temp_rand = random2(8);
                mpr(((temp_rand < 5)
                         ? "You smell something rotten."
                         : (temp_rand == 5)
                               ? "Smell of rotting flesh makes you more hungry."
                               : (temp_rand == 6)
                                     ? "You smell decay. Yum-yum."
                                     : "Wow! There is something tasty in your inventory."),
                    MSGCH_ROTTEN_MEAT);
                break;

            case SP_KOBOLD: //mv: IMO these race aren't so "touchy"
            case SP_OGRE:
            case SP_MINOTAUR:
            case SP_HILL_ORC:
                temp_rand = random2(8);
                mpr(((temp_rand < 5)
                         ? "You smell something rotten."
                         : (temp_rand == 5)
                               ? "You smell rotting flesh."
                               : (temp_rand == 6)
                                     ? "You smell decay."
                                     : "There is something rotten in your inventory."),
                    MSGCH_ROTTEN_MEAT);
                break;

            default:
                temp_rand = random2(8);
                mpr(((temp_rand < 5)
                         ? "You smell something rotten."
                         : (temp_rand == 5)
                               ? "Smell of rotting flesh makes you sick."
                               : (temp_rand == 6)
                                     ? "You smell decay. Yuk..."
                                     : "Ugh! There is something really disgusting in your inventory."),
                    MSGCH_ROTTEN_MEAT);
                break;
        }
    }

    // exercise armour *xor* stealth skill: {dlb}
    if (!player_light_armour()) {
        if (random2(1000) <= mass_item(you.inv[you.equip[EQ_BODY_ARMOUR]])) {
            return;
        }

        if (one_chance_in(6)) // lowered random roll from 7 to 6 -- bwross
            exercise(SK_ARMOUR, 1);
    } else // exercise stealth skill:
    {
        if (you.burden_state != BS_UNENCUMBERED || you.berserker)
            return;

        if (you.special_wield == SPWLD_SHADOW)
            return;

        if (you.equip[EQ_BODY_ARMOUR] != -1
            && random2(mass_item(you.inv[you.equip[EQ_BODY_ARMOUR]])) >= 100) {
            return;
        }

        if (one_chance_in(18))
            exercise(SK_STEALTH, 1);
    }
}

int autopickup_on = 1;

static void autopickup() {
    //David Loewenstern 6/99
    int result, o, next;
    bool did_pickup = false;

    if (autopickup_on == 0 || Options.autopickups == 0L)
        return;

    if (you.attribute[ATTR_TRANSFORMATION] == TRAN_AIR
        && you.duration[DUR_TRANSFORMATION] > 0)
    {
        return;
    }

    if (player_is_levitating() && !wearing_amulet(AMU_CONTROLLED_FLIGHT))
        return;

    o = igrd[you.x_pos][you.y_pos];

    while (o != NON_ITEM)
    {
        next = mitm[o].link;

        if (Options.autopickups & (1L << mitm[o].base_type))
        {
            result = move_item_to_player( o, mitm[o].quantity);

            if (result == 0)
            {
                mpr("You can't carry any more.");
                break;
            }
            else if (result == -1)
            {
                mpr("Your pack is full.");
                break;
            }

            did_pickup = true;
        }

        o = next;
    }

    // move_item_to_player should leave things linked. -- bwr
    // relink_cell(you.x_pos, you.y_pos);

    if (did_pickup)
    {
        you.turn_is_over = 1;
        start_delay( DELAY_AUTOPICKUP, 1 );
    }
    }

int inv_count() {
    auto count = 0;
    for (auto item : you.inv) {
        if (item.valid()) {
            count++;
        }
    }
    return count;
}

#ifdef ALLOW_DESTROY_ITEM_COMMAND
// Started with code from AX-crawl, although its modified to fix some
// serious problems.  -- bwr
//
// Issues to watch for here:
// - no destroying things from the ground since that includes corpses
//   which might be animated by monsters (butchering takes a few turns).
//   This code provides a quicker way to get rid of a corpse, but
//   the player has to be able to lift it first... something that was
//   a valid preventative method before (although this allow the player
//   to get rid of the mass on the next action).
//
// - artefacts can be destroyed
//
// - equipment cannot be destroyed... not only is this the more accurate
//   than testing for curse status (to prevent easy removal of cursed items),
//   but the original code would leave all the equiped items properties
//   (including weight) which would cause a bit of a mess to state.
//
// - no item does anything for just carrying it... if that changes then
//   this code will have to deal with that.
//
// - Do we want the player to be able to remove items from the game?
//   This would make things considerably easier to keep weapons (esp
//   those of distortion) from falling into the hands of monsters.
//   Right now the player has to carry them to a safe area, or otherwise
//   ingeniously dispose of them... do we care about this gameplay aspect?
//
// - Prompt for number to destroy?
//
void cmd_destroy_item( void )
{
    int i;
    char str_pass[ ITEMNAME_SIZE ];

    // ask the item to destroy
    int item = prompt_invent_item( "Destroy which item? ", -1, true, false );
    if (item == PROMPT_ABORT)
        return;

    // Used to check for cursed... but that's not the real problem -- bwr
    for (i = 0; i < NUM_EQUIP; i++)
    {
        if (you.equip[i] == item)
        {
            mesclr( true );
            mpr( "You cannot destroy equipped items!" );
            return;
        }
    }

    // ask confirmation
    // quant_name(you.inv[item], you.inv[item].quantity, DESC_NOCAP_A, str_pass );
    item_name( you.inv[item], DESC_NOCAP_THE, str_pass );
    snprintf( info, INFO_SIZE, "Destroy %s? ", str_pass );

    if (yesno( info, true ))
    {
       //destroy it!!
        snprintf( info, INFO_SIZE, "You destroy %s.", str_pass );
        mpr( info );
        dec_inv_item_quantity( item, you.inv[item].quantity );
        burden_change();
    }
}
#endif
