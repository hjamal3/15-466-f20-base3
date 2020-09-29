#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <time.h>       /* time */

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("arena.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > arena_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("arena.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

// Busybody, written by Bryan Teoh. Taken from https://freepd.com/. Available for non commercial purposes
Load< Sound::Sample > music_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("busybody.wav"));
});
// GoodNightmare, Written by Kevin MacLeod. Taken from https://freepd.com/. Available for non commercial purposes
Load< Sound::Sample > chase_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("Goodnightmare.wav"));
	});
// cc by: Saltbearer https://freesound.org/people/Saltbearer/sounds/536423/
Load< Sound::Sample > ai_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("jabbersynth.wav"));
});

PlayMode::PlayMode() : scene(*arena_scene) {

	// pointers to AI
	ai.resize(5);

	// initial ai orientations for wandering
	srand(static_cast <unsigned> (time(0)));
	for (int i = 0; i < 5; i++)
	{
		ai_orientations.push_back(static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f * 3.14159f);
	}


	for (auto& transform : scene.transforms) {
		/*		if (transform.name == "Hip.FL") hip = &transform;
				else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
				else if (transform.name == "LowerLeg.FL") lower_leg = &transform;*/
		if (transform.name == "Cart") {
			cart = &transform;
		}
		else if (transform.name == "Ball") {
			ball = &transform;
		}
		else if (transform.name == "AI1")
		{
			AI1 = &transform;
			ai[0] = AI1;
		}
		else if (transform.name == "AI2")
		{
			AI2 = &transform;
			ai[1] = AI2;
		}
		else if (transform.name == "AI3")
		{
			AI3 = &transform;
			ai[2] = AI3;
		}
		else if (transform.name == "AI4")
		{
			AI4 = &transform;
			ai[3] = AI4;
		}
		else if (transform.name == "AI5")
		{
			AI5 = &transform;
			ai[4] = AI5;
		}
		else if (transform.name == "Enemy")
		{
			enemy = &transform;
		}

	}
	if (cart == nullptr) throw std::runtime_error("cart not found.");
	if (ball == nullptr) throw std::runtime_error("ball not found.");


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	background_music_loop = Sound::loop(*music_sample, 0.03f,0.0f);

	// specify which AI is the enemy
	// http://www.cplusplus.com/reference/cstdlib/rand/
	srand((int)time(NULL));
	enemy_idx = rand() % 5;
	enemy_loop = Sound::loop_3D(*ai_sample, 1.0f, get_enemy_position(), 0.1f);

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			cart_velocity.x = -1;
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			cart_velocity.x = 1;
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			cart_velocity.y = 1;
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			cart_velocity.y = -1;
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			// if close enough to the enemy, change color etc.
			if (!chase)
			{
				glm::vec3 enemy_pos = get_enemy_position();
				glm::vec3 cart_pos = cart->position;
				glm::vec3 diff = cart_pos - enemy_pos;
				if ((diff.x) * (diff.x) + (diff.y) * (diff.y) < catch_radius * catch_radius)
				{
					enemy->position = enemy_pos;
					ai[enemy_idx]->position.z = -5;
					chase = true;
					background_music_loop->stop();
					enemy_loop->stop();
					chase_music_loop = Sound::loop(*chase_sample, 0.2f, 0.0f);
					ai_speed = 8.0f;
				}
			}
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			if (cart_velocity.x < 0)
			{
				cart_velocity.x = 0;
			}
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			if (cart_velocity.x > 0)
			{
				cart_velocity.x = 0;
			}
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			if (cart_velocity.y > 0)
			{
				cart_velocity.y = 0;
			}
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			if (cart_velocity.y < 0)
			{
				cart_velocity.y = 0;
			}
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (game_over)
	{
		return;
	}

	// move sound to follow enemy
	enemy_loop->set_position(get_enemy_position(), 1.0f / 60.0f);

	// check proximity to AI
	auto check_dist  = [](const glm::uvec3 diff, float dist) -> bool {
		if ((diff.x) * (diff.x) + (diff.y) * (diff.y) < dist * dist)
		{
			return true;
		}
		return false;
	};

	if (chase)
	{	
		total_time += elapsed;
		if (total_time > survival_time)
		{
			// initialize for next time
			total_time = 0;
			chase = false;

			// swap ai and enemy, hack to change color.
			ai[enemy_idx]->position = enemy->position;
			enemy->position.z = -5.0f;
			ai_speed = ai_wander_speed;

			// create new enemy
			srand((int)time(NULL));
			enemy_idx = rand() % 5;
			enemy_loop = Sound::loop_3D(*ai_sample, 1.0f, get_enemy_position(), 0.1f);

			background_music_loop = Sound::loop(*music_sample, 0.03f, 0.0f);
			chase_music_loop->stop(); // plz don't stop da musik

			// update points
			points += 1;
		}
		else
		{
			for (int i = 0; i < ai.size(); i++)
			{
				glm::uvec3 diff = ai[i]->position - cart->position;
				if (check_dist(diff, attack_radius))
				{
					game_over = true;
					break;
				}
			}
			glm::uvec3 diff_enemy = AI1->position - cart->position;
			if (check_dist(diff_enemy, attack_radius))
			{
				game_over = true;
			}
		}
	}

	//move items:
	{
		// move cart first
		//make it so that moving diagonally doesn't go faster:
		if (cart_velocity != glm::vec2(0.0f))
		{
			cart_velocity = glm::normalize(cart_velocity) * cart_speed;
		}
		cart->position.x += elapsed * cart_velocity.x;
		cart->position.y += elapsed * cart_velocity.y;

		if (!chase)
		{
			for (int i = 0; i < ai.size(); i++)
			{
				ai[i]->position += glm::vec3(cos(ai_orientations[i]), sin(ai_orientations[i]), 0) * ai_speed * elapsed;
				
				// add some noise to each orientation for next update
				float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
				float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
				ai_orientations[i] += (r1 - r2) * 0.25f;
			}
		}
		else
		{
			for (int i = 0; i < ai.size(); i++)
			{
				ai[i]->position += glm::normalize(glm::vec3(cart->position.x, cart->position.y, 0) - glm::vec3(ai[i]->position.x, ai[i]->position.y, 0)) * ai_speed * elapsed;
			}
			enemy->position += glm::normalize(glm::vec3(cart->position.x, cart->position.y,0) - glm::vec3(enemy->position.x, enemy->position.y, 0)) * ai_speed * elapsed;
		}

		// prevent objects from leaving map depending on radius
		auto restrict_inside = [](Scene::Transform * object, const float bound, const float object_radius)
		{
			// stop cart from going out of bounds
			if (object->position.x - object_radius < -bound)
			{
				object->position.x = -bound + object_radius;
			}
			else if (object->position.x + object_radius > bound)
			{
				object->position.x = bound - object_radius;
			}
			if (object->position.y - object_radius < -bound)
			{
				object->position.y = -bound + object_radius;
			}
			else if (object->position.y + object_radius > bound)
			{
				object->position.y = bound - object_radius;
			}
		};
		restrict_inside(cart, (float)arena_radius,cart_radius);
		for (int i = 0; i < ai.size(); i++)
		{
			restrict_inside(ai[i], (float)arena_radius, ai_radius);
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = cart->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}


	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		if (!game_over)
		{
			constexpr float H = 0.09f;
			std::string s = "WASD moves player; points: " + std::to_string(points);
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		else
		{
			constexpr float H = 0.09f;
			std::string s = "You lose; points: " + std::to_string(points);

			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_enemy_position()
{
	return ai[enemy_idx]->position;
}

//glm::vec3 PlayMode::get_leg_tip_position() {
//	//the vertex position here was read from the model in blender:
//	//return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
//}
