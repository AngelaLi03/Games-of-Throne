#pragma once
#include "node.hpp"
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
enum class PlayerState
{
	IDLE = 0,
	DODGING = IDLE + 1,
	CHARGING_FLOW = DODGING + 1,
	LIGHT_ATTACK = CHARGING_FLOW + 1,
	HEAVY_ATTACK = LIGHT_ATTACK + 1,
};
struct Player
{
	PlayerState state = PlayerState::IDLE;
	float attack_damage = 20.0f;
	unsigned int current_attack_id = 0;
	bool is_heavy_attack_second_half = false;
};

struct Damage
{
	float damage;
};

enum class EnemyState
{
	IDLE = 0,
	COMBAT = 1,
	ATTACK = 2,
	DEAD = 3,
};
struct Enemy
{
	EnemyState state = EnemyState::IDLE;
	float time_since_last_attack = 0.f;
	float attack_countdown = 500;
	float attack_damage = 10.0f;
	unsigned int last_hit_attack_id = 0;
	// std::vector<Node> path;			// The path to the player
	// int current_path_index = 0; // Index of the next node to follow
};

struct SpinArea
{
};

struct Attachment
{
	Entity parent;
	Attachment(Entity parent_entity) : parent(parent_entity) {}
};

struct Weapon
{
	Entity weapon; // weapon
	vec2 offset;	 // weapon offset relative to player's position
};

// All data relevant to the shape and motion of entities
struct Motion
{
	vec2 position = {0, 0};
	float angle = 0;
	vec2 velocity = {0, 0};
	vec2 scale = {10, 10};
	vec2 bb_scale = {10, 10};		// scale used for bounding box
	vec2 bb_offset = {0, 0};		// offset from motion.position to center of bounding box
	vec2 pivot_offset = {0, 0}; // before scaling
};

enum class AnimationName
{
	LIGHT_ATTACK = 0,
	HEAVY_ATTACK = LIGHT_ATTACK + 1,
};
struct Animation
{
	AnimationName name = AnimationName::LIGHT_ATTACK;
	float total_time = 1000.f;
	float elapsed_time = 0.f;
};

struct Interpolation
{
	float elapsed_time = 0;
	float total_time_to_0_ms = 700; // time to observe effect
	vec2 initial_velocity;					// velocity when button is released
};

struct Bezier
{
	glm::vec2 initial_velocity;
	glm::vec2 target_position;
	glm::vec2 control_point;
	float elapsed_time;
	float total_time_to_0_ms = 2000;
};

struct Flow
{
	float flowLevel;		// This could represent the current flow level
	float maxFlowLevel; // Maximum flow level
};

struct Health
{
	float health;
	float max_health = 100.f;
	Entity healthbar;
	bool is_dead = false;
	void take_damage(float damage);
};

struct HealthBar
{
	vec2 original_scale;
};

enum class BodyType
{
	STATIC = 0, // reserved for walls
	KINEMATIC = STATIC + 1,
	PROJECTILE = KINEMATIC + 1,
	NONE = PROJECTILE + 1
};
struct PhysicsBody
{
	BodyType body_type = BodyType::STATIC;
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(Entity &other) { this->other = other; };
};

enum class PanState
{
	TOWARDS_PLAYER,
	RETURNING,
};

struct Pan
{
	float damage;
	PanState state;
	bool player_hit = false;
	Pan(float dmg)
	{
		this->damage = dmg;
		this->state = PanState::TOWARDS_PLAYER;
	}
};

// Data structure for toggling debug mode
struct Debug
{
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex> &out_vertices, std::vector<uint16_t> &out_vertex_indices, vec2 &out_size);
	vec2 original_size = {1, 1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

struct TexturedMesh
{
	std::vector<TexturedVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

enum class ChefState
{
	PATROL = 0,
	COMBAT = 1,
	ATTACK = 2,
};
enum class ChefAttack
{
	TOMATO = 0,
	PAN = 1,
	DASH = 2,
	SPIN = 3,
	ATTACK_COUNT = 4,
};
struct Chef
{
	ChefState state = ChefState::PATROL;
	ChefAttack current_attack = ChefAttack::TOMATO;

	float time_since_last_patrol = 0;
	float time_since_last_attack = 0;

	bool pan_active = false;
	bool dash_has_damaged = false;

	// when chef just entered combat, play a sound and set back to false
	bool trigger = false;
	float sound_trigger_timer = 0;
};

struct MoveUI
{
};
/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID
{
	FISH = 0,
	EEL = FISH + 1,
	// Added FLOOR_TILE texture.
	FLOOR_TILE = EEL + 1,
	// Added Wall texture.
	WALL = FLOOR_TILE + 1,
	SPY = WALL + 1,
	ENEMY = SPY + 1,
	WEAPON = ENEMY + 1,
	FLOW_METER = WEAPON + 1,
	ENEMY_CORPSE = FLOW_METER + 1,
	ENEMY_ATTACK = ENEMY_CORPSE + 1,
	SPY_CORPSE = ENEMY_ATTACK + 1,
	CHEF = SPY_CORPSE + 1,
	TOMATO = CHEF + 1,
	PAN = TOMATO + 1,
	TEXTURE_COUNT = PAN + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID
{
	COLOURED = 0,
	DEBUG_LINE = COLOURED + 1,
	SALMON = DEBUG_LINE + 1,
	TEXTURED = SALMON + 1,
	WATER = TEXTURED + 1,
	PROGRESS_BAR = WATER + 1,
	LIQUID_FILL = PROGRESS_BAR + 1,
	TEXT = LIQUID_FILL + 1,
	EFFECT_COUNT = TEXT + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID
{
	SALMON = 0,
	SPRITE = SALMON + 1,
	WEAPON = SPRITE + 1,
	DEBUG_LINE = WEAPON + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	PROGRESS_BAR = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = PROGRESS_BAR + 1,
	// // Defined FLOOR_TILE geometry.
	// FLOOR_TILE = GEOMETRY_COUNT + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct SpriteAnimation
{
	std::vector<TEXTURE_ASSET_ID> frames; // Texture IDs for each animation frame
	int current_frame = 0;								// Index of the current frame
	float frame_duration = 0.1f;					// Duration for each frame (seconds)
	float elapsed_time = 0.0f;						// Time since the last frame switch
};

struct RenderRequest
{
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};
