#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include "shader_utils.h"

#include "textures.c"

static GLuint program;
static GLint attribute_coord;
static GLint uniform_mvp;
static GLuint texture;
static GLint uniform_texture;
static GLuint cursor_vbo;

static glm::vec3 position;
static glm::vec3 forward;
static glm::vec3 right;
static glm::vec3 up;
static glm::vec3 lookat;
static glm::vec3 angle;

static int ww, wh;
static int mx, my, mz;
static int face;
static uint8_t buildtype = 1;

static int now;
static unsigned int keys;

// Size of one chunk in blocks
#define CX 16
#define CY 32
#define CZ 16

// Number of chunks in the world
#define SCX 32
#define SCY 2
#define SCZ 32

// Sea level
#define SEALEVEL 4

// Number of VBO slots for chunks
#define CHUNKSLOTS (SCX * SCY * SCZ)

static const int transparent[16] = {2, 0, 0, 0, 1, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0}; 
static const char *blocknames[16] = {
	"air", "dirt", "topsoil", "grass", "leaves", "wood", "stone", "sand",
	"water", "glass", "brick", "ore", "woodrings", "white", "black", "x-y"
};

struct byte4 {
	uint8_t x, y, z, w;
	byte4() {}
	byte4(uint8_t x, uint8_t y, uint8_t z, uint8_t w): x(x), y(y), z(z), w(w) {}
};

static struct chunk *chunk_slot[CHUNKSLOTS] = {0};

struct chunk {
	uint8_t blk[CX][CY][CZ];
	struct chunk *left, *right, *below, *above, *front, *back;
	int slot;
	GLuint vbo;
	int elements;
	time_t lastused;
	bool changed;
	bool noised;
	bool initialized;
	int ax;
	int ay;
	int az;

	chunk(): ax(0), ay(0), az(0) {
		memset(blk, 0, sizeof blk);
		left = right = below = above = front = back = 0;
		lastused = now;
		slot = 0;
		changed = true;
		initialized = false;
		noised = false;
	}

	chunk(int x, int y, int z): ax(x), ay(y), az(z) {
		memset(blk, 0, sizeof blk);
		left = right = below = above = front = back = 0;
		lastused = now;
		slot = 0;
		changed = true;
		initialized = false;
		noised = false;
	}

	uint8_t get(int x, int y, int z) const {
		if(x < 0)
			return left ? left->blk[x + CX][y][z] : 0;
		if(x >= CX)
			return right ? right->blk[x - CX][y][z] : 0;
		if(y < 0)
			return below ? below->blk[x][y + CY][z] : 0;
		if(y >= CY)
			return above ? above->blk[x][y - CY][z] : 0;
		if(z < 0)
			return front ? front->blk[x][y][z + CZ] : 0;
		if(z >= CZ)
			return back ? back->blk[x][y][z - CZ] : 0;
		return blk[x][y][z];
	}

	bool isblocked(int x1, int y1, int z1, int x2, int y2, int z2) {
		// Invisible blocks are always "blocked"
		if(!blk[x1][y1][z1])
			return true;

		// Leaves do not block any other block, including themselves
		if(transparent[get(x2, y2, z2)] == 1)
			return false;

		// Non-transparent blocks always block line of sight
		if(!transparent[get(x2, y2, z2)])
			return true;

		// Otherwise, LOS is only blocked by blocks if the same transparency type
		return transparent[get(x2, y2, z2)] == transparent[blk[x1][y1][z1]];
	}

