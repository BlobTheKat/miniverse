#pragma once
#include <SDL2/SDL.h>
#ifdef GL_HEADERS
#include GL_HEADERS
#ifdef GL_HEADERS2
#include GL_HEADERS2
#endif
#else
#include <glad/glad.h>
#endif
#include <chrono>
#include <iostream>
#include "util/defs.cpp"

#ifdef USE_GLES
const char top[] = "#version 300 es\n#define precision(x) precision x\n";
#else
const char top[] = "#version 410 core\n#define lowp\n#define mediump\n#define highp\n#define precision(x) void main()\n";
#endif

GLuint makePipeline(buffer vert, buffer frag){
	GLuint p = glCreateProgram();
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	int size[2] = {sizeof(top)-1, (int) vert.size};
	const char* a[2] = {top, vert.data};
	glShaderSource(v, 2, a, size);
	glCompileShader(v);
	//glGetShaderiv(v, GL_INFO_LOG_LENGTH, size+1);
	size[1] = (int) frag.size;
	a[1] = frag.data;
	glShaderSource(f, 2, a, size);
	glCompileShader(f);
	//glGetShaderiv(f, GL_INFO_LOG_LENGTH, size+1);
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);
	#ifndef RELEASE
	glGetProgramiv(p, GL_INFO_LOG_LENGTH, size+1);
	if(size[1] > 0){
		int size1; char* error;
		glGetShaderiv(v, GL_INFO_LOG_LENGTH, &size1);
		if(size1 > 0){
			error = (char*) malloc(size1+34);
			SDL_memcpy(error, "GLSL vertex shader error:\n\x1b[31m", 31);
			glGetShaderInfoLog(v, size1, NULL, error+31);
			SDL_memcpy(error+size1+30, "\x1b[m", 4);
			goto end;
		}
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &size1);
		if(size1 > 0){
			error = (char*) malloc(size1+36);
			SDL_memcpy(error, "GLSL fragment shader error:\n\x1b[31m", 33);
			glGetShaderInfoLog(f, size1, NULL, error+33);
			SDL_memcpy(error+size1+32, "\x1b[m", 4);
			goto end;
		}
		error = (char*) malloc(size[1]+33);
		SDL_memcpy(error, "GLSL program link error:\n\x1b[31m", 30);
		glGetProgramInfoLog(p, size[1], NULL, error+30);
		SDL_memcpy(error+size[1]+29, "\x1b[m", 4);
		end:
		puts(error);
		free(error);
		return p;
	}
	printf("\x1b[32mProgram linked successfully (source %zd B)\x1b[m\n", vert.size+frag.size);
	#endif
	return p;
}

inline void init();
inline void frame();
bool pointerLocked;
double t = 0, dt;
struct{
	int x, y, w, h;
	int width, height;
} window;
vec4 mouse;
int keyCount;
const Uint8* keys;
bool run = true;
SDL_Window* win; Uint32 winID;
inline void process_event(SDL_Event& e);
#ifdef RENDER_THREAD
a_lock mut;
#define EVENT_UNLOCK mut.unlock()
#else
#define EVENT_UNLOCK
#endif
int render(void* a){
	win = (SDL_Window*) a;
	winID = SDL_GetWindowID(win);
	SDL_GLContext gl = SDL_GL_CreateContext(win);
	SDL_GL_MakeCurrent(win, gl);
	#ifndef GL_HEADERS
	#ifndef USE_GLES
	if(!gladLoadGL()) return printf("Couldn't load OpenGL\n"), abort(), 0;
	#else
	if(!gladLoadGLES2()) return printf("Couldn't load OpenGL ES\n"), abort(), 0;
	#endif
	#endif
	#ifndef RELEASE
	printf("Loaded OpenGL %s\n", glGetString(GL_VERSION));
	#endif
	if(SDL_GL_SetSwapInterval(-1)) SDL_GL_SetSwapInterval(1);
	init(); 
	uint64_t start = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	while(run){
		double ot=t;
		t = double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()-start)*1e-9;
		dt = t-ot;
		keys = SDL_GetKeyboardState(&keyCount);
		SDL_GetWindowSize(win, &window.w, &window.h);
		SDL_GetWindowPosition(win, &window.x, &window.y);
		SDL_GL_GetDrawableSize(win, &window.width, &window.height);
		glViewport(0, 0, window.width, window.height);
		pointerLocked = SDL_GetRelativeMouseMode();
		#ifdef RENDER_THREAD
		mut.lock();
		#else
		SDL_Event e;
		while(SDL_PollEvent(&e)) process_event(e);
		if(!run) return 0;
		#endif
		frame();
		mouse = 0;
		if(!run){
			SDL_DestroyWindow(win);
			EVENT_UNLOCK;
			return 0;
		}
		EVENT_UNLOCK;
		SDL_GL_SwapWindow(win);
	}
	return 0;
}

inline void process_event(SDL_Event& e){ switch(e.type){
	case SDL_QUIT:
		run = false;
		EVENT_UNLOCK;
		break;
	case SDL_KEYDOWN:
		if(e.key.repeat) break;
		if(e.key.keysym.sym == SDLK_ESCAPE){
			SDL_SetWindowFullscreen(win, SDL_FALSE);
			SDL_SetRelativeMouseMode(SDL_FALSE);
			break;
		}else if(e.key.keysym.sym == SDLK_F11){
			SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win)&SDL_WINDOW_FULLSCREEN_DESKTOP)?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		break;
	case SDL_MOUSEMOTION:
		if(winID == e.motion.windowID){
			float dpi;
			SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(win), &dpi, 0, 0);
			mouse.x += e.motion.xrel*dpi;
			mouse.y -= e.motion.yrel*dpi;
		}
		break;
	case SDL_MOUSEWHEEL:
		mouse.z += e.wheel.preciseX;
		mouse.w += e.wheel.preciseY;
		break;
	case SDL_MOUSEBUTTONDOWN:
		SDL_SetRelativeMouseMode(SDL_TRUE);
		pointerLocked = true;
		break;
} }

int main(int argc, char** argv){
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) return printf("Init fail: %s\n", SDL_GetError());
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	#ifdef USE_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	#endif
	int size; SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &size);
	if(size < 24) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_Window* win = SDL_CreateWindow("Miniverse", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP);
	#ifdef RENDER_THREAD
	SDL_Thread* rt = SDL_CreateThread(render, NULL, win);
	while(run){
		SDL_Event e;
		SDL_WaitEvent(&e);
		mut.lock();
		process_event(e);
		mut.unlock();
	}
	int r;
	SDL_WaitThread(rt, &r);
	return r;
	#else
	return render(win);
	#endif
}
