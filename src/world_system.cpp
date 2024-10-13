// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "iostream"
// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration
int ENEMIES_COUNT = 5;
vec2 player_movement_direction = {0.f, 0.f};
vec2 curr_mouse_position;

// create the underwater world
WorldSystem::WorldSystem()
		: points(0)
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem()
{

	// destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (salmon_dead_sound != nullptr)
		Mix_FreeChunk(salmon_dead_sound);
	if (salmon_eat_sound != nullptr)
		Mix_FreeChunk(salmon_eat_sound);
	if (spy_death_sound != nullptr)
		Mix_FreeChunk(spy_death_sound);
	if (spy_dash_sound != nullptr)
		Mix_FreeChunk(spy_dash_sound);
	if (spy_attack_sound != nullptr)
		Mix_FreeChunk(spy_attack_sound);
	if(break_sound != nullptr)
		Mix_FreeChunk(break_sound);
		

	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace
{
	void glfw_err_cb(int error, const char *desc)
	{
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow *WorldSystem::create_window()
{
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_width_px, window_height_px, "Salmon Game Assignment", nullptr, nullptr);
	if (window == nullptr)
	{
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2, int _3)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow *wnd, double _0, double _1)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_move({_0, _1}); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
	{
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	// background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	background_music = Mix_LoadMUS(audio_path("soundtrack_1.wav").c_str());
	salmon_dead_sound = Mix_LoadWAV(audio_path("death_sound.wav").c_str());
	salmon_eat_sound = Mix_LoadWAV(audio_path("eat_sound.wav").c_str());
	spy_death_sound = Mix_LoadWAV(audio_path("spy_death.wav").c_str());
	spy_dash_sound = Mix_LoadWAV(audio_path("spy_dash.wav").c_str());
	spy_attack_sound = Mix_LoadWAV(audio_path("spy_attack.wav").c_str());
	break_sound = Mix_LoadWAV(audio_path("break.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr || spy_death_sound == nullptr || spy_dash_sound == nullptr || spy_attack_sound == nullptr || break_sound == nullptr)
	{
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
						audio_path("music.wav").c_str(),
						audio_path("death_sound.wav").c_str(),
						audio_path("eat_sound.wav").c_str(),
						audio_path("spy_death.wav").c_str(),
						audio_path("spy_dash.wav").c_str(),
						audio_path("spy_attack.wav").c_str(),
						audio_path("break.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem *renderer_arg)
{
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
	restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
		registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto &motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i)
	{
		Motion &motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f)
		{
			if (!registry.players.has(motions_registry.entities[i])) // don't remove the player
				registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// Processing the salmon state
	assert(registry.screenStates.components.size() <= 1);
	ScreenState &screen = registry.screenStates.components[0];

	float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities)
	{
		// progress timer
		DeathTimer &counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if (counter.counter_ms < min_counter_ms)
		{
			min_counter_ms = counter.counter_ms;
		}

		// restart the game once the death timer expired
		if (counter.counter_ms < 0)
		{
			registry.deathTimers.remove(entity);
			screen.darken_screen_factor = 0;
			restart_game();
			return true;
		}
	}
	// reduce window brightness if the salmon is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	for (Entity entity : registry.interpolations.entities)
	{

		Interpolation &interpolate = registry.interpolations.get(entity);
		Motion &motion = registry.motions.get(entity);

		interpolate.elapsed_time += elapsed_ms_since_last_update;

		float t = (float)interpolate.elapsed_time / (float)interpolate.total_time_to_0_ms;
		t = std::min(t, 1.0f);

		float interpolation_factor = t * t * (3.0f - 2.0f * t); // basically 3t^2 - 2t^3 -> cubic

		motion.velocity.x = (1.0f - interpolation_factor) * interpolate.initial_velocity.x;
		motion.velocity.y = (1.0f - interpolation_factor) * interpolate.initial_velocity.y;

		// Remove the entity if the interpolation is complete (velocity should be near zero)
		if (interpolation_factor >= 1.0f)
		{
			registry.interpolations.remove(entity);
			// std::cout << "removed entity from newton/momentum entities" << std::endl;
			motion.velocity = vec2(0.f, 0.f);
			if (registry.players.has(entity))
			{
				Player &player = registry.players.get(entity);
				player.is_dodging = false;
				std::cout << "Player is not dodging anymore" << std::endl;
				motion.velocity = player_movement_direction * 60.f;
			}
		}

		// std::cout << "Interpolation factor: " << interpolation_factor << std::endl;
	}
	for (Entity health_bar_entity : registry.healthbarlink.entities)
	{
		HealthBarLink &healthbarlink = registry.healthbarlink.get(health_bar_entity);
		Entity owner_entity = healthbarlink.owner;

		if (registry.motions.has(owner_entity) && registry.motions.has(health_bar_entity))
		{
			Motion &owner_motion = registry.motions.get(owner_entity);
			Motion &health_bar_motion = registry.motions.get(health_bar_entity);

			if (owner_entity == player_spy)
			{
				health_bar_motion.position = {50.f, 50.f};
			}
			else
			{
				health_bar_motion.position = owner_motion.position + vec2(-105.f, -75.f);
			}
			if (registry.healthbar.has(owner_entity))
			{
				HealthBar &owner_health = registry.healthbar.get(owner_entity);
				float health_percentage = owner_health.current_health / owner_health.max_health;

				// Get the original scale of health bar
				HealthBar &health_bar = registry.healthbar.get(health_bar_entity);
				health_bar_motion.scale.x = health_bar.original_scale.x * health_percentage;
				health_bar_motion.scale.y = health_bar.original_scale.y;
			}
		}
	}

	for (Entity entity : registry.beziers.entities)
	{
		Bezier &bezier = registry.beziers.get(entity);
		Motion &motion = registry.motions.get(entity);

		bezier.elapsed_time += elapsed_ms_since_last_update;

		float t = (float)bezier.elapsed_time / (float)bezier.total_time_to_0_ms;
		t = std::min(t, 1.0f);

		glm::vec2 new_position = bezierInterpolation(t, motion.position, bezier.control_point, bezier.target_position);

		motion.position = new_position;

		if (t >= 1.0f)
		{
			registry.beziers.remove(entity);
			motion.velocity = vec2(0.f, 0.f);
		}
	}

	auto &health = registry.healths.get(registry.players.entities[0]);
	if (health.isDead)
	{
		if (!registry.deathTimers.has(registry.players.entities[0]))
		{
			Mix_PlayChannel(-1, spy_death_sound, 0);
			registry.deathTimers.emplace(registry.players.entities[0]);
		}
	}

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, eels, ... but that would be more cumbersome
	while (registry.motions.entities.size() > 0)
		registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	int screen_width, screen_height;
	glfwGetWindowSize(window, &screen_width, &screen_height);

	float tile_scale = 60.f;

	// Calculate how many tiles are needed to cover the screen
	int floor_number_width = screen_width / tile_scale + 1;
	int floor_number_height = screen_height / tile_scale + 1;
	// Iterate through tile numbers to generate and set position for each tile.
	for (int i = 0; i < floor_number_width; ++i)
	{
		for (int j = 0; j < floor_number_height; ++j)
		{
			// Create the floor tile
			vec2 pos = {i * tile_scale, j * tile_scale};
			createFloorTile(renderer, pos, tile_scale);

			// Now place walls on certain positions
			// Example: Place walls around the border (edges)
			if (i == 0 || i == floor_number_width - 1 || j == 0 || j == floor_number_height - 1)
			{
				createWall(renderer, pos, tile_scale);
			}
			// Example: Place walls to form a corridor
			else if (((i > 5 && i < 30) || (i > 31)) && (j == 8 || j == 13))
			{
				createWall(renderer, pos, tile_scale);
			}
			else if ((i == 5) && (j <= 8 || j >= 13))
			{
				createWall(renderer, pos, tile_scale);
			}
		}
	}

	// 	// create enemy with random initial position
	// while (registry.enemies.components.size() < ENEMIES_COUNT)
	// {
	// 	createEnemy(renderer, vec2(uniform_dist(rng) * (window_width_px - 100) + 50, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
	// }

	for (int i = 0; i < ENEMIES_COUNT; i++)
	{
		createEnemy(renderer, vec2(window_width_px / 2 + 200 + 100 * i, window_height_px / 2 - 30));
		createEnemy(renderer, vec2(window_width_px / 2 + 100 * i, window_height_px / 2 - 150));
		createEnemy(renderer, vec2(window_width_px - 100 * (i + 1), window_height_px - 150));
	}

	player_spy = createSpy(renderer, {window_width_px / 2, window_height_px - 200});

	// Create the weapon entity
	Entity weapon = createWeapon(renderer, {window_width_px / 2, window_height_px - 200});

	vec2 weapon_offset = vec2(45.f, -28.f); // Adjust this based on your design

	Weapon &player_weapon = registry.weapons.emplace(player_spy);
	player_weapon.weapon = weapon;
	player_weapon.offset = weapon_offset;

	createHealthBar(renderer, {50.f, 50.f}, player_spy);
	// registry.healthbarlink.emplace(health_bar, player_spy);

	flowMeterEntity = createFlowMeter(renderer, {100.f, 100.f}, 100.0f);
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	// std::cout << "handle_collisions()" << std::endl;
	auto &collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++)
	{
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;

		// std::cout << entity << " collided with " << entity_other << std::endl;
		// registry.list_all_components_of(entity);

		// // for now, we are only interested in collisions that involve the salmon
		// if (registry.players.has(entity))
		// {
		// 	// Player& player = registry.players.get(entity);

		// 	// Checking Player - Deadly collisions
		// 	if (registry.deadlys.has(entity_other))
		// 	{
		// 		// initiate death unless already dying
		// 		if (!registry.deathTimers.has(entity))
		// 		{
		// 			// Scream, reset timer, and make the salmon sink
		// 			registry.deathTimers.emplace(entity);
		// 			Mix_PlayChannel(-1, salmon_dead_sound, 0);
		// 		}
		// 	}
		// }
		// 	// Checking Player - Eatable collisions
		// 	else if (registry.eatables.has(entity_other))
		// 	{
		// 		if (!registry.deathTimers.has(entity))
		// 		{
		// 			// chew, count points, and set the LightUp timer
		// 			registry.remove_all_components_of(entity_other);
		// 			Mix_PlayChannel(-1, salmon_eat_sound, 0);
		// 			++points;

		// 			// !!! TODO A1: create a new struct called LightUp in components.hpp and add an instance to the salmon entity by modifying the ECS registry
		// 		}
		// 	}
		// }
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON MOVEMENT HERE
	// key is of 'type' GLFW_KEY_
	// action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		restart_game();
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_F)
	{
		if (registry.flows.has(flowMeterEntity))
		{
			Flow &flow = registry.flows.get(flowMeterEntity);
			flow.flowLevel = std::min(flow.flowLevel + 10.f, flow.maxFlowLevel); // Increase flow up to max
			std::cout << "Flow Level: " << flow.flowLevel << " / " << flow.maxFlowLevel << std::endl;
		}
	}

	// Debugging
	if (key == GLFW_KEY_D)
	{
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA)
	{
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD)
	{
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	Motion &motion = registry.motions.get(registry.players.entities[0]);
	current_speed = fmax(0.f, current_speed);
	// close when esc key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	float speed = 60.f;

	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		if (registry.healthbar.has(player_spy))
		{
			HealthBar &health_bar = registry.healthbar.get(player_spy);
			auto& spy_health = registry.healths.get(player_spy);
			//hardcoded damage, TODO
			health_bar.current_health -= 10.f;
			spy_health.health -= 10.f;
			if (health_bar.current_health < 0.f)
			{
				health_bar.current_health = 0.f;
			}
		}
	}

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		for (Entity enemy_entity : registry.enemies.entities)
		{
			if (registry.healthbar.has(enemy_entity))
			{
				HealthBar &health = registry.healthbar.get(enemy_entity);
				health.current_health -= 10.f;
				if (health.current_health < 0.f)
				{
					health.current_health = 0.f;
				}
			}
		}
	}

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
		{
			// motion.velocity.x += speed;
			player_movement_direction.x += 1.f;
		}
		else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
		{
			// motion.velocity.x -= speed;
			player_movement_direction.x -= 1.f;
		}
		else if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
		{
			// motion.velocity.y -= speed;
			player_movement_direction.y -= 1.f;
		}
		else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
		{
			// motion.velocity.y += speed;
			player_movement_direction.y += 1.f;
		}
		// if (registry.interpolations.has(player_spy))
		// {
		// 	registry.interpolations.remove(player_spy);
		// 	std::cout << "Momentum removed due to active movement" << std::endl;
		// }
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
		{
			// motion.velocity.x -= speed;
			player_movement_direction.x -= 1.f;
		}
		else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
		{
			// motion.velocity.x += speed;
			player_movement_direction.x += 1.f;
		}
		else if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
		{
			// motion.velocity.y += speed;
			player_movement_direction.y += 1.f;
		}
		else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
		{
			// motion.velocity.y -= speed;
			player_movement_direction.y -= 1.f;
		}
	}

	Player &player = registry.players.get(player_spy);
	if (!player.is_dodging)
	{
		motion.velocity = player_movement_direction * speed;
		// std::cout << "Player moving in " << player_movement_direction.x << "," << player_movement_direction.y << std::endl;
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && motion.velocity != vec2(0.f, 0.f))
		{
			player.is_dodging = true;
			std::cout << "Dodging" << std::endl;

			Interpolation interpolate;
			interpolate.elapsed_time = 0.f;
			interpolate.initial_velocity = motion.velocity * 5.f;
			if (!registry.interpolations.has(player_spy))
			{
				registry.interpolations.emplace(player_spy, interpolate);
			}
			Mix_PlayChannel(-1, spy_dash_sound, 0);
		}
	}

	// bezier logic
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		if (!registry.beziers.has(player_spy))
		{
			Bezier bezier;
			bezier.elapsed_time = 0.f;

			Motion &motion = registry.motions.get(player_spy);
			bezier.initial_velocity = motion.velocity;
			bezier.target_position = curr_mouse_position;

			bezier.control_point = calculateControlPoint(motion.position, bezier.target_position,
																									 (motion.position + bezier.target_position) / 2.0f, 0.5f);

			registry.beziers.emplace(player_spy, bezier);
			Mix_PlayChannel(-1, break_sound, 0);
		}
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_position)
{

	curr_mouse_position = mouse_position;
	Motion &spy_motion = registry.motions.get(player_spy);

	vec2 spy_vector = +spy_motion.position - mouse_position;

	// float spy_angle = atan2(spy_vector.y, spy_vector.x);

	// spy_motion.angle = spy_angle;

	if (spy_vector.x > 0)
	{
		spy_motion.scale.x = abs(spy_motion.scale.x);
	}
	else
	{
		spy_motion.scale.x = -abs(spy_motion.scale.x);
	}

	(vec2) mouse_position; // dummy to avoid compiler warning
}