# N-Body Particle Simulation

## Architecture

### Files
- **src/particle.h/cpp** — Particle struct (32-byte GPU-aligned layout)
- **src/particleHandler.h/cpp** — Arena allocator, Barnes-Hut quadtree, CPU physics
- **src/renderer.h/cpp** — WebGPU (Dawn) compute + render pipelines
- **src/main.cpp** — Window creation and main loop

### Arena Allocator
- **Particle storage** — one allocation for all particles, directly uploadable to GPU
- **Quadtree nodes** — separate arena, reset each frame (zero-cost deallocation)

### Quadtree (Barnes-Hut)
- Builds from scratch each frame using the quadtree arena
- Computes center-of-mass per node
- `theta` parameter controls accuracy vs speed
- Falls back to direct computation for nearby particles

### Rendering (WebGPU/Dawn)
- **Compute shader**: O(n²) brute-force GPU n-body (for comparison/GPU mode)
- **Render shader**: Reads particle storage buffer, draws as points colored by velocity
- **Double-buffered** particle SSBOs with ping-pong for compute passes
- Supports both CPU physics (quadtree) and GPU compute modes via `renderer.useGPUCompute`

## Building

```bash
git clone --depth 1 https://github.com/google/dawn.git  # if dawn/ dir is missing
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

First build is slow because of Dawn compilation. Subsequent builds are fast.


## Usage

```bash
./ParticleSim
```

### Configuration (in main.cpp)
- `NUM_PARTICLES` — particle count (default 4096)
- `SPAWN_RADIUS` — initial distribution radius
- `renderer.useGPUCompute` — `true` for GPU n-body, `false` for CPU Barnes-Hut

### Physics tuning (in particleHandler.h)
- `G` — gravitational constant
- `dt` — timestep
- `softening` — prevents singularities at close range
- `theta` — Barnes-Hut opening angle (lower = more accurate, slower)
