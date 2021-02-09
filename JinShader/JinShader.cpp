#include "JinShader.h"


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	JinShaderState* state = (JinShaderState*)glfwGetWindowUserPointer(window);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		state->want_exit = true;
		state->window_is_open = false;
	}

	if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_PRESS)
	{
		state->want_save = true;
	}
}

void focus_callback(GLFWwindow* window, int focus)
{
	JinShaderState* state = (JinShaderState*)glfwGetWindowUserPointer(window);
	state->has_focus = (bool)focus;
}

// maybe helpfull in future
JinShaderState* InitJinShader()
{
	return new JinShaderState();
}

void InitWindow(JinShaderState* state)
{
	if (!glfwInit())
	{
		printf("Failed to Init GLFW! Cannot Continue!\n");
		exit(-1);
	}

	//glfwWindowHint(GLFW_MAXIMIZED, 1);
	state->window = glfwCreateWindow(state->window_width, state->window_height, "JinShader", 0, 0);
	glfwSetWindowUserPointer(state->window, state);
	glfwMakeContextCurrent(state->window);
	glfwSwapInterval(1);
	state->window_is_open = true;
	glfwSetKeyCallback(state->window, key_callback);
	glfwSetWindowFocusCallback(state->window, focus_callback);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to Init GLEW! Cannot Continue!\n");
		glfwDestroyWindow(state->window);
		glfwTerminate();
		exit(-2);
	}
}

void InitImGui(JinShaderState* state)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(state->window, true);
	const char* glsl_version = "#version 150";
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void JinShaderUpdate(JinShaderState* state)
{
	glfwPollEvents();
	state->window_is_open = !glfwWindowShouldClose(state->window);
}
