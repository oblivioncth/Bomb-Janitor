/**
 * @defgroup game_BOMJ Heimlich's Midterm Game
 * @ingroup games
 *
 * This game demonstrates using the Muhlbaier Arcade Game System
 *
 * Note: the map is drawn with extended ASCII chars, to view it properly
 * change translation in PuTTY:
 * Change Settings -> Window -> Translation
 * Then set the Remote character set to CP866
 *
 * This game is a port of minesweeper called "Bomb Janitor. Just like the
 * original you must peruse the minefield using the cursor and inspect
 * tiles for bombs. If a inspected tile has a bomb it will explode and
 * the game will end. Otherwise, the tile will change to a number indicating
 * how many adjacent tiles contain bombs. If no adjacent tiles contain bombs
 * then the tile will become blank and all adjacent tiles will be changed to
 * numbers that indicate how many tiles adjacent to those contain bombs. You
 * can mark any title with a flag if you suspect it has a bomb under it to help
 * you clear the minefield more safely. The goal of the game is to clear the
 * entire field and you score is based on how many tiles you safely reveiled
 * before you blew up if you do not succesfully uncover the whole field.
 *
 *
 * "$game BOMJ play" will start the game.
 * "$game BOMJ help" will print information about the game.
 *
 * Enjoy, and don't blow up!
 *
 * @warning This game runs best at 16MHz or higher. See @ref hal_clock
 *
 *  Created on: May 06, 2017
 *      Author: Heimlich
 *
 *  @{
 */

#ifndef BOMB_JANITOR_H_
#define BOMB_JANITOR_H_

/** BOMJ_Init must be called before the game can be played
 *
 */
void BOMJ_Init(void);

/** @} */

#endif /* BOMB_JANITOR_H_ */
