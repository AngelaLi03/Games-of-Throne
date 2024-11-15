#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	WorldSystem();

	// Creates a window
	GLFWwindow *create_window();

	// starts the game
	void init(RenderSystem *renderer);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over() const;

	std::vector<std::vector<int>> levelMap;

	bool isSGesture();

private:
	void update_camera_view();
	void handle_animations(float elapsed_ms);
	void process_animation(AnimationName name, float t, Entity entity);
	void load_level(const std::string& levelName);

	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 pos);
	void on_mouse_button(int button, int action, int mods);

	// restart level
	void restart_game();

	// OpenGL window handle
	GLFWwindow *window;

	// Game state
	RenderSystem *renderer;
	float current_speed;
	Entity player_salmon;
	Entity player_spy;
	Entity flowMeterEntity;

	// universal attack id, to track unique attack instances (to prevent duplicate damage in the same attack)
	unsigned int attack_id_counter = 0;

	// music references
	Mix_Music *background_music;
	Mix_Chunk *salmon_dead_sound;
	Mix_Chunk *salmon_eat_sound;
	Mix_Chunk *spy_death_sound;
	Mix_Chunk *spy_dash_sound;
	Mix_Chunk *spy_attack_sound;
	Mix_Chunk *break_sound;
	Mix_Chunk *chef_trigger_sound;
	Mix_Chunk *minion_attack_sound;
	Mix_Chunk *heavy_attack_sound;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
