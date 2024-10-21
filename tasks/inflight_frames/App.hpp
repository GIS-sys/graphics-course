#pragma once

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
#include <etna/ComputePipeline.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/BlockingTransferHelper.hpp>
#include <etna/Sampler.hpp>

#include "wsi/OsWindowingManager.hpp"


struct Constants
{
  glm::uvec2 res;
  glm::uvec2 cursor;
  double time;
};


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
  etna::ComputePipeline computePipeline;
  etna::Image image;
  etna::Image computeImage;
  etna::Sampler sampler;
  etna::Sampler computeSampler;
};
