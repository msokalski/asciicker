// nvbug.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <string.h>

#include "gl.h"

#include "imgui/imgui.h"
#include "imgui_impl_opengl3.h"

#include "asciiid_platform.h"
#include "terrain.h"
#include "matrix.h"

Terrain* terrain = 0;
float pos_x = 0, pos_y = 0, pos_z = 0;
float rot_yaw = 0;
float rot_pitch = 90;

int mouse_in = 0;
int g_Time;


void GL_APIENTRY glDebugCall(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	static const char* source_str[] = // 0x8246 - 0x824B
	{
		"API",
		"WINDOW_SYSTEM",
		"SHADER_COMPILER",
		"THIRD_PARTY",
		"APPLICATION",
		"OTHER"
	};

	const char* src = "?";
	if (source >= 0x8246 && source <= 0x824B)
		src = source_str[source - 0x8246];

	static const char* type_str[] = // 0x824C - 0x8251
	{
		"ERROR",
		"DEPRECATED_BEHAVIOR",
		"UNDEFINED_BEHAVIOR",
		"PORTABILITY",
		"PERFORMANCE",
		"OTHER"
	};

	const char* typ = "?";
	if (type >= 0x824C && type <= 0x8251)
		typ = type_str[type - 0x824C];

	static const char* severity_str[] = // 0x9146 - 0x9148 , 0x826B
	{
		"HIGH",
		"MEDIUM",
		"LOW",
		"NOTIFICATION",
	};

	const char* sev = "?";
	if (severity >= 0x9146 && severity <= 0x9148)
		sev = severity_str[severity - 0x9146];
	else
		if (severity == 0x826B)
			sev = severity_str[3];

	printf("src:%s type:%s id:%d severity:%s\n%s\n\n", src, typ, id, sev, (const char*)message);
}

struct RenderContext
{
	double tm[16];

	float* map;

	int patches;

	// 2 vbos
	// 1 is mapped
	// mapped vbo, etc
	// ...
};

void RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie)
{
	RenderContext* rc = (RenderContext*)cookie;

	rc->patches++;
}

void my_render()
{
	// THINGZ
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		ImGui_ImplOpenGL3_NewFrame();
		{
			// Setup time step
			ImGuiIO& io = ImGui::GetIO();
			int current_time = a3dGetTime();

			io.DeltaTime = 10; // (current_time - g_Time) / 1000.0f;
			g_Time = current_time;
			// Start the frame
			ImGui::NewFrame();
		}

		static bool show_demo_window = true;
		static bool show_another_window = false;

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}
	}

	ImGui::Render();

	ImGuiIO& io = ImGui::GetIO();
	glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	RenderContext rc;
	memset(&rc, 0, sizeof(RenderContext));

	double* tm = rc.tm;

	// currently we're assuming: 1 visual cell = 1 font_size

	double font_size = 16; // so every visual cell appears as 16px
	double z_scale = 1.0 / 16.0; // this is a constant, (what fraction of font_size is produced by +1 height_map)

	double rx = 0.5 * io.DisplaySize.x / font_size;
	double ry = 0.5 * io.DisplaySize.y / font_size;

	double pitch = rot_pitch * (M_PI / 180);
	double yaw = rot_yaw * (M_PI / 180);

	tm[0] = +cos(yaw)/rx;
	tm[1] = -sin(yaw)*sin(pitch)/ry;
	tm[2] = 0;
	tm[3] = 0;
	tm[4] = +sin(yaw)/rx;
	tm[5] = +cos(yaw)*sin(pitch)/ry;
	tm[6] = 0;
	tm[7] = 0;
	tm[8] = 0;
	tm[9] = +cos(pitch)*z_scale/ry;
	tm[10] = +2./0xffff;
	tm[11] = 0;
	tm[12] = -(pos_x * tm[0] + pos_y * tm[4] + pos_z * tm[8]);
	tm[13] = -(pos_x * tm[1] + pos_y * tm[5] + pos_z * tm[9]);
	tm[14] = -1.0;
	tm[15] = 1.0;

	// 4 clip planes in clip-space

	double clip_left[4] =   { 1, 0, 0,+1 };
	double clip_right[4] =  {-1, 0, 0,+1 };
	double clip_bottom[4] = { 0, 1, 0,+1 };
	double clip_top[4] =    { 0,-1, 0,+1 };

	// transform them to world-space (mul by tm^-1)

	double clip_world[4][4];
	TransposeProduct(tm, clip_left, clip_world[0]);
	TransposeProduct(tm, clip_right, clip_world[1]);
	TransposeProduct(tm, clip_bottom, clip_world[2]);
	TransposeProduct(tm, clip_top, clip_world[3]);

	int planes = 4;
	int view_flags = 0xAA; // should contain only bits that face viewing direction

	QueryTerrain(terrain, planes, clip_world, view_flags, RenderPatch, &rc);
	printf("rendered %d patches / %d total\n", rc.patches, GetTerrainPatches(terrain));

	if (!io.WantCaptureMouse && mouse_in)
	{
		int rect[4] =
		{
			(int)(io.MousePos.x) - 16,
			(int)(io.DisplaySize.y - io.MousePos.y) - 16,
			(int)(io.MousePos.x) + 16,
			(int)(io.DisplaySize.y - io.MousePos.y) + 16
		};
		// we are free to use mouse
		glEnable(GL_SCISSOR_TEST);
		glScissor(rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
	}

	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	a3dSwapBuffers();
}

