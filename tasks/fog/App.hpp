#pragma once

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/BlockingTransferHelper.hpp>
#include <etna/Sampler.hpp>
#include <etna/Buffer.hpp>

#include "wsi/OsWindowingManager.hpp"
#include <chrono>
#include "ParticleSystem.hpp"

class ImGuiRenderer;

struct ParticleData {
    glm::vec3 position;
    float size;
    glm::vec4 color;
};

struct Constants
{
  glm::vec4 ambientLight;
  glm::uvec2 res;
  glm::uvec2 cursor;
  float time;
  float fogGeneralDensity;
  float diffuseVal;
  float specPow;
  float specVal;
  float fogWindStrength;
  int objectsAmount;
  int mouseControlType;
  int particleCount;
  int fogDivisions;
  int fogEnabled;
};

class App
{
public:
  App();
  ~App();

  void run();

private:
  void initParticlePipeline();
  void updateParticleBuffer(); // New method to update particle buffer
  void drawGui();
  void drawFrame();
  void specificDrawFrameMain(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView);
  void specificDrawFrameImGUI(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView);
  void specificDrawFrameParticles(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView);
  void updateCamera(float deltaTime);

private:
  OsWindowingManager windowing;
  std::unique_ptr<OsWindow> osWindow;

  std::unique_ptr<ImGuiRenderer> guiRenderer;

  glm::uvec2 resolution;
  bool useVsync;

  std::unique_ptr<etna::Window> vkWindow;
  std::unique_ptr<etna::PerFrameCmdMgr> commandManager;

  etna::GraphicsPipeline pipeline;
  etna::GraphicsPipeline computePipeline;
  etna::Image image;
  etna::Image computeImage;
  etna::Sampler sampler;
  etna::Sampler computeSampler;

  int objectsAmount = 5;
  int mouseControlType = 2;

  void InitEmitters();
  std::unique_ptr<ParticleSystem> particleSystem;
  ParticleEmitter::EmitterParams emitterParams;
  std::chrono::steady_clock::time_point lastFrameTime;
  glm::vec3 cameraPosition{0.0f, 0.0f, 3.0f};

  etna::GraphicsPipeline particlePipeline;

  // Add particle storage buffer
  etna::Buffer particleBuffer;
  std::vector<ParticleData> particleData;
  static constexpr int MAX_PARTICLES = 10000;

  etna::Image mainRenderImage;
  etna::Sampler mainRenderSampler;

  // fog
  float fogGeneralDensity = 0.4;
  int fogDivisions = 5;
  bool fogEnabled = true;

  // specs
  glm::vec3 ambientLight{0.2, 0.2, 0.2};
  float diffuseVal = 0.1;
  float specPow = 20.0;
  float specVal = 0.8;

  // Fog texture resources
  void initFogTexturePipeline();
  void updateFogTexture(vk::CommandBuffer& cmdBuf);

  etna::GraphicsPipeline fogTexturePipeline;
  etna::Image fogTextureImage;
  etna::Sampler fogTextureSampler;
  glm::uvec2 fogTextureResolution;
  float fogWindStrength = 5.0;
};

