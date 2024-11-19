// internal
#include "ai_system.hpp"

#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <queue>
#include <unordered_map>
#include "world_system.hpp"
#include "world_init.hpp"

extern std::vector<std::vector<int>> level_grid;
float MINION_SPEED = 80.f;

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

void AISystem::perform_knight_attack(KnightAttack attack)
{
	if (registry.knight.size() == 0 || registry.players.size() == 0)
		return;

	Knight &knight = registry.knight.components[0];
	Entity knight_entity = registry.knight.entities[0];

	if (!registry.motions.has(knight_entity))
		return;

	Motion &knight_motion = registry.motions.get(knight_entity);
	Motion &player_motion = registry.motions.get(registry.players.entities[0]);
	vec2 player_position = player_motion.position + player_motion.bb_offset;
	vec2 knight_position = knight_motion.position + knight_motion.bb_offset;
	vec2 direction = normalize(player_position - knight_position);

	switch (attack)
	{
	case KnightAttack::DASH_ATTACK:
		knight.state = KnightState::ATTACK;
		knight.attack_duration = 10000.f;
		knight.time_since_last_attack = 0.f;
		knight.dash_has_damaged = false;
		break;

	case KnightAttack::SHIELD_HOLD:
		knight.shield_active = true;
		knight.shield_broken = false;
		knight.shield_duration = 3000.f;
		knight_motion.velocity = {0.f, 0.f};
		break;

	case KnightAttack::MULTI_DASH:
		knight.state = KnightState::MULTI_DASH;
		knight.dash_count = 0;
		knight.time_since_last_attack = 0.f;
		knight.dash_has_damaged = false;
		break;

	default:
		break;
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

DecisionNode *AISystem::create_knight_decision_tree()
{
	DecisionNode *knight_decision_tree = new DecisionNode(
			nullptr,
			[](float)
			{
				if (registry.knight.size() == 0)
					return false;

				Knight &knight = registry.knight.components[0];
				return knight.state == KnightState::PATROL;
			});

	// PATROL logic
	DecisionNode *patrol_node = new DecisionNode(
			nullptr,
			[](float)
			{
				Entity &entity = registry.knight.entities[0];
				Motion &motion = registry.motions.get(entity);

				Motion &player_motion = registry.motions.get(registry.players.entities[0]);
				vec2 player_position = player_motion.position + player_motion.bb_offset;
				vec2 knight_position = motion.position + motion.bb_offset;
				return distance_squared(player_position, knight_position) < detection_radius_squared;
			});
	knight_decision_tree->trueBranch = patrol_node;

	DecisionNode *detected_player = new DecisionNode(
			[](float)
			{
				std::cout << "Knight enters combat" << std::endl;
				Knight &knight = registry.knight.components[0];
				Motion &motion = registry.motions.get(registry.knight.entities[0]);
				knight.state = KnightState::COMBAT;
				motion.velocity = {0.f, 0.f};
			},
			nullptr);
	patrol_node->trueBranch = detected_player;

	DecisionNode *patrol_time = new DecisionNode(
			[](float elapsed_ms)
			{
				Knight &knight = registry.knight.components[0];
				knight.time_since_last_patrol += elapsed_ms;
			},
			[](float)
			{
				Knight &knight = registry.knight.components[0];
				return knight.time_since_last_patrol > 2000.f;
			});
	patrol_node->falseBranch = patrol_time;

	DecisionNode *change_patrol_direction = new DecisionNode(
			[](float)
			{
				Knight &knight = registry.knight.components[0];
				Entity &entity = registry.knight.entities[0];
				Motion &motion = registry.motions.get(entity);
				if (motion.velocity.x == 0)
				{
					motion.velocity.x = 50.f; // Set initial patrol speed
				}
				else
				{
					motion.velocity.x *= -1; // Change direction
				}
				knight.time_since_last_patrol = 0.f;
			},
			nullptr);
	patrol_time->trueBranch = change_patrol_direction;

	// COMBAT logic
	DecisionNode *combat_state_check = new DecisionNode(
			nullptr,
			[](float)
			{
				Knight &knight = registry.knight.components[0];
				return knight.state == KnightState::COMBAT;
			});
	knight_decision_tree->falseBranch = combat_state_check;

	DecisionNode *attack_selection = new DecisionNode(
			[this](float)
			{
				Knight &knight = registry.knight.components[0];

				// Randomly select an attack
				int random_attack = rand() % 3;
				knight.current_attack = static_cast<KnightAttack>(random_attack);
				this->perform_knight_attack(knight.current_attack);
			},
			nullptr);

	combat_state_check->trueBranch = attack_selection;

	return knight_decision_tree;
}

AISystem::AISystem()
{
	chef_decision_tree = create_chef_decision_tree();
	knight_decision_tree = create_knight_decision_tree();
}

AISystem::~AISystem()
{
	delete chef_decision_tree;
	delete knight_decision_tree;
}

void AISystem::init(RenderSystem *renderer)
{
	this->renderer = renderer;
}

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

		vec2 adjusted_position = enemy_position;

		// consider enemy as still on the last tile if it has not fully moved onto a new tile
		// -- this solves the issue of turning corners
		if (glm::length(enemy_position - enemy.last_tile_position) < TILE_SCALE)
		{
			adjusted_position = enemy.last_tile_position;
		}

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
				std::cout << "Enemy " << i << " returns to idle" << std::endl;
			}
			else
			{

				// std::cout << "Level Grid:" << std::endl;
				// for (int y = 0; y < level_grid[0].size(); ++y)
				// {
				// 	for (int x = 0; x < level_grid.size(); ++x)
				// 	{
				// 		std::cout << level_grid[x][y] << " ";
				// 	}
				// 	std::cout << std::endl;
				// }
				// return;

				std::vector<vec2> path = findPathAStar(adjusted_position, player_position);

				if (debugging.in_debug_mode)
				{
					for (int i = 0; i < path.size(); i++)
					{
						vec2 path_point = path[i];
						createLine(path_point, {10.f, 10.f}, {1.f, 0.f, 0.f}, 0.f);
					}
				}

				if (path.size() >= 1)
				{
					// stagger update last_tile_position only when enemy is fully on the next tile
					if (glm::length(enemy_position - enemy.last_tile_position) > TILE_SCALE)
					{
						// std::cout << "LAST TILE POS UPDATED" << std::endl;
						enemy.last_tile_position = path[0];
					}
				}

				if (path.size() >= 2)
				{
					// Move towards the next point in the path
					vec2 target_position = path[1]; // The next node in the path
					vec2 direction = normalize(target_position - enemy_position);
					motion.velocity = direction * MINION_SPEED;
					// std::cout << "enemy_position: " << enemy_position.x << ", " << enemy_position.y << "; adjusted_position: " << adjusted_position.x << ", " << adjusted_position.y << "; direction: " << direction.x << ", " << direction.y << std::endl;

					enemy.path = path;
				}
				else
				{
					// No path found or already at the goal
					motion.velocity = {0.f, 0.f};
				}

				if (distance_to_player <= attack_radius_squared)
				{
					motion.velocity = {0.f, 0.f};
					enemy.time_since_last_attack += elapsed_ms;
					if (enemy.time_since_last_attack > 2000.f)
					{
						// Attack logic
						enemy.state = EnemyState::ATTACK;

						// get motion of the enemy
						auto &enemy_motion = registry.motions.get(entity);
						enemy_motion.scale.x *= 1.1;

						enemy.time_since_last_attack = 0.f;

						createDamageArea(entity, motion.position, {100.f, 70.f}, 10.f, 500.f, 0.f, true, {50.f, 50.f});
					}
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
		Entity knight_entity = registry.knight.entities[0];
		Health &knight_health = registry.healths.get(knight_entity);
		if (!knight_health.is_dead)
		{
			knight_decision_tree->execute(elapsed_ms);
			Knight &knight = registry.knight.get(knight_entity);

			Motion &knight_motion = registry.motions.get(knight_entity);
			Motion &player_motion = registry.motions.get(registry.players.entities[0]);
			vec2 player_position = player_motion.position + player_motion.bb_offset;
			vec2 knight_position = knight_motion.position + knight_motion.bb_offset;

			if (knight.state == KnightState::PATROL)
			{
				// Patrol movement is handled in the decision tree
			}
			else if (knight.state == KnightState::COMBAT)
			{
				// Move towards player
				vec2 direction = normalize(player_position - knight_position);
				knight_motion.velocity = direction * 100.f; // Adjust speed as needed
			}
			else if (knight.state == KnightState::SHIELD)
			{
				knight.shield_duration -= elapsed_ms;
				if (knight.shield_duration <= 0.f || knight.shield_broken)
				{
					knight.shield_active = false;
					knight.state = KnightState::COMBAT;
				}
				knight_motion.velocity = {0.f, 0.f}; // Stay stationary during shield
			}
			else if (knight.state == KnightState::ATTACK)
			{
				knight.attack_duration -= elapsed_ms;
				knight.time_since_last_attack += elapsed_ms;

				if (knight.attack_duration <= 0.f)
				{
					knight.state = KnightState::COMBAT;
					knight_motion.velocity = {0.f, 0.f};
				}
				else if (knight.time_since_last_attack >= 2000.f)
				{
					vec2 direction = normalize(player_position - knight_position);
					knight_motion.velocity = direction * 300.f;
					knight.dash_has_damaged = false;
					knight.time_since_last_attack = 0.f;

					// Start attack animation
					auto &bossAnimation = registry.bossAnimations.get(knight_entity);
					bossAnimation.is_attacking = true;
					bossAnimation.elapsed_time = 0.f;
					bossAnimation.attack_id = static_cast<int>(KnightAttack::DASH_ATTACK);
					bossAnimation.current_frame = 0;
				}
			}
			else if (knight.state == KnightState::MULTI_DASH)
			{
				knight.time_since_last_attack += elapsed_ms;

				if (knight.dash_count >= 3)
				{
					create_damage_field(knight_entity);
					knight.state = KnightState::COMBAT;
					knight_motion.velocity = {0.f, 0.f};
				}
				else if (knight.time_since_last_attack >= 1500.f)
				{
					vec2 direction = normalize(player_position - knight_position);
					knight_motion.velocity = direction * 400.f;
					knight.dash_has_damaged = false;
					knight.time_since_last_attack = 0.f;
					knight.dash_count++;

					// Start attack animation
					auto &bossAnimation = registry.bossAnimations.get(knight_entity);
					bossAnimation.is_attacking = true;
					bossAnimation.elapsed_time = 0.f;
					bossAnimation.attack_id = static_cast<int>(KnightAttack::MULTI_DASH);
					bossAnimation.current_frame = 0;
				}
			}
		}

		auto &bossAnimation = registry.bossAnimations.get(knight_entity);

		if (bossAnimation.is_attacking)
		{
			boss_attack(knight_entity, bossAnimation.attack_id, elapsed_ms);
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

	if (frames.empty())
	{
		std::cerr << "Error: No frames available for Knight's attack animation (attack_id: " << attack_id << ")." << std::endl;
		bossAnimation.is_attacking = false;
		return;
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

float heuristic(int x1, int y1, int x2, int y2)
{
	return static_cast<float>(std::abs(x1 - x2) + std::abs(y1 - y2)); // Manhattan distance
}

bool isWalkable(int x, int y)
{
	if (x >= 0 && y >= 0 && x < level_grid.size() && y < level_grid[x].size())
	{
		return level_grid[x][y] == 1;
	}
	return false;
}

int nodeKey(int x, int y, int maxHeight)
{
	return x * maxHeight + y;
}

std::vector<vec2> AISystem::findPathAStar(vec2 startPos, vec2 goalPos)
{
	const int TILE_SIZE = 60;
	int startX = static_cast<int>(startPos.x / TILE_SIZE);
	int startY = static_cast<int>(startPos.y / TILE_SIZE);
	int goalX = static_cast<int>(goalPos.x / TILE_SIZE);
	int goalY = static_cast<int>(goalPos.y / TILE_SIZE);

	// std::cout << "===========================" << std::endl;

	// for (int dy = -2; dy <= 2; dy++)
	// {
	// 	for (int dx = -2; dx <= 2; dx++)
	// 	{
	// 		std::cout << isWalkable(goalX + dx, goalY + dy) << " ";
	// 	}
	// 	std::cout << std::endl;
	// }
	// std::cout << "===========================" << std::endl;

	int maxHeight = level_grid[0].size();

	// Comparator for priority queue
	struct CompareAStarNode
	{
		bool operator()(AStarNode *a, AStarNode *b)
		{
			return a->fCost > b->fCost;
		}
	};

	std::priority_queue<AStarNode *, std::vector<AStarNode *>, CompareAStarNode> openSet;
	std::unordered_map<int, AStarNode *> allNodes;

	int key = nodeKey(startX, startY, maxHeight);
	AStarNode *startNode = new AStarNode{startX, startY, 0, heuristic(startX, startY, goalX, goalY), 0, nullptr};
	startNode->fCost = startNode->gCost + startNode->hCost;
	openSet.push(startNode);
	allNodes[key] = startNode;

	std::vector<std::pair<int, int>> directions = {
			{0, -1},	// Up
			{1, 0},		// Right
			{0, 1},		// Down
			{-1, 0},	// Left
			{-1, -1}, // Up-Left
			{1, -1},	// Up-Right
			{1, 1},		// Down-Right
			{-1, 1}		// Down-Left
	};

	while (!openSet.empty())
	{
		// Get the node with the lowest fCost
		AStarNode *current = openSet.top();
		openSet.pop();

		// If we've reached the goal, reconstruct and return the path
		if (current->x == goalX && current->y == goalY)
		{
			// Reconstruct path
			std::vector<vec2> path;
			AStarNode *node = current;
			while (node != nullptr)
			{
				path.push_back(vec2{node->x * TILE_SIZE + TILE_SIZE / 2.0f,
														node->y * TILE_SIZE + TILE_SIZE / 2.0f});
				node = node->parent;
			}
			std::reverse(path.begin(), path.end());

			// Clean up all dynamically allocated nodes
			for (auto &pair : allNodes)
			{
				delete pair.second;
			}
			return path;
		}

		// Explore all valid neighbors of the current node
		for (const auto &dir : directions)
		{
			int nx = current->x + dir.first;	// Neighbor x-coordinate
			int ny = current->y + dir.second; // Neighbor y-coordinate

			if (dir.first != 0 && dir.second != 0)
			{ // Diagonal movement
				if (!isWalkable(current->x + dir.first, current->y) ||
						!isWalkable(current->x, current->y + dir.second))
				{
					continue;
				}
			}

			if (isWalkable(nx, ny))
			{
				int neighborKey = nodeKey(nx, ny, maxHeight); // Generate unique key for the neighbor
				// float gCost = current->gCost + ((dir.first != 0 && dir.second != 0) ? 1.4f : 1.0f); // Diagonal cost is ~?2 (1.4)
				float baseCost = (dir.first != 0 && dir.second != 0) ? 1.4f : 1.0f;
				float gCost = current->gCost + baseCost;
				float hCost = heuristic(nx, ny, goalX, goalY); // Calculate heuristic cost (Manhattan distance)
				float fCost = gCost + hCost;									 // Total cost = gCost + hCost

				if (allNodes.find(neighborKey) == allNodes.end() || gCost < allNodes[neighborKey]->gCost)
				{
					// Create or update the neighbor node
					AStarNode *neighbor = new AStarNode{nx, ny, gCost, hCost, fCost, current};
					openSet.push(neighbor);						// Add to the open set
					allNodes[neighborKey] = neighbor; // Update the allNodes map
				}
			}
		}
	}

	// No path found; clean up dynamically allocated nodes
	for (auto &pair : allNodes)
	{
		delete pair.second;
	}

	return {};
}

// sources for A*:
// https://www.youtube.com/watch?v=-L-WgKMFuhE
// https://www.youtube.com/watch?v=NJOf_MYGrYs&t=876s

void AISystem::create_damage_field(Entity knight_entity)
{
	Motion &knight_motion = registry.motions.get(knight_entity);

	Entity damage_field = createDamageArea(knight_entity, knight_motion.position, vec2(200.f, 200.f), 3.0f * 50.f, 1000.f);

	registry.deathTimers.emplace(damage_field, DeathTimer{2000.f}); // 2 seconds duration
}
