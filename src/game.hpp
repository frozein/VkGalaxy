#ifndef GAME_H
#define GAME_H

#include "game_draw.hpp"

//----------------------------------------------------------------------------//

struct GameState
{
    DrawState* drawState;
    Camera* cam;
};

//----------------------------------------------------------------------------//

bool game_init(GameState** state);
void game_quit(GameState* state);

void game_main_loop(GameState* state);

#endif