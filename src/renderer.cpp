#include "renderer.h"
#include "particleHandler.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <webgpu/webgpu_glfw.h>

// --- WGSL Shaders ---

static const char *computeShaderSrc = R"(
struct Particle {
    pos: vec2<f32>,
    vel: vec2<f32>,
    mass: f32,
    radius: f32,
    _pad1: f32,
    _pad2: f32,
};

struct SimParams {
    G: f32,
    dt: f32,
    numParticles: u32,
    softening: f32,
};

@group(0) @binding(0) var<uniform> params: SimParams;
@group(0) @binding(1) var<storage, read>       particlesIn:  array<Particle>;
@group(0) @binding(2) var<storage, read_write>  particlesOut: array<Particle>;

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let idx = gid.x;
    if (idx >= params.numParticles) { return; }

    var p = particlesIn[idx];
    var force = vec2<f32>(0.0, 0.0);

    for (var i = 0u; i < params.numParticles; i++) {
        if (i == idx) { continue; }
        let other = particlesIn[i];
        let diff = other.pos - p.pos;
        let distSq = dot(diff, diff) + params.softening * params.softening;
        let invDist = inverseSqrt(distSq);
        let invDist3 = invDist * invDist * invDist;
        force += diff * (params.G * other.mass * invDist3);
    }

    let accel = force / p.mass;
    p.vel += accel * params.dt;
    p.pos += p.vel * params.dt;
    particlesOut[idx] = p;
}
)";

static const char *renderShaderSrc = R"(
struct Particle {
    pos: vec2<f32>,
    vel: vec2<f32>,
    mass: f32,
    radius: f32,
    _pad1: f32,
    _pad2: f32,
};

struct VSOut {
    @builtin(position) pos: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@group(0) @binding(0) var<storage, read> particles: array<Particle>;

@vertex
fn vs(@builtin(vertex_index) vi: u32) -> VSOut {
    let p = particles[vi];
    var out: VSOut;
    out.pos = vec4<f32>(p.pos * 0.01, 0.0, 1.0);

    let speed = length(p.vel);
    let t = clamp(speed * 0.5, 0.0, 1.0);
    out.color = mix(vec3<f32>(0.2, 0.5, 1.0), vec3<f32>(1.0, 0.4, 0.1), t);
    return out;
}

@fragment
fn fs(in: VSOut) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}
)";

// Helper: blocking adapter/device request

struct AdapterUserData {
  WGPUAdapter adapter = nullptr;
  bool done = false;
};

struct DeviceUserData {
  WGPUDevice device = nullptr;
  bool done = false;
};

static void onAdapterReady(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                           WGPUStringView message, void *userdata1, void *) {
  auto *data = static_cast<AdapterUserData *>(userdata1);
  if (status == WGPURequestAdapterStatus_Success) {
    data->adapter = adapter;
  } else {
    fprintf(stderr, "Adapter request failed: %.*s\n", (int)message.length,
            message.data);
  }
  data->done = true;
}

static void onDeviceReady(WGPURequestDeviceStatus status, WGPUDevice device,
                          WGPUStringView message, void *userdata1, void *) {
  auto *data = static_cast<DeviceUserData *>(userdata1);
  if (status == WGPURequestDeviceStatus_Success) {
    data->device = device;
  } else {
    fprintf(stderr, "Device request failed: %.*s\n", (int)message.length,
            message.data);
  }
  data->done = true;
}

static void onDeviceError(WGPUDevice const *, WGPUErrorType type,
                          WGPUStringView message, void *, void *) {
  fprintf(stderr, "WebGPU device error (%d): %.*s\n", type, (int)message.length,
          message.data);
}

static void onDeviceLost(WGPUDevice const *, WGPUDeviceLostReason reason,
                         WGPUStringView message, void *, void *) {
  fprintf(stderr, "WebGPU device lost (%d): %.*s\n", reason,
          (int)message.length, message.data);
}

// Renderer

