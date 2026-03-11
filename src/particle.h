#pragma once

#include <cstdint>

// Matches GPU layout: 32 bytes, naturally aligned
struct Particle {
  float pos[2];
  float vel[2];
  float mass;
  float radius;
  float _pad[2];
};
static_assert(sizeof(Particle) == 32,
              "Particle must be 32 bytes for GPU alignment");
