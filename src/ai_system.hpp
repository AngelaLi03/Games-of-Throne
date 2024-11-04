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
	AISystem();
	void step(float elapsed_ms);
	// struct Node
	// {
	// 	int x, y;
	// 	float gCost, hCost, fCost;
	// 	Node *parent; // to trace the path back once we find the target

	// 	// Comparator for priority queue
	// 	bool operator>(const Node &other) const
	// 	{
	// 		return fCost > other.fCost;
	// 	}
	// };
	// std::vector<Node> findPathAStar(int startX, int startY, int targetX, int targetY, const std::vector<std::vector<int>> &grid);
	// void updateEnemyPathfinding(int enemyX, int enemyY, int playerX, int playerY, const std::vector<std::vector<int>> &grid);

private:
	Mix_Chunk *spy_attack_sound;
};