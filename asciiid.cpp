// nvbug.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <string.h>

#include "gl.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h" // beta: ImGuiItemFlags_Disabled
#include "imgui_impl_opengl3.h"

#include "asciiid_platform.h"

#include "texheap.h"
#include "terrain.h"

#include "matrix.h"




static unsigned int g_seed = 0x87654321;

// Used to seed the generator.           
inline void fast_srand(int seed) {
	g_seed = seed;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
inline int fast_rand(void) {
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}



Terrain* terrain = 0;

float font_size = 10;// 0.125;// 16; // so every visual cell appears as 16px
float rot_yaw = 45;
float rot_pitch = 30;//90;
bool spin_anim = false;
float pos_x = 0, pos_y = 0, pos_z = 0;
int mouse_in = 0;
int panning = 0;
int panning_x = 0;
int panning_y = 0;
double panning_dx = 0;
double panning_dy = 0;

int painting = 0;
float br_xyra[4] = { 0,0, 10, 0.5 };

// these are to keep constant 25% of blobsize distances between stamps
double paint_pos_z = 0; // always keep mouse on this level
double paint_last_x=0, paint_last_y=0; // last coord of mouse AFTER last stamp
double stamp_last_x=0, stamp_last_y=0; // last position of stamp rasterization

uint64_t g_Time; // in microsecs

#define CODE(...) #__VA_ARGS__

struct RenderContext
{
	void Create()
	{
		int err = glGetError();
		glCreateBuffers(1, &vbo);
		err = glGetError();
		int max_batch_size = 789; // of patches (each 16 quads), each batch item (single patch), is stored as x,y,u,v
		glNamedBufferStorage(vbo, max_batch_size * sizeof(GLint[4]), 0, GL_DYNAMIC_STORAGE_BIT);
		err = glGetError();

		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribIPointer(0, 4, GL_UNSIGNED_INT, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		const char* vs_src = CODE(
			#version 450\n
			layout(location = 0) in ivec4 in_xyuv;
			out ivec4 xyuv;

			void main()
			{
				int duv = 5;
				xyuv = in_xyuv;
				xyuv.zw *= duv;
			}
		);

		const char* gs_src = CODE(
			#version 450\n

			layout(points) in;
			layout(triangle_strip, max_vertices = 64) out;

			uniform vec4 br;
			uniform usampler2D z_tex;
			uniform mat4 tm;

			in ivec4 xyuv[];

			out vec2 world_xy;
			out vec3 uvh;
			flat out vec3 normal;
			
			void main()
			{
				uint z;
				vec4 v;
				ivec2 xy;

				ivec3 xyz[4];

				int dxy = 4;

				float pfi = log(0.1) / (2.0*br.z*br.z);

				for (int y = 0; y < 4; y++)
				{
					for (int x = 0; x < 4; x++)
					{
						xy = ivec2(x, y + 1);
						z = texelFetch(z_tex, xyuv[0].zw + xy, 0).r;
						xyz[0] = ivec3(xyuv[0].xy + xy*dxy, z);

						xy = ivec2(x, y);
						z = texelFetch(z_tex, xyuv[0].zw + xy, 0).r;
						xyz[1] = ivec3(xyuv[0].xy + xy*dxy, z);

						xy = ivec2(x + 1, y + 1);
						z = texelFetch(z_tex, xyuv[0].zw + xy, 0).r;
						xyz[2] = ivec3(xyuv[0].xy + xy*dxy, z);

						xy = ivec2(x + 1, y);
						z = texelFetch(z_tex, xyuv[0].zw + xy, 0).r;
						xyz[3] = ivec3(xyuv[0].xy + xy*dxy, z);

						if (br.w != 0.0)
						{
							for (int i = 0; i < 4; i++)
							{
								vec2 d = xyz[i].xy - br.xy;
								float gauss = exp(pfi*dot(d, d)) * br.w;
								xyz[i].z += int(round(gauss * 256));
								xyz[i].z = clamp(xyz[i].z, 0, 0xffff);
							}
						}
						
						normal = cross(vec3(xyz[3] - xyz[0]), vec3(xyz[2] - xyz[1]));
						normal.xy *= 1.0 / 16.0; // isn't zscale = 1/16 ?

						for (int i = 0; i < 4; i++)
						{
							world_xy = xyz[i].xy;
							uvh.xyz = xyz[i] - ivec3(xyuv[0].xy, 0);
							uvh.xyz /= vec3(4.0, 4.0, 16.0);

							gl_Position = tm * vec4(xyz[i], 1.0);
							EmitVertex();
						}

						EndPrimitive();
					}
				}
			}
		);

		const char* fs_src = CODE(
			#version 450\n
		
			layout(location = 0) out vec4 color;

			uniform vec4 br;
			flat in vec3 normal;
			in vec3 uvh;
			in vec2 world_xy;
			
			void main()
			{
				vec3 light_pos = normalize(vec3(1, 0, 0));
				//float light = 0.5 * (1.0 + dot(light_pos, normalize(normal)));
				float light = 0.5 + 0.5*dot(light_pos, normalize(normal));
				color = vec4(vec3(light),1.0);

				vec2 duv = fwidth(uvh.xy);
				if (duv.x < 0.25)
				{
					float a = min(1.0, -log2(duv.x * 4));
					float m = smoothstep(0, duv.x, uvh.x) * smoothstep(4.0, 4.0 - duv.x, uvh.x);
					color.rgb *= mix(1.0,m,a);
				}
				if (duv.y < 0.25)
				{
					float a = min(1.0, -log2(duv.y * 4));
					float m = smoothstep(0, duv.y, uvh.y) * smoothstep(4.0, 4.0 - duv.y, uvh.y);
					color.rgb *= mix(1.0, m, a);
				}

				vec2 pq = fract(uvh.xy);
				if (duv.x < 0.125)
				{
					float a = min(1.0, -log2(duv.x * 8));
					float m = smoothstep(0, 2.0*duv.x, pq.x) * smoothstep(1.0, 1.0 - 2.0*duv.x, pq.x);
					color.rgb *= mix(1.0, m, a);
				}
				if (duv.y < 0.125)
				{
					float a = min(1.0, -log2(duv.y * 8));
					float m = smoothstep(0, 2.0*duv.y, pq.y) * smoothstep(1.0, 1.0 - 2.0*duv.y, pq.y);
					color.rgb *= mix(1.0, m, a);
				}

				pq = fract(4.0*uvh.xy);
				if (duv.x < 0.0625)
				{
					float a = min(1.0, -log2(duv.x * 16));
					float m = smoothstep(0, 4.0*duv.x, pq.x) * smoothstep(1.0, 1.0 - 4.0*duv.x, pq.x);
					color.rgb *= mix(1.0, m, a);
				}
				if (duv.y < 0.0625)
				{
					float a = min(1.0, -log2(duv.y * 16));
					float m = smoothstep(0, 4.0*duv.y, pq.y) * smoothstep(1.0, 1.0 - 4.0*duv.y, pq.y);
					color.rgb *= mix(1.0, m, a);
				}


				// brush preview
				if (br.w != 0.0)
				{
					float len = length(world_xy - br.xy) / 2.33;
					float alf = (br.z - len) / br.z;
					alf *= abs(br.w);

					float dalf = fwidth(alf);
					float silh = smoothstep(-dalf, 0, alf) * smoothstep(+dalf, 0, alf);

					alf = max(0.0, alf);
					color.gb *= 1.0 - alf;
					color.rgb *= 1.0 - silh*0.25;
				}
			}
		);

		err = glGetError();
		prg = glCreateProgram();

		GLenum st[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };
		const char* src[3] = { vs_src, gs_src, fs_src };
		GLuint shader[3];

		GLsizei loglen = 999;
		char logstr[1000];

		for (int i = 0; i < 3; i++)
		{
			shader[i] = glCreateShader(st[i]);
			GLint len = (GLint)strlen(src[i]);
			glShaderSource(shader[i], 1, &(src[i]), &len);
			glCompileShader(shader[i]);

			loglen = 999;
			glGetShaderInfoLog(shader[i], loglen, &loglen, logstr);
			logstr[loglen] = 0;

			if (loglen)
				printf("%s", logstr);

			glAttachShader(prg, shader[i]);
		}

		glLinkProgram(prg);

		for (int i = 0; i < 3; i++)
			glDeleteShader(shader[i]);

		loglen = 999;
		glGetProgramInfoLog(prg, loglen, &loglen, logstr);
		logstr[loglen] = 0;

		if (loglen)
			printf("%s", logstr);

		tm_loc = glGetUniformLocation(prg, "tm");
		z_tex_loc = glGetUniformLocation(prg, "z_tex");
		br_loc = glGetUniformLocation(prg, "br");
	}

	void Delete()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteProgram(prg);
	}

	void BeginPatches(const double* tm, const float* br)
	{
		glUseProgram(prg);

		static const float br_off[] = { 0,0,1,0 };
		if (!br)
			br = br_off;

		//glUniformMatrix4dv(tm_loc, 1, GL_FALSE, tm);
		float ftm[16];// NV bug! workaround
		for (int i = 0; i < 16; i++)
			ftm[i] = (float)tm[i];

		glUniformMatrix4fv(tm_loc, 1, GL_FALSE, ftm);
		glUniform1i(z_tex_loc, 0);
		glUniform4fv(br_loc, 1, br);
		glBindVertexArray(vao);

		head = 0;
		patches = 0;
		draws = 0;
		changes = 0;
		page_tex = 0;

		render_time = a3dGetTime();
	}

	static void RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie)
	{
		RenderContext* rc = (RenderContext*)cookie;
		TexAlloc* ta = GetTerrainTexAlloc(p);

		rc->patches++;

		TexPageBuffer* buf = (TexPageBuffer*)ta->page->user;

		if (buf->size == 0)
		{
			if (rc->head)
				((TexPageBuffer*)rc->head->user)->prev = ta->page;
			buf->prev = 0;
			buf->next = rc->head;
			rc->head = ta->page;
		}

		GLint* patch = buf->data + 4 * buf->size;

		patch[0] = x;
		patch[1] = y;
		patch[2] = ta->x;
		patch[3] = ta->y;

		buf->size++;

		if (buf->size == 789)
		{
			rc->draws++;
			
			if (rc->page_tex != ta->page->tex)
			{
				rc->changes++;
				rc->page_tex = ta->page->tex;
				glBindTextureUnit(0, rc->page_tex);
			}

			glNamedBufferSubData(rc->vbo, 0, sizeof(GLint[4]) * buf->size, buf->data);
			glDrawArrays(GL_POINTS, 0, buf->size);

			if (buf->prev)
				((TexPageBuffer*)buf->prev->user)->next = buf->next;
			else
				rc->head = buf->next;

			if (buf->next)
				((TexPageBuffer*)buf->next->user)->prev = buf->prev;

			buf->size = 0;
			buf->next = 0;
			buf->prev = 0;
		}

	}

	void EndPatches()
	{
		TexPage* tp = head;
		while (tp)
		{
			TexPageBuffer* buf = (TexPageBuffer*)tp->user;

			if (page_tex != tp->tex)
			{
				changes++;
				page_tex = tp->tex;
				glBindTextureUnit(0, page_tex);
			}

			draws++;
			glNamedBufferSubData(vbo, 0, sizeof(GLint[4]) * buf->size, buf->data);
			glDrawArrays(GL_POINTS, 0, buf->size);

			tp = buf->next;
			buf->size = 0;
			buf->next = 0;
			buf->prev = 0;
		}

		page_tex = 0;
		head = 0;

		glBindTextureUnit(0,0);
		glBindVertexArray(0);
		glUseProgram(0);

		render_time = a3dGetTime() - render_time;
	}

	GLint tm_loc; // uniform
	GLint z_tex_loc;
	GLint br_loc;

	GLuint prg;
	GLuint vao;
	GLuint vbo;

	GLuint page_tex;
	TexPage* head;

	int patches; // rendered stats
	int draws;
	int changes;
	uint64_t render_time;
};

