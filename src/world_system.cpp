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
float FLOW_CHARGE_PER_SECOND = 33.f;
vec2 player_movement_direction = {0.f, 0.f};
vec2 curr_mouse_position;
bool show_fps = false;
bool show_help_text = true;
bool is_right_mouse_button_down = false;

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
	if (break_sound != nullptr)
		Mix_FreeChunk(break_sound);
	if (chef_trigger_sound != nullptr)
		Mix_FreeChunk(chef_trigger_sound);
	if (minion_attack_sound != nullptr)
		Mix_FreeChunk(minion_attack_sound);
	if (heavy_attack_sound != nullptr)
		Mix_FreeChunk(heavy_attack_sound);

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
	window = glfwCreateWindow(window_width_px, window_height_px, "Games of Throne", nullptr, nullptr);
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
	auto mouse_button_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_redirect);

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
	chef_trigger_sound = Mix_LoadWAV(audio_path("chef_trigger.wav").c_str());
	minion_attack_sound = Mix_LoadWAV(audio_path("minion_attack.wav").c_str());
	heavy_attack_sound = Mix_LoadWAV(audio_path("heavy_attack.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr || spy_death_sound == nullptr || spy_dash_sound == nullptr || spy_attack_sound == nullptr || break_sound == nullptr)
	{
		fprintf(stderr, "Failed to load sounds: %s\n %s\n %s\n %s\n %s\n %s\n %s\n make sure the data directory is present",
						audio_path("soundtrack_1.wav").c_str(),
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
	// lower volume
	Mix_VolumeMusic(30);
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
	restart_game();
}

float elapsed_time = 0.f;
float frame_count = 0.f;
float fps = 0.f;

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{

	elapsed_time += elapsed_ms_since_last_update / 1000.f; // ms to second convert
	frame_count++;

	if (elapsed_time >= 1)
	{
		// Update FPS every second
		fps = frame_count / elapsed_time;
		frame_count = 0.f;
		elapsed_time = 0.f;
	}

	// Update window title
	std::stringstream title_ss;
	title_ss << "Frames Per Second: " << fps;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
		registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto &motions_registry = registry.motions;

	// TODO: Remove entities that leave the screen, using new check accounting for camera view
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	// for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i)
	// {
	// 	Motion &motion = motions_registry.components[i];
	// 	if (motion.position.x + abs(motion.scale.x) < 0.f)
	// 	{
	// 		if (!registry.players.has(motions_registry.entities[i])) // don't remove the player
	// 			registry.remove_all_components_of(motions_registry.entities[i]);
	// 	}
	// }

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
				if (player.state == PlayerState::DODGING)
				{
					player.state = PlayerState::IDLE;
				}
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
				if (!registry.cameraUI.has(health_bar_entity)) {
					registry.cameraUI.emplace(health_bar_entity);

				}

				health_bar_motion.position = vec2(50.f, 50.f);
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
		else
		{
			registry.motions.get(registry.players.entities[0]).velocity = {0.f, 0.f};
		}
	}

	handle_animations(elapsed_ms_since_last_update);

	if (is_right_mouse_button_down && registry.flows.has(flowMeterEntity))
	{
		Player &player = registry.players.get(player_spy);
		if (player.state == PlayerState::IDLE)
		{
			std::cout << "Player is charging flow" << std::endl;
			player.state = PlayerState::CHARGING_FLOW;
		}
		if (player.state == PlayerState::CHARGING_FLOW)
		{
			Flow &flow = registry.flows.get(flowMeterEntity);
			flow.flowLevel = std::min(flow.flowLevel + FLOW_CHARGE_PER_SECOND * elapsed_ms_since_last_update / 1000.f, flow.maxFlowLevel); // Increase flow up to max
		}
	}

	// update animations
	for (Entity entity : registry.spriteAnimations.entities)
	{
		// if in attack mode, change sprite to attack sprite
		if (registry.enemies.has(entity) && registry.enemies.get(entity).state == EnemyState::ATTACK)
		{
			auto &animation = registry.spriteAnimations.get(entity);
			auto &render_request = registry.renderRequests.get(entity);

			// Increment elapsed time
			animation.elapsed_time += elapsed_ms_since_last_update;

			// Check if enough time has passed to switch to the next frame
			if (animation.elapsed_time >= animation.frame_duration)
			{
				// Reset elapsed time for the next frame
				animation.elapsed_time = 0.0f;

				// Move to the next frame
				animation.current_frame = (animation.current_frame + 1) % animation.frames.size();

				// Update the texture to the current frame
				render_request.used_texture = animation.frames[animation.current_frame];
			}
		}
	}

	// update animations
	for (Entity entity : registry.spriteAnimations.entities)
	{
		// if in attack mode, change sprite to attack sprite
		if (registry.enemies.has(entity) && registry.enemies.get(entity).state == EnemyState::ATTACK)
		{
			auto &animation = registry.spriteAnimations.get(entity);
			auto &render_request = registry.renderRequests.get(entity);

			// Increment elapsed time
			animation.elapsed_time += elapsed_ms_since_last_update;

			// Check if enough time has passed to switch to the next frame
			if (animation.elapsed_time >= animation.frame_duration)
			{
				// Reset elapsed time for the next frame
				animation.elapsed_time = 0.0f;

				// Move to the next frame
				animation.current_frame = (animation.current_frame + 1) % animation.frames.size();

				// Update the texture to the current frame
				render_request.used_texture = animation.frames[animation.current_frame];
			}
		}
	}

	// play attack sound for each enemy that is attacking
	for (Entity entity : registry.enemies.entities)
	{
		auto &enemy = registry.enemies.get(entity);
		if (enemy.state == EnemyState::ATTACK)
		{
			// lower this audio
			Mix_Volume(-1, 5);
			Mix_PlayChannel(-1, minion_attack_sound, 0);
		}
	}

	// play chef_trigger audio if chef just entered combat mode
	for (Entity entity : registry.chef.entities)
	{
		auto &enemy = registry.enemies.get(entity);
		if (registry.chef.get(entity).trigger == true)
		{
			// volume up
			Mix_Volume(-1, 80);
			Mix_PlayChannel(-1, chef_trigger_sound, 0);
			if (registry.chef.get(entity).sound_trigger_timer >= elapsed_ms_since_last_update)
			{
				registry.chef.get(entity).sound_trigger_timer -= elapsed_ms_since_last_update;
			}
			else
			{
				registry.chef.get(entity).sound_trigger_timer = 0;
			}
		}
	}

	// move camera if necessary
	update_camera_view();

	return true;
}

void WorldSystem::update_camera_view()
{
	float THRESHOLD = 0.3f; // will move camera if player is within 30% of the screen edge
	Motion &player_motion = registry.motions.get(player_spy);
	vec2 &camera_position = renderer->camera_position;
	vec2 on_screen_pos = player_motion.position - camera_position;
	if (on_screen_pos.x > window_width_px * (1 - THRESHOLD))
	{
		camera_position.x += on_screen_pos.x - window_width_px * (1 - THRESHOLD);
	}
	else if (on_screen_pos.x < window_width_px * THRESHOLD)
	{
		camera_position.x -= window_width_px * THRESHOLD - on_screen_pos.x;
	}
	if (on_screen_pos.y > window_height_px * (1 - THRESHOLD))
	{
		camera_position.y += on_screen_pos.y - window_height_px * (1 - THRESHOLD);
	}
	else if (on_screen_pos.y < window_height_px * THRESHOLD)
	{
		camera_position.y -= window_height_px * THRESHOLD - on_screen_pos.y;
	}
}

void createRoom(std::vector<std::vector<int>> &levelMap, int x_start, int y_start, int width, int height)
{
	for (int i = x_start; i < x_start + width; ++i)
	{
		for (int j = y_start; j < y_start + height; ++j)
		{
			// Place walls around the edges of the room
			if (i == x_start || i == x_start + width - 1 || j == y_start || j == y_start + height - 1)
			{
				levelMap[i][j] = 1; // 1 for wall
			}
			else
			{
				levelMap[i][j] = 2; // 2 for floor
			}
		}
	}
}

void createCorridor(std::vector<std::vector<int>> &levelMap, int x_start, int y_start, int length, int width,
										bool add_wall_left = true, bool add_wall_right = true, bool add_wall_top = true, bool add_wall_bottom = true)
{
	for (int i = x_start; i < x_start + length; ++i)
	{
		for (int j = y_start; j < y_start + width; ++j)
		{
			bool is_edge = (i == x_start || i == x_start + length - 1 || j == y_start || j == y_start + width - 1);

			if (is_edge)
			{
				if ((i == x_start && add_wall_left) || (i == x_start + length - 1 && add_wall_right) ||
						(j == y_start && add_wall_top) || (j == y_start + width - 1 && add_wall_bottom))
				{
					levelMap[i][j] = 1; // Wall
				}
				else
				{
					levelMap[i][j] = 2; // Floor at the edges without walls
				}
			}
			else
			{
				levelMap[i][j] = 2; // Floor in the corridor's interior
			}
		}
	}
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

	// Expanded grid dimensions to cover a larger map
	int floor_number_width = screen_width * 4 / tile_scale + 1;
	int floor_number_height = screen_height * 4 / tile_scale + 1;

	std::vector<std::vector<int>> levelMap(floor_number_width, std::vector<int>(floor_number_height, 0));

	int center_x = floor_number_width / 2;
	int center_y = floor_number_height / 2;
	// Central room (15x15 tiles)
	createRoom(levelMap, center_x - 7, center_y - 7, 15, 15);

	// Top room (10x10 tiles) and corridor connecting to central room
	createRoom(levelMap, center_x - 5, center_y - 20, 10, 10);
	createCorridor(levelMap, center_x - 2, center_y - 11, 5, 5, true, true, false, false); // No wall on bottom side

	// Bottom room (10x10 tiles) and corridor connecting to central room
	createRoom(levelMap, center_x - 5, center_y + 10, 10, 10);
	createCorridor(levelMap, center_x - 1, center_y + 7, 5, 4, true, true, false, false); // No wall on top side

	// Left room (10x10 tiles) and corridor connecting to central room
	createRoom(levelMap, center_x - 25, center_y - 5, 10, 10);
	createCorridor(levelMap, center_x - 16, center_y - 1, 10, 4, false, false, true, true); // No wall on right side

	// Right room (10x10 tiles) and corridor connecting to central room
	createRoom(levelMap, center_x + 15, center_y - 5, 10, 10);
	createCorridor(levelMap, center_x + 7, center_y - 1, 9, 4, false, false, true, true); // No wall on left side

	// Additional rooms with corridors
	createRoom(levelMap, center_x + 15, center_y - 20, 10, 10);
	createCorridor(levelMap, center_x + 4, center_y - 15, 12, 4, false, false, true, true);

	createRoom(levelMap, center_x + 15, center_y + 10, 7, 8);
	createCorridor(levelMap, center_x + 16, center_y + 4, 4, 7, true, true, false, false);

	for (int i = 0; i < levelMap.size(); ++i)
	{
		for (int j = 0; j < levelMap[i].size(); ++j)
		{
			vec2 pos = {i * tile_scale, j * tile_scale};

			if (levelMap[i][j] == 1)
			{
				createWall(renderer, pos, tile_scale); // Render wall
			}
			else if (levelMap[i][j] == 2)
			{
				createFloorTile(renderer, pos, tile_scale); // Render floor
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

	Entity weapon = createWeapon(renderer, {window_width_px / 2, window_height_px - 200});
	player_spy = createSpy(renderer, {window_width_px / 2, window_height_px - 200});

	vec2 weapon_offset = vec2(45.f, -50.f);

	Weapon &player_weapon = registry.weapons.emplace(player_spy);
	player_weapon.weapon = weapon;
	player_weapon.offset = weapon_offset;

	createHealthBar(renderer, {50.f, 50.f}, player_spy);
	// registry.healthbarlink.emplace(health_bar, player_spy);

	flowMeterEntity = createFlowMeter(renderer, {window_width_px - 100.f, window_height_px - 100.f}, 100.0f);
	Entity chef = createChef(renderer, {window_width_px - 300.f, window_height_px - 200.f});
}

void process_animation(AnimationName name, float t, Entity entity)
{
	if (name == AnimationName::LIGHT_ATTACK)
	{
		Weapon &player_weapon = registry.weapons.get(entity);
		Motion &weapon_motion = registry.motions.get(player_weapon.weapon);
		weapon_motion.angle = 0.5f * sin(t * M_PI) + M_PI / 6;
		// std::cout << "Angle = " << weapon_motion.angle / M_PI * 180 << std::endl;

		if (t >= 1.0f)
		{
			weapon_motion.angle = M_PI / 6;
			Player &player = registry.players.get(entity);
			player.state = PlayerState::IDLE;
		}
	}
	else if (name == AnimationName::HEAVY_ATTACK)
	{
		Weapon &player_weapon = registry.weapons.get(entity);
		Motion &weapon_motion = registry.motions.get(player_weapon.weapon);
		float angle = t * 4 * M_PI + M_PI / 6;
		while (angle > 2 * M_PI)
		{
			angle -= 2 * M_PI;
		}
		weapon_motion.angle = angle;
		// std::cout << "Angle = " << weapon_motion.angle / M_PI * 180 << std::endl;

		if (t >= 1.0f)
		{
			weapon_motion.angle = M_PI / 6;
			Player &player = registry.players.get(entity);
			player.state = PlayerState::IDLE;
		}
	}
}

void WorldSystem::handle_animations(float elapsed_ms)
{
	for (int i = registry.animations.size() - 1; i >= 0; i--)
	{
		Animation &animation = registry.animations.components[i];
		Entity entity = registry.animations.entities[i];
		animation.elapsed_time += elapsed_ms;
		float t = animation.elapsed_time / animation.total_time;
		if (t >= 1.0f)
		{
			t = 1.0f;
		}
		// std::cout << "i = " << i << " t = " << t << std::endl;
		process_animation(animation.name, t, entity);

		if (animation.elapsed_time >= animation.total_time)
		{
			registry.animations.remove(entity);
		}
	}
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// draw collision bounding boxes (debug lines)
	if (debugging.in_debug_mode)
	{
		for (uint i = 0; i < registry.physicsBodies.components.size(); i++)
		{
			Entity entity = registry.physicsBodies.entities[i];
			Motion &motion = registry.motions.get(entity);
			vec3 color = {1.f, 0.f, 0.f};
			if (registry.collisions.has(entity))
			{
				color = {0.f, 1.f, 0.f};
			}
			createBox(motion.position + motion.bb_offset, get_bounding_box(motion), color);
			if (motion.pivot_offset.x != 0 || motion.pivot_offset.y != 0)
			{
				createBox(motion.position - motion.pivot_offset * motion.scale, {5.f, 5.f}, {0.f, 0.f, 1.f});
			}
		}

		// Draw weapon mesh (for debugging)
		Weapon &player_weapon = registry.weapons.get(player_spy);
		Motion &weapon_motion = registry.motions.get(player_weapon.weapon);
		// createBox(weapon_motion.position + weapon_motion.bb_offset, get_bounding_box(weapon_motion), {1.f, 1.f, 0.f});
		TexturedMesh &weapon_mesh = renderer->getTexturedMesh(GEOMETRY_BUFFER_ID::WEAPON);
		glm::mat3 transform = get_transform(weapon_motion);
		for (uint i = 0; i < weapon_mesh.vertex_indices.size(); i += 3)
		{
			vec2 p1 = xy(transform * weapon_mesh.vertices[weapon_mesh.vertex_indices[i]].position);
			vec2 p2 = xy(transform * weapon_mesh.vertices[weapon_mesh.vertex_indices[i + 1]].position);
			vec2 p3 = xy(transform * weapon_mesh.vertices[weapon_mesh.vertex_indices[i + 2]].position);
			// std::cout << "p1 = " << p1.x << "," << p1.y << std::endl;
			// std::cout << "p2 = " << p2.x << "," << p2.y << std::endl;
			// std::cout << "p3 = " << p3.x << "," << p3.y << std::endl;
			// std::cout << weapon_motion.scale.x << "," << weapon_motion.scale.y << std::endl;

			// draw line from p1 to p2, where createLine is function createLine(vec2 position, vec2 scale, vec3 color, float angle)

			createLine((p1 + p2) / 2.f, {glm::length(p2 - p1), 3.f}, {1.f, 1.f, 0.f}, atan2(p2.y - p1.y, p2.x - p1.x));
			createLine((p2 + p3) / 2.f, {glm::length(p3 - p2), 3.f}, {1.f, 1.f, 0.f}, atan2(p3.y - p2.y, p3.x - p2.x));
			createLine((p3 + p1) / 2.f, {glm::length(p1 - p3), 3.f}, {1.f, 1.f, 0.f}, atan2(p1.y - p3.y, p1.x - p3.x));
		}
		// for (uint i = 0; i < weapon_mesh.vertices.size(); i++)
		// {
		// 	vec2 p = xy(transform * weapon_mesh.vertices[i].position);
		// 	createBox(weapon_motion.position + p, {5.f, 5.f}, {1.f, 1.f, 0.f});
		// }
	}

	// Loop over all collisions detected by the physics system
	// std::cout << "handle_collisions()" << std::endl;
	auto &collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++)
	{
		// The entity and its collider
		// Entity entity = collisionsRegistry.entities[i];
		// Entity entity_other = collisionsRegistry.components[i].other;

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
	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		restart_game();
	}

	// For debugging flow meter:
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
	if (key == GLFW_KEY_D && (mod & GLFW_MOD_SHIFT) && action == GLFW_PRESS)
	{
		debugging.in_debug_mode = !debugging.in_debug_mode;
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

	// FPS toggle
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		show_fps = !show_fps;
		std::cout << "Show FPS: " << (show_fps ? "ON" : "OFF") << std::endl;
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		show_help_text = !show_help_text;
		std::cout << "Show Help Text: " << (show_help_text ? "ON" : "OFF") << std::endl;
	}

	float speed = 60.f;

	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		if (registry.healthbar.has(player_spy))
		{
			HealthBar &health_bar = registry.healthbar.get(player_spy);
			auto &spy_health = registry.healths.get(player_spy);
			// hardcoded damage, TODO
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
	if (player.state == PlayerState::LIGHT_ATTACK || player.state == PlayerState::HEAVY_ATTACK || player.state == PlayerState::CHARGING_FLOW)
	{
		motion.velocity = vec2(0.f, 0.f);
	}
	else if (player.state != PlayerState::DODGING)
	{
		motion.velocity = player_movement_direction * speed;
		// std::cout << "Player moving in " << player_movement_direction.x << "," << player_movement_direction.y << std::endl;
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && motion.velocity != vec2(0.f, 0.f))
		{
			player.state = PlayerState::DODGING;
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
			bezier.target_position = curr_mouse_position + renderer->camera_position;

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

	vec2 spy_vector = spy_motion.position - renderer->camera_position - mouse_position;

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

void WorldSystem::on_mouse_button(int button, int action, int mods)
{
	std::cout << "Mouse button " << button << " " << action << " " << mods << std::endl;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		Player &player = registry.players.get(player_spy);
		if (player.state == PlayerState::IDLE)
		{
			std::cout << "Player light attack" << std::endl;
			player.state = PlayerState::LIGHT_ATTACK;
			if (registry.animations.has(player_spy))
			{
				registry.animations.remove(player_spy);
			}
			Animation animation;
			animation.name = AnimationName::LIGHT_ATTACK;
			animation.elapsed_time = 0.f;
			animation.total_time = 500.f;
			registry.animations.emplace(player_spy, animation);
		}
		// trigger player attack sound
		// volume up
		Mix_Volume(1, 100);
		Mix_PlayChannel(1, spy_attack_sound, 0); // TODO: player sound halted (volume decreased) by other playing sound
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			is_right_mouse_button_down = true;
		}
		else if (action == GLFW_RELEASE)
		{
			is_right_mouse_button_down = false;
			Player &player = registry.players.get(player_spy);
			if (player.state == PlayerState::CHARGING_FLOW)
			{
				player.state = PlayerState::IDLE;
			}
			if (registry.flows.has(flowMeterEntity))
			{
				Flow &flow = registry.flows.get(flowMeterEntity);
				if (flow.flowLevel == flow.maxFlowLevel)
				{
					if (player.state == PlayerState::IDLE || player.state == PlayerState::CHARGING_FLOW)
					{
						std::cout << "Player heavy attack" << std::endl;
						Mix_Volume(2, 120);
						Mix_PlayChannel(2, heavy_attack_sound, 0);
						player.state = PlayerState::HEAVY_ATTACK;
						flow.flowLevel = 0;
						if (registry.animations.has(player_spy))
						{
							registry.animations.remove(player_spy);
						}
						Animation animation;
						animation.name = AnimationName::HEAVY_ATTACK;
						animation.elapsed_time = 0.f;
						animation.total_time = 1000.f;
						registry.animations.emplace(player_spy, animation);
					}
				}
			}
		}
	}
}
