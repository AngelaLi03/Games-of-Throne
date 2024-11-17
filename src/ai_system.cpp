// internal
#include "ai_system.hpp"

#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <queue>
#include <unordered_map>

#include "world_init.hpp"

float distance_squared(vec2 a, vec2 b)
{
	return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
}

float detection_radius_squared = 280.f * 280.f;
float attack_radius_squared = 110.f * 110.f;

void AISystem::perform_chef_attack(ChefAttack attack)
{
	Chef &chef = registry.chef.components[0];
	Entity chef_entity = registry.chef.entities[0];
	Motion &chef_motion = registry.motions.get(chef_entity);
	Motion &player_motion = registry.motions.get(registry.players.entities[0]);
	vec2 player_position = player_motion.position + player_motion.bb_offset;
	vec2 chef_position = chef_motion.position + chef_motion.bb_offset;
	if (attack == ChefAttack::TOMATO)
	{
		vec2 direction = normalize(player_position - chef_position);
		for (int i = 0; i < 5; i++)
		{
			float angle_offset = -0.3f + (0.6f / 9.0f) * i;
			vec2 spread_direction = vec2((direction.x * cos(angle_offset) - direction.y * sin(angle_offset)), (direction.x * sin(angle_offset) + direction.y * cos(angle_offset)));
			vec2 velocity = spread_direction * 200.f;
			createTomato(renderer, chef_position, velocity);
		}
	}
	else if (attack == ChefAttack::PAN)
	{
		if (chef.pan_active)
		{
			return;
		}
		chef.pan_active = true;
		vec2 direction = normalize(player_position - chef_position);
		vec2 velocity = direction * 500.f;
		createPan(renderer, chef_position, velocity);
	}
	else if (attack == ChefAttack::DASH)
	{
		float dash_speed = 600.f;
		chef.dash_has_damaged = false;

		vec2 direction = normalize(player_position - chef_position);
		chef_motion.velocity = direction * dash_speed;
	}
}