RenderContext render_context;

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
		{
			return;
			sev = severity_str[3];
		}

	printf("src:%s type:%s id:%d severity:%s\n%s\n\n", src, typ, id, sev, (const char*)message);
}

static void StampCB(Patch* p, int x, int y, int view_flags, void* cookie)
{
	double* xy = (double*)cookie;
	uint16_t* map = GetTerrainHeightMap(p);

	double pfi = log(0.1) / (2.0*br_xyra[2] * br_xyra[2]);
	const static double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;

	for (int i = 0, hy = 0; hy <= HEIGHT_CELLS; hy++)
	{
		double dy = y + sxy * hy - xy[1];
		dy *= dy;
		for (int hx = 0; hx <= HEIGHT_CELLS; hx++, i++)
		{
			double dx = x + sxy * hx - xy[0];
			dx *= dx;

			double gauss = exp(pfi*(dx+dy)) * br_xyra[3];
			map[i] += (int)(round(gauss * 256));
		}
	}

	TexAlloc* ta = GetTerrainTexAlloc(p);
	UpdateTerrainPatch(p);
}

void Stamp(double x, double y)
{
	// query all patches int radial range br_xyra[2] from x,y
	// get their heightmaps apply brush on height samples and update TexHeap pages 

	double thresh = 1.0 / 513; 	// we need to query radius at which we have gauss < 1/512
	double r = sqrt(2.0 * log(thresh / fabs(br_xyra[3])) / log(0.1) ) * br_xyra[2];

	double xy[2] = { x,y };
	QueryTerrain(terrain, x, y, r, 0x00, StampCB, xy);
}

