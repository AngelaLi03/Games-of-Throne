// internal
#include "physics_system.hpp"
#include "world_init.hpp"

#include <iostream>

// Returns the local bounding coordinates scaled by the current size of the entity (no longer used)
vec2 get_bounding_box(const Motion &motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return {abs(motion.scale.x), abs(motion.scale.y)};
}

bool collides(const vec2 &p1, const vec2 &b1, const vec2 &p2, const vec2 &b2)
{
	// AABB collision test
	return p1.y < p2.y + b2.y && p2.y < p1.y + b1.y && p1.x < p2.x + b2.x && p2.x < p1.x + b1.x;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto &motion_registry = registry.motions;
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		Motion &motion = motion_registry.components[i];
		// Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		motion.position += motion.velocity * step_seconds;
	}

	// Check for collisions between all moving entities
	ComponentContainer<PhysicsBody> &physicsBody_container = registry.physicsBodies;
	for (uint i = 0; i < physicsBody_container.components.size(); i++)
	{
		PhysicsBody &physicsBody_i = physicsBody_container.components[i];
		if (physicsBody_i.body_type == BodyType::STATIC)
		{
			// ensure i is kinematic body
			continue;
		}
		Entity entity_i = physicsBody_container.entities[i];
		Motion &motion_i = motion_registry.get(entity_i);

		for (uint j = 0; j < physicsBody_container.components.size(); j++)
		{
			if (j == i)
			{
				continue;
			}
			PhysicsBody &physicsBody_j = physicsBody_container.components[j];
			if (j < i && physicsBody_j.body_type == BodyType::KINEMATIC)
			{
				// this pair is already processed
				continue;
			}
			Entity entity_j = physicsBody_container.entities[j];
			Motion &motion_j = motion_registry.get(entity_j);

			vec2 b1 = physicsBody_i.bounding_box;
			vec2 b2 = physicsBody_j.bounding_box;
			vec2 p1 = motion_i.position + physicsBody_i.offset;
			vec2 p2 = motion_j.position + physicsBody_j.offset;

			if (collides(p1, b1, p2, b2))
			{
				// std::cout << "position of i: " << p1.x << "," << p1.y << "; position of j: " << p2.x << "," << p2.y << std::endl;
				// std::cout << "bb of i: " << b1.x << "," << b1.y << "; bb of j: " << b2.x << "," << b2.y << std::endl;

				// aabb collision resolution
				float overlap_x = min(p1.x + b1.x - p2.x, p2.x + b2.x - p1.x);
				float overlap_y = min(p1.y + b1.y - p2.y, p2.y + b2.y - p1.y);
				// std::cout << "overlap: " << overlap_x << "," << overlap_y << std::endl;
				if (physicsBody_j.body_type == BodyType::STATIC)
				{
					if (overlap_x < overlap_y)
					{
						if (p1.x < p2.x)
						{
							motion_i.position.x -= overlap_x;
						}
						else
						{
							motion_i.position.x += overlap_x;
						}
					}
					else
					{
						if (p1.y < p2.y)
						{
							motion_i.position.y -= overlap_y;
						}
						else
						{
							motion_i.position.y += overlap_y;
						}
					}
				}
				else
				{
					if (overlap_x < overlap_y)
					{
						if (p1.x < p2.x)
						{
							motion_i.position.x -= overlap_x / 2;
							motion_j.position.x += overlap_x / 2;
						}
						else
						{
							motion_i.position.x += overlap_x / 2;
							motion_j.position.x -= overlap_x / 2;
						}
					}
					else
					{
						if (p1.y < p2.y)
						{
							motion_i.position.y -= overlap_y / 2;
							motion_j.position.y += overlap_y / 2;
						}
						else
						{
							motion_i.position.y += overlap_y / 2;
							motion_j.position.y -= overlap_y / 2;
						}
					}
				}

				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
			// Motion &motion_j = physicsBody_container.components[j];
			// if (collides(motion_i, motion_j))
			// {
			// 	Entity entity_j = physicsBody_container.entities[j];
			// 	// Create a collisions event
			// 	// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
			// 	registry.collisions.emplace_with_duplicates(entity_i, entity_j);
			// 	registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			// }
		}
	}
}