DecisionNode *AISystem::create_chef_decision_tree()
{
	DecisionNode *chef_decision_tree = new DecisionNode(
			nullptr,
			[](float)
			{
				Chef &chef = registry.chef.components[0];
				return chef.state == ChefState::PATROL;
			});

	// Patrol logic

	DecisionNode *patrol_node = new DecisionNode(
			nullptr,
			[](float)
			{
				Entity &entity = registry.chef.entities[0];
				Motion &motion = registry.motions.get(entity);

				Motion &player_motion = registry.motions.get(registry.players.entities[0]);
				vec2 player_position = player_motion.position + player_motion.bb_offset;
				vec2 chef_position = motion.position + motion.bb_offset;
				return distance_squared(player_position, chef_position) < 330.f * 330.f;
			});
	chef_decision_tree->trueBranch = patrol_node;

	DecisionNode *detected_player = new DecisionNode(
			[](float)
			{
				std::cout << "Chef enters combat" << std::endl;
				Chef &chef = registry.chef.components[0];
				Motion &motion = registry.motions.get(registry.chef.entities[0]);
				chef.state = ChefState::COMBAT;
				chef.trigger = true;
				chef.sound_trigger_timer = 1200.f;
				motion.velocity = {0.f, 0.f};
			},
			nullptr);
	patrol_node->trueBranch = detected_player;

	DecisionNode *patrol_time = new DecisionNode(
			[](float elapsed_ms)
			{
				Chef &chef = registry.chef.components[0];
				chef.time_since_last_patrol += elapsed_ms;
			},
			[](float)
			{
				Chef &chef = registry.chef.components[0];
				return chef.time_since_last_patrol > 2000.f;
			});
	patrol_node->falseBranch = patrol_time;

	DecisionNode *change_patrol_direction = new DecisionNode(
			[](float)
			{
				// std::cout << "Chef is patrolling" << std::endl;
				Chef &chef = registry.chef.components[0];
				Entity &entity = registry.chef.entities[0];
				Motion &motion = registry.motions.get(entity);
				motion.velocity.x *= -1;
				chef.time_since_last_patrol = 0.f;
			},
			nullptr);
	patrol_time->trueBranch = change_patrol_direction;

	// Combat logic

	DecisionNode *combat_state_check = new DecisionNode(
			nullptr,
			[](float)
			{
				Chef &chef = registry.chef.components[0];
				return chef.state == ChefState::COMBAT;
			});
	chef_decision_tree->falseBranch = combat_state_check;

	DecisionNode *combat_node = new DecisionNode(
			[](float elapsed_ms)
			{
				Chef &chef = registry.chef.components[0];
				chef.time_since_last_attack += elapsed_ms;
			},
			[](float)
			{
				Chef &chef = registry.chef.components[0];
				return chef.time_since_last_attack > 3000.f;
			});
	combat_state_check->trueBranch = combat_node;

	DecisionNode *initiate_attack = new DecisionNode(
			[](float)
			{
				Chef &chef = registry.chef.components[0];
				chef.state = ChefState::ATTACK;
			},
			nullptr);
	combat_node->trueBranch = initiate_attack;

	DecisionNode *attack_node = new DecisionNode(
			[this](float)
			{
				Chef &chef = registry.chef.components[0];
				std::cout << "Chef attacks " << (int)chef.current_attack << std::endl;

				this->perform_chef_attack(chef.current_attack);
				// set is attacking to true
				auto &bossAnimation = registry.bossAnimations.get(registry.chef.entities[0]);
				bossAnimation.is_attacking = true;
				bossAnimation.elapsed_time = 0.f;
				bossAnimation.attack_id = static_cast<int>(chef.current_attack); // Cast to int
				bossAnimation.current_frame = 1;
				// Reset attack and choose next attack
				chef.time_since_last_attack = 0.f;
				chef.current_attack = static_cast<ChefAttack>(rand() % static_cast<int>(ChefAttack::ATTACK_COUNT));
				if (chef.current_attack == ChefAttack::SPIN)
				{
					// TODO: not implemented yet
					chef.current_attack = ChefAttack::DASH;
				}
				chef.state = ChefState::COMBAT;
			},
			nullptr);
	combat_state_check->falseBranch = attack_node;

	return chef_decision_tree;
}

AISystem::AISystem()
{
	chef_decision_tree = create_chef_decision_tree();
}

AISystem::~AISystem()
{
	delete chef_decision_tree;
}