void my_render()
{
	bool br_enabled = false;

	// THINGZ
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		ImGui_ImplOpenGL3_NewFrame();
		{
			// Setup time step
			ImGuiIO& io = ImGui::GetIO();
			uint64_t current_time = a3dGetTime();
			uint64_t delta = current_time - g_Time;
			io.DeltaTime = delta>0 ? delta / 1000000.0f : FLT_MIN;
			g_Time = current_time;
			// Start the frame
			ImGui::NewFrame();
		}

		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("Examples"))
		{
			{
				static bool s = false;
				ImGui::MenuItem("gogo", NULL, &s);
			}
			{
				static bool s = false;
				ImGui::MenuItem("zogo", NULL, &s);
			}
			{
				static bool s = false;
				ImGui::MenuItem("fogo", NULL, &s);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::SliderFloat("PITCH", &rot_pitch, +30.0f, +90.0f);

		ImGui::SliderFloat("YAW", &rot_yaw, -180.0f, +180.0f); ImGui::SameLine();
		ImGui::Checkbox("Spin", &spin_anim);

		ImGui::SliderFloat("ZOOM", &font_size, 0.16f, 16.0f);

		ImGui::Text("PATCHES: %d, DRAWS: %d, CHANGES: %d", render_context.patches, render_context.draws, render_context.changes);
		ImGui::Text("RENDER TIME: %6I64d [" /*micro*/"\xc2\xb5"/*utf8*/ "s]", render_context.render_time);

		ImGui::SliderFloat("BRUSH RADIUS", br_xyra + 2, 1, 100*16);
		ImGui::SliderFloat("BRUSH ALPHA", br_xyra + 3, -1, 1);

		static int paint_mode=0;

		ImGui::RadioButton("VIEW POSITION", &paint_mode, 0); // or hold 'space' to interrupt current mode
		ImGui::RadioButton("PAINT", &paint_mode, 1); ImGui::SameLine();
		ImGui::RadioButton("SCULPT", &paint_mode, 2); ImGui::SameLine();
		ImGui::RadioButton("SMOOTH", &paint_mode, 3);

		if (paint_mode == 0)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		static int tool_mode = 0;
		ImGui::RadioButton("POINT", &tool_mode, 0); ImGui::SameLine();
		ImGui::RadioButton("LINE", &tool_mode, 1); ImGui::SameLine();
		ImGui::RadioButton("OVAL", &tool_mode, 2);

		if (paint_mode == 0)
		{
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}

		ImGui::End();

		static bool show_demo_window = true;
		static bool show_another_window = false;

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		/*
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		*/

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		/*
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

			ImGui::Text("PATCHES: %d, DRAWS: %d, CHANGES: %d", render_context.patches, render_context.draws, render_context.changes);

			ImGui::End();
		}
		*/

		// 3. Show another simple window.
		/*
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}
		*/
	}

	ImGui::Render();

	ImGuiIO& io = ImGui::GetIO();
	glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	RenderContext* rc = &render_context;
	double tm[16];

	// currently we're assuming: 1 visual cell = 1 font_size

	double z_scale = 1.0 / 16.0; // this is a constant, (what fraction of font_size is produced by +1 height_map)

	double rx = 0.5 * io.DisplaySize.x / font_size;
	double ry = 0.5 * io.DisplaySize.y / font_size;

	double pitch = rot_pitch * (M_PI / 180);
	double yaw = rot_yaw * (M_PI / 180);


	if (spin_anim)
	{
		rot_yaw += 0.1f;
		if (rot_yaw > 180)
			rot_yaw -= 360;
	}

	if (!io.MouseDown[0])
	{
		painting = 0;
	}

	if (!io.WantCaptureMouse && panning)
	{
		double mdx = panning_x - round(io.MousePos.x);
		double mdy = - (panning_y - round(io.MousePos.y)) / sin(pitch);
		pos_x = panning_dx + (mdx*cos(yaw) - mdy*sin(yaw))/font_size;
		pos_y = panning_dy + (mdx*sin(yaw) + mdy*cos(yaw))/font_size;

		panning_x = round(io.MousePos.x);
		panning_y = round(io.MousePos.y);

		panning_dx = pos_x;
		panning_dy = pos_y;
	}

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

	br_enabled = false;

	if (!io.WantCaptureMouse && mouse_in)
	{
		// all coords in world space!
		double itm[16];
		Invert(tm, itm);

		double ray_p[4];
		double ray_v[4];

		// mouse ray
		double clip_mouse[4] =
		{
			2.0 * io.MousePos.x / io.DisplaySize.x - 1.0,
			1.0 - 2.0 * io.MousePos.y / io.DisplaySize.y,
			-1.1, // bit under floor
			1
		};

		Product(itm, clip_mouse, ray_p);

		clip_mouse[2] = -1.2; // bit under bit under floor

		Product(itm, clip_mouse, ray_v);

		ray_v[0] -= ray_p[0];
		ray_v[1] -= ray_p[1];
		ray_v[2] -= ray_p[2];

		double hit[4];

		Patch* p = HitTerrain(terrain, ray_p, ray_v, hit);

		if (p)
		{
			br_xyra[0] = (float)hit[0];
			br_xyra[1] = (float)hit[1];

			if (io.MouseDown[0])
			{
				if (!painting)
				{
					painting = 1;
					paint_pos_z = hit[2];
					paint_last_x = hit[0];
					paint_last_y = hit[1];

					Stamp(hit[0], hit[1]);
					stamp_last_x = hit[0];
					stamp_last_y = hit[1];
				}
				else
				{
					/*
					double mdx = panning_x - round(io.MousePos.x);
					double mdy = -(panning_y - round(io.MousePos.y)) / sin(pitch);

					double mouse_x = paint_last_x + (mdx*cos(yaw) - mdy * sin(yaw)) / font_size;
					double mouse_y = paint_last_y + (mdx*sin(yaw) + mdy * cos(yaw)) / font_size;

					mdx = mouse_x - paint_last_x;
					mdy = mouse_y - paint_last_y;

					double len = sqrt(mdx*mdx + mdy * mdy);

					if (paint_len + len > br_xyra[2])
					{
						// interpolate between mouse and oaint_last
						// so stamp_pos gives len = br_xyra[2]
						stamp_x =
						stamp_y = ;
					}

					paint_last_x = mouse_x;
					paint_last_y = mouse_y;
					*/
				}
			}

			if (!painting)
				br_enabled = true;
		}
	}

	// 4 clip planes in clip-space

	double clip_left[4] =   { 1, 0, 0,+1 };
	double clip_right[4] =  {-1, 0, 0,+1 };
	double clip_bottom[4] = { 0, 1, 0,+1 }; 
	double clip_top[4] =    { 0,-1, 0,+1 }; // adjust by max brush descent

	double brush_extent = cos(pitch) * br_xyra[3] * 256 * z_scale / ry;

	// adjust by max brush ASCENT
	if (br_xyra[3]>0)
		clip_bottom[3] += brush_extent;

	// adjust by max brush DESCENT
	if (br_xyra[3]<0)
		clip_top[3] -= brush_extent;

	// transform them to world-space (mul by tm^-1)

	double clip_world[4][4];
	TransposeProduct(tm, clip_left, clip_world[0]);
	TransposeProduct(tm, clip_right, clip_world[1]);
	TransposeProduct(tm, clip_bottom, clip_world[2]);
	TransposeProduct(tm, clip_top, clip_world[3]);

	int planes = 4;
	int view_flags = 0xAA; // should contain only bits that face viewing direction

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);
	rc->BeginPatches(tm, br_enabled ? br_xyra : 0);
	QueryTerrain(terrain, planes, clip_world, view_flags, RenderContext::RenderPatch, rc);
	//printf("rendered %d patches / %d total\n", rc.patches, GetTerrainPatches(terrain));
	rc->EndPatches();
	glDisable(GL_DEPTH_TEST);


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
			io.MouseWheel -= 1.0;
			break;
		case MouseInfo::WHEEL_UP:
			io.MouseWheel += 1.0;
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
			panning = 1;
			panning_x = x;
			panning_y = y;
			panning_dx = pos_x;
			panning_dy = pos_y;
			io.MouseDown[2] = true;
			break;
		case MouseInfo::MIDDLE_UP:
			panning = 0;
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
	render_context.Create();

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

	// ALTERNATIVE:
	// terrain = CreateTerrain(int x, int y, int w, int h, uint16_t* data);
	// xywh coords are in patches, so data is w*4+1,h*4+1 !!!!!!!!!!!!!!!!

	const int num1 = 256;// 256;
	const int num2 = num1*num1;

	uint32_t rnd[num2];
	int n = num2;
	for (int i = 0; i < num2; i++)
		rnd[i] = i;

	for (int i = 0; i < num2; i++)
	{
		int r = (fast_rand() + fast_rand()*(RAND_MAX+1)) % n;

		uint32_t uv = rnd[r];
		rnd[r] = rnd[--n];
		uint32_t u = uv % num1;
		uint32_t v = uv / num1;
		AddTerrainPatch(terrain, u, v, 0/*fast_rand()&0x7F*/);
	}

	pos_x = num1 * VISUAL_CELLS / 2;
	pos_y = num1 * VISUAL_CELLS / 2;
	pos_z = 0x7F;

	a3dSetTitle(L"ASCIIID");

	int full[] = { -1280,0,800,600};
	//int full[] = { 0,0,1920,1080};
	a3dSetRect(full, true);

	a3dSetVisible(true);
}

