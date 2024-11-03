// internal
#include "ai_system.hpp"

#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

AISystem::AISystem()
{
	// Loading music and sounds with SDL
	// if (SDL_Init(SDL_INIT_AUDIO) < 0)
	// {
	// 	fprintf(stderr, "Failed to initialize SDL Audio");
	// 	return nullptr;
	// }
	// if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
	// {
	// 	fprintf(stderr, "Failed to open audio device");
	// 	return nullptr;
	// }
	spy_attack_sound = Mix_LoadWAV(audio_path("spy_attack.wav").c_str());
}

float distance_squared(vec2 a, vec2 b)
{
	return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
}

float detection_radius_squared = 240.f * 240.f;
float attack_radius_squared = 110.f * 110.f;

void AISystem::step(float elapsed_ms)
{
	Entity player = registry.players.entities[0];
	assert(player);
	Motion &player_motion = registry.motions.get(player);
	ComponentContainer<Enemy> &enemies = registry.enemies;
	for (uint i = 0; i < enemies.components.size(); i++)
	{
		Enemy &enemy = enemies.components[i];
		Entity entity = enemies.entities[i];
		Motion &motion = registry.motions.get(entity);
		float distance_to_player = distance_squared(player_motion.position, motion.position);
		if (enemy.state == EnemyState::IDLE)
		{
			if (distance_to_player < detection_radius_squared)
			{
				enemy.state = EnemyState::COMBAT;
				// if enemy is a chef
				if (registry.chef.has(entity))
				{
					std::cout << "Chef enters combat" << std::endl;
				}
				else
					std::cout << "Enemy " << i << " enters combat" << std::endl;
			}
		}
		else if (enemy.state == EnemyState::COMBAT)
		{

			if (distance_to_player > detection_radius_squared * 2)
			{
				enemy.state = EnemyState::IDLE;
				motion.velocity = {0.f, 0.f};
				std::cout << "Enemy " << i << " enters idle" << std::endl;
			}
			else if (distance_to_player > attack_radius_squared)
			{
				// move towards player
				vec2 direction = player_motion.position - motion.position;
				direction = normalize(direction);
				motion.velocity = direction * 50.f;
			}
			else
			{
				motion.velocity = {0.f, 0.f};
				enemy.time_since_last_attack += elapsed_ms;
				if (enemy.time_since_last_attack > 1000.f)
				{
					// attack player
					std::cout << "Enemy " << i << " attacks player" << std::endl;
					enemy.time_since_last_attack = 0.f;
					Mix_PlayChannel(-1, spy_attack_sound, 0);
				}
			}
		}
	}
}