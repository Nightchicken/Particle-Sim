#include "particleHandler.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

// --- Arena Allocator ---

void ArenaAllocator::create(size_t cap) {
  buffer = static_cast<uint8_t *>(malloc(cap));
  capacity = cap;
  offset = 0;
}

void *ArenaAllocator::alloc(size_t size, size_t alignment) {
  size_t aligned = (offset + alignment - 1) & ~(alignment - 1);
  if (aligned + size > capacity)
    return nullptr;
  void *ptr = buffer + aligned;
  offset = aligned + size;
  return ptr;
}

void ArenaAllocator::reset() { offset = 0; }

void ArenaAllocator::destroy() {
  free(buffer);
  buffer = nullptr;
  capacity = 0;
  offset = 0;
}


bool AABB::contains(float px, float py) const {
  return px >= cx - hw && px <= cx + hw && py >= cy - hh && py <= cy + hh;
}

bool AABB::intersects(const AABB &other) const {
  return !(other.cx - other.hw > cx + hw || other.cx + other.hw < cx - hw ||
           other.cy - other.hh > cy + hh || other.cy + other.hh < cy - hh);
}


void Quadtree::build(ArenaAllocator *a, Particle *p, uint32_t count,
                     AABB bounds) {
  arena = a;
  particles = p;
  arena->reset();

  root = static_cast<QTNode *>(arena->alloc(sizeof(QTNode), alignof(QTNode)));
  *root = {};
  root->bounds = bounds;

  for (uint32_t i = 0; i < count; i++) {
    if (bounds.contains(p[i].pos[0], p[i].pos[1])) {
      insert(root, i, 0);
    }
  }

  computeCOM(root);
}

void Quadtree::insert(QTNode *node, uint32_t index, int depth) {
  if (!node->bounds.contains(particles[index].pos[0], particles[index].pos[1]))
    return;

  if (!node->divided && node->count < QT_MAX_PARTICLES) {
    node->particleIndices[node->count++] = index;
    return;
  }

  if (!node->divided) {
    subdivide(node);

    // Re-insert existing particles into children
    for (uint32_t i = 0; i < node->count; i++) {
      for (int c = 0; c < 4; c++) {
        insert(node->children[c], node->particleIndices[i], depth + 1);
      }
    }
    node->count = 0;
  }

  if (depth < QT_MAX_DEPTH) {
    for (int c = 0; c < 4; c++) {
      insert(node->children[c], index, depth + 1);
    }
  }
}

void Quadtree::subdivide(QTNode *node) {
  float qw = node->bounds.hw * 0.5f;
  float qh = node->bounds.hh * 0.5f;
  float cx = node->bounds.cx;
  float cy = node->bounds.cy;

  AABB childBounds[4] = {
      {cx - qw, cy + qh, qw, qh}, // NW
      {cx + qw, cy + qh, qw, qh}, // NE
      {cx - qw, cy - qh, qw, qh}, // SW
      {cx + qw, cy - qh, qw, qh}, // SE
  };

  for (int i = 0; i < 4; i++) {
    node->children[i] =
        static_cast<QTNode *>(arena->alloc(sizeof(QTNode), alignof(QTNode)));
    *node->children[i] = {};
    node->children[i]->bounds = childBounds[i];
  }
  node->divided = true;
}

void Quadtree::computeCOM(QTNode *node) {
  if (!node)
    return;

  node->comX = 0.0f;
  node->comY = 0.0f;
  node->totalMass = 0.0f;

  if (!node->divided) {
    for (uint32_t i = 0; i < node->count; i++) {
      Particle &p = particles[node->particleIndices[i]];
      node->comX += p.pos[0] * p.mass;
      node->comY += p.pos[1] * p.mass;
      node->totalMass += p.mass;
    }
    if (node->totalMass > 0.0f) {
      node->comX /= node->totalMass;
      node->comY /= node->totalMass;
    }
    return;
  }

  for (int c = 0; c < 4; c++) {
    computeCOM(node->children[c]);
    node->comX += node->children[c]->comX * node->children[c]->totalMass;
    node->comY += node->children[c]->comY * node->children[c]->totalMass;
    node->totalMass += node->children[c]->totalMass;
  }
  if (node->totalMass > 0.0f) {
    node->comX /= node->totalMass;
    node->comY /= node->totalMass;
  }
}

