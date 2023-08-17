#include <iostream>
#include "game_draw.hpp"

int main()
{
	DrawState* state;
	if(!gamedraw_init(&state))
		return -1;

	while(!glfwWindowShouldClose(state->instance->window))
	{
		glfwPollEvents();
	}

	gamedraw_quit(state);

	return 0;
}