	void set(int x, int y, int z, uint8_t type) {
		// If coordinates are outside this chunk, find the right one.
		if(x < 0) {
			if(left)
				left->set(x + CX, y, z, type);
			return;
		}
		if(x >= CX) {
			if(right)
				right->set(x - CX, y, z, type);
			return;
		}
		if(y < 0) {
			if(below)
				below->set(x, y + CY, z, type);
			return;
		}
		if(y >= CY) {
			if(above)
				above->set(x, y - CY, z, type);
			return;
		}
		if(z < 0) {
			if(front)
				front->set(x, y, z + CZ, type);
			return;
		}
		if(z >= CZ) {
			if(back)
				back->set(x, y, z - CZ, type);
			return;
		}

		// Change the block
		blk[x][y][z] = type;
		changed = true;

		// When updating blocks at the edge of this chunk,
		// visibility of blocks in the neighbouring chunk might change.
		if(x == 0 && left)
			left->changed = true;
		if(x == CX - 1 && right)
			right->changed = true;
		if(y == 0 && below)
			below->changed = true;
		if(y == CY - 1 && above)
			above->changed = true;
		if(z == 0 && front)
			front->changed = true;
		if(z == CZ - 1 && back)
			back->changed = true;
	}

	static float noise2d(float x, float y, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;

		for(int i = 0; i < octaves; i++) {
			sum += strength * glm::simplex(glm::vec2(x, y) * scale);
			scale *= 2.0;
			strength *= persistence;
		}

		return sum;
	}

	static float noise3d_abs(float x, float y, float z, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;

		for(int i = 0; i < octaves; i++) {
			sum += strength * fabs(glm::simplex(glm::vec3(x, y, z) * scale));
			scale *= 2.0;
			strength *= persistence;
		}

		return sum;
	}

	void noise(int seed) {
		if(noised)
			return;
		else
			noised = true;

		for(int x = 0; x < CX; x++) {
			for(int z = 0; z < CZ; z++) {
				// Land height
				float n = noise2d((x + ax * CX) / 256.0, (z + az * CZ) / 256.0, seed, 5, 0.8) * 4;
				int h = n * 2;
				int y = 0;

				// Land blocks
				for(y = 0; y < CY; y++) {
					// Are we above "ground" level?
					if(y + ay * CY >= h) {
						// If we are not yet up to sea level, fill with water blocks
						if(y + ay * CY < SEALEVEL) {
							blk[x][y][z] = 8;
							continue;
						// Otherwise, we are in the air
						} else {
							// A tree!
							if(get(x, y - 1, z) == 3 && (rand() & 0xff) == 0) {
								// Trunk
								h = (rand() & 0x3) + 3;
								for(int i = 0; i < h; i++)
									set(x, y + i, z, 5);

								// Leaves
								for(int ix = -3; ix <= 3; ix++) { 
									for(int iy = -3; iy <= 3; iy++) { 
										for(int iz = -3; iz <= 3; iz++) { 
											if(ix * ix + iy * iy + iz * iz < 8 + (rand() & 1) && !get(x + ix, y + h + iy, z + iz))
												set(x + ix, y + h + iy, z + iz, 4);
										}
									}
								}
							}
							break;
						}
					}

					// Random value used to determine land type
					float r = noise3d_abs((x + ax * CX) / 16.0, (y + ay * CY) / 16.0, (z + az * CZ) / 16.0, -seed, 2, 1);

					// Sand layer
					if(n + r * 5 < 4)
						blk[x][y][z] = 7;
					// Dirt layer, but use grass blocks for the top
					else if(n + r * 5 < 8)
						blk[x][y][z] = (h < SEALEVEL || y + ay * CY < h - 1) ? 1 : 3;
					// Rock layer
					else if(r < 1.25)
						blk[x][y][z] = 6;
					// Sometimes, ores!
					else
						blk[x][y][z] = 11;
				}
			}
		}
		changed = true;
	}

