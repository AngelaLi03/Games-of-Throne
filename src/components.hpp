#pragma once
#include "node.hpp"
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

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

// Player component
enum class PlayerState
{
	IDLE = 0,
	DODGING = IDLE + 1,
	CHARGING_FLOW = DODGING + 1,
	LIGHT_ATTACK = CHARGING_FLOW + 1,
	HEAVY_ATTACK = LIGHT_ATTACK + 1,
	DYING = HEAVY_ATTACK + 1,
	DASHING = DYING + 1,
	SPRINTING = DASHING + 1,
};
struct Player
{
	PlayerState state = PlayerState::IDLE;
	Entity weapon;			// current weapon
	vec2 weapon_offset; // weapon offset relative to player's position
	float attack_damage = 20.0f;
	unsigned int current_attack_id = 0;
	bool is_heavy_attack_second_half = false;
	bool consumedLightAttackEnergy = false;
	bool consumedDodgeEnergy = false;
	float dash_cooldown_remaining_ms = 0.0f;
	bool current_dodge_is_perfect = false;
	Motion current_dodge_original_motion;
	bool damage_prevented = false;

	// must call can_take_damage before dealing damage to player
	bool can_take_damage(bool will_take_damage = true)
	{
		// player does not take damage during dodging or dying
		if (state == PlayerState::DODGING)
		{
			if (will_take_damage)
				damage_prevented = true;
			return false;
		}
		return state != PlayerState::DYING;
	}
};

// for perfect dodge mechanic
struct PlayerRemnant
{
	float time_until_destroyed = 500.f;
};

struct Damage
{
	float damage;
};

struct DamageArea
{
	float time_until_active;
	float time_until_destroyed;
	Entity owner;
	bool relative_position = false;
	vec2 offset_from_owner;
	bool active = false;
	bool single_damage = true;
	float damage_cooldown = 0;
	float time_since_last_damage = 0;
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
	std::vector<vec2> path;
	size_t current_path_index;
	int pathfinding_counter;
	vec2 last_tile_position = {0, 0};
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

enum class WeaponType
{
	SWORD,
	DAGGER
};

enum class WeaponLevel
{
	BASIC,
	RARE,
	LEGENDARY
};

struct Weapon
{
	WeaponType type;
	WeaponLevel level;
	float damage = 20.f;
	float attack_speed = 1.0f;
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

// struct Bezier
//{
//	glm::vec2 initial_velocity;
//	glm::vec2 target_position;
//	glm::vec2 control_point;
//	float elapsed_time;
//	float total_time_to_0_ms = 2000;
// };

struct Dash
{
	float elapsed_time = 0.f;
	float total_time_ms = 10000.f;
};

struct PopupUI
{
	bool active = true;
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

struct Energy
{
	float energy;
	float max_energy = 100.f;
	Entity energybar;
};

struct EnergyBar
{
	vec2 original_scale;
};

struct Fountain
{
	// bool is_active = false;
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

enum class KnightState
{
	PATROL = 0,
	COMBAT = 1,
	ATTACK = 2,
	MULTI_DASH = 3,
	SHIELD = 4,
};

enum class ChefAttack
{
	TOMATO = 0,
	PAN = 1,
	DASH = 2,
	SPIN = 3,
	ATTACK_COUNT = 4,
};

enum class KnightAttack
{
	DASH_ATTACK = 0,
	SHIELD_HOLD = 1,
	MULTI_DASH = 2,
	ATTACK_COUNT = 3,
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

struct Knight
{
	KnightState state = KnightState::PATROL;
	KnightAttack current_attack = KnightAttack::DASH_ATTACK;

	float time_since_last_attack = 0.f;
	float time_since_last_patrol = 0.f;
	float attack_duration = 0.f;
	int dash_count = 0;
	bool shield_active = false;
	bool shield_broken = false;
	float shield_duration = 0.f;
	bool dash_has_damaged = false;
};

struct MoveUI
{
};

enum class TreasureBoxItem
{
	MAX_HEALTH = 0,
	MAX_ENERGY = MAX_HEALTH + 1,
	WEAPON = MAX_ENERGY + 1, // TODO: later change to different types of weapon
	NONE = WEAPON + 1,
};
struct TreasureBox
{
	bool is_open = false;
	TreasureBoxItem item = TreasureBoxItem::NONE;
	Entity item_entity = Entity(0);
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
	SWORD_BASIC = PAN + 1,
	SWORD_RARE = SWORD_BASIC + 1,
	SWORD_LEGENDARY = SWORD_RARE + 1,
	DAGGER_BASIC = SWORD_LEGENDARY + 1,
	DAGGER_RARE = DAGGER_BASIC + 1,
	DAGGER_LEGENDARY = DAGGER_RARE + 1,
	KNIGHT = DAGGER_LEGENDARY + 1,
	FOUNTAIN = KNIGHT + 1,
	STEALTH = FOUNTAIN + 1,
	TREASURE_BOX = STEALTH + 1,
	TREASURE_BOX_OPEN = TREASURE_BOX + 1,
	UI_FRAME = TREASURE_BOX_OPEN + 1,
	MAX_HEALTH = UI_FRAME + 1,
	MAX_ENERGY = MAX_HEALTH + 1,
	DIALOGUE_WINDOW = MAX_ENERGY + 1,

