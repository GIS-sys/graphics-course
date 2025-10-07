#pragma once

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
//#include <etna/ComputePipeline.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/BlockingTransferHelper.hpp>
#include <etna/Sampler.hpp>

#include "wsi/OsWindowingManager.hpp"
#include <chrono>
#include "ParticleSystem.hpp"


class ImGuiRenderer;


struct Constants
{
  glm::uvec2 res;
  glm::uvec2 cursor;
  float time;
  int objectsAmount;
  int mouseControlType;
};


class App
{
public:
  App();
  ~App();

  void run();

private:
  void initParticlePipeline(); // TODO
  void drawGui();
  void drawFrame();
  void specificDrawFrameMain(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView); // TODO
  void specificDrawFrameParticles(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView); // TODO

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
  int mouseControlType = 0;

  void InitEmitters();
  std::unique_ptr<ParticleSystem> particleSystem;
  ParticleEmitter::EmitterParams emitterParams;
  std::chrono::steady_clock::time_point lastFrameTime;
  glm::vec3 cameraPosition{0.0f, 0.0f, 3.0f};

  etna::GraphicsPipeline particlePipeline;
  etna::Buffer particleVertexBuffer;
  etna::Buffer particleInstanceBuffer;
  struct CameraData {
    glm::mat4 viewProj;
    glm::vec3 cameraPos;
    float padding;
  };
};
