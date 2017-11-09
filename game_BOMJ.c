/*
 * game_BOMJ.c
 *
 *  Created on: May 06, 2017
 *      Author: Heimlich
 */
////////////////////////////MMmmbbbb

#include "system.h"
#include "random_int.h"
#include "strings.h"
#include "game.h"
#include "timing.h"
#include "task.h"
#include "terminal.h"
#include "random_int.h"
#include "game_BOMJ.h"
#ifdef USE_MODULE_GAME_CONTROLLER
#include "game_controller_host.h"
#include "game_controller.h"
#endif

#define MAP_WIDTH 52 // Game width in tile count
#define MAP_HEIGHT 22 // Game height in tile count

#define NUM_BOMBS 134 // Number of bombs in game (134 = 1/8 of map)

/// game structure
struct BOMJ_game_t {
    char x; ///< x coordinate of cursor
    char y; ///< y coordinate of cursor
    char c; ///< character used for cursor
    int score; ///< number of tiles uncovered
    char visual_tiles[MAP_WIDTH][MAP_HEIGHT]; ///< keeps track of what player sees
    int bomb_tiles[MAP_WIDTH][MAP_HEIGHT]; ///< keeps track of hidden bomb locations
    int cursorMoveFlag; ///< cursor movement state
    int cursorState; ///< cursor blink state
    uint8_t id; ///< ID of game (system generated)
};

static struct BOMJ_game_t game;

// note the user doesn't need to access these functions directly so they are
// defined here instead of in the .h file
// further they are made static so that no other files can access them
// ALSO OTHER MODULES CAN USE THESE SAME NAMES SINCE THEY ARE STATIC
static void Callback(int argc, char * argv[]);
static void Receiver(char c);

static void Play(void);
static void Help(void);

static void BlinkCustomCursor(void);
static void Inspect(void);
static void Mark(void);
static void BombCalculator(int tileX, int tileY);
static int BombCounter(int tileX, int tileY);
static void GameOver(void);
static void MoveRight(void);
static void MoveLeft(void);
static void MoveUp(void);
static void MoveDown(void);

void BOMJ_Init(void) {
    // Register the module with the game system and give it the name "BOMJ"
    game.id = Game_Register("BOMJ", "port of minesweeper", Play, Help);
    // Register a callback with the game system.
    // this is only needed if your game supports more than "play", "help" and "highscores"
    Game_RegisterCallback(game.id, Callback);
}

void Help(void) { 
#ifdef USE_MODULE_GAME_CONTROLLER
    Game_Printf("Use left, right, up, and down to move the cursor. \r\n"
            "Press the A button to INSPECT and the B button to MARK\r\n");
#else
    Game_Printf("Use W, A, S, and D to move the cursor. \r\n"
            "Press the SPACEBAR to INSPECT and F to MARK\r\n");
#endif
}


void Callback(int argc, char * argv[]) {
    // "play" and "help" are called automatically so just process "reset" here
    if(argc == 0) Game_Log(game.id, "too few args");
    if(strcasecmp(argv[0],"reset") == 0) {
        // reset scores
        game.score = 0;
        Game_Log(game.id, "Scores reset");
    }else Game_Log(game.id, "command not supported");
}

// Map game controller buttons
#ifdef USE_MODULE_GAME_CONTROLLER
static void A_ButtonPressed(controller_buttons_t b, void * ptr) {
    Inspect();
}

static void B_ButtonPressed(controller_buttons_t b, void * ptr) {
    Mark();
}

static void LeftPressed(controller_buttons_t b, void * ptr) {
    MoveLeft();
}

static void RightPressed(controller_buttons_t b, void * ptr) {
    MoveRight();
}

static void UpPressed(controller_buttons_t b, void * ptr) {
    MoveUp();
}

static void DownPressed(controller_buttons_t b, void * ptr) {
    MoveDown();
}
#endif

