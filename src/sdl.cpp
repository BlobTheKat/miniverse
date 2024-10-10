#include <SDL2/SDL.h>
#include <glad/gl.h>
#include <chrono>
#include <iostream>
#include "defs.cpp"

#define asset(a) ([]{extern char _binary_assets_ ## a ## _start[],_binary_assets_ ## a ## _end[];return buffer{_binary_assets_ ## a ## _start, (size_t)(_binary_assets_ ## a ## _end-_binary_assets_ ## a ## _start)};}());
typedef struct{char* data; size_t size;} buffer;

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
			error = (char*) malloc(size1+35);
			SDL_memcpy(error, "GLSL vertex shader error:\n\x1b[31m", 31);
			glGetShaderInfoLog(v, size1, NULL, error+31);
			*(int*)(error+size1+30) = *(int*)"\x1b[m";
			goto end;
		}
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &size1);
		if(size1 > 0){
			error = (char*) malloc(size1+37);
			SDL_memcpy(error, "GLSL fragment shader error:\n\x1b[31m", 33);
			glGetShaderInfoLog(f, size1, NULL, error+33);
			*(int*)(error+size1+30) = *(int*)"\x1b[m";
			goto end;
		}
		error = (char*) malloc(size+34);
		SDL_memcpy(error, "GLSL program link error:\n\x1b[31m", 30);
		glGetProgramInfoLog(v, size, NULL, error+30);
		*(int*)(error+size1+30) = *(int*)"\x1b[m";
		end:
		puts(error);
		free(error);
		return p;
	}
	printf("\x1b[32mProgram linked successfully (source %zd B)\x1b[m\n", vert.size+frag.size);
	#endif
	return p;
}

inline void init(SDL_Window*);
inline void frame();
bool playing;
double t, dt;
struct{
	int x, y, w, h;
	int width, height;
} window;
vec4 mouse;
SDL_mutex* mut;
int render(void* a){
	SDL_Window* win = (SDL_Window*) a;
	SDL_GLContext gl = SDL_GL_CreateContext(win);
	SDL_GL_MakeCurrent(win, gl);
	gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
	#ifndef RELEASE
	printf("Loaded OpenGL %s\n", glGetString(GL_VERSION));
	#endif
	if(SDL_GL_SetSwapInterval(-1)) SDL_GL_SetSwapInterval(1);
	init(win);
	uint64_t start = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	while(1){
		double ot=t;
		t = double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()-start)*1e-9;
		dt = t-ot;
		SDL_GetWindowSize(win, &window.w, &window.h);
		SDL_GetWindowPosition(win, &window.x, &window.y);
		SDL_GL_GetDrawableSize(win, &window.width, &window.height);
		glViewport(0, 0, window.width, window.height);
		glClearColor(.2, .6, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		playing = SDL_GetRelativeMouseMode();
		SDL_LockMutex(mut);
		frame();
		mouse = 0;
		SDL_UnlockMutex(mut);
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
	int size; SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &size);
	if(size < 24) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_Window* win = SDL_CreateWindow("Womp womp simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	mut = SDL_CreateMutex();
	SDL_CreateThread(render, NULL, win);
	while(1){
		SDL_Event e;
		SDL_WaitEvent(&e);
		SDL_LockMutex(mut);
		switch(e.type){
			case SDL_QUIT: return 0;
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
			playing = true;
			break;
		}
		SDL_UnlockMutex(mut);
	}
	return 0;
}