	void update() {
		byte4 vertex[CX * CY * CZ * 18];
		int i = 0;
		int merged = 0;
		bool vis = false;;

		// View from negative x

		for(int x = CX - 1; x >= 0; x--) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					// Line of sight blocked?
					if(isblocked(x, y, z, x - 1, y, z)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];

					// Grass block has dirt sides and bottom
					if(top == 3) {
						bottom = 1;
						side = 2;
					// Wood blocks have rings on top and bottom
					} else if(top == 5) {
						top = bottom = 12;
					}

					// Same block as previous one? Extend it.
					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 5] = byte4(x, y, z + 1, side);
						vertex[i - 2] = byte4(x, y, z + 1, side);
						vertex[i - 1] = byte4(x, y + 1, z + 1, side);
						merged++;
					// Otherwise, add a new quad.
					} else {
						vertex[i++] = byte4(x, y, z, side);
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
					}
					
					vis = true;
				}
			}
		}

		// View from positive x

		for(int x = 0; x < CX; x++) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x + 1, y, z)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];

					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}

					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 4] = byte4(x + 1, y, z + 1, side);
						vertex[i - 2] = byte4(x + 1, y + 1, z + 1, side);
						vertex[i - 1] = byte4(x + 1, y, z + 1, side);
						merged++;
					} else {
						vertex[i++] = byte4(x + 1, y, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
					}
					vis = true;
				}
			}
		}

		// View from negative y

		for(int x = 0; x < CX; x++) {
			for(int y = CY - 1; y >= 0; y--) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x, y - 1, z)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];

					if(top == 3) {
						bottom = 1;
					} else if(top == 5) {
						top = bottom = 12;
					}

					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 4] = byte4(x, y, z + 1, bottom + 128);
						vertex[i - 2] = byte4(x + 1, y, z + 1, bottom + 128);
						vertex[i - 1] = byte4(x, y, z + 1, bottom + 128);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z, bottom + 128);
						vertex[i++] = byte4(x, y, z + 1, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z, bottom + 128);
						vertex[i++] = byte4(x + 1, y, z + 1, bottom + 128);
						vertex[i++] = byte4(x, y, z + 1, bottom + 128);
					}
					vis = true;
				}
			}
		}

		// View from positive y

		for(int x = 0; x < CX; x++) {
			for(int y = 0; y < CY; y++) {
				for(int z = 0; z < CZ; z++) {
					if(isblocked(x, y, z, x, y + 1, z)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];

					if(top == 3) {
						bottom = 1;
					} else if(top == 5) {
						top = bottom = 12;
					}

					if(vis && z != 0 && blk[x][y][z] == blk[x][y][z - 1]) {
						vertex[i - 5] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i - 2] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i - 1] = byte4(x + 1, y + 1, z + 1, top + 128);
						merged++;
					} else {
						vertex[i++] = byte4(x, y + 1, z, top + 128);
						vertex[i++] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z, top + 128);
						vertex[i++] = byte4(x, y + 1, z + 1, top + 128);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, top + 128);
					}
					vis = true;
				}
			}
		}

		// View from negative z

		for(int x = 0; x < CX; x++) {
			for(int z = CZ - 1; z >= 0; z--) {
				for(int y = 0; y < CY; y++) {
					if(isblocked(x, y, z, x, y, z - 1)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];

					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}

					if(vis && y != 0 && blk[x][y][z] == blk[x][y - 1][z]) {
						vertex[i - 5] = byte4(x, y + 1, z, side);
						vertex[i - 3] = byte4(x, y + 1, z, side);
						vertex[i - 2] = byte4(x + 1, y + 1, z, side);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z, side);
						vertex[i++] = byte4(x, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y + 1, z, side);
						vertex[i++] = byte4(x + 1, y, z, side);
					}
					vis = true;
				}
			}
		}

		// View from positive z

		for(int x = 0; x < CX; x++) {
			for(int z = 0; z < CZ; z++) {
				for(int y = 0; y < CY; y++) {
					if(isblocked(x, y, z, x, y, z + 1)) {
						vis = false;
						continue;
					}

					uint8_t top = blk[x][y][z];
					uint8_t bottom = blk[x][y][z];
					uint8_t side = blk[x][y][z];

					if(top == 3) {
						bottom = 1;
						side = 2;
					} else if(top == 5) {
						top = bottom = 12;
					}

					if(vis && y != 0 && blk[x][y][z] == blk[x][y - 1][z]) {
						vertex[i - 4] = byte4(x, y + 1, z + 1, side);
						vertex[i - 3] = byte4(x, y + 1, z + 1, side);
						vertex[i - 1] = byte4(x + 1, y + 1, z + 1, side);
						merged++;
					} else {
						vertex[i++] = byte4(x, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
						vertex[i++] = byte4(x, y + 1, z + 1, side);
						vertex[i++] = byte4(x + 1, y, z + 1, side);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, side);
					}
					vis = true;
				}
			}
		}

		changed = false;
		elements = i;

		// If this chunk is empty, no need to allocate a chunk slot.
		if(!elements)
			return;

		// If we don't have an active slot, find one
		if(chunk_slot[slot] != this) {
			int lru = 0;
			for(int i = 0; i < CHUNKSLOTS; i++) {
				// If there is an empty slot, use it
				if(!chunk_slot[i]) {
					lru = i;
					break;
				}
				// Otherwise try to find the least recently used slot
				if(chunk_slot[i]->lastused < chunk_slot[lru]->lastused)
					lru = i;
			}

			// If the slot is empty, create a new VBO
			if(!chunk_slot[lru]) {
				glGenBuffers(1, &vbo);
			// Otherwise, steal it from the previous slot owner
			} else {
				vbo = chunk_slot[lru]->vbo;
				chunk_slot[lru]->changed = true;
			}

			slot = lru;
			chunk_slot[slot] = this;
		}

		// Upload vertices

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, i * sizeof *vertex, vertex, GL_STATIC_DRAW);
	}

	void render() {
		if(changed)
			update();

		lastused = now;

		if(!elements)
			return;

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(attribute_coord, 4, GL_BYTE, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, elements);
	}
};