	CHEF1_0 = DIALOGUE_WINDOW + 1,
	CHEF1_1 = CHEF1_0 + 1,
	CHEF1_2 = CHEF1_1 + 1,
	CHEF1_3 = CHEF1_2 + 1,
	CHEF1_4 = CHEF1_3 + 1,
	CHEF1_5 = CHEF1_4 + 1,
	CHEF1_6 = CHEF1_5 + 1,
	CHEF1_7 = CHEF1_6 + 1,
	CHEF1_8 = CHEF1_7 + 1,
	CHEF1_9 = CHEF1_8 + 1,
	CHEF1_10 = CHEF1_9 + 1,
	CHEF1_11 = CHEF1_10 + 1,

	// chef attack 2
	CHEF2_0 = CHEF1_11 + 1,
	CHEF2_1 = CHEF2_0 + 1,
	CHEF2_2 = CHEF2_1 + 1,
	CHEF2_3 = CHEF2_2 + 1,
	CHEF2_4 = CHEF2_3 + 1,
	CHEF2_5 = CHEF2_4 + 1,
	CHEF2_6 = CHEF2_5 + 1,
	CHEF2_7 = CHEF2_6 + 1,
	CHEF2_8 = CHEF2_7 + 1,
	CHEF2_9 = CHEF2_8 + 1,
	CHEF2_10 = CHEF2_9 + 1,
	CHEF2_11 = CHEF2_10 + 1,
	CHEF2_12 = CHEF2_11 + 1,
	CHEF2_13 = CHEF2_12 + 1,
	CHEF2_14 = CHEF2_13 + 1,
	CHEF2_15 = CHEF2_14 + 1,
	CHEF2_16 = CHEF2_15 + 1,
	CHEF2_17 = CHEF2_16 + 1,
	CHEF2_18 = CHEF2_17 + 1,
	CHEF2_19 = CHEF2_18 + 1,
	CHEF2_20 = CHEF2_19 + 1,

	CHEF3_0 = CHEF2_20 + 1,
	CHEF3_1 = CHEF3_0 + 1,
	CHEF3_2 = CHEF3_1 + 1,
	CHEF3_3 = CHEF3_2 + 1,
	CHEF3_4 = CHEF3_3 + 1,
	CHEF3_5 = CHEF3_4 + 1,
	CHEF3_6 = CHEF3_5 + 1,
	CHEF3_7 = CHEF3_6 + 1,
	CHEF3_8 = CHEF3_7 + 1,
	CHEF3_9 = CHEF3_8 + 1,
	CHEF3_10 = CHEF3_9 + 1,
	CHEF3_11 = CHEF3_10 + 1,
	CHEF3_12 = CHEF3_11 + 1,
	CHEF3_13 = CHEF3_12 + 1,
	CHEF3_14 = CHEF3_13 + 1,
	CHEF3_15 = CHEF3_14 + 1,
	CHEF3_16 = CHEF3_15 + 1,
	CHEF3_17 = CHEF3_16 + 1,
	CHEF3_18 = CHEF3_17 + 1,
	CHEF3_19 = CHEF3_18 + 1,
	CHEF3_20 = CHEF3_19 + 1,

	TEXTURE_COUNT = CHEF3_20 + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID
{
	DEBUG_LINE = 0,
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
	DAGGER = PROGRESS_BAR + 1,
	GEOMETRY_COUNT = DAGGER + 1,
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
	bool isattacking = false;
};

struct BossAnimation
{
	std::vector<TEXTURE_ASSET_ID> attack_1; // Frames for basic attack animation
	std::vector<TEXTURE_ASSET_ID> attack_2;
	std::vector<TEXTURE_ASSET_ID> attack_3;
	std::vector<TEXTURE_ASSET_ID> attack_4;
	std::vector<TEXTURE_ASSET_ID> attack_5;

	int current_frame = 0;			 // Index of the current frame
	float frame_duration = 0.1f; // Duration for each frame (seconds)
	float elapsed_time = 0.0f;	 // Time since the last frame switch
	bool is_attacking = false;
	int attack_id = -1;
};

struct RenderRequest
{
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};
