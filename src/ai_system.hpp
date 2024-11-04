#pragma once

#include <vector>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include "node.hpp"

class AISystem
{

public:
    AISystem();
    void step(float elapsed_ms, std::vector<std::vector<int>> &levelMap);
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
    // bool isWalkable(int x, int y, const std::vector<std::vector<int>>& grid);
    // std::vector<Node> findPathBFS(int startX, int startY, int targetX, int targetY, const std::vector<std::vector<int>>& grid);

private:
    Mix_Chunk *spy_attack_sound;
};