struct superchunk {
	chunk *c[SCX][SCY][SCZ];
	time_t seed;

	superchunk() {
		seed = time(NULL);
		for(int x = 0; x < SCX; x++)
			for(int y = 0; y < SCY; y++)
				for(int z = 0; z < SCZ; z++)
					c[x][y][z] = new chunk(x - SCX / 2, y - SCY / 2, z - SCZ / 2);

		for(int x = 0; x < SCX; x++)
			for(int y = 0; y < SCY; y++)
				for(int z = 0; z < SCZ; z++) {
					if(x > 0)
						c[x][y][z]->left = c[x - 1][y][z];
					if(x < SCX - 1)
						c[x][y][z]->right = c[x + 1][y][z];
					if(y > 0)
						c[x][y][z]->below = c[x][y - 1][z];
					if(y < SCY - 1)
						c[x][y][z]->above = c[x][y + 1][z];
					if(z > 0)
						c[x][y][z]->front = c[x][y][z - 1];
					if(z < SCZ - 1)
						c[x][y][z]->back = c[x][y][z + 1];
				}
	}

	uint8_t get(int x, int y, int z) const {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;

		if(cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ)
			return 0;

		return c[cx][cy][cz]->get(x & (CX - 1), y & (CY - 1), z & (CZ - 1));
	}

	void set(int x, int y, int z, uint8_t type) {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;

		if(cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ)
			return;

		c[cx][cy][cz]->set(x & (CX - 1), y & (CY - 1), z & (CZ - 1), type);
	}

	void render(const glm::mat4 &pv) {
		float ud = 1.0 / 0.0;
		int ux = -1;
		int uy = -1;
		int uz = -1;

		for(int x = 0; x < SCX; x++) {
			for(int y = 0; y < SCY; y++) {
				for(int z = 0; z < SCZ; z++) {
					glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(c[x][y][z]->ax * CX, c[x][y][z]->ay * CY, c[x][y][z]->az * CZ));
					glm::mat4 mvp = pv * model;

					// Is this chunk on the screen?
					glm::vec4 center = mvp * glm::vec4(CX / 2, CY / 2, CZ / 2, 1);

					float d = glm::length(center);
					center.x /= center.w;
					center.y /= center.w;

					// If it is behind the camera, don't bother drawing it
					if(center.z < -CY / 2)
						continue;

					// If it is outside the screen, don't bother drawing it
					if(fabsf(center.x) > 1 + fabsf(CY * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CY * 2 / center.w))
						continue;

					// If this chunk is not initialized, skip it
					if(!c[x][y][z]->initialized) {
						// But if it is the closest to the camera, mark it for initialization
						if(ux < 0 || d < ud) {
							ud = d;
							ux = x;
							uy = y;
							uz = z;
						}
						continue;
					}

					glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

					c[x][y][z]->render();
				}
			}
		}

