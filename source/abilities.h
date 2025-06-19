// GENERATED, DO NOT EDIT! See scripts/gen-abilities.py
#ifndef ABILITIES_H
#define ABILITIES_H

#include "enum.h"

// Structure for representing an ability:
struct ability_def {
    int                 ability;
    const char *        name;
    unsigned int        mp_cost;        // magic cost of ability
    unsigned int        hp_cost;        // hit point cost of ability
    unsigned int        food_cost;      // + rand2avg( food_cost, 2 )
    unsigned int        piety_cost;     // + random2( (piety_cost + 1) / 2 + 1 )
    unsigned int        flags;          // used for additonal cost notices
};

constexpr ability_def Ability_List[] = {
    { ABIL_NON_ABILITY, "No ability", 0, 0, 0, 0, ABFLAG_NONE },
    { ABIL_SPIT_POISON, "Spit Poison", 0, 0, 40, 0, ABFLAG_BREATH },
    { ABIL_GLAMOUR, "Glamour", 5, 0, 40, 0, ABFLAG_DELAY },
    { ABIL_MAPPING, "Sense Surroundings", 0, 0, 30, 0, ABFLAG_NONE },
    { ABIL_TELEPORTATION, "Teleportation", 3, 0, 200, 0, ABFLAG_NONE },
    { ABIL_BLINK, "Blink", 1, 0, 50, 0, ABFLAG_NONE },
    { ABIL_BREATHE_FIRE, "Breathe Fire", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_FROST, "Breathe Frost", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_POISON, "Breathe Poison Gas", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_LIGHTNING, "Breathe Lightning", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_POWER, "Breathe Power", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_STICKY_FLAME, "Breathe Sticky Flame", 0, 0, 125, 0, ABFLAG_BREATH },
    { ABIL_BREATHE_STEAM, "Breathe Steam", 0, 0, 75, 0, ABFLAG_BREATH },
    { ABIL_SPIT_ACID, "Spit Acid", 0, 0, 125, 0, ABFLAG_NONE },
    { ABIL_FLY, "Fly", 3, 0, 100, 0, ABFLAG_NONE },
    { ABIL_SUMMON_MINOR_DEMON, "Summon Minor Demon", 3, 3, 75, 0, ABFLAG_NONE },
    { ABIL_SUMMON_DEMON, "Summon Demon", 5, 5, 150, 0, ABFLAG_NONE },
    { ABIL_HELLFIRE, "Hellfire", 8, 8, 200, 0, ABFLAG_NONE },
    { ABIL_TORMENT, "Torment", 9, 0, 250, 0, ABFLAG_PAIN },
    { ABIL_RAISE_DEAD, "Raise Dead", 5, 5, 150, 0, ABFLAG_NONE },
    { ABIL_CONTROL_DEMON, "Control Demon", 4, 4, 100, 0, ABFLAG_NONE },
    { ABIL_TO_PANDEMONIUM, "Gate Yourself to Pandemonium", 7, 0, 200, 0, ABFLAG_NONE },
    { ABIL_CHANNELING, "Channeling", 1, 0, 30, 0, ABFLAG_NONE },
    { ABIL_THROW_FLAME, "Throw Flame", 1, 1, 50, 0, ABFLAG_NONE },
    { ABIL_THROW_FROST, "Throw Frost", 1, 1, 50, 0, ABFLAG_NONE },
    { ABIL_BOLT_OF_DRAINING, "Bolt of Draining", 4, 4, 100, 0, ABFLAG_NONE },
    { ABIL_FLY_II, "Fly", 0, 0, 25, 0, ABFLAG_NONE },
    { ABIL_DELAYED_FIREBALL, "Release Delayed Fireball", 0, 0, 0, 0, ABFLAG_INSTANT },
    { ABIL_MUMMY_RESTORATION, "Restoration", 1, 0, 0, 0, ABFLAG_PERMANENT_MP },
    { ABIL_EVOKE_MAPPING, "Evoke Sense Surroundings", 0, 0, 30, 0, ABFLAG_NONE },
    { ABIL_EVOKE_TELEPORTATION, "Evoke Teleportation", 3, 0, 200, 0, ABFLAG_NONE },
    { ABIL_EVOKE_BLINK, "Evoke Blink", 1, 0, 50, 0, ABFLAG_NONE },
    { ABIL_EVOKE_BERSERK, "Evoke Berserk Rage", 0, 0, 0, 0, ABFLAG_NONE },
    { ABIL_EVOKE_TURN_INVISIBLE, "Evoke Invisibility", 2, 0, 250, 0, ABFLAG_NONE },
    { ABIL_EVOKE_TURN_VISIBLE, "Turn Visible", 0, 0, 0, 0, ABFLAG_NONE },
    { ABIL_EVOKE_LEVITATE, "Evoke Levitation", 1, 0, 100, 0, ABFLAG_NONE },
    { ABIL_EVOKE_STOP_LEVITATING, "Stop Levitating", 0, 0, 0, 0, ABFLAG_NONE },
    { ABIL_END_TRANSFORMATION, "End Transformation", 0, 0, 0, 0, ABFLAG_NONE },
    { ABIL_ZIN_REPEL_UNDEAD, "Repel Undead", 1, 0, 100, 0, ABFLAG_NONE },
    { ABIL_ZIN_HEALING, "Minor Healing", 2, 0, 50, 1, ABFLAG_NONE },
    { ABIL_ZIN_PESTILENCE, "Pestilence", 3, 0, 100, 2, ABFLAG_NONE },
    { ABIL_ZIN_HOLY_WORD, "Holy Word", 6, 0, 150, 3, ABFLAG_NONE },
    { ABIL_ZIN_SUMMON_GUARDIAN, "Summon Guardian", 7, 0, 150, 4, ABFLAG_NONE },
    { ABIL_TSO_REPEL_UNDEAD, "Repel Undead", 1, 0, 100, 0, ABFLAG_NONE },
    { ABIL_TSO_SMITING, "Smiting", 3, 0, 50, 2, ABFLAG_NONE },
    { ABIL_TSO_ANNIHILATE_UNDEAD, "Annihilate Undead", 3, 0, 50, 2, ABFLAG_NONE },
    { ABIL_TSO_THUNDERBOLT, "Thunderbolt", 5, 0, 100, 2, ABFLAG_NONE },
    { ABIL_TSO_SUMMON_DAEVA, "Summon Daeva", 8, 0, 150, 4, ABFLAG_NONE },
    { ABIL_KIKU_RECALL_UNDEAD_SLAVES, "Recall Undead Slaves", 2, 0, 50, 0, ABFLAG_NONE },
    { ABIL_KIKU_ENSLAVE_UNDEAD, "Enslave Undead", 4, 0, 150, 3, ABFLAG_NONE },
    { ABIL_KIKU_INVOKE_DEATH, "Invoke Death", 4, 0, 250, 3, ABFLAG_NONE },
    { ABIL_YRED_ANIMATE_CORPSE, "Animate Corpse", 1, 0, 50, 0, ABFLAG_NONE },
    { ABIL_YRED_RECALL_UNDEAD, "Recall Undead Slaves", 2, 0, 50, 0, ABFLAG_NONE },
    { ABIL_YRED_ANIMATE_DEAD, "Animate Dead", 3, 0, 100, 1, ABFLAG_NONE },
    { ABIL_YRED_DRAIN_LIFE, "Drain Life", 6, 0, 200, 2, ABFLAG_NONE },
    { ABIL_YRED_CONTROL_UNDEAD, "Control Undead", 5, 0, 150, 2, ABFLAG_NONE },
    { ABIL_VEHUMET_CHANNEL_ENERGY, "Channel Energy", 0, 0, 50, 0, ABFLAG_NONE },
    { ABIL_OKAWARU_MIGHT, "Might", 2, 0, 50, 1, ABFLAG_NONE },
    { ABIL_OKAWARU_HEALING, "Healing", 2, 0, 75, 1, ABFLAG_NONE },
    { ABIL_OKAWARU_HASTE, "Haste", 5, 0, 100, 3, ABFLAG_NONE },
    { ABIL_MAKHLEB_MINOR_DESTRUCTION, "Minor Destruction", 1, 0, 20, 0, ABFLAG_NONE },
    { ABIL_MAKHLEB_LESSER_SERVANT_OF_MAKHLEB, "Lesser Servant of Makhleb", 2, 0, 50, 1, ABFLAG_NONE },
    { ABIL_MAKHLEB_MAJOR_DESTRUCTION, "Major Destruction", 4, 0, 100, 2, ABFLAG_NONE },
    { ABIL_MAKHLEB_GREATER_SERVANT_OF_MAKHLEB, "Greater Servant of Makhleb", 6, 0, 100, 3, ABFLAG_NONE },
    { ABIL_SIF_MUNA_FORGET_SPELL, "Forget Spell", 5, 0, 0, 8, ABFLAG_NONE },
    { ABIL_TROG_BERSERK, "Berserk", 0, 0, 200, 0, ABFLAG_NONE },
    { ABIL_TROG_MIGHT, "Might", 0, 0, 200, 1, ABFLAG_NONE },
    { ABIL_TROG_HASTE_SELF, "Haste Self", 0, 0, 250, 3, ABFLAG_NONE },
    { ABIL_ELYVILON_LESSER_HEALING, "Lesser Healing", 1, 0, 100, 0, ABFLAG_NONE },
    { ABIL_ELYVILON_PURIFICATION, "Purification", 2, 0, 150, 1, ABFLAG_NONE },
    { ABIL_ELYVILON_HEALING, "Healing", 2, 0, 250, 2, ABFLAG_NONE },
    { ABIL_ELYVILON_RESTORATION, "Restoration", 3, 0, 400, 3, ABFLAG_NONE },
    { ABIL_ELYVILON_GREATER_HEALING, "Greater Healing", 6, 0, 600, 4, ABFLAG_NONE },
    { ABIL_CHARM_SNAKE, "Charm Snake", 6, 0, 200, 5, ABFLAG_NONE },
    { ABIL_TRAN_SERPENT_OF_HELL, "Turn into Demonic Serpent", 16, 0, 600, 8, ABFLAG_NONE },
    { ABIL_BREATHE_HELLFIRE, "Breathe Hellfire", 0, 8, 200, 0, ABFLAG_BREATH },
    { ABIL_ROTTING, "Rotting", 4, 4, 0, 2, ABFLAG_NONE },
    { ABIL_TORMENT_II, "Call Torment", 9, 0, 0, 3, ABFLAG_PAIN },
    { ABIL_SHUGGOTH_SEED, "Sow Shuggoth Seed", 12, 8, 0, 6, ABFLAG_NONE },
    { ABIL_RENOUNCE_RELIGION, "Renounce Religion", 0, 0, 0, 0, ABFLAG_NONE },
};

#endif
