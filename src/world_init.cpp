#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "ai_system.hpp"
#include "iostream"
#include "world_system.hpp"

// Create floor tile entity and add to registry.
Entity createFloorTile(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = {mesh.original_size.x * TILE_SCALE, mesh.original_size.x * TILE_SCALE};
	// std::cout << mesh.original_size.x << "," << mesh.original_size.y << std::endl;
	// create an empty floor tile component for our character
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::FLOOR_TILE,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createWall(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Get the mesh for the wall, similar to how you got the floor mesh
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Set initial motion values, ensuring the wall is properly scaled and positioned
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos; // Use the same position or adjust for wall placement
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = {mesh.original_size.x * TILE_SCALE, mesh.original_size.x * TILE_SCALE};
	motion.bb_scale = motion.scale;
	motion.bb_offset = {0.f, 0.f};

	// Print mesh size for debugging if needed
	// std::cout << mesh.original_size.x << "," << mesh.original_size.y << std::endl;

	registry.physicsBodies.insert(entity, {BodyType::STATIC});

	// Set up the render request for the wall
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::WALL, // Make sure to use a wall texture
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

// void createRoom(RenderSystem* renderer, WallMap& wallMap, int x_start, int y_start, int width, int height, float tile_scale) {
//	for (int i = x_start; i < x_start + width; ++i) {
//		for (int j = y_start; j < y_start + height; ++j) {
//			// Calculate tile position in the game world
//			vec2 pos = { i * tile_scale, j * tile_scale };
//
//			// Place walls around the edges of the room
//			if (i == x_start || i == x_start + width - 1 || j == y_start || j == y_start + height - 1) {
//				createWall(renderer, pos, tile_scale);
//				wallMap.addWall(i, j);
//			}
//			else {
//				// Place floor tiles inside the room
//				createFloorTile(renderer, pos, tile_scale);
//			}
//		}
//	}
// }

Entity createSpy(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = mesh.original_size * 150.f;
	motion.scale.x *= -0.8;
	motion.bb_scale = {60.f, 60.f};
	motion.bb_offset = {0.f, 40.f};

	// create an empty Spy component for our character
	registry.players.emplace(entity);
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::SPY, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	Entity healthbar = createHealthBar(renderer, {50.f, 50.f}, entity);
	registry.healths.insert(entity, {100.f, 100.f, healthbar});

	return entity;
}

Entity createChef(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	// motion.velocity = {0.f, 0.f};
	motion.velocity = {50.f, 0.f};
	motion.scale = mesh.original_size * 300.f;
	motion.scale.x *= 1.6;
	motion.bb_scale = {150.f, 130.f};
	motion.bb_offset = {0.f, 40.f};

	// create an empty Spy component for our character
	registry.chef.emplace(entity);
	registry.enemies.emplace(entity);

	auto &bossAnimation = registry.bossAnimations.emplace(entity);
	bossAnimation.attack_1 = std::vector<TEXTURE_ASSET_ID>{
			TEXTURE_ASSET_ID::CHEF1_0,
			TEXTURE_ASSET_ID::CHEF1_1,
			TEXTURE_ASSET_ID::CHEF1_2,
			TEXTURE_ASSET_ID::CHEF1_3,
			TEXTURE_ASSET_ID::CHEF1_4,
			TEXTURE_ASSET_ID::CHEF1_5,
			TEXTURE_ASSET_ID::CHEF1_6,
			TEXTURE_ASSET_ID::CHEF1_7,
			TEXTURE_ASSET_ID::CHEF1_8,
			TEXTURE_ASSET_ID::CHEF1_9,
			TEXTURE_ASSET_ID::CHEF1_10,
			TEXTURE_ASSET_ID::CHEF1_11,
	};

	bossAnimation.attack_2 = std::vector<TEXTURE_ASSET_ID>{
			TEXTURE_ASSET_ID::CHEF2_0,
			TEXTURE_ASSET_ID::CHEF2_1,
			TEXTURE_ASSET_ID::CHEF2_2,
			TEXTURE_ASSET_ID::CHEF2_3,
			TEXTURE_ASSET_ID::CHEF2_4,
			TEXTURE_ASSET_ID::CHEF2_5,
			TEXTURE_ASSET_ID::CHEF2_6,
			TEXTURE_ASSET_ID::CHEF2_7,
			TEXTURE_ASSET_ID::CHEF2_8,
			TEXTURE_ASSET_ID::CHEF2_9,
			TEXTURE_ASSET_ID::CHEF2_10,
			TEXTURE_ASSET_ID::CHEF2_11,
			TEXTURE_ASSET_ID::CHEF2_12,
			TEXTURE_ASSET_ID::CHEF2_13,
			TEXTURE_ASSET_ID::CHEF2_14,
			TEXTURE_ASSET_ID::CHEF2_15,
			TEXTURE_ASSET_ID::CHEF2_16,
			TEXTURE_ASSET_ID::CHEF2_17,
			TEXTURE_ASSET_ID::CHEF2_18,
			TEXTURE_ASSET_ID::CHEF2_19,
			TEXTURE_ASSET_ID::CHEF2_20,
	};

	bossAnimation.attack_3 = std::vector<TEXTURE_ASSET_ID>{
			TEXTURE_ASSET_ID::CHEF3_0,
			TEXTURE_ASSET_ID::CHEF3_1,
			TEXTURE_ASSET_ID::CHEF3_2,
			TEXTURE_ASSET_ID::CHEF3_3,
			TEXTURE_ASSET_ID::CHEF3_4,
			TEXTURE_ASSET_ID::CHEF3_5,
			TEXTURE_ASSET_ID::CHEF3_6,
			TEXTURE_ASSET_ID::CHEF3_7,
			TEXTURE_ASSET_ID::CHEF3_8,
			TEXTURE_ASSET_ID::CHEF3_9,
			TEXTURE_ASSET_ID::CHEF3_10,
			TEXTURE_ASSET_ID::CHEF3_11,
			TEXTURE_ASSET_ID::CHEF3_12,
			TEXTURE_ASSET_ID::CHEF3_13,
			TEXTURE_ASSET_ID::CHEF3_14,
			TEXTURE_ASSET_ID::CHEF3_15,
			TEXTURE_ASSET_ID::CHEF3_16,
			TEXTURE_ASSET_ID::CHEF3_17,
			TEXTURE_ASSET_ID::CHEF3_18,
			TEXTURE_ASSET_ID::CHEF3_19,
			TEXTURE_ASSET_ID::CHEF3_20,
	};

	bossAnimation.frame_duration = 100.f; // 0.1s per frame
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC});
	registry.renderRequests.insert(
			entity,
			{bossAnimation.attack_1[bossAnimation.current_frame], // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	Entity healthbar = createHealthBar(renderer, motion.position + vec2(0.f, 100.f), entity, vec3(1.f, 0.f, 0.f));
	registry.healths.insert(entity, {500.f, 500.f, healthbar});

	return entity;
}

Entity createKnight(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	// motion.velocity = {0.f, 0.f};
	motion.velocity = {50.f, 0.f};
	motion.scale = mesh.original_size * 300.f;
	motion.scale.x *= 1.2;
	motion.bb_scale = {150.f, 130.f};
	motion.bb_offset = {0.f, 40.f};

	registry.knight.emplace(entity);
	registry.enemies.emplace(entity);

	auto &bossAnimation = registry.bossAnimations.emplace(entity);
	// bossAnimation.attack_1 = std::vector<TEXTURE_ASSET_ID>{
	// TODO: follow chef loading to load all images
	// };

	bossAnimation.frame_duration = 100.f; // 0.1s per frame
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::KNIGHT, // bossAnimation.attack_1[bossAnimation.current_frame], change when loaded all images
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	Entity healthbar = createHealthBar(renderer, motion.position + vec2(0.f, 100.f), entity, vec3(1.f, 0.f, 0.f));
	registry.healths.insert(entity, {500.f, 500.f, healthbar});

	return entity;
}

Entity createWeapon(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::WEAPON);
	registry.meshPtrs.emplace(entity, &mesh);
	TexturedMesh &weapon_mesh = renderer->getTexturedMesh(GEOMETRY_BUFFER_ID::WEAPON);
	registry.texturedMeshPtrs.emplace(entity, &weapon_mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = M_PI / 6; // 30 degrees
	// motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = mesh.original_size * 100.f;
	motion.scale.x *= 0.45;
	motion.scale.y *= 1.7;
	motion.bb_scale = {max(motion.scale.x, motion.scale.y) * 2.f, max(motion.scale.x, motion.scale.y) * 2.f};
	motion.bb_offset = {0.f, 50.f};
	motion.pivot_offset = {0.f, -0.35f};

	registry.physicsBodies.insert(entity, {BodyType::NONE});

	// create an empty Spy component for our character
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::WEAPON,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::WEAPON});

	return entity;
}

