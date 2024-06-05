#include <SDL2/sdl.h>
#include <glad/gl.h>
#include <stdio.h>
#define _LARGEFILE64_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;
#if UINTPTR_MAX == UINT64_MAX
typedef double fsize;
#else
typedef float fsize;
#endif
typedef size_t usize;
typedef ssize_t isize;
typedef char byte;

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
	printf("\x1b[32mProgram linked successfully (source %d B)\x1b[m\n", vert.size+frag.size);
	#endif
	return p;
}

inline void init();
inline void frame();

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
	SDL_Window* window = SDL_CreateWindow("Womp womp simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext gl = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl);
	gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
	#ifndef RELEASE
	printf("Loaded OpenGL %s\n", glGetString(GL_VERSION));
	#endif
	SDL_GL_SetSwapInterval(1);
	init();
	while(1){
		SDL_Event e;
		while(SDL_PollEvent(&e)) switch(e.type){
			case SDL_QUIT: return 0;
			case SDL_WINDOWEVENT:
			if(e.window.event==SDL_WINDOWEVENT_RESIZED){
				int w, h;
				SDL_GetWindowSizeInPixels(window, &w, &h);
				glViewport(0, 0, w, h);
			}
			break;
		}
		glClearColor(0., 0., 0., 0.);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		frame();
		SDL_GL_SwapWindow(window);
	}
	return 0;
}