void Quadtree::calcForce(QTNode *node, uint32_t index, float G, float softening,
                         float theta, float *outFx, float *outFy) {
  if (!node || node->totalMass == 0.0f)
    return;

  float dx = node->comX - particles[index].pos[0];
  float dy = node->comY - particles[index].pos[1];
  float distSq = dx * dx + dy * dy + softening * softening;
  float size = node->bounds.hw * 2.0f;

  // If leaf node, compute direct forces
  if (!node->divided) {
    for (uint32_t i = 0; i < node->count; i++) {
      if (node->particleIndices[i] == index)
        continue;
      Particle &other = particles[node->particleIndices[i]];
      float pdx = other.pos[0] - particles[index].pos[0];
      float pdy = other.pos[1] - particles[index].pos[1];
      float pDistSq = pdx * pdx + pdy * pdy + softening * softening;
      float invDist = 1.0f / sqrtf(pDistSq);
      float invDist3 = invDist * invDist * invDist;
      *outFx += G * other.mass * pdx * invDist3;
      *outFy += G * other.mass * pdy * invDist3;
    }
    return;
  }

  // Barnes-Hut: if node is far enough, treat as single body
  if ((size * size) / distSq < theta * theta) {
    float invDist = 1.0f / sqrtf(distSq);
    float invDist3 = invDist * invDist * invDist;
    *outFx += G * node->totalMass * dx * invDist3;
    *outFy += G * node->totalMass * dy * invDist3;
    return;
  }

  // Otherwise recurse
  for (int c = 0; c < 4; c++) {
    calcForce(node->children[c], index, G, softening, theta, outFx, outFy);
  }
}

// --- Particle Handler ---

void ParticleHandler::init(uint32_t numParticles, float spawnRadius) {
  count = numParticles;

  // Areana Block ready for GPU
  arena.create(count * sizeof(Particle) + 64);
  particles = static_cast<Particle *>(
      arena.alloc(count * sizeof(Particle), alignof(Particle)));

  for (uint32_t i = 0; i < count; i++) {
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159265f;
    float r = spawnRadius * sqrtf(static_cast<float>(rand()) / RAND_MAX);

    particles[i].pos[0] = cosf(angle) * r;
    particles[i].pos[1] = sinf(angle) * r;

    float speed = 0.3f * sqrtf(spawnRadius / (r + 1.0f));
    particles[i].vel[0] = -sinf(angle) * speed;
    particles[i].vel[1] = cosf(angle) * speed;

    particles[i].mass = 1.0f;
    particles[i].radius = 0.2f;
    particles[i]._pad[0] = 0.0f;
    particles[i]._pad[1] = 0.0f;
  }

  // Quadtree arena: enough for worst-case node count, reset every frame
  size_t qtSize = sizeof(QTNode) * (count * 2 + 256);
  qtArena.create(qtSize);
}

void ParticleHandler::update() {
  // Find bounds
  float minX = particles[0].pos[0], maxX = minX;
  float minY = particles[0].pos[1], maxY = minY;
  for (uint32_t i = 1; i < count; i++) {
    if (particles[i].pos[0] < minX)
      minX = particles[i].pos[0];
    if (particles[i].pos[0] > maxX)
      maxX = particles[i].pos[0];
    if (particles[i].pos[1] < minY)
      minY = particles[i].pos[1];
    if (particles[i].pos[1] > maxY)
      maxY = particles[i].pos[1];
  }
  float cx = (minX + maxX) * 0.5f;
  float cy = (minY + maxY) * 0.5f;
  float hw = (maxX - minX) * 0.5f + 1.0f;
  float hh = (maxY - minY) * 0.5f + 1.0f;
  float halfSize = hw > hh ? hw : hh;

  AABB bounds = {cx, cy, halfSize, halfSize};
  quadtree.build(&qtArena, particles, count, bounds);

  // Compute forces and integrate (symplectic Euler)
  for (uint32_t i = 0; i < count; i++) {
    float fx = 0.0f, fy = 0.0f;
    quadtree.calcForce(quadtree.root, i, G, softening, theta, &fx, &fy);

    float ax = fx / particles[i].mass;
    float ay = fy / particles[i].mass;

    particles[i].vel[0] += ax * dt;
    particles[i].vel[1] += ay * dt;
    particles[i].pos[0] += particles[i].vel[0] * dt;
    particles[i].pos[1] += particles[i].vel[1] * dt;
  }
}

void ParticleHandler::destroy() {
  qtArena.destroy();
  arena.destroy();
  particles = nullptr;
  count = 0;
}
