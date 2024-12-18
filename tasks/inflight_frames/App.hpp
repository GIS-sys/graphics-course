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
#include "shaders/UniformParams.h"

#define INFLIGHT_FRAMES_AMOUNT 2


class App
{
public:
  App();
  ~App();

  void run();

private:
  void drawFrame();
  void update();

private:
  OsWindowingManager windowing;
  std::unique_ptr<OsWindow> osWindow;

  glm::uvec2 resolution;
  bool useVsync;

  std::unique_ptr<etna::Window> vkWindow;
  std::unique_ptr<etna::PerFrameCmdMgr> commandManager;

  etna::GraphicsPipeline pipeline;
  etna::GraphicsPipeline computePipeline;
  etna::Image image;
  etna::Image computeImage;
  etna::Sampler computeSampler;

  //UniformParams uniformParams[INFLIGHT_FRAMES_AMOUNT];
  etna::Buffer constants[INFLIGHT_FRAMES_AMOUNT];
  int step = 0;
};
