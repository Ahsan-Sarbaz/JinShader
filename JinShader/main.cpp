#include "JinShader.h"
#include "texteditor/TextEditor.h"


//this is borrowed from the imgui_demo.cpp
struct ConsoleLog
{
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool                AutoScroll;  // Keep scrolling if already at the bottom.

	ConsoleLog()
	{
		AutoScroll = true;
		Clear();
	}

	void Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	void AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size + 1);
	}

	void Draw(const char* title, bool* p_open = NULL)
	{
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &AutoScroll);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of
			// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
			// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
			// within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
			// on your side is recommended. Using ImGuiListClipper requires
			// - A) random access into your data
			// - B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display
			// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
			// it possible (and would be recommended if you want to search through tens of thousands of entries).
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
		ImGui::End();
	}
};

static ConsoleLog consoleLogger;



int main()
{

	JinShaderState* state = InitJinShader();
	state->window_width = 1200;
	state->window_height = 675;

	InitWindow(state);
	InitImGui(state);
	
	unsigned int program = 0, shader = 0;
	int codeBufferSize = 1024 * 1024 * 4;
	char* codeBuffer = (char*)calloc(1, codeBufferSize);

	unsigned int fbo;
	unsigned int colorAttachMent = 0;
	glGenFramebuffers(1, &fbo);

	const char* vertexShaderSource = 
		"#version 330 core\n"
		"layout(location = 0) in vec4 in_position;\n"
		"void main()\n"
		"{\n"
			"gl_Position = in_position;\n"
		"}\n";

	auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(vertexShader);
	int result = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		int len = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)malloc(len);
		glGetShaderInfoLog(vertexShader, len, 0, log);
		printf("Vertex Shader Compilation Failed! : %s\n", log);
		free(log);
	}

	const char* commonShaderSource =
		"#version 330 core\n"
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
		"void main()\n"
		"{\n"
		"\tmainImage(FinalColor, gl_FragCoord.xy);\n"
		"}\n";

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
	float framebufferSizeXLast = 0;
	float framebufferSizeYLast = 0;

	float quadVerts[4*4] = 
	{
		-1,  1,  0, 1,
		-1, -1,  0, 1,
		 1, -1,  0, 1,
		 1,  1,  0, 1
	};
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, quadVerts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	bool showAboutImGui = false;
	bool showAboutJinShader = false;
	bool showCode = true;
	bool showLog = true;

	TextEditor editor;
	editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	editor.SetText(codeBuffer);
	editor.SetShowWhitespaces(false);
	editor.SetImGuiChildIgnored(false);
	TextEditor::ErrorMarkers errorMarkers;

	while (state->window_is_open)
	{
		JinShaderUpdate(state);
		if (state->has_focus)
		{
			iFrame++;
			iTime = (float)glfwGetTime();
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport();
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Exit"))
						state->want_exit = true;
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					ImGui::MenuItem("Show Code", 0, &showCode);
					ImGui::MenuItem("Show Log", 0, &showLog); 

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("About"))
				{
					if (ImGui::MenuItem("JinShader"))
					{
						showAboutJinShader = true;
					}
					if (ImGui::MenuItem("Dear ImGui"))
					{
						showAboutImGui = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			//ImGui::ShowDemoWindow();

			if (showAboutImGui)
			{
				ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.2f);
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				ImGui::ShowAboutWindow(&showAboutImGui);
			}
			if (showAboutJinShader)
			{
				ImGui::OpenPopup("About JinShader");
				ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal("About JinShader", &showAboutJinShader, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Version 0.4 pre-release");
					ImGui::Separator();
					ImGui::Text("Author: Ahsan Ullah Sarbaz");
					ImGui::EndPopup();
				}
			}

			if (showCode)
			{
				editor.SetErrorMarkers(errorMarkers);
				editor.Render("Code", &showCode);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			
			if (showLog)
			{
				consoleLogger.Draw("Log", &showLog);
			}
			

			ImGui::Begin("View", 0);
			auto avail = ImGui::GetContentRegionAvail();
			ImGui::Image(reinterpret_cast<void*>(colorAttachMent), avail, { 0,1 }, {1,0});
			framebufferSizeX = avail.x;
			framebufferSizeY = avail.y;

			if (framebufferSizeXLast != framebufferSizeX || framebufferSizeYLast != framebufferSizeY)
			{
				state->want_update = true;
			}

			framebufferSizeXLast = framebufferSizeX;
			framebufferSizeYLast = framebufferSizeY;
			state->fb_width = (int)framebufferSizeX;
			state->fb_height = (int)framebufferSizeY;

			ImGui::End();
			ImGui::PopStyleVar();

			if (state->want_exit)
			{
				ImGui::OpenPopup("Exit?");
				ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal("Exit?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Are You Sure Want to Exit?");
					ImGui::Separator();
		
					if (ImGui::Button("OK", ImVec2(120, 0)))
					{
						ImGui::CloseCurrentPopup();
						state->want_exit = false;
						glfwSetWindowShouldClose(state->window, 1);
						state->window_is_open = false;
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0)))
					{ 
						ImGui::CloseCurrentPopup();
						state->want_exit = false;
					}
					ImGui::EndPopup();
				}
			}


			if (state->want_save)
			{
				auto sourceLen = editor.GetText().size();
				char* shaderSource = (char*)calloc(1, sourceLen + commonShaderSourceLen);
				
				std::string editorString = editor.GetText();
				const char* editorCode = editorString.c_str();
				//strcpy(shaderSource, commonShaderSource);
				strcat(shaderSource, commonShaderSource);
				strcat(shaderSource, editorCode);

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
				state->compile_success = (bool)result;
				if (!result)
				{
					int len = 0;
					glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
					char* log = (char*)malloc(len);
					glGetShaderInfoLog(shader, len, 0, log);
					std::vector<std::string> errorStrings;
					char* token = strtok(log, "\n");
					while (token != NULL)
					{
						if(std::regex_search(std::string(token), std::regex(R"(((ERROR: \d:\d*:) | (\s*:\s*error)))")))
							errorStrings.emplace_back(std::string(token));
						token = strtok(NULL, "\n");
					}
					for (auto& error : errorStrings)
					{
						std::string expression = R"((?::|\()\d*(?::|\)))";
						auto regexp = std::regex(expression);
						std::smatch match;
						std::regex_search(error, match, regexp);
						regexp = std::regex(R"(\d+)");
						auto line = match[0].str();
						std::smatch match2;
						std::regex_search(line, match2, regexp);
						line = match2[0].str();
						int num = std::stoi(line) - 13;
						auto newError = std::regex_replace(error, std::regex(expression), (":" + std::to_string(num) + ":"));
						errorMarkers.insert(std::make_pair<int, std::string>(int(num), std::string(newError)));
						consoleLogger.AddLog("Shader Compilation Failed %s", newError.c_str());
					}

					free(log);
				}
				glAttachShader(program, vertexShader);
				glAttachShader(program, shader);
				glLinkProgram(program);
				glGetProgramiv(program, GL_LINK_STATUS, &result);
				if (!result)
				{
					int len = 0;
					glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
					char* log = (char*)malloc(len);
					glGetProgramInfoLog(program, len, 0, log);
					consoleLogger.AddLog("Shader Link Failed %s", log);
					printf("Shader Link Failed! : %s\n", log);
					free(log);
				}
				else
				{
					errorMarkers.clear();
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
				state->want_save = false;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			static int updateCount = 0;
			if (state->want_update)
			{
				updateCount++;
				glDeleteTextures(1, &colorAttachMent);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				glGenTextures(1, &colorAttachMent);
				glBindTexture(GL_TEXTURE_2D, colorAttachMent);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, state->fb_width, state->fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachMent, 0);
				state->want_update = false;
			}

			glUniform1f(iTimeLocation, iTime);
			glUniform3f(iResolutionLocation, state->fb_width, state->fb_height, 0.0f);
			glUniform1f(iTimeDeltaLocation, iTimeDelta);
			glUniform1i(iFrameLocation, iFrame);
			double mouse_x = 0, mouse_y = 0;
			float left_click = 0, right_click = 0;
			left_click = (float)glfwGetMouseButton(state->window, GLFW_MOUSE_BUTTON_LEFT);
			right_click = (float)glfwGetMouseButton(state->window, GLFW_MOUSE_BUTTON_RIGHT);
			glfwGetCursorPos(state->window, &mouse_x, &mouse_y);
			glUniform4f(iMouseLocation, (float)mouse_x, (float)mouse_y, left_click, right_click);
			glUseProgram(program);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glViewport(0, 0, state->fb_width, state->fb_height);
			glDrawArrays(GL_QUADS, 0, 4);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef _DEBUG
			ImGui::Begin("State", 0, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text("Region Avail %f, %f", avail.x, avail.y);
			ImGui::Text("Texture Size %d, %d", state->fb_width, state->fb_height);

			ImGui::Text("Update Count %d", updateCount);
			ImGui::End();
#endif // _DEBUG

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(state->window);

		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(state->window);
	glfwTerminate();

	return 0;
}