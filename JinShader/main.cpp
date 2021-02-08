#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

static bool want_exit = false;
static bool want_save = false;
static bool want_update = false;
static bool compile_success = false;
static bool has_focus = true;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		want_exit = true;
	}

	if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_PRESS)
	{
		want_update = true;
		want_save = true;
	}
}

void focus_callback(GLFWwindow* window, int focus)
{
	has_focus = (bool)focus;
}

int main()
{
	if (!glfwInit())
	{
		printf("Failed to Init GLFW! Cannot Continue!\n");
		return -1;
	}

	//glfwWindowHint(GLFW_MAXIMIZED, 1);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "JinShader", 0, 0);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowFocusCallback(window, focus_callback);
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
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 150";
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	unsigned int program = 0, shader = 0;
	int codeBufferSize = 1024 * 1024 * 4;
	char* codeBuffer = (char*)calloc(1, codeBufferSize);

	//const char* vertexShaderSource = 
	//	"#version 460\n"
	//	"layout(location = 0) in vec2 in_position;\n"
	//	//"out vec2 fragCoord;\n"
	//	"void main()\n"
	//	"{\n"
	//		"gl_Position = vec4(in_position.x, in_position.y, 0.0, 1.0);\n"
	//	"}\n";

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	
	unsigned int colorAttachMent;
	glGenTextures(1, &colorAttachMent);
	glBindTexture(GL_TEXTURE_2D, colorAttachMent);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachMent, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/*auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(vertexShader);
	int result = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	compile_success = (bool)result;
	if (!result)
	{
		int len = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)malloc(len);
		glGetShaderInfoLog(vertexShader, len, 0, log);
		printf("Shader Compilation Failed! : %s\n", log);
		free(log);
	}*/


	const char* commonShaderSource =
		"#version 450\n"
		"out vec4 FinalColor;\n"
		"in vec4 fragCoord;\n"
		"uniform vec3 iResolution;\n"           // viewport resolution (in pixels)
		"uniform float iTime;\n"           // shader playback time (in seconds)
		"uniform float iTimeDelta;\n"          // render time (in seconds)
		"uniform int iFrame;\n"         // shader playback frame
		//"uniform float iChannelTime[4];\n"      // channel playback time (in seconds)
		//"uniform vec3 iChannelResolution[4];\n" // channel resolution (in pixels)
		"uniform vec4 iMouse;\n"                // mouse pixel coords. xy: current (if MLB down), zw: click
		//"uniform vec4 iDate;\n"                 // (year, month, day, time in seconds)
		//"uniform float iSampleRate;\n";           // sound sample rate (i.e., 44100)
		"void mainImage( out vec4 fragColor, in vec2 fragCoord );\n"
		"void main() {\n"
		"mainImage(FinalColor, gl_FragCoord.xy);"
		"};\n";

	auto commonShaderSourceLen = strlen(commonShaderSource);

	const char* initialCode = 
		"void mainImage( out vec4 fragColor, in vec2 fragCoord )\n"
		"{\n"
		"\t// Normalized pixel coordinates (from 0 to 1)\n"
		"\tvec2 uv = fragCoord / iResolution.xy;\n\n"
		"\t// Time varying pixel color\n"
		"\tvec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));\n\n"
		"\t// Output to screen\n"
		"\tfragColor = vec4(col, 1.0);\n"
		"}\n";
	strcat(codeBuffer, initialCode);

	float iTime = 0, iTimeDelta = 0;
	int iTimeLocation = 0, iResolutionLocation = 0, iTimeDeltaLocation = 0, iFrameLocation = 0, iChannelTimeLocation = 0, iChannelResolutionLocation = 0, iMouseLocation = 0, iDateLocation = 0, iSampleRateLocation = 0;
	int iFrame = 0;

	float framebufferSizeX = 0;
	float framebufferSizeY = 0;


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if (has_focus)
		{
			iFrame++;
			iTime = (float)glfwGetTime();
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			ImGui::DockSpaceOverViewport();
			ImGui::Begin("Code", 0, ImGuiWindowFlags_NoResize);
			auto avail = ImGui::GetContentRegionAvail();
			ImGui::InputTextMultiline("##GLSL", codeBuffer, codeBufferSize, avail);
			ImGui::End();
			ImGui::Begin("View", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
			avail = ImGui::GetContentRegionMax();
			ImGui::Image(reinterpret_cast<void*>(colorAttachMent), avail, { 0,1 }, {1,0});
			framebufferSizeX = avail.x;
			framebufferSizeY = avail.y;
			ImGui::End();
			ImGui::PopStyleVar();

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
		
					if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); want_exit = false; glfwSetWindowShouldClose(window, 1); }
					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); want_exit = false; }
					ImGui::EndPopup();
				}
			}


			if (want_save)
			{
				auto sourceLen = strlen(codeBuffer);
				char* shaderSource = (char*)calloc(1, sourceLen + commonShaderSourceLen);
				//strcpy(shaderSource, commonShaderSource);
				strcat(shaderSource, commonShaderSource);
				strcat(shaderSource, codeBuffer);

				if(shader)
					glDeleteShader(shader);
				if(program)
					glDeleteProgram(program);

				program = glCreateProgram();
				shader = glCreateShader(GL_FRAGMENT_SHADER);
				glShaderSource(shader, 1, &shaderSource, 0);
				glCompileShader(shader);
				int result = 0;
				glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
				compile_success = (bool)result;
				if (!result)
				{
					int len = 0;
					glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
					char* log = (char*)malloc(len);
					glGetShaderInfoLog(shader, len, 0, log);
					printf("Shader Compilation Failed! : %s\n", log);
					free(log);
				}
				//glAttachShader(program, vertexShader);
				glAttachShader(program, shader);
				glLinkProgram(program);
				glGetProgramiv(program, GL_LINK_STATUS, &result);
				if (!result)
				{
					int len = 0;
					glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
					char* log = (char*)malloc(len);
					glGetProgramInfoLog(program, len, 0, log);
					printf("Shader Link Failed! : %s\n", log);
					free(log);
				}
				else
				{
					printf("Compile Success!\n");

					iTimeLocation = glGetUniformLocation(program, "iTime");
					iResolutionLocation = glGetUniformLocation(program, "iResolution");;
					iTimeDeltaLocation = glGetUniformLocation(program, "iTimeDelta");;
					iFrameLocation = glGetUniformLocation(program, "iFrame");;
					//iChannelTimeLocation = glGetUniformLocation(program, "iChannelTime");
					//iChannelResolutionLocation = glGetUniformLocation(program, "iChannelResolution");;
					iMouseLocation = glGetUniformLocation(program, "iMouse");;
					//iDateLocation = glGetUniformLocation(program, "iDate");;
					//iSampleRateLocation = glGetUniformLocation(program, "iSampleRate");
				}
				want_save = false;
			}

			if (want_update)
			{
				want_update = false;
			}


			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glUniform1f(iTimeLocation, iTime);
			int x_size = 0, y_size = 0;
			glfwGetFramebufferSize(window, &x_size, &y_size);
			glUniform3f(iResolutionLocation, (float)x_size, (float)y_size, 0.0f);
			glUniform1f(iTimeDeltaLocation, iTimeDelta);
			glUniform1i(iFrameLocation, iFrame);
			double mouse_x = 0, mouse_y = 0;
			float left_click = 0, right_click = 0;
			left_click = (float)glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
			right_click = (float)glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			glUniform4f(iMouseLocation, (float)mouse_x, (float)mouse_y, left_click, right_click);
			glUseProgram(program);
			glBegin(GL_QUADS);
			glVertex2f(-1, 1);
			glVertex2f(-1, -1);
			glVertex2f( 1, -1);
			glVertex2f(1, 1);
			glEnd();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window);

		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}