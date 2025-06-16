/*
 *  File:       dungeon.cc
 *  Summary:    Functions used when building new levels.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *               <1>     -/--/--        LRH             Created
 */


#ifndef DUNGEON_H
#define DUNGEON_H

#include "FixVec.h"
#include "externs.h"

#define MAKE_GOOD_ITEM          351

void item_colour( item_def &item );

/**
 * @brief Build a new level.
 * @param level_number The depth of the level
 * @param level_type The type of the level
 */
void builder(int level_number, LEVEL_TYPES level_type);

int items( int allow_uniques, int force_class, int force_type,
           bool dont_place, int item_level, int item_race );

void give_item(int mid, int level_number);

void define_zombie(int mid, int ztype, int cs, int power);

#endif