void Play(void) {
    // Initialize game controller mask
#ifdef USE_MODULE_GAME_CONTROLLER
    controller_buttons_t mask;
    mask.all_buttons = 0;
    mask.button.A = 1;
    GameControllerHost_RegisterPressCallback(0, A_ButtonPressed, mask, 0);
    mask.all_buttons = 0;
    mask.button.B = 1;
    GameControllerHost_RegisterPressCallback(0, B_ButtonPressed, mask, 0);
    mask.all_buttons = 0;
    mask.button.left = 1;
    GameControllerHost_RegisterPressCallback(0, LeftPressed, mask, 0);
    mask.all_buttons = 0;
    mask.button.right = 1;
    GameControllerHost_RegisterPressCallback(0, RightPressed, mask, 0);
    mask.all_buttons = 0;
    mask.button.up = 1;
    GameControllerHost_RegisterPressCallback(0, UpPressed, mask, 0);
    mask.all_buttons = 0;
    mask.button.down = 1;
    GameControllerHost_RegisterPressCallback(0, DownPressed, mask, 0);
#endif
    // Clear the screen
    Game_ClearScreen();

    // Initialize general game variables
    game.x = (MAP_WIDTH / 2) + 1;
    game.y = (MAP_HEIGHT / 2) + 1;
    game.c = 219;
    game.score = 0;
    game.cursorMoveFlag = 0;
    game.cursorState = 1;

    // Loop iterators
    volatile uint8_t i,j;

    // Initialize tile states
    for(i = 1; i < MAP_WIDTH; i++)
    {
        for(j = 1; j < MAP_HEIGHT; j++)
        {
            game.visual_tiles[i][j] = '#';
            game.bomb_tiles[i][j] = 0;
        }
    }

    // Determine bomb locations
    int bombX, bombY;
    for (i = 0; i < NUM_BOMBS; i++)
    {
        do
        {
            bombX = random_int(1, MAP_WIDTH);
            bombY = random_int(1, MAP_HEIGHT);
        }
        while(game.bomb_tiles[bombX][bombY] == 1);

        game.bomb_tiles[bombX][bombY] = 1;
    }

    // Draw a box around our map
    Game_DrawRect(0, 0, MAP_WIDTH, MAP_HEIGHT);

    // Fill in tiles
    Game_FillRect('#', 1, 1, MAP_WIDTH, MAP_HEIGHT);

    // Set custom cursor to middle
    Game_CharXY(219, (MAP_WIDTH/2) + 1, (MAP_HEIGHT/2) + 1);

    // Set receiver
    Game_RegisterPlayer1Receiver(Receiver);

    // Hide cursor
    Game_HideCursor();

    // Start cursor blink
    Task_Schedule(BlinkCustomCursor, 0, 450, 450);

}

// Keyboard key press receiver: Assigns key presses to game functions
void Receiver(char c) {
    switch (c) {
        case 'a':
        case 'A':
            MoveLeft();
            break;
        case 'd':
        case 'D':
            MoveRight();
            break;
        case 'w':
        case 'W':
            MoveUp();
            break;
        case 's':
        case 'S':
            MoveDown();
            break;
        case ' ':
            Inspect();
            break;
        case 'f':
        case 'F':
            Mark();
            break;
        default:
            break;
    }
}

// Keyboard key press receiver: Handles the blinking of the game specific cursor
void BlinkCustomCursor(void)
{
    if(game.cursorMoveFlag == 1) // Set cursor state to not moving
        game.cursorMoveFlag = 0;
    else
    {
        if(game.cursorState == 0) // Blink cursor on
        {
            Game_CharXY(game.c, game.x, game.y);
            game.cursorState = 1;
        }
        else // Blink cursor off
        {
            Game_CharXY(game.visual_tiles[game.x][game.y], game.x, game.y);
            game.cursorState = 0;
        }

    }
}

