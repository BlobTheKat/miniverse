#include "sdl.cpp"
#include "scene.h"
#include "sim/sim.cpp"

GLint whUni, whUniStar, xyUni, difftUni;
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
	xyUni = glGetUniformLocation(objShader, "xy_off");
	difftUni = glGetUniformLocation(objShader, "dt");
	starShader = makePipeline(asset(fullscreen_vert), asset(star_frag));
	whUniStar = glGetUniformLocation(starShader, "wh");

	glGenBuffers(1, &buf);
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(physics::Sprite), 0);
	glVertexAttribPointer(1, 1, GL_FLOAT, false, sizeof(physics::Sprite), (void*)offsetof(physics::Sprite, radius));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(physics::Sprite), (void*)offsetof(physics::Sprite, color));
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glBindVertexArray(0);
	sim.add_attraction_rule({
		.prop = 0,
		.activator = 0,
		.dir = vec2(.25, 1),
		.name = "Gravity"
	});
	srand(time(0));
	for(int i=0; i < 100'000; i++){
		f32 th = f32(rand()) * (PI2/RAND_MAX);
		f32 ax = sin(th), ay = cos(th);
		f32 dx = ay*10, dy = ax*-10;
		f32 r = physics::fast_sqrt(i*10);
		sim.add_node(ax*r, ay*r, 1, .1, dx, dy);
	}
}

dvec2 cam{0, 0};
f32 tzoom = 0., zoom = 0.;
vec2 stars_pos;
f32 diffta = 0;
inline void frame(){
	if(mouse.w) tzoom += mouse.w*-.3;
	f32 a = pow(.001, dt);
	zoom = tzoom*(1-a) + zoom*a;
	f32 renderZoom = pow(2, zoom);
	sim.update(dt, t, cam.x, cam.y, window.w*renderZoom*.125, window.h*renderZoom*.125);
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
		cout << draw->build_time << endl;
		f32 difft = t - draw->t;
		diffta += (difft-diffta)*dt*.2;
		difft -= diffta;
		glUniform1f(difftUni, difft);
		glUniform2f(xyUni, cam.x - draw->cam_x, cam.y - draw->cam_y);
		for(int i = draw->draw_count; i;){
			vector<physics::Sprite>& buf = draw->draw_data[--i];
			usize sz = buf.size();
			glBufferData(GL_ARRAY_BUFFER, sz*sizeof(physics::Sprite), buf.data(), GL_STREAM_DRAW);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sz);
		}
	}
}