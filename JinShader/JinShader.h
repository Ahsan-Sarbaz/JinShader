#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

struct JinShaderState
{
	GLFWwindow* window;
	int window_width, window_height;
	int fb_width, fb_height;
	bool want_exit = false;
	bool want_save = false;
	bool want_update = false;
	bool compile_success = false;
	bool has_focus = true;
	bool window_is_open;
};

JinShaderState* InitJinShader();
void InitWindow(JinShaderState* state);
void InitImGui(JinShaderState* state);
void JinShaderUpdate(JinShaderState* state);