		if(ux >= 0) {
			c[ux][uy][uz]->noise(seed);
			if(c[ux][uy][uz]->left)
				c[ux][uy][uz]->left->noise(seed);
			if(c[ux][uy][uz]->right)
				c[ux][uy][uz]->right->noise(seed);
			if(c[ux][uy][uz]->below)
				c[ux][uy][uz]->below->noise(seed);
			if(c[ux][uy][uz]->above)
				c[ux][uy][uz]->above->noise(seed);
			if(c[ux][uy][uz]->front)
				c[ux][uy][uz]->front->noise(seed);
			if(c[ux][uy][uz]->back)
				c[ux][uy][uz]->back->noise(seed);
			c[ux][uy][uz]->initialized = true;
		}
	}
};

static superchunk *world;

// Calculate the forward, right and lookat vectors from the angle vector
static void update_vectors() {
	forward.x = sinf(angle.x);
	forward.y = 0;
	forward.z = cosf(angle.x);

	right.x = -cosf(angle.x);
	right.y = 0;
	right.z = sinf(angle.x);

	lookat.x = sinf(angle.x) * cosf(angle.y);
	lookat.y = sinf(angle.y);
	lookat.z = cosf(angle.x) * cosf(angle.y);

	up = glm::cross(right, lookat);
}

