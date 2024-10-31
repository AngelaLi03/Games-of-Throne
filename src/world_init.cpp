#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "iostream"

// Create floor tile entity and add to registry.
Entity createFloorTile(RenderSystem *renderer, vec2 pos, float tile_scale)
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
	motion.scale = {mesh.original_size.x * tile_scale, mesh.original_size.x * tile_scale};
	// std::cout << mesh.original_size.x << "," << mesh.original_size.y << std::endl;
	// create an empty floor tile component for our character
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::FLOOR_TILE,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createWall(RenderSystem *renderer, vec2 pos, float wall_scale)
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
	motion.scale = {mesh.original_size.x * wall_scale, mesh.original_size.x * wall_scale};

	// Print mesh size for debugging if needed
	// std::cout << mesh.original_size.x << "," << mesh.original_size.y << std::endl;

	registry.physicsBodies.insert(entity, {BodyType::STATIC, {mesh.original_size.x * wall_scale, mesh.original_size.x * wall_scale}, {0.f, 0.f}});

	// Set up the render request for the wall
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::WALL, // Make sure to use a wall texture
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

//void createRoom(RenderSystem* renderer, WallMap& wallMap, int x_start, int y_start, int width, int height, float tile_scale) {
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
//}

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

	// create an empty Spy component for our character
	registry.players.emplace(entity);
	registry.healthbar.emplace(entity, HealthBar(100.f, motion.scale));
	registry.healths.insert(entity, {100.f});
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC, {60.f, 60.f}, {0.f, 40.f}});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::SPY, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

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
	motion.velocity = {0.f, 0.f};
	motion.scale = mesh.original_size * 300.f;
	motion.scale.x *= 1.1;
	createHealthBar(renderer, motion.position + vec2(0.f, 100.f), entity);

	// create an empty Spy component for our character
	registry.chef.emplace(entity);
	registry.enemies.emplace(entity);
	registry.healthbar.emplace(entity, HealthBar(100.f, motion.scale));
	registry.healths.insert(entity, {100.f});
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC, {150.f, 130.f}, {0.f, 40.f}});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::CHEF, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createWeapon(RenderSystem *renderer, vec2 pos)
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
	motion.scale = mesh.original_size * 100.f;
	motion.scale.x *= -0.45;
	motion.scale.y *= 1.7;

	// create an empty Spy component for our character
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::WEAPON, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createHealthBar(RenderSystem *renderer, vec2 pos, Entity owner_entity)
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

	registry.healthbar.emplace(entity, HealthBar(100.f, motion.scale));
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::PROGRESS_BAR,
			 GEOMETRY_BUFFER_ID::PROGRESS_BAR});
	// Set to green
	registry.colors.emplace(entity, vec3(0.f, 1.f, 0.f));

	// Link the owner and health bar, for player and enemy.
	registry.healthbarlink.emplace(entity, owner_entity);

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

	// Create an (empty) Bug component to be able to refer to all bug
	registry.enemies.emplace(entity);
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC, {60.f, 60.f}, {0.f, 40.f}});
	registry.healths.insert(entity, {100.f});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::ENEMY,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	createHealthBar(renderer, position + vec2(0.f, 50.f), entity);
	return entity;
}

Entity createFish(RenderSystem *renderer, vec2 position)
{
	// Reserve en entity
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and physics components
	auto &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0, 50};
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({-FISH_BB_WIDTH, FISH_BB_HEIGHT});

	// Create an (empty) Bug component to be able to refer to all bug
	registry.eatables.emplace(entity);
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::FISH,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createEel(RenderSystem *renderer, vec2 position)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0, 100.f};
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({-EEL_BB_WIDTH, EEL_BB_HEIGHT});

	// create an empty Eel component to be able to refer to all eels
	registry.deadlys.emplace(entity);
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::EEL,
			 EFFECT_ASSET_ID::TEXTURED,
			 GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
			entity, {TEXTURE_ASSET_ID::TEXTURE_COUNT,
							 EFFECT_ASSET_ID::EGG,
							 GEOMETRY_BUFFER_ID::DEBUG_LINE});

	// Create motion
	Motion &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0, 0};
	motion.position = position;
	motion.scale = scale;

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createEgg(vec2 pos, vec2 size)
{
	auto entity = Entity();

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = size;

	// create an empty component for our eggs
	registry.deadlys.emplace(entity);
	registry.renderRequests.insert(
			entity, {TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
							 EFFECT_ASSET_ID::EGG,
							 GEOMETRY_BUFFER_ID::EGG});

	return entity;
}