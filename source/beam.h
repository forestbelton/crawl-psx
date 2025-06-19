/*
 *  File:       beam.cc
 *  Summary:    Functions related to ranged attacks.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *               <1>     -/--/--        LRH             Created
 */


#ifndef BEAM_H
#define BEAM_H


#include "externs.h"

dice_def calc_dice( int num_dice, int max_damage );


/* ***********************************************************************
 * called from: bang - it_use2 - monstuff - mstuff2
 * *********************************************************************** */
void fire_beam(bolt &pbolt, item_def *item = nullptr);

// last updated 19apr2001 {gdl}
/* ***********************************************************************
 * called from: beam
 * *********************************************************************** */
bool nasty_beam(monsters *mon, bolt &beam);


// last updated 12may2000 {dlb}
/* ***********************************************************************
 * called from: ability - it_use3 - item_use - mstuff2 - religion -
 *              spells - spells4
 * *********************************************************************** */
void explosion(bolt &pbolt, bool hole_in_the_middle = false);


// last updated 22jan2001 {gdl}
/* ***********************************************************************
 * called from: effects - spells2 - spells4
 * *********************************************************************** */
int mons_adjust_flavoured(monsters *monster, bolt &pbolt, int hurted, bool doFlavouredEffects = true);


/* ***********************************************************************
 * called from: ability - item_use - spell
 * returns true if messages were generated during the enchantment
 * *********************************************************************** */
bool mass_enchantment(int wh_enchant, int pow, int who);


/* ***********************************************************************
 * called from: fight - monstuff - mstuff2
 * *********************************************************************** */
int mons_ench_f2(monsters *monster, bolt &pbolt);


/* ***********************************************************************
 * called from: fight - monstuff - spells2
 * *********************************************************************** */
void poison_monster(monsters *monster, bool fromPlayer, int levels = 1, bool force = false);

/* ***********************************************************************
 * called from: monstuff
 * *********************************************************************** */
void fire_tracer(monsters *monster, bolt &pbolt);


/* ***********************************************************************
 * called from: monstuff
 * *********************************************************************** */
void mimic_alert(monsters *mimic);


void zapping(char ztype, int power, bolt &pbolt);

#endif
