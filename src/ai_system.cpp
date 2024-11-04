// internal
#include "ai_system.hpp"

#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <queue>
#include <unordered_map>

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
	// spy_attack_sound = Mix_LoadWAV(audio_path("spy_attack.wav").c_str());
}

// Calculate Manhattan distance heuristic
// float calculateHCost(int x1, int y1, int x2, int y2)
// {
// 	return std::abs(x1 - x2) + std::abs(y1 - y2);
// }

// // Check if a position is within the grid bounds and walkable
// bool isWalkable(int x, int y, const std::vector<std::vector<int>> &grid)
// {
// 	return (x >= 0 && y >= 0 && x < grid.size() && y < grid[0].size() && grid[x][y] == 0);
// }

// std::vector<AISystem::Node> findPathAStar(int startX, int startY, int targetX, int targetY, const std::vector<std::vector<int>> &grid)
// {

// 	std::priority_queue<AISystem::Node, std::vector<AISystem::Node>, std::greater<AISystem::Node>> openSet;
// 	std::unordered_map<int, AISystem::Node> allNodes; // To keep track of all created nodes

// 	// Lambda for calculating unique node keys based on position
// 	auto nodeKey = [](int x, int y, int width)
// 	{ return y * width + x; };

// 	// Initialize the start node
// 	AISystem::Node startNode = {startX, startY, 0, calculateHCost(startX, startY, targetX, targetY), 0, nullptr};
// 	openSet.push(startNode);
// 	allNodes[nodeKey(startX, startY, grid[0].size())] = startNode;

// 	// Define possible directions (up, down, left, right)
// 	std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

// 	while (!openSet.empty())
// 	{
// 		AISystem::Node current = openSet.top();
// 		openSet.pop();

// 		// Check if we reached the target
// 		if (current.x == targetX && current.y == targetY)
// 		{
// 			std::vector<AISystem::Node> path;
// 			AISystem::Node *pathNode = &current;
// 			while (pathNode)
// 			{
// 				path.push_back(*pathNode);
// 				pathNode = pathNode->parent;
// 			}
// 			std::reverse(path.begin(), path.end());
// 			return path;
// 		}

// 		// Explore neighbors
// 		for (const auto &dir : directions)
// 		{
// 			int newX = current.x + dir.first;
// 			int newY = current.y + dir.second;

// 			if (isWalkable(newX, newY, grid))
// 			{
// 				float newGCost = current.gCost + 1;
// 				float newHCost = calculateHCost(newX, newY, targetX, targetY);
// 				float newFCost = newGCost + newHCost;

// 				// Check if this path to neighbor is better or if it hasn't been visited yet
// 				int key = nodeKey(newX, newY, grid[0].size());
// 				if (allNodes.find(key) == allNodes.end() || newGCost < allNodes[key].gCost)
// 				{
// 					AISystem::Node neighbor = {newX, newY, newGCost, newHCost, newFCost, &allNodes[nodeKey(current.x, current.y, grid[0].size())]};
// 					openSet.push(neighbor);
// 					allNodes[key] = neighbor;
// 				}
// 			}
// 		}
// 	}

// 	// Return empty path if no path is found
// 	return {};
// }

// // Use findPathAStar to update enemy positions based on player location
// void updateEnemyPathfinding(int enemyX, int enemyY, int playerX, int playerY, const std::vector<std::vector<int>> &grid)
// {
// 	auto path = findPathAStar(enemyX, enemyY, playerX, playerY, grid);

// 	if (!path.empty())
// 	{
// 		// Example of using the path: move enemy to the next position in the path
// 		AISystem::Node nextPosition = path[1]; // path[0] would be the current position
// 		// Here, you would set the enemy’s position or direction towards nextPosition.x, nextPosition.y
// 	}
// }

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
					// get chef
					auto &chef = registry.chef.get(entity);
					chef.trigger = true;
					chef.sound_trigger_timer = 1200;
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
					// change to attack animation sprite
					enemy.state = EnemyState::ATTACK;
					auto &animation = registry.spriteAnimations.get(entity);
        			auto &render_request = registry.renderRequests.get(entity);

					animation.current_frame = 1; 
					render_request.used_texture = animation.frames[animation.current_frame];

					// get motion of the enemy
					auto &enemy_motion = registry.motions.get(entity);
					enemy_motion.scale.x *= 1.1;

					enemy.time_since_last_attack = 0.f;
					// Mix_PlayChannel(-1, spy_attack_sound, 0);

					float damage = 10.f; // Define the damage value
                    if (registry.healthbar.has(player))
                    {
                        HealthBar &health_bar = registry.healthbar.get(player);
                        health_bar.current_health -= damage;
                        if (health_bar.current_health < 0.f)
                            health_bar.current_health = 0.f;							
                    }
                    if (registry.healths.has(player))
                    {
                        Health &player_health = registry.healths.get(player);
                        player_health.health -= damage;
                        if (player_health.health < 0.f)
                            player_health.health = 0.f;
						for (Entity health_bar_entity : registry.healthbarlink.entities)
						{
							HealthBarLink &healthbarlink = registry.healthbarlink.get(health_bar_entity);
							if (healthbarlink.owner == player)
							{
								if (registry.healthbar.has(health_bar_entity))
								{
									HealthBar &health_bar = registry.healthbar.get(health_bar_entity);
									health_bar.current_health = player_health.health;
								}
								break;
							}
						}
                    }
					
				}
			}
			// if chef, set trigger to false
			if (registry.chef.has(entity))
			{
				auto &chef = registry.chef.get(entity);
				if (chef.sound_trigger_timer == 0)
					chef.trigger = false;
			}
		}
		// reset state to combat after attack
		else if (enemy.state == EnemyState::ATTACK)
		{
			auto &animation = registry.spriteAnimations.get(entity);
			auto &render_request = registry.renderRequests.get(entity);
			enemy.attack_countdown -= elapsed_ms;

			if (enemy.attack_countdown <= 0)
			{
				enemy.state = EnemyState::COMBAT;
				printf("Enemy %d finish attack\n", i);
				enemy.attack_countdown = 500;
				// change to combat animation sprite
				render_request.used_texture = animation.frames[0];
				// get motion of the enemy
				auto &enemy_motion = registry.motions.get(entity);
				enemy_motion.scale.x /= 1.1;
			}
		}
		// reset state to combat after attack
		else if (enemy.state == EnemyState::ATTACK)
		{
			auto &animation = registry.spriteAnimations.get(entity);
			auto &render_request = registry.renderRequests.get(entity);
			enemy.attack_countdown -= elapsed_ms;

			if (enemy.attack_countdown <= 0)
			{
				enemy.state = EnemyState::COMBAT;
				printf("Enemy %d finish attack\n", i);
				enemy.attack_countdown = 500;
				// change to combat animation sprite
				render_request.used_texture = animation.frames[0];
				// get motion of the enemy
				auto &enemy_motion = registry.motions.get(entity);
				enemy_motion.scale.x /= 1.1;
			}
		}

	}
}