#pragma once

#include <vector>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

class AISystem
{
public:
	void step(float elapsed_ms);
private:
	Mix_Chunk* spy_attack_sound;
};