static int init_resources() {
	/* Create shaders */

	program = create_program("glescraft.v.glsl", "glescraft.f.glsl");

	if(program == 0)
		return 0;

	attribute_coord = get_attrib(program, "coord");
	uniform_mvp = get_uniform(program, "mvp");

	if(attribute_coord == -1 || uniform_mvp == -1)
		return 0;

	/* Create and upload the texture */

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures.width, textures.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures.pixel_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	/* Create the world */

	world = new superchunk;

	position = glm::vec3(0, CY + 1, 0);
	angle = glm::vec3(0, -0.5, 0);
	update_vectors();

	/* Create a VBO for the cursor */

	glGenBuffers(1, &cursor_vbo);

	/* OpenGL settings that do not change while running this program */

	glUseProgram(program);
	glUniform1i(uniform_texture, 0);
	glClearColor(0.6, 0.8, 1.0, 0.0);
	glEnable(GL_CULL_FACE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use GL_NEAREST_MIPMAP_LINEAR if you want to use mipmaps
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glPolygonOffset(1, 1);

	glEnableVertexAttribArray(attribute_coord);

	return 1;
}

static void reshape(int w, int h) {
	ww = w;
	wh = h;
	glViewport(0, 0, w, h);
}

static void render() {
	glm::mat4 view = glm::lookAt(position, position + lookat, up);
	glm::mat4 projection = glm::perspective(45.0f, 1.0f*ww/wh, 0.01f, 1000.0f);

	glm::mat4 mvp = projection * view;

	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);

	/* Then draw chunks */

	world->render(mvp);

	/* At which voxel are we looking? */
	/* Very naive ray casting algorithm to find out which block we are looking at */

	glm::vec3 testpos = position;
	glm::vec3 prevpos = position;

	for(int i = 0; i < 100; i++) {
		/* Advance from our currect position to the direction we are looking at, in small steps */

		prevpos = testpos;
		testpos += lookat * 0.1f;

		mx = floorf(testpos.x);
		my = floorf(testpos.y);
		mz = floorf(testpos.z);

		/* If we find a block that is not air, we are done */

		if(world->get(mx, my, mz))
			break;
	}

	/* Find out which face of the block we are looking at */

	int px = floorf(prevpos.x);
	int py = floorf(prevpos.y);
	int pz = floorf(prevpos.z);

	if(px > mx)
		face = 0;
	else if(px < mx)
		face = 3;
	else if(py > my)
		face = 1;
	else if(py < my)
		face = 4;
	else if(pz > mz)
		face = 2;
	else if(pz < mz)
		face = 5;

	/* If we are looking at air, move the cursor out of sight */

	if(!world->get(mx, my, mz))
		mx = my = mz = 99999;

	float bx = mx;
	float by = my;
	float bz = mz;

	/* Render a box around the block we are pointing at */

	float box[24][4] = {
		{bx + 0, by + 0, bz + 0, 14},
		{bx + 1, by + 0, bz + 0, 14},
		{bx + 0, by + 1, bz + 0, 14},
		{bx + 1, by + 1, bz + 0, 14},
		{bx + 0, by + 0, bz + 1, 14},
		{bx + 1, by + 0, bz + 1, 14},
		{bx + 0, by + 1, bz + 1, 14},
		{bx + 1, by + 1, bz + 1, 14},

		{bx + 0, by + 0, bz + 0, 14},
		{bx + 0, by + 1, bz + 0, 14},
		{bx + 1, by + 0, bz + 0, 14},
		{bx + 1, by + 1, bz + 0, 14},
		{bx + 0, by + 0, bz + 1, 14},
		{bx + 0, by + 1, bz + 1, 14},
		{bx + 1, by + 0, bz + 1, 14},
		{bx + 1, by + 1, bz + 1, 14},

		{bx + 0, by + 0, bz + 0, 14},
		{bx + 0, by + 0, bz + 1, 14},
		{bx + 1, by + 0, bz + 0, 14},
		{bx + 1, by + 0, bz + 1, 14},
		{bx + 0, by + 1, bz + 0, 14},
		{bx + 0, by + 1, bz + 1, 14},
		{bx + 1, by + 1, bz + 0, 14},
		{bx + 1, by + 1, bz + 1, 14},
	};

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINES, 0, 24);

	/* Draw a cross in the center of the screen */

	float cross[4][4] = {
		{-0.05, 0, 0, 13},
		{+0.05, 0, 0, 13},
		{0, -0.05, 0, 13},
		{0, +0.05, 0, 13},
	};

	glDisable(GL_DEPTH_TEST);
	glm::mat4 one(1);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(one));
	glBufferData(GL_ARRAY_BUFFER, sizeof cross, cross, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINES, 0, 4);

	/* And we are done */
}

static void keyDown(SDL_KeyboardEvent *ev) {
	switch(ev->keysym.scancode) {
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_A:
			keys |= 1;
			break;
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_D:
			keys |= 2;
			break;
		case SDL_SCANCODE_UP:
		case SDL_SCANCODE_W:
			keys |= 4;
			break;
		case SDL_SCANCODE_DOWN:
		case SDL_SCANCODE_S:
			keys |= 8;
			break;
		case SDL_SCANCODE_PAGEUP:
		case SDL_SCANCODE_SPACE:
			keys |= 16;
			break;
		case SDL_SCANCODE_PAGEDOWN:
		case SDL_SCANCODE_C:
			keys |= 32;
			break;
		case SDL_SCANCODE_HOME:
			position = glm::vec3(0, CY + 1, 0);
			angle = glm::vec3(0, -0.5, 0);
			update_vectors();
			break;
		case SDL_SCANCODE_END:
			position = glm::vec3(0, CX * SCX, 0);
			angle = glm::vec3(0, -M_PI * 0.49, 0);
			update_vectors();
			break;
		default:
			break;
	}
}

static void keyUp(SDL_KeyboardEvent *ev) {
	switch(ev->keysym.scancode) {
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_A:
			keys &= ~1;
			break;
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_D:
			keys &= ~2;
			break;
		case SDL_SCANCODE_UP:
		case SDL_SCANCODE_W:
			keys &= ~4;
			break;
		case SDL_SCANCODE_DOWN:
		case SDL_SCANCODE_S:
			keys &= ~8;
			break;
		case SDL_SCANCODE_PAGEUP:
		case SDL_SCANCODE_SPACE:
			keys &= ~16;
			break;
		case SDL_SCANCODE_PAGEDOWN:
		case SDL_SCANCODE_C:
			keys &= ~32;
			break;
		default:
			break;
	}
}

