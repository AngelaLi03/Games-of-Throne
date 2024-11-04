#pragma once

#include <array>
#include <utility>
#include <map>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

// font character structure
struct Character
{
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2 Size;				// Size of glyph
	glm::ivec2 Bearing;			// Offset from baseline to left/top of glyph
	unsigned int Advance;		// Offset to advance to next glyph
	char character;
};

glm::mat3 get_transform(const Motion &motion);

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem
{
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count> texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
			{
					std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::SALMON, mesh_path("salmon.obj"))
					// specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
			textures_path("green_fish.png"),
			textures_path("eel.png"),
			textures_path("floor_tile.png"),
			textures_path("wall.png"),
			textures_path("spy_no_weapon.png"),
			textures_path("enemy_small.png"),
			textures_path("weapon.png"),
			textures_path("flow_meter.png"),
			textures_path("enemy_corpse.png"),
			textures_path("enemy_attack.png"),
			textures_path("spy_corpse.png"),
			textures_path("chef.png"),
	};

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = {
			shader_path("coloured"),
			shader_path("debug_line"),
			shader_path("salmon"),
			shader_path("textured"),
			shader_path("water"),
			shader_path("progress_bar"),
			shader_path("liquid_fill"),
			shader_path("text")};

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;
	std::array<TexturedMesh, geometry_count> textured_meshes;

public:
	// Initialize the window
	bool init(GLFWwindow *window);

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();
	Mesh &getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };
	TexturedMesh &getTexturedMesh(GEOMETRY_BUFFER_ID id) { return textured_meshes[(int)id]; };

	void initializeGlGeometryBuffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the wind
	// shader
	bool initScreenTexture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw();
	void renderText(const std::string &text, float x, float y, float scale, vec3 color);
	// void initializeTextRendering();
	mat3 createProjectionMatrix();
	void initTextRendering(); // Add this method declaration
	void loadFont(const std::string &fontPath);

private:
	// Internal drawing functions for each entity type
	void drawTexturedMesh(Entity entity, const mat3 &projection);
	void drawToScreen();

	// Window handle
	GLFWwindow *window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	GLuint vao;

	GLuint textVAO;
	GLuint textVBO;
	GLuint textProgram;
	Entity screen_state_entity;
	std::map<char, Character> characters;
	std::vector<std::string> gameInstructions = {
	"Controls:",
	"W / Up Arrow: Move Up",
	"A / Left Arrow: Move Left",
	"S / Down Arrow: Move Down",
	"D / Right Arrow: Move Right",
	"X: Stealth Travel (Point mouse and press X)",
	"H: Decrease Player Health",
	"F: Increase Flow",
	"Space Bar: Dash (when already moving)",
	"Escape: Exit Game",
	"Press 'H' ten times to die",
	"Press 'P' to see FPS",
	"Press 'O' to toggle instructions",


	};
};

bool loadEffectFromFile(
		const std::string &vs_path, const std::string &fs_path, GLuint &out_program);
