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

	glfwSetInputMode(s->drawState->instance->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

	cam->orient = {0.0f, 0.0f, 0.0f};
	cam->pos = {0.0f, 0.0f, 1.0f};
	cam->front = {0.0f, 0.0f, -1.0f};
	cam->up = {0.0f, 1.0f, 0.0f};

	cam->fov = 90.0f;
	cam->nearPlane = 0.1f;
	cam->farPlane = 100.0f;

	return true;
}

void _game_camera_quit(Camera* cam)
{
	free(cam);
}

void _game_camera_update(Camera* cam, float dt, GLFWwindow* window)
{
	float camSpeed = 1.0f * dt;

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam->pos = cam->pos + (qm::normalize({cam->front.x, 0.0f, cam->front.z}) * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam->pos = cam->pos - (qm::normalize({cam->front.x, 0.0f, cam->front.z}) * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam->pos = cam->pos - (qm::normalize(qm::cross(cam->front, cam->up)) * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam->pos = cam->pos + (qm::normalize(qm::cross(cam->front, cam->up)) * camSpeed);
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		cam->pos = cam->pos - cam->up * camSpeed; 
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		cam->pos = cam->pos + cam->up * camSpeed;

	qm::mat3 camRotate = qm::top_left(qm::rotate(cam->orient));
	cam->front = camRotate * qm::vec3(0.0f, 0.0f, -1.0f);
}

void _game_camera_cursor_moved(Camera* cam, float x, float y)
{
	static float lastX = 0.0f, lastY = 0.0f;
	
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
		cam->orient.x = -89.0f;
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