static void physics() {
	static int prev = 0;
	static const float movespeed = 10;

	now = SDL_GetTicks();
	float dt = (now - prev) * 1.0e-3;
	prev = now;
	
	if(keys & 1)
		position -= right * movespeed * dt;
	if(keys & 2)
		position += right * movespeed * dt;
	if(keys & 4)
		position += forward * movespeed * dt;
	if(keys & 8)
		position -= forward * movespeed * dt;
	if(keys & 16)
		position.y += movespeed * dt;
	if(keys & 32)
		position.y -= movespeed * dt;
}

static void mouseMotion(SDL_MouseMotionEvent *ev) {
	static const float mousespeed = 0.001;

	angle.x -= ev->xrel * mousespeed;
	angle.y -= ev->yrel * mousespeed;

	if(angle.x < -M_PI)
		angle.x += M_PI * 2;
	if(angle.x > M_PI)
		angle.x -= M_PI * 2;
	if(angle.y < -M_PI / 2)
		angle.y = -M_PI / 2;
	if(angle.y > M_PI / 2)
		angle.y = M_PI / 2;

	update_vectors();
}

static void mouseWheel(SDL_MouseWheelEvent *ev) {
	if(ev->y) {
		if(ev->y > 0)
			buildtype--;
		else
			buildtype++;

		buildtype &= 0xf;
		fprintf(stderr, "Building blocks of type %u (%s)\n", buildtype, blocknames[buildtype]);
		return;
	}
}

static void mouseButtonDown(SDL_MouseButtonEvent *ev) {
	fprintf(stderr, "Clicked on %d, %d, %d, face %d, button %d\n", mx, my, mz, face, ev->button);

	if(ev->button == SDL_BUTTON_LEFT) {
		if(face == 0)
			mx++;
		if(face == 3)
			mx--;
		if(face == 1)
			my++;
		if(face == 4)
			my--;
		if(face == 2)
			mz++;
		if(face == 5)
			mz--;
		world->set(mx, my, mz, buildtype);
	} else if(ev->button == SDL_BUTTON_RIGHT) {
		world->set(mx, my, mz, 0);
	}
}

static void windowEvent(SDL_WindowEvent *ev) {
	switch(ev->event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			reshape(ev->data1, ev->data2);
			break;
		default:
			break;
	}
}

static void free_resources() {
	glDeleteProgram(program);
}

static void mainLoop(SDL_Window *window) {
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				keyDown(&ev.key);
				break;
			case SDL_KEYUP:
				keyUp(&ev.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouseButtonDown(&ev.button);
				break;
			case SDL_MOUSEWHEEL:
				mouseWheel(&ev.wheel);
				break;
			case SDL_MOUSEMOTION:
				mouseMotion(&ev.motion);
				break;
			case SDL_WINDOWEVENT:
				windowEvent(&ev.window);
				break;
			default:
				break;
			}
		}

		physics();
		render();
		SDL_GL_SwapWindow(window);
	}
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	// Select an OpenGL ES 2.0 profile.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_Window *window = SDL_CreateWindow("GLEScraft",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 480,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	// Set relative mouse mode, this will grab the cursor.
	SDL_SetRelativeMouseMode(SDL_TRUE);

	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	reshape(640, 480);

	print_opengl_info();
	printf("Use the mouse to look around.\n");
	printf("Use cursor keys, pageup and pagedown to move around.\n");
	printf("Use home and end to go to two predetermined positions.\n");
	printf("Press the left mouse button to build a block.\n");
	printf("Press the right mouse button to remove a block.\n");
	printf("Use the scrollwheel to select different types of blocks.\n");

	if (!init_resources())
		return EXIT_FAILURE;

	mainLoop(window);

	free_resources();
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
