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
// #include "dialogue_system.hpp"

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

	Weapon &switchWeapon(Entity player, RenderSystem *renderer, WeaponType newType, WeaponLevel newLevel);
	void trigger_dialogue(std::vector<std::string> dialogue);

	bool is_paused = false;

private:
	void update_camera_view();
	void handle_animations(float elapsed_ms);
	void process_animation(AnimationName name, float t, Entity entity);
	void update_energy(float energy_time);
	void load_level(const std::string &levelName, const int levelNumber);
	void player_action_finished();

	void draw_mesh_debug(Entity mesh_entity);

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
	Entity player_salmon;
	Entity player_spy;
	Entity flowMeterEntity;
	std::vector<std::string> background_dialogue = {
			"Astraeth, 1543",
			"Palace of the royal throne.",
			"In the cursed kingdom of Astraeth, the throne is a symbol of cruelty and fear. ",
			"King Aldred rules with fear, crushing anyone who stands in his way, leaving the public in poverty and despair.",
			"You play as Alexa, a skilled assassin. ",
			"You have trained in the shadows, honing your skills to become the deadliest weapon the people have ever known.",
			"Now, your mission is clear: Overthrow the evil throne and bring hope back to Astraeth.",
	};
	std::vector<std::string> chef_being_attacked_dialogue = {
			"Chef: Who dares to challenge me?",
			"You: Bring it on, I'm not afraid of you!",
			"Chef: You will regret this.",
	};
	std::vector<std::string> chef_death_dialogue = {
			"Chef: You may have defeated me, but you will never defeat the king.",
			"You: Why not?",
			"Chef: The king... is far crueler than I ever was. Good luck, assassin.",
	};
	std::vector<std::string> minion_death_dialogue = {
			"Minion: How can you... I have been trained for years...",
			"You: You must have been sleeping during the years.",
			"Minion: My friends will not let you take over the throne!!!",
	};

	// universal attack id, to track unique attack instances (to prevent duplicate damage in the same attack)
	unsigned int attack_id_counter = 0;

	// music references
	Mix_Music *background_music;
	Mix_Chunk *salmon_dead_sound;
	Mix_Chunk *perfect_dodge_sound;
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
