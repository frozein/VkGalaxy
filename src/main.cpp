#include <iostream>
#include "game_draw.hpp"

int main()
{
	DrawState* state;
	if(!gamedraw_init(&state))
		return -1;

	while(!glfwWindowShouldClose(state->instance->window))
	{
		gamedraw_draw(state);
		glfwPollEvents();
	}

	vkDeviceWaitIdle(state->instance->device);
	gamedraw_quit(state);

	return 0;
}