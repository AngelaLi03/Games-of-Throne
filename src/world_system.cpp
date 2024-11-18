// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "iostream"
// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"
#include "LDtkLoader/Project.hpp"

// Game configuration
int ENEMIES_COUNT = 2;
float FLOW_CHARGE_PER_SECOND = 33.f;
vec2 player_movement_direction = {0.f, 0.f};
vec2 curr_mouse_position;
bool show_fps = false;
bool show_help_text = false;
bool is_right_mouse_button_down = false;
bool enlarged_player = false;
float enlarge_countdown = 3000.f;
float speed = 100.f;
bool unlocked_stealth_ability = false;
bool dashAvailable = true;
bool dashInUse = false;

std::vector<vec2> mousePath; // Stores points along the mouse path
bool isTrackingMouse = false;
// Threshold constants for gesture detection
const float S_THRESHOLD1 = 20.0f;
const float MIN_S_LENGTH = 100.0f;

// create the underwater world
WorldSystem::WorldSystem()
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

float bezzy(float t, float P0, float P1, float P2)
{
	return (1 - t) * (1 - t) * P0 + 2 * (1 - t) * t * P1 + t * t * P2;
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{

	elapsed_time += elapsed_ms_since_last_update / 1000.f; // ms to second convert
	frame_count++;

	Player &player = registry.players.get(player_spy);
	Motion &spy_motion = registry.motions.get(player_spy);
	dashAvailable = player.state != PlayerState::DASHING && player.dash_cooldown_remaining_ms <= 0.0f;
	dashInUse = (player.state == PlayerState::DASHING);

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

	// TODO: Remove entities that leave the screen, using new check accounting for camera view
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	// auto &motions_registry = registry.motions;
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

	// Update health bar percentage
	for (unsigned int i = 0; i < registry.healths.size(); i++)
	{
		Health &health = registry.healths.components[i];
		Entity owner_entity = registry.healths.entities[i];
		Entity health_bar_entity = health.healthbar;

		if (registry.motions.has(owner_entity) && registry.motions.has(health_bar_entity))
		{
			Motion &owner_motion = registry.motions.get(owner_entity);
			Motion &health_bar_motion = registry.motions.get(health_bar_entity);

			// TODO: refactor the following lines code by introducing healthbar_offset and put code in renderer.draw()
			if (owner_entity == player_spy)
			{
				if (!registry.cameraUI.has(health_bar_entity))
				{
					registry.cameraUI.emplace(health_bar_entity);
				}

				health_bar_motion.position = vec2(50.f, 50.f);
			}
			else if (registry.chef.has(owner_entity))
			{
				health_bar_motion.position = owner_motion.position + vec2(-95.f, -175.f);
			}
			else
			{
				health_bar_motion.position = owner_motion.position + vec2(-105.f, -75.f);
			}

			float health_percentage = health.health / health.max_health;
			if (registry.healthbar.has(health_bar_entity))
			{
				HealthBar &health_bar = registry.healthbar.get(health_bar_entity);
				health_bar_motion.scale.x = health_bar.original_scale.x * health_percentage;
				health_bar_motion.scale.y = health_bar.original_scale.y;
			}
		}
	}

	for (Entity entity : registry.dashes.entities)
	{
		Motion &spy_motion = registry.motions.get(player_spy);
		dashAvailable = player.state != PlayerState::DASHING && player.dash_cooldown_remaining_ms <= 0.0f;
		dashInUse = (player.state == PlayerState::DASHING);
		Dash &dash = registry.dashes.get(entity);
		dash.elapsed_time += elapsed_ms_since_last_update;

		if (dash.elapsed_time <= dash.total_time_ms)
		{
			float t = dash.elapsed_time / dash.total_time_ms;
			float scaling_factor = bezzy(t, 1.0f, 2.0f, 1.0f);
			spy_motion.velocity = player_movement_direction * speed * scaling_factor * 5.f;
			// std::cout << "dash active, velocity is (" << spy_motion.velocity.x << ", " << spy_motion.velocity.y << ")" << std::endl;
		}
		else
		{
			player.state = PlayerState::IDLE;
			dash.elapsed_time = 0;
			player.dash_cooldown_remaining_ms = dash.total_time_ms;
			registry.dashes.remove(entity);
			std::cout << "Dash ended, player back to IDLE" << std::endl;
		}
	}

	if (player.dash_cooldown_remaining_ms > 0.0f)
	{
		player.dash_cooldown_remaining_ms -= elapsed_ms_since_last_update;
	}

	auto &health = registry.healths.get(player_spy);
	if (health.is_dead)
	{
		if (!registry.deathTimers.has(player_spy))
		{
			Mix_PlayChannel(-1, spy_death_sound, 0);
			registry.deathTimers.emplace(player_spy);

			// remove collision from player
			registry.physicsBodies.remove(player_spy);
		}
		if (player.state != PlayerState::DYING)
		{
			player.state = PlayerState::DYING;
		}
		registry.motions.get(player_spy).velocity = {0.f, 0.f};
	}

	// Pan returns to chef logic
	for (Entity pan_entity : registry.pans.entities)
	{
		Pan &pan = registry.pans.get(pan_entity);
		Motion &pan_motion = registry.motions.get(pan_entity);

		if (pan.state == PanState::RETURNING)
		{
			for (Entity chef_entity : registry.chef.entities)
			{
				Motion &chef_motion = registry.motions.get(chef_entity);
				float distance_to_chef = length(chef_motion.position - pan_motion.position);
				// Pan returns to chef when distance < 50.
				if (distance_to_chef < 50.f)
				{
					registry.remove_all_components_of(pan_entity);
					Chef &chef = registry.chef.get(chef_entity);
					chef.pan_active = false;
				}
			}
		}
	}

	handle_animations(elapsed_ms_since_last_update);

	if (is_right_mouse_button_down && registry.flows.has(flowMeterEntity))
	{
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

	// TODO: create audio_system class and delegate audio playing to it, so this logic does not have to be in world_system
	// play chef_trigger audio if chef just entered combat mode
	for (Entity entity : registry.chef.entities)
	{
		Chef &chef = registry.chef.get(entity);
		if (chef.trigger == true)
		{
			// volume up
			Mix_Volume(-1, 50);
			Mix_PlayChannel(-1, chef_trigger_sound, 0);
			if (chef.sound_trigger_timer >= elapsed_ms_since_last_update)
			{
				chef.sound_trigger_timer -= elapsed_ms_since_last_update;
			}
			else
			{
				chef.sound_trigger_timer = 0;
				chef.trigger = false;
			}
		}
	}

	// move camera if necessary
	update_camera_view();

	// mouse gesture enlarge countdown
	if (enlarged_player)
	{
		enlarge_countdown -= elapsed_ms_since_last_update;
		if (enlarge_countdown <= 0)
		{
			// reset player size
			Motion &player_motion = registry.motions.get(player_spy);
			player_motion.scale *= 0.65f;
			player_motion.bb_scale *= 0.65f;

			enlarged_player = false;
			enlarge_countdown = 5000.f;
		}
	}

	// handle chef death
	if (registry.chef.size() > 0)
	{
		Entity chef_entity = registry.chef.entities[0];
		Health &chef_health = registry.healths.get(chef_entity);
		if (chef_health.is_dead)
		{
			registry.remove_all_components_of(chef_entity);

			// gain stealth ability
			unlocked_stealth_ability = true;

			// TODO: this darkens everything; create a screen mask with opacity so only non-popup elements are darkened
			// screen.darken_screen_factor = 0.5;

			Entity ability_sprite = createSprite(renderer, {window_width_px / 2.f, window_height_px / 2.f - 150.f}, {250.f, 250.f}, TEXTURE_ASSET_ID::STEALTH);
			registry.cameraUI.emplace(ability_sprite);

			// TODO: render text (make it generic so other popups can also render their custom text)
			// popup_text = "You have gained the stealth ability!";

			active_popup = {[this]()
											{
												load_level("Level_1", 1);
											}};
			has_popup = true;
			is_paused = true;
		}
	}

	// if (registry.knight.size() > 0)
	// {
	// 	Entity knight_entity = registry.knight.entities[0];
	// 	Health &knight_health = registry.healths.get(knight_entity);
	// 	if (knight_health.is_dead)
	// 	{
	// 		registry.remove_all_components_of(knight_entity);
	// 		load_level("Level_2", 2);
	// 	}
	// }

	return true;
}

void WorldSystem::update_camera_view()
{
	float THRESHOLD = 0.4f; // will move camera if player is within 40% of the screen edge
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

	int screen_width, screen_height;
	glfwGetWindowSize(window, &screen_width, &screen_height);

	load_level("Level_0", 0);
}

void WorldSystem::load_level(const std::string &levelName, const int levelNumber)
{
	while (registry.motions.entities.size() > 0)
		registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	ldtk::Project ldtk_project;
	ldtk_project.loadFromFile(data_path() + "/levels/levels.ldtk");
	const ldtk::World &world = ldtk_project.getWorld();
	const ldtk::Level &level = world.getLevel(levelName);
	const std::vector<ldtk::Layer> &layers = level.allLayers();
	for (const auto &layer : level.allLayers())
	{
		if (layer.getType() == ldtk::LayerType::Tiles)
		{
			for (const auto &tile : layer.allTiles())
			{
				// Get tile position in pixels
				int px = tile.getPosition().x;
				int py = tile.getPosition().y;
				vec2 position = {static_cast<float>(px), static_cast<float>(py)};
				if (layer.getName() == "Floor_Tiles")
				{
					createFloorTile(renderer, position);
				}
				else if (layer.getName() == "Wall_Tiles")
				{
					createWall(renderer, position);
				}
			}
		}
	}
	for (const auto &layer : level.allLayers())
	{
		if (layer.getType() == ldtk::LayerType::Entities)
		{
			for (const auto &entity : layer.allEntities())
			{
				std::string entity_name = entity.getName();
				int px = entity.getPosition().x;
				int py = entity.getPosition().y;
				vec2 position = {static_cast<float>(px), static_cast<float>(py)};

				if (entity_name == "Chest")
				{
					int itemId = (int)(uniform_dist(rng) * (float)TreasureBoxItem::NONE);
					// TODO: use levelNumber to determine weapon spawn
					createTreasureBox(renderer, position, (TreasureBoxItem)itemId);
				}
				else if (entity_name == "Fountain")
				{
					// createFountain(renderer, position);
				}
				else if (entity_name == "Minions")
				{
					createEnemy(renderer, position);
				}
				else if (entity_name == "Chef")
				{
					createChef(renderer, position);
				}
				else if (entity_name == "Knight")
				{
					// createKnight (renderer, position);
				}
				else if (entity_name == "Prince")
				{
					// createPrince (renderer, position);
				}
				else if (entity_name == "Spy")
				{
					player_spy = createSpy(renderer, position);
				}
			}
		}
	}
	Entity weapon = createWeapon(renderer, {window_width_px * 2 + 200, window_height_px * 2});

	vec2 weapon_offset = vec2(45.f, -50.f);

	Weapon &player_weapon = registry.weapons.emplace(player_spy);
	player_weapon.weapon = weapon;
	player_weapon.offset = weapon_offset;

	flowMeterEntity = createFlowMeter(renderer, {window_width_px - 100.f, window_height_px - 100.f}, 100.0f);
}

void WorldSystem::process_animation(AnimationName name, float t, Entity entity)
{
	if (name == AnimationName::LIGHT_ATTACK)
	{
		Weapon &player_weapon = registry.weapons.get(entity);
		Motion &weapon_motion = registry.motions.get(player_weapon.weapon);
		float angle = sin(t * M_PI) + M_PI / 6;

		if (t >= 1.0f)
		{
			angle = M_PI / 6;
			Player &player = registry.players.get(entity);
			player.state = PlayerState::IDLE;
		}

		if (player_weapon.offset.x < 0)
		{
			weapon_motion.angle = -angle;
		}
		else
		{
			weapon_motion.angle = angle;
		}
		// std::cout << "Angle = " << weapon_motion.angle / M_PI * 180 << std::endl;
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

		if (t >= 0.5f)
		{
			Player &player = registry.players.get(entity);
			if (!player.is_heavy_attack_second_half)
			{
				player.is_heavy_attack_second_half = true;
				player.current_attack_id = ++attack_id_counter;
				// std::cout << "Heavy attack second half " << player.current_attack_id << std::endl;
			}
		}

		if (t >= 1.0f)
		{
			angle = M_PI / 6;
			Player &player = registry.players.get(entity);
			player.state = PlayerState::IDLE;
		}

		if (player_weapon.offset.x < 0)
		{
			weapon_motion.angle = -angle;
		}
		else
		{
			weapon_motion.angle = angle;
		}
		// std::cout << "Angle = " << weapon_motion.angle / M_PI * 180 << std::endl;
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

		// Draw damage areas
		for (uint i = 0; i < registry.damageAreas.components.size(); i++)
		{
			Entity entity = registry.damageAreas.entities[i];
			DamageArea &damage_area = registry.damageAreas.components[i];
			Motion &motion = registry.motions.get(entity);
			vec3 color = {0.6f, 0.6f, 0.f};
			if (!damage_area.active)
			{
				color = {0.3f, 0.3f, 0.f};
			}
			createBox(motion.position + motion.bb_offset, get_bounding_box(motion), color);
		}
	}

	// Loop over all collisions detected by the physics system
	// std::cout << "handle_collisions()" << std::endl;
	Entity player = player_spy;
	Weapon &player_weapon_comp = registry.weapons.get(player);
	Entity player_weapon = player_weapon_comp.weapon;
	Player &player_comp = registry.players.get(player);
	auto &collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++)
	{
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;
		// std::cout << "entity = " << entity << " entity_other = " << entity_other << std::endl;

		bool entity_is_damage_area = registry.damageAreas.has(entity);
		if (entity_is_damage_area && entity_other == player)
		{
			DamageArea &damage_area = registry.damageAreas.get(entity);
			if (damage_area.active)
			{
				if (!player_comp.can_take_damage())
				{
					continue;
				}

				std::cout << "Damage area hit player" << std::endl;
				Damage &damage = registry.damages.get(entity);
				Health &player_health = registry.healths.get(player);
				player_health.take_damage(damage.damage);

				if (damage_area.single_damage)
				{
					damage_area.time_until_inactive = 0; // deactivate damage area asap to avoid another damage
				}
				else
				{
					// todo multiple damages with cooldown
				}
			}
		}

		// When tomato hits player
		// bool entity_is_tomato = registry.tomatos.has(entity);
		// bool entity_other_is_tomato = registry.tomatos.has(entity_other);
		// if ((entity_is_tomato && entity_other == player_spy) || (entity_other_is_tomato && entity == player_spy))
		// {
		// 	Entity tomato_entity = entity_is_tomato ? entity : entity_other;
		// 	std::cout << "11" << std::endl;
		// 	Tomato &tomato = registry.tomatos.get(tomato_entity);
		// 	std::cout << "22" << std::endl;
		// 	Health &player_spy_health = registry.healths.get(player_spy);
		// 	std::cout << "33" << std::endl;

		// 	player_spy_health.health -= tomato.damage;
		// 	if (player_spy_health.health <= 0.f)
		// 	{
		// 		player_spy_health.health = 0.f;
		// 		player_spy_health.is_dead = true;
		// 		if (!registry.deathTimers.has(player_spy))
		// 		{
		// 			registry.deathTimers.emplace(player_spy);
		// 			Mix_PlayChannel(-1, spy_death_sound, 0);
		// 		}
		// 	}
		// 	registry.remove_all_components_of(tomato_entity);
		// }

		// When tomato hits wall
		// bool entity_is_wall = registry.physicsBodies.has(entity) && registry.physicsBodies.get(entity).body_type == BodyType::STATIC;
		// bool entity_other_is_wall = registry.physicsBodies.has(entity_other) && registry.physicsBodies.get(entity_other).body_type == BodyType::STATIC;

		// if ((entity_is_wall && entity_other_is_tomato) || (entity_other_is_wall && entity_is_tomato))
		// {
		// 	Entity tomato_entity = entity_is_tomato ? entity : entity_other;
		// 	registry.remove_all_components_of(tomato_entity);
		// }

		// When pan hits player
		bool entity_is_pan = registry.pans.has(entity);
		if (entity_is_pan && entity_other == player)
		{
			Pan &pan = registry.pans.get(entity);

			if (pan.player_hit == false)
			{
				pan.player_hit = true;
				if (player_comp.can_take_damage())
				{
					Health &player_spy_health = registry.healths.get(player_spy);
					player_spy_health.take_damage(pan.damage);
				}
			}
			pan.state = PanState::RETURNING;
			for (Entity chef_entity : registry.chef.entities)
			{
				Motion &chef_motion = registry.motions.get(chef_entity);
				Motion &pan_motion = registry.motions.get(entity);
				vec2 direction_to_chef = normalize(chef_motion.position - pan_motion.position);
				pan_motion.velocity = direction_to_chef * 400.f;
			}
		}

		// When pan hits wall
		bool entity_other_is_wall = registry.physicsBodies.has(entity_other) && registry.physicsBodies.get(entity_other).body_type == BodyType::STATIC;
		if (entity_is_pan && entity_other_is_wall)
		{
			Pan &pan = registry.pans.get(entity);
			pan.state = PanState::RETURNING;
			for (Entity chef_entity : registry.chef.entities)
			{
				Motion &chef_motion = registry.motions.get(chef_entity);
				Motion &pan_motion = registry.motions.get(entity);
				vec2 direction_to_chef = normalize(chef_motion.position - pan_motion.position);
				pan_motion.velocity = direction_to_chef * 400.f;
			}
		}

		// When dash hits player
		bool entity_is_chef = registry.chef.has(entity);
		if (entity_is_chef && entity_other == player)
		{
			Chef &chef = registry.chef.get(entity);
			Health &player_spy_health = registry.healths.get(player_spy);

			if (!chef.dash_has_damaged && player_comp.can_take_damage())
			{
				float damage = 10.f;
				player_spy_health.take_damage(damage);
				chef.dash_has_damaged = true;
			}
		}

		if (entity == player_weapon && registry.enemies.has(entity_other))
		{
			if (registry.healths.has(entity_other))
			{
				Health &enemy_health = registry.healths.get(entity_other);
				if (enemy_health.is_dead)
				{
					// Enemy is dead; skip collision processing
					continue;
				}
			}

			Enemy &enemy = registry.enemies.get(entity_other);

			if (player_comp.state == PlayerState::LIGHT_ATTACK || player_comp.state == PlayerState::HEAVY_ATTACK)
			{
				if (enemy.last_hit_attack_id != player_comp.current_attack_id)
				{
					enemy.last_hit_attack_id = player_comp.current_attack_id;
				}
				else
				{
					// Skip collision processing if the enemy has already been hit by this attack
					continue;
				}

				float damage = (player_comp.state == PlayerState::LIGHT_ATTACK) ? 20.f : 40.f; // Define damage values

				if (registry.healths.has(entity_other))
				{
					Health &enemy_health = registry.healths.get(entity_other);
					enemy_health.take_damage(damage);

					if (player_comp.state == PlayerState::LIGHT_ATTACK)
					{
						Flow &flow = registry.flows.get(flowMeterEntity);
						flow.flowLevel = std::min(flow.flowLevel + 10.f, flow.maxFlowLevel);
					}

					// std::cout << "Enemy health: " << enemy_health.health << ", damage: " << damage << std::endl;
				}
			}
		} // The entity and its collider
		// Entity entity = collisionsRegistry.entities[i];
		// Entity entity_other = collisionsRegistry.components[i].other;

		// registry.list_all_components_of(entity);

		// std::cout << "entity = " << entity << " entity_other = " << entity_other << " DONE" << std::endl;
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
	// close when esc key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	// Pausing game
	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
	{
		if (!has_popup)
		{
			is_paused = !is_paused;
		}
	}

	// track player movement direction (needs to happen when paused as well)
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
		{
			player_movement_direction.x += 1.f;
		}
		else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
		{
			player_movement_direction.x -= 1.f;
		}
		else if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
		{
			player_movement_direction.y -= 1.f;
		}
		else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
		{
			player_movement_direction.y += 1.f;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
		{
			player_movement_direction.x -= 1.f;
		}
		else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
		{
			player_movement_direction.x += 1.f;
		}
		else if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
		{
			player_movement_direction.y += 1.f;
		}
		else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
		{
			player_movement_direction.y -= 1.f;
		}
	}

	if (is_paused)
	{
		// skip processing all other types of keys when game is paused
		return;
	}

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

	Motion &motion = registry.motions.get(player_spy);

	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		Motion &player_motion = registry.motions.get(player_spy);
		// check for treasure box interaction
		for (unsigned int i = 0; i < registry.treasureBoxes.components.size(); i++)
		{
			Entity treasure_box_entity = registry.treasureBoxes.entities[i];
			Motion &treasure_box_motion = registry.motions.get(treasure_box_entity);
			float distance_to_player = length(treasure_box_motion.position - player_motion.position);
			if (distance_to_player < 150.f)
			{
				TreasureBox &treasure_box = registry.treasureBoxes.components[i];
				if (!treasure_box.is_open)
				{
					std::cout << "Treasure box opened" << std::endl;
					treasure_box.is_open = true;
					// play sound
					// Mix_PlayChannel(-1, break_sound, 0);

					// create ui frame background
					createSprite(renderer, {treasure_box_motion.position.x, treasure_box_motion.position.y - 100.f}, {100.f, 100.f}, TEXTURE_ASSET_ID::UI_FRAME);

					// create box item rendering
					if (treasure_box.item != TreasureBoxItem::NONE)
					{
						TEXTURE_ASSET_ID item_texture_id;
						if (treasure_box.item == TreasureBoxItem::MAX_ENERGY)
						{
							item_texture_id = TEXTURE_ASSET_ID::MAX_ENERGY;
						}
						else if (treasure_box.item == TreasureBoxItem::MAX_HEALTH)
						{
							item_texture_id = TEXTURE_ASSET_ID::MAX_HEALTH;
						}
						else if (treasure_box.item == TreasureBoxItem::WEAPON)
						{
							item_texture_id = TEXTURE_ASSET_ID::WEAPON;
						}
						else
						{
							std::cout << "Invalid item type" << std::endl;
							continue;
						}
						treasure_box.item_entity = createSprite(renderer, {treasure_box_motion.position.x, treasure_box_motion.position.y - 100.f}, {50.f, 50.f}, item_texture_id);
					}

					RenderRequest &render_request = registry.renderRequests.get(treasure_box_entity);
					render_request.used_texture = TEXTURE_ASSET_ID::TREASURE_BOX_OPEN;
				}
				else
				{
					std::cout << "Treasure box already opened" << std::endl;
					if (treasure_box.item != TreasureBoxItem::NONE && treasure_box.item_entity != 0)
					{
						if (treasure_box.item == TreasureBoxItem::MAX_HEALTH)
						{
							Health &player_health = registry.healths.get(player_spy);
							player_health.max_health += 30.f;
						}

						// remove item rendering
						registry.remove_all_components_of(treasure_box.item_entity);

						treasure_box.item_entity = Entity(0);
						treasure_box.item = TreasureBoxItem::NONE;
					}
				}
			}
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

	if (action == GLFW_PRESS && key == GLFW_KEY_X)
	{
		Player &player = registry.players.get(player_spy);

		if (!unlocked_stealth_ability)
		{
			std::cout << "Stealth ability not unlocked" << std::endl;
			return;
		}

		if (player.state == PlayerState::IDLE && player.dash_cooldown_remaining_ms <= 0.0f)
		{
			player.state = PlayerState::DASHING;
			std::cout << "DASHING NOW" << std::endl;

			// Add dash component and play dash sound
			Dash dash;
			registry.dashes.emplace(player_spy, dash);
			Mix_PlayChannel(-1, spy_dash_sound, 0);
		}
		else if (player.dash_cooldown_remaining_ms > 0.0f)
		{
			std::cout << "Dash on cooldown: " << player.dash_cooldown_remaining_ms << "ms remaining." << std::endl;
		}
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_position)
{
	if (is_paused)
	{
		// skip processing mouse movement when game is paused
		return;
	}

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

	if (isTrackingMouse)
	{
		// Store the current mouse position in the path
		mousePath.push_back({mouse_position.x, mouse_position.y});
	}
	// (vec2) mouse_position; // dummy to avoid compiler warning
}

bool WorldSystem::isSGesture()
{
	if (mousePath.size() < 5)
		return false; // Not enough points for an "S" shape

	bool leftToRight = false, rightToLeft = false;
	float length = 0.0f;

	for (size_t i = 1; i < mousePath.size(); ++i)
	{
		float dx = mousePath[i].x - mousePath[i - 1].x;
		float dy = mousePath[i].y - mousePath[i - 1].y;
		length += std::sqrt(dx * dx + dy * dy);

		if (dx > S_THRESHOLD1)
		{
			leftToRight = true;
		}
		else if (dx < -S_THRESHOLD1 && leftToRight)
		{
			rightToLeft = true;
		}

		// O should not be detected as S
		// if (dy > S_THRESHOLD || dy < -S_THRESHOLD)
		// {
		// 	return false;
		// }
	}
	if (abs(mousePath[0].y - mousePath[mousePath.size() - 1].y) < 35.f)
	{
		return false;
	}

	if (leftToRight && rightToLeft && length > MIN_S_LENGTH)
	{
		return true;
	}
	// if start y coordinate is far from end y coordinate, not S
	return false;
}

void WorldSystem::on_mouse_button(int button, int action, int mods)
{
	if (has_popup && action == GLFW_PRESS)
	{
		// dismiss popup
		has_popup = false;
		is_paused = false;
		if (active_popup.onDismiss != nullptr)
			active_popup.onDismiss();
		active_popup = {};
		return;
	}
	if (is_paused)
	{
		// skip processing mouse button events when game is paused
		return;
	}

	// std::cout << "Mouse button " << button << " " << action << " " << mods << std::endl;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isTrackingMouse = true; // tracking mouse gesture if any
		mousePath.clear();
		Player &player = registry.players.get(player_spy);
		if (player.state == PlayerState::IDLE)
		{
			std::cout << "Player light attack" << std::endl;
			player.state = PlayerState::LIGHT_ATTACK;
			player.current_attack_id = ++attack_id_counter;

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
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isTrackingMouse = false;
		if (WorldSystem::isSGesture() && enlarged_player == false)
		{
			// make spy grow in size
			Motion &spy_motion = registry.motions.get(player_spy);
			spy_motion.scale = spy_motion.scale * 1.5f;
			spy_motion.bb_scale *= 1.5f;
			printf("S gesture detected\n");
			enlarged_player = true;
		}
		mousePath.clear();
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
						player.current_attack_id = ++attack_id_counter;
						player.is_heavy_attack_second_half = false;
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
