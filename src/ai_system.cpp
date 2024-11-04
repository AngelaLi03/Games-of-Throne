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

// // Check if a position is within the grid bounds and walkable
// bool AISystem::isWalkable(int x, int y, const std::vector<std::vector<int>>& grid) {
//     return (x >= 0 && y >= 0 && x < grid.size() && y < grid[0].size() && grid[x][y] == 0);
// }

// // Breadth-First Search to find the shortest path in an unweighted grid
// std::vector<Node> AISystem::findPathBFS(int startX, int startY, int targetX, int targetY, const std::vector<std::vector<int>>& grid) {
//     std::queue<Node> openSet;  // Queue for BFS
//     std::unordered_map<int, Node> allNodes;  // To keep track of all visited nodes

//     // Helper lambda for generating a unique key based on coordinates
//     auto nodeKey = [&](int x, int y) { return y * grid[0].size() + x; };

//     // Initialize the start node
//     Node startNode = {startX, startY, 0, 0, 0, nullptr};
//     openSet.push(startNode);
//     allNodes[nodeKey(startX, startY)] = startNode;

//     // Possible movement directions (up, right, down, left)
//     std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

//     while (!openSet.empty()) {
//         Node current = openSet.front();
//         openSet.pop();

//         // Check if we reached the target
//         if (current.x == targetX && current.y == targetY) {
//             // Reconstruct path by tracing back from the target node
//             std::vector<Node> path;
//             Node* pathNode = &current;
//             while (pathNode) {
//                 path.push_back(*pathNode);
//                 pathNode = pathNode->parent;
//             }
//             std::reverse(path.begin(), path.end());  // Path is constructed backward
//             return path;
//         }

//         // Explore neighbors
//         for (const auto& dir : directions) {
//             int newX = current.x + dir.first;
//             int newY = current.y + dir.second;

//             if (isWalkable(newX, newY, grid)) {
//                 int key = nodeKey(newX, newY);

//                 // Only add the neighbor if it hasn’t been visited before
//                 if (allNodes.find(key) == allNodes.end()) {
//                     Node neighbor = {newX, newY, 0, 0, 0, &allNodes[nodeKey(current.x, current.y)]};
//                     openSet.push(neighbor);
//                     allNodes[key] = neighbor;
//                 }
//             }
//         }
//     }

//     // Return an empty path if no path is found
//     return {};
// }

float distance_squared(vec2 a, vec2 b)
{
	return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
}

float detection_radius_squared = 280.f * 280.f;
float attack_radius_squared = 110.f * 110.f;

void AISystem::step(float elapsed_ms, std::vector<std::vector<int>> &levelMap)
{
	Entity player = registry.players.entities[0];
	assert(player);
	Motion &player_motion = registry.motions.get(player);
	vec2 player_position = player_motion.position + player_motion.bb_offset;
	ComponentContainer<Enemy> &enemies = registry.enemies;
	for (uint i = 0; i < enemies.components.size(); i++)
	{
		Enemy &enemy = enemies.components[i];
		Entity entity = enemies.entities[i];
		Motion &motion = registry.motions.get(entity);
		vec2 enemy_position = motion.position + motion.bb_offset;
		float distance_to_player = distance_squared(player_position, enemy_position);
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
				vec2 direction = player_position - enemy_position;
				direction = normalize(direction);
				motion.velocity = direction * 50.f;

				// int tile_x = std::round(enemy_position.x / TILE_SCALE);
				// int tile_y = std::round(enemy_position.y / TILE_SCALE);
				// std::cout << "At tile " << tile_x << ", " << tile_y << "; " << levelMap[tile_x][tile_y - 1] << "; " << levelMap[tile_x][tile_y] << "; " << levelMap[tile_x][tile_y + 1] << "; " << levelMap[tile_x][tile_y + 2] << std::endl;

				// // find path to player
				// enemy.path = findPathBFS(motion.position.x, motion.position.y, player_motion.position.x, player_motion.position.y, levelMap);
				// if (enemy.path.size() > 0)
				// {
				// 	printf("path found\n");
				// 	// move towards player
				// 	vec2 direction = {enemy.path[enemy.current_path_index].x, enemy.path[enemy.current_path_index].y};
				// 	direction = normalize(direction);
				// 	motion.velocity = direction * 50.f;
				// 	// if enemy is at the current path node, move to the next node
				// 	if (distance_squared(motion.position, glm::vec2(enemy.path[enemy.current_path_index].x, enemy.path[enemy.current_path_index].y)) < 10.f)
				// 	{
				// 		enemy.current_path_index++;
				// 		if (enemy.current_path_index >= enemy.path.size())
				// 		{
				// 			enemy.current_path_index = 0;
				// 		}
				// 	}
				// }
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
		else if (enemy.state == EnemyState::DEAD)
		{
			motion.velocity = {0.f, 0.f};
            continue;
		}
	}
}