void AISystem::init(RenderSystem *renderer)
{
	this->renderer = renderer;
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

void AISystem::step(float elapsed_ms, std::vector<std::vector<int>> &levelMap)
{
	Entity player = registry.players.entities[0];
	assert(player);

	Player &player_comp = registry.players.get(player);
	if (player_comp.state == PlayerState::DYING)
	{
		// skip all ai processing if player is dead; also make enemies stop moving/attacking
		ComponentContainer<Enemy> &enemies = registry.enemies;
		for (uint i = 0; i < enemies.components.size(); i++)
		{
			Enemy &enemy = enemies.components[i];
			Entity entity = enemies.entities[i];
			Motion &motion = registry.motions.get(entity);
			motion.velocity = {0.f, 0.f};
			if (registry.spriteAnimations.has(entity))
			{
				auto &animation = registry.spriteAnimations.get(entity);
				auto &render_request = registry.renderRequests.get(entity);
				animation.current_frame = 0;
				render_request.used_texture = animation.frames[animation.current_frame];
				registry.spriteAnimations.remove(entity);
			}
			enemy.state = EnemyState::IDLE;
		}
		return;
	}

	Motion &player_motion = registry.motions.get(player);
	vec2 player_position = player_motion.position + player_motion.bb_offset;
	ComponentContainer<Enemy> &enemies = registry.enemies;
	for (uint i = 0; i < enemies.components.size(); i++)
	{
		Enemy &enemy = enemies.components[i];
		Entity entity = enemies.entities[i];

		if (registry.chef.has(entity) || registry.knight.has(entity)) // Skip all bosses
		{
			continue;
		}

		Motion &motion = registry.motions.get(entity);
		vec2 enemy_position = motion.position + motion.bb_offset;
		float distance_to_player = distance_squared(player_position, enemy_position);
		if (enemy.state == EnemyState::IDLE)
		{
			if (distance_to_player < detection_radius_squared)
			{
				enemy.state = EnemyState::COMBAT;
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
				if (enemy.time_since_last_attack > 2000.f)
				{
					// attack player
					// std::cout << "Enemy " << i << " attacks player" << std::endl;
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

					createDamageArea(entity, motion.position, {100.f, 70.f}, 10.f, 200.f, 500.f, 0.f, true, {50.f, 50.f});
				}
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

	if (registry.chef.size() > 0)
	{
		// special behavior for chef
		Entity chef_entity = registry.chef.entities[0];
		Health &chef_health = registry.healths.get(chef_entity);
		if (!chef_health.is_dead)
		{
			chef_decision_tree->execute(elapsed_ms);
		}
		// perform animation if chef is attacking
		auto &bossAnimation = registry.bossAnimations.get(chef_entity);

		if (bossAnimation.is_attacking)
		{
			boss_attack(chef_entity, bossAnimation.attack_id, elapsed_ms);
		}
	}

	if (registry.knight.size() > 0)
	{
		// special behavior for knight
		Entity knight_entity = registry.knight.entities[0];
		Health &knight_health = registry.healths.get(knight_entity);
		if (!knight_health.is_dead)
		{
			// TODO: knight_decision_tree->execute(elapsed_ms);
		}
		// perform animation if knight is attacking
		auto &bossAnimation = registry.bossAnimations.get(knight_entity);

		if (bossAnimation.is_attacking)
		{
			printf("knight is attacking\n");
			// TODO: uncomment when knight enum created and attack implemented
			// boss_attack(knight_entity, bossAnimation.attack_id, elapsed_ms);
		}
	}
}

void AISystem::boss_attack(Entity entity, int attack_id, float elapsed_ms)
{
	// change to attack animation sprite
	auto &bossAnimation = registry.bossAnimations.get(entity);
	auto &render_request = registry.renderRequests.get(entity);
	bossAnimation.elapsed_time += elapsed_ms;

	std::vector<TEXTURE_ASSET_ID> &frames = bossAnimation.attack_1;
	// get motion
	auto &motion = registry.motions.get(entity);

	switch (attack_id)
	{
	case 0:
		frames = bossAnimation.attack_1;
		bossAnimation.frame_duration = 100.f;
		break;
	case 1:
		frames = bossAnimation.attack_2;
		bossAnimation.frame_duration = 70.f;
		break;
	case 2:
		frames = bossAnimation.attack_3;
		bossAnimation.frame_duration = 100.f;
		break;
	case 3:
		frames = bossAnimation.attack_1; // modify if have >3 animations, default to attack_1
		break;
	case 4:
		frames = bossAnimation.attack_1;
		break;
	}

	if (bossAnimation.elapsed_time >= bossAnimation.frame_duration)
	{
		motion.scale.x /= 1.6;
		// Reset elapsed time for the next frame
		bossAnimation.elapsed_time = 0.0f;

		// Move to the next frame
		bossAnimation.current_frame = (bossAnimation.current_frame + 1) % frames.size();
		render_request.used_texture = frames[bossAnimation.current_frame];
		motion.scale.x *= 1.6;
	}
	if (bossAnimation.current_frame == 0 && bossAnimation.is_attacking)
	{
		bossAnimation.is_attacking = false;
		// change to combat animation sprite
		render_request.used_texture = frames[0];
	}
}