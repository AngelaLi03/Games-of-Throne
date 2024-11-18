#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

void createBox(vec2 position, vec2 scale, vec3 color, float angle = 0.f);

// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size, vec3 color, float angle = 0.f);

// the floor tile
Entity createFloorTile(RenderSystem *renderer, vec2 pos);

// a Wall
Entity createWall(RenderSystem *renderer, vec2 pos);

Entity createSpy(RenderSystem *renderer, vec2 pos);

Entity createEnemy(RenderSystem *renderer, vec2 pos);

Entity createWeapon(RenderSystem *renderer, vec2 pos, WeaponType type, WeaponLevel level);

Entity createHealthBar(RenderSystem *renderer, vec2 pos, Entity owner_entity, vec3 color = vec3(0.f, 1.f, 0.f));

Entity createEnergyBar(RenderSystem *renderer, vec2 pos, Entity owner_entity, vec3 color = vec3(0.f, 0.f, 1.f));

Entity createFlowMeter(RenderSystem *renderer, vec2 pos, float scale);

Entity createChef(RenderSystem *renderer, vec2 pos);

Entity createTomato(RenderSystem *renderer, vec2 pos, vec2 velocity);

Entity createPan(RenderSystem *renderer, vec2 pos, vec2 velocity);

Entity createSpinArea(Entity chef_entity);

Entity createDamageArea(Entity owner, vec2 position, vec2 scale, float damage, float duration, float damage_cooldown = 0, bool relative_position = false, vec2 offset_from_owner = vec2(0, 0));

Entity createKnight(RenderSystem *renderer, vec2 pos);

Entity createBackdrop(RenderSystem *renderer);

Entity createDialogueWindow(RenderSystem *renderer, vec2 position, vec2 scale);

Entity createFountain(RenderSystem *renderer, vec2 pos);

Entity createTreasureBox(RenderSystem *renderer, vec2 pos, TreasureBoxItem item);

Entity createPlayerRemnant(RenderSystem *renderer, Motion motion);

// Generic function to create a static sprite without collision
Entity createSprite(RenderSystem *renderer, vec2 pos, vec2 scale, TEXTURE_ASSET_ID texture_id);