// Tile inspect function: Checks the uncovered tile for bombs
void Inspect(void)
{
    if(game.visual_tiles[game.x][game.y] == '#' || game.visual_tiles[game.x][game.y] == 'f') // Only check for bomb if area is uncovered
    {
        if(game.bomb_tiles[game.x][game.y] == 1) // Bomb Condition
            GameOver();
        else
            BombCalculator(game.x,game.y); // Check for adjacent bombs
    }
}

// Flag marker function: Marks the uncovered tile with a flag
static void Mark(void)
{
    if(game.visual_tiles[game.x][game.y] == '#') // Mark with flag if area is uncovered
    {
        game.visual_tiles[game.x][game.y] = 'f';
    }
    else if(game.visual_tiles[game.x][game.y] == 'f') // Or remove a flag if it is already present
    {
        game.visual_tiles[game.x][game.y] = '#';
    }

    // Do nothing otherwise
}

// Main bomb function: handles the various conditions for when a tile is inspected
void BombCalculator(int tileX, int tileY)
{
    int InspectBombCount;

    // Loop iterators
    volatile uint8_t i,j;
    InspectBombCount = BombCounter(tileX, tileY); // Count bombs
    if(InspectBombCount > 0) // Adjacent Bombs Condition
        game.visual_tiles[tileX][tileY] = 48 + InspectBombCount;
    else // No adjacent Bombs Condition
    {
        for(i = 0; i < 3; i++)
        {
            for(j = 0; j < 3; j ++)
            {
                while (Game_IsTransmitting()) DelayMs(2);
                if(!(i == 1 && j == 1)) //Skip the middle tile since its the one being inspected
                {
                    if((tileX - 1 + i > 0 && tileX - 1 + i < MAP_WIDTH) && (tileY - 1 + j > 0 && tileY - 1 + j < MAP_HEIGHT)) // Check for valid tile
                    {
                        InspectBombCount = BombCounter(tileX - 1 + i, tileY - 1 + j);
                        if(InspectBombCount > 0)
                        {
                            game.visual_tiles[tileX - 1 + i][tileY - 1 + j] = 48 + InspectBombCount;
                            Game_CharXY(48 + InspectBombCount, tileX - 1 + i, tileY - 1 + j); // Set tile to adjacent bomb count
                        }
                        else
                        {
                            game.visual_tiles[tileX - 1 + i][tileY - 1 + j] = ' ';
                            Game_CharXY(' ', tileX - 1 + i, tileY - 1 + j); // Set tile to blank space
                            //BombCalculator(tileX - 1 + i, tileY - 1 + j);
                        }
                    }
                }
            }
        }
        game.visual_tiles[tileX][tileY] = ' '; // Set tile to blank space
    }
}

// Bomb counter function: Helper function for BombCalculator that handles tallying adjacent bombs
int BombCounter(int tileX, int tileY)
{
    int bombCount = 0;

    if(tileY - 1 > 0) // Top 3 tiles
    {
        if(game.bomb_tiles[tileX][tileY - 1] == 1) // N tile
            bombCount++;
        if(tileX - 1 > 0) // NW tile
            if(game.bomb_tiles[tileX - 1][tileY - 1] == 1)
                bombCount++;
        if(tileX + 1 < MAP_WIDTH) // NE tile
            if(game.bomb_tiles[tileX + 1][tileY - 1] == 1)
                bombCount++;
    }
    if(tileY + 1 < MAP_HEIGHT)  // Bottom 3 tiles
    {
        if(game.bomb_tiles[tileX][tileY + 1] == 1) // S tile
            bombCount++;
        if(tileX - 1 > 0) // SW tile
            if(game.bomb_tiles[tileX - 1][tileY + 1] == 1)
                bombCount++;
        if(tileX + 1 < MAP_WIDTH) // SE tile
            if(game.bomb_tiles[tileX + 1][tileY + 1] == 1)
                bombCount++;
    }
    if(tileX - 1 > 0) // W tile
        if(game.bomb_tiles[tileX - 1][tileY] == 1)
            bombCount++;
    if(tileX + 1 < MAP_WIDTH) // E tile
        if(game.bomb_tiles[tileX + 1][tileY] == 1)
            bombCount++;

    return bombCount;
}

