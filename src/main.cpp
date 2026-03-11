#include "particleHandler.h"
#include "renderer.h"
#include <GLFW/glfw3.h>
#include <cstdio>

static constexpr uint32_t NUM_PARTICLES = 4096;
static constexpr uint32_t WIN_WIDTH = 800;
static constexpr uint32_t WIN_HEIGHT = 800;
static constexpr float SPAWN_RADIUS = 50.0f;

int main() {
  ParticleHandler handler;
  handler.init(NUM_PARTICLES, SPAWN_RADIUS);

  if (!glfwInit()) {
    fprintf(stderr, "Failed to init GLFW\n");
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window =
      glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "N-Body Sim", nullptr, nullptr);
  if (!window) {
    fprintf(stderr, "Failed to create window\n");
    glfwTerminate();
    return 1;
  }

  Renderer renderer;
  renderer.useGPUCompute = true;

  if (!renderer.init(window, handler)) {
    fprintf(stderr, "Failed to init renderer\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    handler.destroy();
    return 1;
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (renderer.useGPUCompute) {
      renderer.dispatchCompute();
    } else {
      handler.update();
      renderer.uploadParticles(handler);
    }

    renderer.render();
  }

  renderer.cleanup();
  glfwDestroyWindow(window);
  glfwTerminate();
  handler.destroy();
  return 0;
}
