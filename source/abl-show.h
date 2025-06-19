// Functions related to special abilities
#ifndef ABLSHOW_H
#define ABLSHOW_H

#include "string-compat.h"

// Structure for representing an ability:
struct ability_def {
    int          ability;
    const char * name;
    unsigned int mp_cost;    // magic cost of ability
    unsigned int hp_cost;    // hit point cost of ability
    unsigned int food_cost;  // + rand2avg( food_cost, 2 )
    unsigned int piety_cost; // + random2( (piety_cost + 1) / 2 + 1 )
    unsigned int flags;      // used for additonal cost notices
};

enum ABILITY_FLAGS {
    ABFLAG_NONE         = 0x00000000,
    ABFLAG_BREATH       = 0x00000001, // ability uses DUR_BREATH_WEAPON
    ABFLAG_DELAY        = 0x00000002, // ability has its own delay (ie glamour)
    ABFLAG_PAIN         = 0x00000004, // ability must hurt player (ie torment)
    ABFLAG_EXHAUSTION   = 0x00000008, // fails if you.exhausted
    ABFLAG_INSTANT      = 0x00000010, // doesn't take time to use
    ABFLAG_PERMANENT_HP = 0x00000020, // costs permanent HPs
    ABFLAG_PERMANENT_MP = 0x00000040  // costs permanent MPs
};

/**
 * @brief Retrieve the ability definition given its ID.
 * @param ability_id ID of ability to find
 * @return The ability, or ABILITY_NO_ABILITY if no match
 */
const ability_def &get_ability_def(int ability_id);

/**
 * @brief Retrieve the name of an ability given its index.
 * @param index Index in ability list
 * @return Name of ability
 */
const char *get_ability_name_by_index(char index);

/**
 * @brief Compute the cost description for an ability
 * @param abil Ability to use
 * @return A description of the ability's cost
 */
const string make_cost_description(const ability_def &abil);

/**
* @brief Activates a menu which gives player access to all of their non-spell
         special abilities - Eg naga's spit poison, or the Invocations you get
         from worshipping. Generated dynamically - the function checks to see which
         abilities you have every time.
 * @return True if an ability was activated
 */
bool activate_ability();

/**
 * @brief Lists any abilities the player may possess.
 * @return
 */
char show_abilities();

/**
 * @brief Determine the player's current ability set.
 * @return True if the player has any abilities
 */
bool generate_abilities();

/**
 * @brief Assign abilities to the player based on their religion.
 */
void set_god_ability_slots();

#endif