void my_keyb_char(wchar_t chr)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter((unsigned short)chr);
}

void my_keyb_key(KeyInfo ki, bool down)
{
	ImGuiIO& io = ImGui::GetIO();
	if (ki < IM_ARRAYSIZE(io.KeysDown))
		io.KeysDown[ki] = down;
	
	io.KeysDown[A3D_ENTER] = a3dGetKeyb(A3D_ENTER) || a3dGetKeyb(A3D_NUMPAD_ENTER);
	io.KeyAlt = a3dGetKeyb(A3D_LALT) || a3dGetKeyb(A3D_RALT);
	io.KeyCtrl = a3dGetKeyb(A3D_LCTRL) || a3dGetKeyb(A3D_RCTRL);
	io.KeyShift = a3dGetKeyb(A3D_LSHIFT) || a3dGetKeyb(A3D_RSHIFT);
}

void my_keyb_focus(bool set)
{
}

void my_close()
{
	a3dClose();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	render_context.Delete();
}



int main(int argc, char *argv[]) 
{
	PlatformInterface pi;
	pi.close = my_close;
	pi.render = my_render;
	pi.resize = my_resize;
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
	gd.flags = (GraphicsDesc::FLAGS) (/*GraphicsDesc::DEBUG_CONTEXT | */GraphicsDesc::DOUBLE_BUFFER);

	a3dOpen(&pi, &gd);

	return 0;
}
