/*
 *  File:       items.cc
 *  Summary:    Misc (mostly) inventory related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *               <2>     6/9/99         DML             Autopickup
 *               <1>     -/--/--        LRH             Created
 */


#ifndef ITEMS_H
#define ITEMS_H

#include "externs.h"

extern int autopickup_on;

extern long autopickups;

/**
 * @brief Reduce quantity of an inventory item, do cleanup if item goes away.
 * @param obj Index of the inventory item
 * @param amount Amount to reduce by
 * @return True if the stack of items no longer exists
 */
bool dec_inv_item_quantity(int obj, int amount);

/**
 * @brief Reduce quantity of a monster/grid item, do cleanup if item goes away.
 * @param obj Index of the monster/grid item
 * @param amount Amount to reduce by
 * @return True if the stack of items no longer exists
 */
bool dec_mitm_item_quantity(int obj, int amount);

/**
 * @brief Increase quantity of an inventory item.
 * @param obj Index of the inventory item
 * @param amount Amount to increase by
 */
void inc_inv_item_quantity(int obj, int amount);

/**
 * @brief Increase quantity of a monster/grid item.
 * @param obj Index of the monster/grid item
 * @param amount Amount to increase by
 */
void inc_mitm_item_quantity(int obj, int amount);

/**
 * @brief Moves a monster/grid item to a new location.
 * @param obj Index of the monster/grid item, updated to be the index of the final object
 * @param x X-coordinate to move item to
 * @param y Y-coordinate to move item to
 */
void move_item_to_grid(int *obj, int x, int y);

void move_item_stack_to_grid(int x, int y, int targ_x, int targ_y);

/**
 * @brief Move a monster/grid item into the player's inventory.
 * @param obj Index of monster/grid item to move
 * @param quant_got
 * @param quiet If true, skip adding message logs
 * @return The quantity of items moved, or -1 if the player's inventory is full
 */
int move_item_to_player(int obj, int quant_got, bool quiet = false);

/**
 * @brief Determine if two items can be combined into a stack
 *
 * TODO: Should be converted into a method off of `item_def`
 *
 * @param item1 First item to check
 * @param item2 Second item to check
 * @return True if the items can stack together
 */
bool items_stack(const item_def &item1, const item_def &item2);

/**
 * @brief Reset/initialize a monster/grid item.
 * @param item Index of the monster/grid item
 */
void init_item(int item);

/**
 * @brief Use the item coordinates to relink all the item grids.
 */
void link_items();

/**
 * @brief Make sure item coordinates are correct to the stack they're in.
 */
void fix_item_coordinates();

/**
 * @brief Clean up items on the grid according to heuristics.
 * @return Index number of first available space or NON_ITEM for unsuccessful cleanup
 */
int cull_items();

/**
 * @brief Get the first unused monster/grid item slot
 * @param reserve The number of trailing item slots to not check. If reserve <= 10, items may be culled
 * @return An unused monster/grid item index, or NON_ITEM if none available
 */
int get_item_slot(int reserve = 50);

void unlink_item(int dest);

/**
 * @brief Destroy a monster/grid item
 * @param dest The index of the monster/grid item
 */
void destroy_item(int dest);

/**
 * @brief Destroy a stack of monster/grid items
 * @param x X-coordinate of the stack
 * @param y Y-coordinate of the stack
 */
void destroy_item_stack(int x, int y);

/**
 * @brief Display the items at the character's position
 * @param keyin If ';', display a long list of items
 */
void item_check(char keyin);

/**
 * @brief Attempt to pick up items at the player's position.
 */
void pickup();

/**
 * @brief Place an item onto the game grid.
 * @param item Item to place
 * @param x_plos X-coordinate to place at
 * @param y_plos Y-coordinate to place at
 * @param quant_drop How many items to drop, `item.quantity` if -1
 * @return True if the item was successfully placed
 */
bool copy_item_to_grid(const item_def &item, int x_plos, int y_plos, int quant_drop = -1);

/**
 * @brief Move the top item of a stack to a new location.
 * @param src_x X-coordinate of item
 * @param src_y Y-coordinate of item
 * @param dest_x X-coordinate to move to
 * @param dest_y Y-coordinate to move to
 * @return True if an item was moved
 */
bool move_top_item(int src_x, int src_y, int dest_x, int dest_y);

/**
 * @brief Prompt the user for an item to drop.
 */
void drop();

/**
 * @brief Drop an item from the user's inventory.
 * @param item_dropped The inventory slot index
 * @param quant_drop The quantity of items to drop
 */
void drop_item(int item_dropped, int quant_drop);

/**
 * @brief Update all the corpses and food chunks on the floor.
 *
 * The elapsed time is a double because this is called when we re-enter
 * a level and a *long* time may have elapsed.
 *
 * @param elapsedTime Amount of time to progress
 */
void update_corpses(double elapsedTime);

/**
 * @brief Update the level when the player has returned to it.
 * @param elapsedTime Amount of time to progress
 */
void update_level(double elapsedTime);

/**
 * @brief Do various time related actions.
 * @param time_delta
 */
void handle_time(long time_delta);

/**
 * @brief Get the total number of items in the player's inventory.
 * @return Item inventory count
 */
int inv_count();

#ifdef ALLOW_DESTROY_ITEM_COMMAND
void cmd_destroy_item();
#endif

#endif
