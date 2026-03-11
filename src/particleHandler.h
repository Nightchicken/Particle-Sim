#pragma once

#include "particle.h"
#include <cstddef>
#include <cstdint>

// Arena Allocator
struct ArenaAllocator {
  uint8_t *buffer = nullptr;
  size_t capacity = 0;
  size_t offset = 0;

  void create(size_t cap);
  void *alloc(size_t size, size_t alignment = alignof(float));
  void reset();
  void destroy();
};

// Quadtree
struct AABB {
  float cx, cy; // center
  float hw, hh; // half-width, half-height

  bool contains(float px, float py) const;
  bool intersects(const AABB &other) const;
};

static constexpr int QT_MAX_PARTICLES = 8;
static constexpr int QT_MAX_DEPTH = 12;

struct QTNode {
  AABB bounds;
  uint32_t particleIndices[QT_MAX_PARTICLES];
  uint32_t count = 0;
  bool divided = false;
  QTNode *children[4] = {}; // NW, NE, SW, SE

  // Center of mass for Barnes-Hut approximation
  float comX = 0.0f;
  float comY = 0.0f;
  float totalMass = 0.0f;
};

struct Quadtree {
  ArenaAllocator *arena = nullptr;
  QTNode *root = nullptr;
  Particle *particles = nullptr;

  void build(ArenaAllocator *arena, Particle *particles, uint32_t count,
             AABB bounds);
  void insert(QTNode *node, uint32_t index, int depth);
  void subdivide(QTNode *node);
  void computeCOM(QTNode *node);
  void calcForce(QTNode *node, uint32_t index, float G, float softening,
                 float theta, float *outFx, float *outFy);
};

struct ParticleHandler {
  ArenaAllocator arena;
  Particle *particles = nullptr;
  uint32_t count = 0;

  ArenaAllocator
      qtArena; // separate arena for quadtree nodes (reset each frame)
  Quadtree quadtree;

  float G = 0.5f;
  float dt = 0.005f;
  float softening = 0.5f;
  float theta = 0.7f; // Barnes-Hut opening angle

  void init(uint32_t numParticles, float spawnRadius);
  void update(); // rebuild quadtree, compute forces, integrate
  void destroy();
};
