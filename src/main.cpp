#include "sdl.cpp"
#include "scene.h"
#include "sim/sim.cpp"

GLint whUni, whUniStar;
GLuint objShader, starShader;

GLuint buf, va;

physics::Simulation sim;

inline void init(){
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	objShader = makePipeline(asset(obj_vert), asset(obj_frag));
	whUni = glGetUniformLocation(objShader, "i_wh");
	starShader = makePipeline(asset(fullscreen_vert), asset(star_frag));
	whUniStar = glGetUniformLocation(starShader, "wh");

	glGenBuffers(1, &buf);
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(physics::Sprite), 0);
	glVertexAttribDivisor(0, 1);
	glBindVertexArray(0);
	sim.add_node(0, 0, 10, 100);
	srand(time(0));
	for(int i=0; i < 40'000; i++){
		f32 r = f32(rand()) * (PI2/RAND_MAX);
		f32 ax = sin(r), ay = cos(r);
		sim.add_node(ax*i, ay*i, 1, 1);
	}
}

dvec2 cam{0, 0};
f32 tzoom = 0., zoom = 0.;
vec2 stars_pos;
inline void frame(){
	sim.update(dt, cam.x, cam.y);
	if(mouse.w) tzoom += mouse.w*-.3;
	f32 a = pow(.001, dt);
	zoom = tzoom*(1-a) + zoom*a;
	f32 renderZoom = pow(2, zoom);
	if(pointerLocked){
		cam -= mouse.xy*(renderZoom*.03);
		Uint8 controls = keys[SDL_SCANCODE_W] | keys[SDL_SCANCODE_S]<<1 | keys[SDL_SCANCODE_A]<<2 | keys[SDL_SCANCODE_D]<<3 | keys[SDL_SCANCODE_SPACE]<<4 | keys[SDL_SCANCODE_LSHIFT]<<5 | keys[SDL_SCANCODE_LCTRL]<<5;
		if(controls){
			f32 mv = renderZoom*dt;
			if(controls&1) cam.y += mv;
			if(controls&2) cam.y -= mv;
			if(controls&4) cam.x -= mv;
			if(controls&8) cam.x += mv;
			if(controls&16) tzoom -= dt*2;
			if(controls&32) tzoom += dt*2;
		}
	}
	a = 10./(renderZoom*window.w*window.h);
	f32 w = a*window.h, h = a*window.w;
	glUseProgram(starShader);
	glUniform2f(whUniStar, window.w*.005, window.h*.005);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glUseProgram(objShader);
	glUniform2f(whUni, w, h);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBindVertexArray(va);
	physics::UpdateResultHandle draw = sim.result();
	if(draw){
		for(int i = draw->draw_count; i;){
			vector<physics::Sprite>& buf = draw->draw_data[--i];
			usize sz = buf.size();
			glBufferData(GL_ARRAY_BUFFER, sz*sizeof(physics::Sprite), buf.data(), GL_STREAM_DRAW);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sz);
		}
	}
}