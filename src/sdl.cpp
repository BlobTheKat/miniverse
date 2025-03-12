#pragma once
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <chrono>
#include <iostream>
#include "util/defs.cpp"

GLuint makePipeline(buffer vert, buffer frag){
	GLuint p = glCreateProgram();
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	int size = (int) vert.size;
	glShaderSource(v, 1, &vert.data, &size);
	glCompileShader(v);
	glGetShaderiv(v, GL_INFO_LOG_LENGTH, &size);
	size = (int) frag.size;
	glShaderSource(f, 1, &frag.data, &size);
	glCompileShader(f);
	glGetShaderiv(f, GL_INFO_LOG_LENGTH, &size);
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);
	#ifndef RELEASE
	glGetProgramiv(p, GL_INFO_LOG_LENGTH, &size);
	if(size > 0){
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
		error = (char*) malloc(size+33);
		SDL_memcpy(error, "GLSL program link error:\n\x1b[31m", 30);
		glGetProgramInfoLog(p, size, NULL, error+30);
		SDL_memcpy(error+size+29, "\x1b[m", 4);
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
double t, dt;
struct{
	int x, y, w, h;
	int width, height;
} window;
vec4 mouse;
int keyCount;
const Uint8* keys;
a_lock mut;
bool run = true;
SDL_Window* win;
int render(void* a){
	win = (SDL_Window*) a;
	SDL_GLContext gl = SDL_GL_CreateContext(win);
	SDL_GL_MakeCurrent(win, gl);
	#ifndef USE_GLES
	if(!gladLoadGL()) return printf("Couldn't load OpenGL\n"), abort(), 0;
	#else
	#error GLES is unimplemented
	if(!gladLoadGLES2()) return printf("Couldn't load OpenGL\n"), abort(), 0;
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
		mut.lock();
		frame();
		mouse = 0;
		if(!run){
			SDL_DestroyWindow(win);
			mut.unlock();
			return 0;
		}
		mut.unlock();
		SDL_GL_SwapWindow(win);
	}
	return 0;
}

int main(int argc, char** argv){
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) return printf("Init fail: %s\n", SDL_GetError());
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	int size; SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &size);
	if(size < 24) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_Window* win = SDL_CreateWindow("Miniverse", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Thread* r = SDL_CreateThread(render, NULL, win);
	while(1){
		SDL_Event e;
		SDL_WaitEvent(&e);
		mut.lock();
		switch(e.type){
			case SDL_QUIT:
				run = false;
				mut.unlock();
				SDL_WaitThread(r, NULL);
				return 0;
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
			mouse.x += e.motion.xrel;
			mouse.y -= e.motion.yrel;
			break;
			case SDL_MOUSEWHEEL:
			mouse.z += e.wheel.preciseX;
			mouse.w += e.wheel.preciseY;
			break;
			case SDL_MOUSEBUTTONDOWN:
			SDL_SetRelativeMouseMode(SDL_TRUE);
			pointerLocked = true;
			break;
		}
		mut.unlock();
	}
	return 0;
}