// Move right function: Moves the game specific cursor one tile right
void MoveRight(void) {
    while (Game_IsTransmitting()) DelayMs(2);
    // make sure we can move right
    if (game.x < MAP_WIDTH - 1) {
        // restore tile hidden under cursor
        Game_CharXY(game.visual_tiles[game.x][game.y], game.x, game.y);
        // PUT IN LOGIC FOR RESTORING THE APPROPRIATE CHARACTER
        game.x++;
        // update
        Game_CharXY(game.c, game.x, game.y);
        game.cursorMoveFlag = 1;
        game.cursorState = 1;
    }
}

// Move left function: Moves the game specific cursor one tile left
void MoveLeft(void) {
    while (Game_IsTransmitting()) DelayMs(2);
    // make sure we can move right
    if (game.x > 1) {
        // restore tile hidden under cursor
        Game_CharXY(game.visual_tiles[game.x][game.y], game.x, game.y);
        // PUT IN LOGIC FOR RESTORING THE APPROPRIATE CHARACTER
        game.x--;
        // update
        Game_CharXY(game.c, game.x, game.y);
        game.cursorMoveFlag = 1;
        game.cursorState = 1;
    }
}
// Move up function: Moves the game specific cursor one tile left
void MoveUp(void) {
    while (Game_IsTransmitting()) DelayMs(2);
    // make sure we can move right
    if (game.y > 1) {
        /// restore tile hidden under cursor
        Game_CharXY(game.visual_tiles[game.x][game.y], game.x, game.y);
        // PUT IN LOGIC FOR RESTORING THE APPROPRIATE CHARACTER
        game.y--;
        // update
        Game_CharXY(game.c, game.x, game.y);
        game.cursorMoveFlag = 1;
        game.cursorState = 1;
    }
}

// Move down function: Moves the game specific cursor one tile left
void MoveDown(void) {
    while (Game_IsTransmitting()) DelayMs(2);
    // make sure we can move right
    if (game.y < MAP_HEIGHT -1) {
        // restore tile hidden under cursor
        Game_CharXY(game.visual_tiles[game.x][game.y], game.x, game.y);
        // PUT IN LOGIC FOR RESTORING THE APPROPRIATE CHARACTER
        game.y++;
        // update
        Game_CharXY(game.c, game.x, game.y);
        game.cursorMoveFlag = 1;
        game.cursorState = 1;
    }
}

void GameOver(void) {
    // play loss sound
    Game_Bell();
    // end cursor blink task
    Task_Remove(BlinkCustomCursor, 0);
    // draw bombs and determine score
    volatile uint8_t i,j;
    for (i = 1; i < MAP_WIDTH; i++)
    {
        for (j = 1; j < MAP_HEIGHT; j++)
        {
            while (Game_IsTransmitting()) DelayMs(2);
            if((game.visual_tiles[i][j] > '0' && game.visual_tiles[i][j] < ':') || game.visual_tiles[i][j] == ' ') // Uncovered tile
                game.score++;
            if(game.bomb_tiles[i][j] == 1)
                Game_CharXY('X', i, j);
        }
    }

    // if a controller was used then remove the callbacks
#ifdef USE_MODULE_GAME_CONTROLLER
    GameControllerHost_RemoveCallback(A_ButtonPressed, 0);
    GameControllerHost_RemoveCallback(RightPressed, 0);
    GameControllerHost_RemoveCallback(LeftPressed, 0);
#endif
    // set cursor below bottom of map
    Game_CharXY('\r', 0, MAP_HEIGHT + 1);
    // show score
    Game_Printf("Game Over! Final score: %d", game.score);
    // unregister the receiver used to run the game
    Game_UnregisterPlayer1Receiver(Receiver);
    // show cursor (it was hidden at the beginning
    Game_ShowCursor();
    Game_GameOver();
}
