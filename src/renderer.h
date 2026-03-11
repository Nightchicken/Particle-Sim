#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

struct GLFWwindow;
struct ParticleHandler;

struct Renderer {
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;
  WGPUSurface surface = nullptr;
  WGPUTextureFormat surfaceFormat = WGPUTextureFormat_BGRA8Unorm;

  // Compute pipeline (GPU n-body)
  WGPUShaderModule computeShader = nullptr;
  WGPUComputePipeline computePipeline = nullptr;
  WGPUBindGroupLayout computeBGL = nullptr;
  WGPUBindGroup computeBindGroups[2] = {};
  WGPUBuffer particleBuffers[2] = {};
  WGPUBuffer simParamsBuffer = nullptr;

  // Render pipeline
  WGPUShaderModule renderShader = nullptr;
  WGPURenderPipeline renderPipeline = nullptr;
  WGPUBindGroupLayout renderBGL = nullptr;
  WGPUBindGroup renderBindGroups[2] = {};

  uint32_t particleCount = 0;
  uint32_t frame = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  bool useGPUCompute = true;

  bool init(GLFWwindow *window, ParticleHandler &handler);
  void
  uploadParticles(ParticleHandler &handler); // CPU -> GPU after CPU physics
  void dispatchCompute();                    // GPU n-body step
  void render();                             // draw particles
  void cleanup();
};
