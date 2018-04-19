#include "Terrain.h"

Terrain::Terrain()
{
}

Terrain::~Terrain()
{
}

void Terrain::Init()
{
	// Load in necessary shaders
	terrainEffect.add_shader("res/shaders/terrain.vert", GL_VERTEX_SHADER);
	terrainEffect.add_shader("res/shaders/terrain.frag", GL_FRAGMENT_SHADER);
	// terrainEffect.add_shader("shaders/part_direction.frag", GL_FRAGMENT_SHADER);
	terrainEffect.add_shader("res/shaders/part_weighted_texture.frag", GL_FRAGMENT_SHADER);
	// Build effect
	terrainEffect.build();
}

void Terrain::SetTextures(texture textures[4])
{
	for (int i = 0; i < 4; ++i)
	{
		tex[i] = textures[i];
	}
}

void Terrain::Render(mat4 VP)
{
	renderer::bind(terrainEffect);
	// Create MVP matrix
	auto M = terr.get_transform().get_transform_matrix();
	auto MVP = VP * M;
	// Set MVP matrix uniform
	glUniformMatrix4fv(terrainEffect.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	// Set M matrix uniform
	glUniformMatrix4fv(terrainEffect.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
	// Set N matrix uniform
	glUniformMatrix3fv(terrainEffect.get_uniform_location("N"), 1, GL_FALSE, value_ptr(terr.get_transform().get_normal_matrix()));
	//Bind Terrian Material
	renderer::bind(terr.get_material(), "mat");
	// Bind Tex[0] to TU 0, set uniform
	renderer::bind(tex[0], 0);
	glUniform1i(terrainEffect.get_uniform_location("tex[0]"), 0);
	//Bind Tex[1] to TU 1, set uniform
	renderer::bind(tex[1], 1);
	glUniform1i(terrainEffect.get_uniform_location("tex[1]"), 1);
	// Bind Tex[2] to TU 2, set uniform
	renderer::bind(tex[2], 2);
	glUniform1i(terrainEffect.get_uniform_location("tex[2]"), 2);
	// Bind Tex[3] to TU 3, set uniform
	renderer::bind(tex[3], 3);
	glUniform1i(terrainEffect.get_uniform_location("tex[3]"), 3);
	// Render terrain
	renderer::render(terr);
}

mesh * Terrain::GetMesh()
{
	return &terr;
}

void Terrain::LoadTerrain(const texture & heightMap, unsigned int width, unsigned int depth, float height_scale)
{
	// Contains our position data
	vector<vec3> positions;
	// Contains our normal data
	vector<vec3> normals;
	// Contains our texture coordinate data
	vector<vec2> tex_coords;
	// Contains our texture weights
	vector<vec4> tex_weights;
	// Contains our index data
	vector<unsigned int> indices;

	// Extract the texture data from the image
	glBindTexture(GL_TEXTURE_2D, heightMap.get_id());
	auto data = new vec4[heightMap.get_width() * heightMap.get_height()];
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (void *)data);

	// Determine ratio of height map to geometry
	float width_point = static_cast<float>(width) / static_cast<float>(heightMap.get_width());
	float depth_point = static_cast<float>(depth) / static_cast<float>(heightMap.get_height());

	// Point to work on
	vec3 point;

	// Part 1 - Iterate through each point, calculate vertex and add to vector
	for (int x = 0; x < heightMap.get_width(); ++x) {
		// Calculate x position of point
		point.x = -(width / 2.0f) + (width_point * static_cast<float>(x));

		for (int z = 0; z < heightMap.get_height(); ++z) {
			// *********************************
			// Calculate z position of point
			point.z = -(depth / 2.0f) + (depth_point * static_cast<float>(z));
			// *********************************
			// Y position based on red component of height map data
			point.y = data[(z * heightMap.get_width()) + x].y * height_scale;
			// Add point to position data
			positions.push_back(point);
		}
	}

	// Part 1 - Add index data
	for (unsigned int x = 0; x < heightMap.get_width() - 1; ++x) {
		for (unsigned int y = 0; y < heightMap.get_height() - 1; ++y) {
			// Get four corners of patch
			unsigned int top_left = (y * heightMap.get_width()) + x;
			unsigned int top_right = (y * heightMap.get_width()) + x + 1;
			// *********************************
			unsigned int bottom_left = ((y + 1) * heightMap.get_width()) + x;
			unsigned int bottom_right = ((y + 1) * heightMap.get_height()) + x + 1;

			// *********************************
			// Push back indices for triangle 1 (tl,br,bl)
			indices.push_back(top_left);
			indices.push_back(bottom_right);
			indices.push_back(bottom_left);
			// Push back indices for triangle 2 (tl,tr,br)
			// *********************************
			indices.push_back(top_left);
			indices.push_back(top_right);
			indices.push_back(bottom_right);
			// *********************************
		}
	}

	// Resize the normals buffer
	normals.resize(positions.size());

	// Part 2 - Calculate normals for the height map
	for (unsigned int i = 0; i < indices.size() / 3; ++i) {
		// Get indices for the triangle
		auto idx1 = indices[i * 3];
		auto idx2 = indices[i * 3 + 1];
		auto idx3 = indices[i * 3 + 2];

		// Calculate two sides of the triangle
		vec3 side1 = positions[idx2] - positions[idx1];
		vec3 side2 = positions[idx3] - positions[idx1];

		// Normal is normal(cross product) of these two sides
		vec3 normal = cross(side1, side2);
		// Add to normals in the normal buffer using the indices for the triangle
		normals[idx1] += normal;
		normals[idx2] += normal;
		normals[idx3] += normal;
	}

	// Normalize all the normals
	for (auto &n : normals) {
		n = normalize(n);
	}

	// Part 3 - Add texture coordinates for geometry
	for (unsigned int x = 0; x < heightMap.get_width(); ++x) {
		for (unsigned int z = 0; z < heightMap.get_height(); ++z) {
			tex_coords.push_back(vec2(width_point * x, depth_point * z));
		}
	}

	// Part 4 - Calculate texture weights for each vertex
	for (unsigned int x = 0; x < heightMap.get_width(); ++x) {
		for (unsigned int z = 0; z < heightMap.get_height(); ++z) {
			// Calculate tex weight
			vec4 tex_weight(clamp(1.0f - abs(data[(heightMap.get_width() * z) + x].y - 0.0f) / 0.25f, 0.0f, 1.0f),
				clamp(1.0f - abs(data[(heightMap.get_width() * z) + x].y - 0.15f) / 0.25f, 0.0f, 1.0f),
				clamp(1.0f - abs(data[(heightMap.get_width() * z) + x].y - 0.5f) / 0.25f, 0.0f, 1.0f),
				clamp(1.0f - abs(data[(heightMap.get_width() * z) + x].y - 0.9f) / 0.25f, 0.0f, 1.0f));

			// *********************************
			// Sum the components of the vector
			float sum = tex_weight.r + tex_weight.g + tex_weight.b + tex_weight.a;
			// Divide weight by sum
			tex_weight /= sum;
			// Add tex weight to weights
			tex_weights.push_back(tex_weight);
			// *********************************
		}
	}

	// Add necessary buffers to the geometry
	geom.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	geom.add_buffer(normals, BUFFER_INDEXES::NORMAL_BUFFER);
	geom.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);
	geom.add_buffer(tex_weights, BUFFER_INDEXES::TEXTURE_COORDS_1);
	geom.add_index_buffer(indices);

	terr = mesh(geom);

	terr.get_material().set_diffuse(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	terr.get_material().set_specular(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	terr.get_material().set_shininess(20.0f);
	terr.get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));

	// Delete data
	delete[] data;
}