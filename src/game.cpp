#include "game.hpp"

//----------------------------------------------------------------------------//

bool _game_camera_init(Camera** cam);
void _game_camera_quit(Camera* cam);
void _game_camera_update(Camera* cam, float dt, GLFWwindow* window);
void _game_camera_cursor_moved(Camera* cam, float x, float y);

//----------------------------------------------------------------------------//

void _game_cursor_pos_callback(GLFWwindow* window, double x, double y);
void _game_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

//----------------------------------------------------------------------------//

static void _game_message_log(const char* message, const char* file, int32 line);
#define MSG_LOG(m) _game_message_log(m, __FILE__, __LINE__)

static void _game_error_log(const char* message, const char* file, int32 line);
#define ERROR_LOG(m) _game_error_log(m, __FILE__, __LINE__)

//----------------------------------------------------------------------------//

bool game_init(GameState** state)
{
	*state = (GameState*)malloc(sizeof(GameState));
	GameState* s = *state;

	if(!s)
	{
		ERROR_LOG("failed to allocate GameState struct");
		return false;
	}

	if(!gamedraw_init(&s->drawState))
	{
		ERROR_LOG("failed to intialize rendering");
		return false;
	}

	if(!_game_camera_init(&s->cam))
	{
		ERROR_LOG("failed to initialize camera");
		return false;
	}

	//glfwSetInputMode(s->drawState->instance->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowUserPointer(s->drawState->instance->window, s);
	glfwSetCursorPosCallback(s->drawState->instance->window, _game_cursor_pos_callback);
	glfwSetKeyCallback(s->drawState->instance->window, _game_key_callback);

	return true;
}

void game_quit(GameState* s)
{
	_game_camera_quit(s->cam);
	gamedraw_quit(s->drawState);
	free(s);
}

//----------------------------------------------------------------------------//

void game_main_loop(GameState* s)
{
	float lastTime = (float)glfwGetTime();

	while(!glfwWindowShouldClose(s->drawState->instance->window))
	{
		float curTime = (float)glfwGetTime();
		float dt = curTime - lastTime;
		lastTime = curTime;

		_game_camera_update(s->cam, dt, s->drawState->instance->window);
		gamedraw_draw(s->drawState, s->cam);
	
		glfwPollEvents();
	}
}

//----------------------------------------------------------------------------//

bool _game_camera_init(Camera** camera)
{
	*camera = (Camera*)malloc(sizeof(Camera));
	Camera* cam = *camera;

	if(!cam)
	{
		ERROR_LOG("failed to allocate Camera struct");
		return false;
	}

	cam->up = {0.0f, 1.0f, 0.0f};
	cam->nearPlane = -100.0f; //TODO: figure out why this only works with weird near/far plane values
	cam->farPlane = 100.0f;
	cam->scale = 5.0f;

	cam->dist = 10.0f;
	cam->angle = 45.0f;

	return true;
}

void _game_camera_quit(Camera* cam)
{
	free(cam);
}

void _game_camera_update(Camera* cam, float dt, GLFWwindow* window)
{
	float camSpeed = 5.0f * dt;
	float angleSpeed = 30.0f * dt;

	qm::vec4 forward4 = qm::rotate(cam->up, cam->angle) * qm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	qm::vec4 side4 = qm::rotate(cam->up, cam->angle) * qm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	qm::vec3 forward = qm::vec3(forward4.x, forward4.y, forward4.z);
	qm::vec3 side = qm::vec3(side4.x, side4.y, side4.z);

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam->center = cam->center + (forward * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam->center = cam->center - (forward * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam->center = cam->center + (side * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam->center = cam->center - (side * camSpeed);

	if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cam->angle -= angleSpeed;
	if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cam->angle += angleSpeed;

	qm::vec4 toPos = qm::rotate(side, 30.0f) * qm::vec4(forward, 1.0f);
	cam->pos = cam->center + cam->dist * qm::normalize(qm::vec3(toPos.x, toPos.y, toPos.z));
}

void _game_camera_cursor_moved(Camera* cam, float x, float y)
{
	/*static float lastX = 0.0f, lastY = 0.0f;
	
	static bool firstMouse = true;
	if(firstMouse)
	{
		lastX = x;
        lastY = y;
        firstMouse = false;
	}

	float offsetX = x - lastX;
	float offsetY = lastY - y;
	lastX = x;
	lastY = y;

	const float sens = 0.05f;
	offsetX *= sens;
	offsetY *= sens;

	cam->orient.y -= offsetX;
	cam->orient.x -= offsetY;

	if(cam->orient.x > 89.0f)
		cam->orient.x = 89.0f;
	if(cam->orient.x < -89.0f)
		cam->orient.x = -89.0f;*/
}

//----------------------------------------------------------------------------//

void _game_cursor_pos_callback(GLFWwindow* window, double x, double y)
{
	GameState* s = (GameState*)glfwGetWindowUserPointer(window);

	_game_camera_cursor_moved(s->cam, (float)x, (float)y);
}

void _game_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

//----------------------------------------------------------------------------//

static void _game_message_log(const char* message, const char* file, int32 line)
{
	printf("GAME MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _game_error_log(const char* message, const char* file, int32 line)
{
	printf("GAME ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}