bool Renderer::init(GLFWwindow *window, ParticleHandler &handler) {
  particleCount = handler.count;
  glfwGetFramebufferSize(window, (int *)&width, (int *)&height);

  // Instance
  WGPUInstanceDescriptor instanceDesc = {};
  instance = wgpuCreateInstance(&instanceDesc);
  if (!instance) {
    fprintf(stderr, "Failed to create WebGPU instance\n");
    return false;
  }

  // Surface
  surface =
      wgpu::glfw::CreateSurfaceForWindow(instance, window).MoveToCHandle();
  if (!surface) {
    fprintf(stderr, "Failed to create surface\n");
    return false;
  }

  // Adapter (blocking)
  AdapterUserData adapterData;
  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.compatibleSurface = surface;
  WGPURequestAdapterCallbackInfo adapterCbInfo = {};
  adapterCbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
  adapterCbInfo.callback = onAdapterReady;
  adapterCbInfo.userdata1 = &adapterData;
  wgpuInstanceRequestAdapter(instance, &adapterOpts, adapterCbInfo);
  while (!adapterData.done) {
    wgpuInstanceProcessEvents(instance);
  }
  adapter = adapterData.adapter;
  if (!adapter)
    return false;

  // Device (blocking)
  DeviceUserData deviceData;
  WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
  deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
  deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
  WGPURequestDeviceCallbackInfo deviceCbInfo = {};
  deviceCbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
  deviceCbInfo.callback = onDeviceReady;
  deviceCbInfo.userdata1 = &deviceData;
  wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCbInfo);
  while (!deviceData.done) {
    wgpuInstanceProcessEvents(instance);
  }
  device = deviceData.device;
  if (!device)
    return false;
  queue = wgpuDeviceGetQueue(device);

  // Configure surface
  WGPUSurfaceCapabilities caps = {};
  wgpuSurfaceGetCapabilities(surface, adapter, &caps);
  surfaceFormat = caps.formats[0];

  WGPUSurfaceConfiguration surfConfig = {};
  surfConfig.device = device;
  surfConfig.format = surfaceFormat;
  surfConfig.usage = WGPUTextureUsage_RenderAttachment;
  surfConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
  surfConfig.width = width;
  surfConfig.height = height;
  surfConfig.presentMode = WGPUPresentMode_Fifo;
  wgpuSurfaceConfigure(surface, &surfConfig);
  wgpuSurfaceCapabilitiesFreeMembers(caps);

  // Buffers
  size_t bufSize = particleCount * sizeof(Particle);

  for (int i = 0; i < 2; i++) {
    WGPUBufferDescriptor bufDesc = {};
    bufDesc.size = bufSize;
    bufDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst |
                    WGPUBufferUsage_CopySrc;
    bufDesc.mappedAtCreation = false;
    particleBuffers[i] = wgpuDeviceCreateBuffer(device, &bufDesc);
  }

  // Upload initial particle data to buffer 0
  wgpuQueueWriteBuffer(queue, particleBuffers[0], 0, handler.particles,
                       bufSize);

  // Sim params uniform
  struct {
    float G;
    float dt;
    uint32_t n;
    float softening;
  } params;
  params.G = handler.G;
  params.dt = handler.dt;
  params.n = handler.count;
  params.softening = handler.softening;

  WGPUBufferDescriptor paramsDesc = {};
  paramsDesc.size = sizeof(params);
  paramsDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  simParamsBuffer = wgpuDeviceCreateBuffer(device, &paramsDesc);
  wgpuQueueWriteBuffer(queue, simParamsBuffer, 0, &params, sizeof(params));

  // Compute pipeline
  {
    WGPUShaderSourceWGSL wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code = {computeShaderSrc, strlen(computeShaderSrc)};
    WGPUShaderModuleDescriptor smDesc = {};
    smDesc.nextInChain = &wgslDesc.chain;
    computeShader = wgpuDeviceCreateShaderModule(device, &smDesc);

    // Bind group layout
    WGPUBindGroupLayoutEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Compute;
    entries[0].buffer.type = WGPUBufferBindingType_Uniform;
    entries[0].buffer.minBindingSize = sizeof(params);

    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Compute;
    entries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[1].buffer.minBindingSize = bufSize;

    entries[2].binding = 2;
    entries[2].visibility = WGPUShaderStage_Compute;
    entries[2].buffer.type = WGPUBufferBindingType_Storage;
    entries[2].buffer.minBindingSize = bufSize;

    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 3;
    bglDesc.entries = entries;
    computeBGL = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

    // Two bind groups for ping-pong
    for (int i = 0; i < 2; i++) {
      WGPUBindGroupEntry bgEntries[3] = {};
      bgEntries[0].binding = 0;
      bgEntries[0].buffer = simParamsBuffer;
      bgEntries[0].size = sizeof(params);

      bgEntries[1].binding = 1;
      bgEntries[1].buffer = particleBuffers[i];
      bgEntries[1].size = bufSize;

      bgEntries[2].binding = 2;
      bgEntries[2].buffer = particleBuffers[1 - i];
      bgEntries[2].size = bufSize;

      WGPUBindGroupDescriptor bgDesc = {};
      bgDesc.layout = computeBGL;
      bgDesc.entryCount = 3;
      bgDesc.entries = bgEntries;
      computeBindGroups[i] = wgpuDeviceCreateBindGroup(device, &bgDesc);
    }

    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &computeBGL;
    WGPUPipelineLayout pipelineLayout =
        wgpuDeviceCreatePipelineLayout(device, &plDesc);

    WGPUComputePipelineDescriptor cpDesc = {};
    cpDesc.layout = pipelineLayout;
    cpDesc.compute.module = computeShader;
    cpDesc.compute.entryPoint = {"main", 4};
    computePipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);

    wgpuPipelineLayoutRelease(pipelineLayout);
  }

  // Render pipeline
  {
    WGPUShaderSourceWGSL wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code = {renderShaderSrc, strlen(renderShaderSrc)};
    WGPUShaderModuleDescriptor smDesc = {};
    smDesc.nextInChain = &wgslDesc.chain;
    renderShader = wgpuDeviceCreateShaderModule(device, &smDesc);

    // Bind group layout: storage buffer for particles
    WGPUBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Vertex;
    entry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entry.buffer.minBindingSize = bufSize;

    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 1;
    bglDesc.entries = &entry;
    renderBGL = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

    // Two bind groups, one per buffer
    for (int i = 0; i < 2; i++) {
      WGPUBindGroupEntry bgEntry = {};
      bgEntry.binding = 0;
      bgEntry.buffer = particleBuffers[i];
      bgEntry.size = bufSize;

      WGPUBindGroupDescriptor bgDesc = {};
      bgDesc.layout = renderBGL;
      bgDesc.entryCount = 1;
      bgDesc.entries = &bgEntry;
      renderBindGroups[i] = wgpuDeviceCreateBindGroup(device, &bgDesc);
    }

    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &renderBGL;
    WGPUPipelineLayout pipelineLayout =
        wgpuDeviceCreatePipelineLayout(device, &plDesc);

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = surfaceFormat;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState = {};
    fragState.module = renderShader;
    fragState.entryPoint = {"fs", 2};
    fragState.targetCount = 1;
    fragState.targets = &colorTarget;

    WGPURenderPipelineDescriptor rpDesc = {};
    rpDesc.layout = pipelineLayout;
    rpDesc.vertex.module = renderShader;
    rpDesc.vertex.entryPoint = {"vs", 2};
    rpDesc.primitive.topology = WGPUPrimitiveTopology_PointList;
    rpDesc.fragment = &fragState;
    rpDesc.multisample.count = 1;
    rpDesc.multisample.mask = 0xFFFFFFFF;

    renderPipeline = wgpuDeviceCreateRenderPipeline(device, &rpDesc);
    wgpuPipelineLayoutRelease(pipelineLayout);
  }

  return true;
}

