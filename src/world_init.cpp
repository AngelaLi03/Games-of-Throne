#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "iostream"

// Create floor tile entity and add to registry. Utilizing the existing createSalmon code as template.
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
	registry.physicsBodies.insert(entity, {BodyType::KINEMATIC, {60.f, 60.f}, {0.f, 40.f}});
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::SPY, // TEXTURE_COUNT indicates that no texture is needed
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
	registry.weapons.emplace(entity);
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

Entity createSalmon(RenderSystem *renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SALMON);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.scale = mesh.original_size * 300.f;
	motion.scale.y *= -1; // point front to the right

	// create an empty Salmon component for our character
	registry.players.emplace(entity);
	registry.renderRequests.insert(
			entity,
			{TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			 EFFECT_ASSET_ID::SALMON,
			 GEOMETRY_BUFFER_ID::SALMON});

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