#define QD_IMPLEMENTATION
#include "libs/vkh/quickdata.h"

#include <iostream>
#include "game.hpp"

int main()
{
	GameState* state;
	if (!game_init(&state))
		return -1;

	game_main_loop(state);
	game_quit(state);

	return 0;
}