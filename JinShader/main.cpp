#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

int main()
{
	if (!glfwInit())
	{
		printf("Failed to Init GLFW! Cannot Continue!\n");
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "JinShader", 0, 0);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to Init GLEW! Cannot Continue!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return -2;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 150";
	ImGui_ImplOpenGL3_Init(glsl_version);

	unsigned int program = glCreateProgram();
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource();
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	bool showDemo = true;
	bool want_exit = false;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow(&showDemo);

		ImGui::Begin("Status");
		ImGui::Text("Want Exit? %s", want_exit ? "Yes" : "No");
		ImGui::End();

	
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			want_exit = true;
		}

		if (want_exit)
		{
			ImGui::OpenPopup("Exit?");
			// Always center this window when appearing
			ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Exit?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Are You Sure Want to Exit?");
				ImGui::Separator();

				//static int unused_i = 0;
				//ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");
		
				if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); want_exit = false; glfwSetWindowShouldClose(window, 1); }
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); want_exit = false; }
				ImGui::EndPopup();
			}
		}

		


		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{

		}

		glBegin(GL_QUADS);
		glVertex2f(-1, 1);
		glVertex2f(-1, -1);
		glVertex2f( 1, -1);
		glVertex2f(1, 1);
		glEnd();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}