Entity createHealthBar(RenderSystem *renderer, vec2 pos, Entity owner_entity, vec3 color)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::PROGRESS_BAR);
	registry.meshPtrs.emplace(entity, &mesh);
	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = {200.f, 20.f};

	registry.healthbar.insert(entity, {motion.scale});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::PROGRESS_BAR,
			 GEOMETRY_BUFFER_ID::PROGRESS_BAR});
	// Set to green
	registry.colors.emplace(entity, color);

	return entity;
}

Entity createFlowMeter(RenderSystem *renderer, vec2 pos, float scale)
{
	auto entity = Entity();

	// Store a reference to the mesh object (assumed you've already defined it)
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and motion components
	auto &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.scale = {scale, scale};

	// Initialize the flow component
	Flow &flow = registry.flows.emplace(entity);
	flow.flowLevel = 0.f;			 // Start with no flow
	flow.maxFlowLevel = 100.f; // Max flow level can be adjusted as needed

	// Create a render request for the flow meter texture
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::FLOW_METER, EFFECT_ASSET_ID::LIQUID_FILL, GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createDamageArea(Entity owner, vec2 position, vec2 scale, float damage, float time_until_active, float duration, float damage_cooldown, bool relative_position, vec2 offset_from_owner)
{
	Entity entity = Entity();

	Motion &motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.scale = scale;
	motion.bb_scale = scale;

	registry.damages.insert(entity, {damage});
	registry.physicsBodies.insert(entity, {BodyType::NONE});

	DamageArea &damage_area = registry.damageAreas.emplace(entity);
	damage_area.owner = owner;
	damage_area.time_until_active = time_until_active;
	if (time_until_active == 0.f)
	{
		damage_area.active = true;
	}
	else
	{
		damage_area.active = false;
	}
	damage_area.time_until_inactive = duration;
	damage_area.relative_position = relative_position;
	damage_area.offset_from_owner = offset_from_owner;

	if (damage_cooldown == 0)
	{
		damage_area.single_damage = true;
	}
	else
	{
		damage_area.single_damage = false;
		damage_area.damage_cooldown = damage_cooldown;
	}
}

Entity createEnemy(RenderSystem *renderer, vec2 position)
{
	// Reserve en entity
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and physics components
	auto &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0, 0};
	motion.position = position;
	motion.scale = mesh.original_size * 100.f;
	motion.scale.x *= -0.8;
	motion.bb_scale = {60.f, 60.f};
	motion.bb_offset = {-5.f, 25.f};

	// Initialize the animation component with frames
	auto &spriteAnimation = registry.spriteAnimations.emplace(entity);
	spriteAnimation.frames = std::vector<TEXTURE_ASSET_ID>{
			TEXTURE_ASSET_ID::ENEMY,
			TEXTURE_ASSET_ID::ENEMY_ATTACK,
	};
	spriteAnimation.frame_duration = 1000.f; // Set duration for each frame (adjust as needed)

	// Create an (empty) Bug component to be able to refer to all bug
	registry.enemies.emplace(entity);
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC});
	registry.renderRequests.insert(
			entity,
			{spriteAnimation.frames[spriteAnimation.current_frame],
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	Entity healthbar = createHealthBar(renderer, position + vec2(0.f, 50.f), entity);
	registry.healths.insert(entity, {100.f, 100.f, healthbar});

	return entity;
}

Entity createTomato(RenderSystem *renderer, vec2 position, vec2 velocity)
{
	// Reserve en entity
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and physics components
	auto &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = velocity;
	motion.position = position;
	motion.scale = mesh.original_size * 100.f;
	motion.scale.x *= -0.8;
	motion.bb_scale = {30.f, 30.f};
	motion.bb_offset = {0.f, 0.f};

	std::cout << "create tomato" << std::endl;
	registry.damages.insert(entity, {10.f});
	registry.physicsBodies.insert(entity, {BodyType::PROJECTILE});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::TOMATO,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	// TODO: Need to disappear after hitting player or wall or boundaries.
	return entity;
}

Entity createPan(RenderSystem *renderer, vec2 position, vec2 velocity)
{
	// Reserve en entity
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and physics components
	auto &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = velocity;
	motion.position = position;
	motion.scale = mesh.original_size * 200.f;
	motion.scale.x *= -0.8;
	motion.bb_scale = {30.f, 30.f};
	motion.bb_offset = {0.f, 0.f};

	// Create an (empty) Bug component to be able to refer to all bug
	registry.pans.emplace(entity, Pan(25.f));
	registry.physicsBodies.insert(entity, {BodyType::NONE});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::PAN,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	// TODO: Need to disappear after hitting player or wall or boundaries.
	return entity;
}

void createBox(vec2 position, vec2 scale, vec3 color, float angle)
{
	float width = 5;
	// scale = {abs(scale.x), abs(scale.y)};
	createLine({position.x, position.y - scale.y / 2.f}, {scale.x + width, width}, color, angle);
	createLine({position.x - scale.x / 2.f, position.y}, {width, scale.y + width}, color, angle);
	createLine({position.x + scale.x / 2.f, position.y}, {width, scale.y + width}, color, angle);
	createLine({position.x, position.y + scale.y / 2.f}, {scale.x + width, width}, color, angle);
}

Entity createLine(vec2 position, vec2 scale, vec3 color, float angle)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
			entity, {TEXTURE_ASSET_ID::TEXTURE_COUNT,
							 EFFECT_ASSET_ID::DEBUG_LINE,
							 GEOMETRY_BUFFER_ID::DEBUG_LINE});

	// Create motion
	Motion &motion = registry.motions.emplace(entity);
	motion.angle = angle;
	motion.velocity = {0, 0};
	motion.position = position;
	motion.scale = scale;

	registry.colors.insert(entity, color);

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createSpinArea(Entity chef_entity)
{
	Entity entity = Entity();
	Motion &chef_motion = registry.motions.get(chef_entity);

	// // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	// registry.renderRequests.insert(
	// 		entity, {TEXTURE_ASSET_ID::TEXTURE_COUNT,
	// 						 EFFECT_ASSET_ID::DEBUG_LINE,
	// 						 GEOMETRY_BUFFER_ID::DEBUG_LINE});

	// Create motion
	Motion &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0, 0};
	motion.position = chef_motion.position;
	motion.scale = {200.f, 200.f};
	motion.bb_scale = motion.scale;
	motion.bb_offset = vec2(0.f, 0.f);

	registry.attachments.emplace(entity, Attachment(chef_entity));
	registry.spinareas.emplace(entity, SpinArea());
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC});
	return entity;
}