void my_mouse(int x, int y, MouseInfo mi)
{
	if ((mi & 0xF) == MouseInfo::LEAVE)
	{
		mouse_in = 0;
		return;
	}
	else
	if ((mi & 0xF) == MouseInfo::ENTER)
		mouse_in = 1;

	ImGuiIO& io = ImGui::GetIO();

	io.MousePos = ImVec2((float)x, (float)y);

	switch (mi & 0xF)
	{
		case MouseInfo::WHEEL_DN:
			io.MouseWheel += 1.0;
			break;
		case MouseInfo::WHEEL_UP:
			io.MouseWheel -= 1.0;
			break;
		case MouseInfo::LEFT_DN:
			io.MouseDown[0] = true;
			break;
		case MouseInfo::LEFT_UP:
			io.MouseDown[0] = false;
			break;
		case MouseInfo::RIGHT_DN:
			io.MouseDown[1] = true;
			break;
		case MouseInfo::RIGHT_UP:
			io.MouseDown[1] = false;
			break;
		case MouseInfo::MIDDLE_DN:
			io.MouseDown[2] = true;
			break;
		case MouseInfo::MIDDLE_UP:
			io.MouseDown[2] = false;
			break;
	}
}

void my_resize(int w, int h)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)w, (float)h);
}

void my_init()
{
	g_Time = a3dGetTime();

	glDebugMessageCallback(glDebugCall, 0/*cookie*/);

	// Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "imgui_impl_a3d";

	io.KeyMap[ImGuiKey_Tab] = A3D_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = A3D_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = A3D_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = A3D_UP;
	io.KeyMap[ImGuiKey_DownArrow] = A3D_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = A3D_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = A3D_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = A3D_HOME;
	io.KeyMap[ImGuiKey_End] = A3D_END;
	io.KeyMap[ImGuiKey_Insert] = A3D_INSERT;
	io.KeyMap[ImGuiKey_Delete] = A3D_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = A3D_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = A3D_SPACE;
	io.KeyMap[ImGuiKey_Enter] = A3D_ENTER;
	io.KeyMap[ImGuiKey_Escape] = A3D_ESCAPE;
	io.KeyMap[ImGuiKey_A] = A3D_A;
	io.KeyMap[ImGuiKey_C] = A3D_C;
	io.KeyMap[ImGuiKey_V] = A3D_V;
	io.KeyMap[ImGuiKey_X] = A3D_X;
	io.KeyMap[ImGuiKey_Y] = A3D_Y;
	io.KeyMap[ImGuiKey_Z] = A3D_Z;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ImGui_ImplOpenGL3_Init();

	terrain = CreateTerrain();

	for (int y = 0; y < 256; y++)
		for (int x = 0; x < 256; x++)
			AddTerrainPatch(terrain, x, y, 15);

	pos_x = 0x80 * VISUAL_CELLS;
	pos_y = 0x80 * VISUAL_CELLS;
	pos_z = 0x0F;

	a3dSetTitle(L"ASCIIID");
	a3dShow(true);
}

void my_idle()
{
	my_render();
}

void my_keyb_char(wchar_t chr)
{
}

void my_keyb_key(KeyInfo ki, bool down)
{
	ImGuiIO& io = ImGui::GetIO();
	if (ki < IM_ARRAYSIZE(io.KeysDown))
		io.KeysDown[ki] = down;
}

void my_keyb_focus(bool set)
{
}

void my_close()
{
	a3dClose();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();
}



int main(int argc, char *argv[]) 
{
	PlatformInterface pi;
	pi.close = my_close;
	pi.render = my_render;
	pi.resize = my_resize;
	pi.idle = my_idle;
	pi.init = my_init;
	pi.keyb_char = my_keyb_char;
	pi.keyb_key = my_keyb_key;
	pi.keyb_focus = my_keyb_focus;
	pi.mouse = my_mouse;

	GraphicsDesc gd;
	gd.color_bits = 32;
	gd.alpha_bits = 8;
	gd.depth_bits = 24;
	gd.stencil_bits = 8;
	gd.flags = (GraphicsDesc::FLAGS) (GraphicsDesc::DEBUG_CONTEXT | GraphicsDesc::DOUBLE_BUFFER);

	a3dOpen(&pi, &gd);

	return 0;
}
