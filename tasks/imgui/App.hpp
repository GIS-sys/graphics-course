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


class ImGuiRenderer;


struct Constants
{
  glm::uvec2 res;
  glm::uvec2 cursor;
  float time;
  int objectsAmount;
};


class App
{
public:
  App();
  ~App();

  void run();

private:
  void drawFrame();

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
};
