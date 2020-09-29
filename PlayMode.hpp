#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform* ball = nullptr;
	Scene::Transform* cart = nullptr;
	Scene::Transform* AI1 = nullptr;
	Scene::Transform* AI2 = nullptr;
	Scene::Transform* AI3 = nullptr;
	Scene::Transform* AI4 = nullptr;
	Scene::Transform* AI5 = nullptr;
	Scene::Transform* enemy = nullptr;
	std::vector<Scene::Transform*> ai;

	glm::vec2 cart_velocity = glm::vec2(0.0f, 0.0f);
	const float cart_speed = 10.0f;
	const float cart_radius = 0.5f;
	const int arena_radius = 20;

	std::vector<float> ai_orientations;
	const float ai_wander_speed = 2.0f;
	float ai_speed = ai_wander_speed;
	const float ai_radius = 1.0f;

	glm::vec3 get_enemy_position();
	int enemy_idx = -1;
	const float catch_radius = 5.0f;
	bool chase = false;
	bool game_over = false;
	const float attack_radius = 1.0f;

	float total_time = 0.0f;
	const float survival_time = 5.0f;

	int points = 0;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > background_music_loop;
	std::shared_ptr< Sound::PlayingSample > chase_music_loop;
	std::shared_ptr< Sound::PlayingSample > enemy_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