void Renderer::uploadParticles(ParticleHandler &handler) {
  uint32_t dst = frame % 2;
  wgpuQueueWriteBuffer(queue, particleBuffers[dst], 0, handler.particles,
                       handler.count * sizeof(Particle));
}

void Renderer::dispatchCompute() {
  uint32_t src = frame % 2;

  WGPUCommandEncoderDescriptor encDesc = {};
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

  WGPUComputePassDescriptor passDesc = {};
  WGPUComputePassEncoder pass =
      wgpuCommandEncoderBeginComputePass(encoder, &passDesc);
  wgpuComputePassEncoderSetPipeline(pass, computePipeline);
  wgpuComputePassEncoderSetBindGroup(pass, 0, computeBindGroups[src], 0,
                                     nullptr);
  uint32_t workgroups = (particleCount + 255) / 256;
  wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);
  wgpuComputePassEncoderEnd(pass);
  wgpuComputePassEncoderRelease(pass);

  WGPUCommandBufferDescriptor cmdDesc = {};
  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdDesc);
  wgpuQueueSubmit(queue, 1, &cmd);

  wgpuCommandBufferRelease(cmd);
  wgpuCommandEncoderRelease(encoder);
}

void Renderer::render() {
  // The buffer to read from is the one just written to
  uint32_t readBuf = useGPUCompute ? (1 - frame % 2) : (frame % 2);

  WGPUSurfaceTexture surfTex;
  wgpuSurfaceGetCurrentTexture(surface, &surfTex);
  if (surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
    return;

  WGPUTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  WGPUTextureView view = wgpuTextureCreateView(surfTex.texture, &viewDesc);

  WGPUCommandEncoderDescriptor encDesc = {};
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

  WGPURenderPassColorAttachment colorAtt = {};
  colorAtt.view = view;
  colorAtt.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
  colorAtt.loadOp = WGPULoadOp_Clear;
  colorAtt.storeOp = WGPUStoreOp_Store;
  colorAtt.clearValue = {0.0, 0.0, 0.02, 1.0};

  WGPURenderPassDescriptor rpDesc = {};
  rpDesc.colorAttachmentCount = 1;
  rpDesc.colorAttachments = &colorAtt;

  WGPURenderPassEncoder pass =
      wgpuCommandEncoderBeginRenderPass(encoder, &rpDesc);
  wgpuRenderPassEncoderSetPipeline(pass, renderPipeline);
  wgpuRenderPassEncoderSetBindGroup(pass, 0, renderBindGroups[readBuf], 0,
                                    nullptr);
  wgpuRenderPassEncoderDraw(pass, particleCount, 1, 0, 0);
  wgpuRenderPassEncoderEnd(pass);
  wgpuRenderPassEncoderRelease(pass);

  WGPUCommandBufferDescriptor cmdDesc = {};
  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdDesc);
  wgpuQueueSubmit(queue, 1, &cmd);

  wgpuCommandBufferRelease(cmd);
  wgpuCommandEncoderRelease(encoder);
  wgpuTextureViewRelease(view);
  wgpuTextureRelease(surfTex.texture);
  wgpuSurfacePresent(surface);

  frame++;
}

void Renderer::cleanup() {
  if (computePipeline)
    wgpuComputePipelineRelease(computePipeline);
  if (computeShader)
    wgpuShaderModuleRelease(computeShader);
  if (computeBGL)
    wgpuBindGroupLayoutRelease(computeBGL);
  for (int i = 0; i < 2; i++) {
    if (computeBindGroups[i])
      wgpuBindGroupRelease(computeBindGroups[i]);
    if (particleBuffers[i])
      wgpuBufferRelease(particleBuffers[i]);
    if (renderBindGroups[i])
      wgpuBindGroupRelease(renderBindGroups[i]);
  }
  if (simParamsBuffer)
    wgpuBufferRelease(simParamsBuffer);
  if (renderPipeline)
    wgpuRenderPipelineRelease(renderPipeline);
  if (renderShader)
    wgpuShaderModuleRelease(renderShader);
  if (renderBGL)
    wgpuBindGroupLayoutRelease(renderBGL);
  if (surface)
    wgpuSurfaceRelease(surface);
  if (queue)
    wgpuQueueRelease(queue);
  if (device)
    wgpuDeviceRelease(device);
  if (adapter)
    wgpuAdapterRelease(adapter);
  if (instance)
    wgpuInstanceRelease(instance);
}
