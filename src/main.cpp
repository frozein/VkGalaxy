#include <iostream>
#include "render.hpp"

int main()
{
	RenderInstance* render;
	if(!render_init(&render, 1920, 1080, "VulkanCraft"))
		return -1;

	while(!glfwWindowShouldClose(render->window))
	{
		glfwPollEvents();
	}

	render_quit(render);

	return 0;
}