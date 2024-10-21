#include "App.hpp"

#include <etna/Etna.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/Profiling.hpp>
#include <etna/RenderTargetStates.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <chrono>
#include <thread>
#include <iostream>


App::App()
  : resolution{1280, 720}
  , useVsync{false}
{
  // First, we need to initialize Vulkan, which is not trivial because
  // extensions are required for just about anything.
  {
    // GLFW tells us which extensions it needs to present frames to the OS window.
    // Actually rendering anything to a screen is optional in Vulkan, you can
    // alternatively save rendered frames into files, send them over network, etc.
    // Instance extensions do not depend on the actual GPU, only on the OS.
    auto glfwInstExts = windowing.getRequiredVulkanInstanceExtensions();

    std::vector<const char*> instanceExtensions{glfwInstExts.begin(), glfwInstExts.end()};

    // We also need the swapchain device extension to get access to the OS
    // window from inside of Vulkan on the GPU.
    // Device extensions require HW support from the GPU.
    // Generally, in Vulkan, we call the GPU a "device" and the CPU/OS combination a "host."
    std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Etna does all of the Vulkan initialization heavy lifting.
    // You can skip figuring out how it works for now.
    etna::initialize(etna::InitParams{
      .applicationName = "Local Shadertoy",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = instanceExtensions,
      .deviceExtensions = deviceExtensions,
      // Replace with an index if etna detects your preferred GPU incorrectly
      .physicalDeviceIndexOverride = {},
      .numFramesInFlight = 1,
    });
  }

  // Now we can create an OS window
  osWindow = windowing.createWindow(OsWindow::CreateInfo{
    .resolution = resolution,
  });

  // But we also need to hook the OS window up to Vulkan manually!
  {
    // First, we ask GLFW to provide a "surface" for the window,
    // which is an opaque description of the area where we can actually render.
    auto surface = osWindow->createVkSurface(etna::get_context().getInstance());

    // Then we pass it to Etna to do the complicated work for us
    vkWindow = etna::get_context().createWindow(etna::Window::CreateInfo{
      .surface = std::move(surface),
    });

    // And finally ask Etna to create the actual swapchain so that we can
    // get (different) images each frame to render stuff into.
    // Here, we do not support window resizing, so we only need to call this once.
    auto [w, h] = vkWindow->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });

    // Technically, Vulkan might fail to initialize a swapchain with the requested
    // resolution and pick a different one. This, however, does not occur on platforms
    // we support. Still, it's better to follow the "intended" path.
    resolution = {w, h};
  }

  // Next, we need a magical Etna helper to send commands to the GPU.
  // How it is actually performed is not trivial, but we can skip this for now.
  commandManager = etna::get_context().createPerFrameCmdMgr();


  // TODO: Initialize any additional resources you require here!

  etna::create_program("local_shadertoy2_texture", {INFLIGHT_FRAMES_SHADERS_ROOT "texture.comp.spv"});
  computePipeline = etna::get_context().getPipelineManager().createComputePipeline("local_shadertoy2_texture", {});
  computeImage = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "local_shadertoy2_image",
    .format = vk::Format::eR8G8B8A8Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
  });
  computeSampler = etna::Sampler(etna::Sampler::CreateInfo{.name = "computeSampler"});

  etna::create_program(
    "shader",
    {
      INFLIGHT_FRAMES_SHADERS_ROOT "toy.vert.spv",
      INFLIGHT_FRAMES_SHADERS_ROOT "toy.frag.spv"
    }
  );

  pipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vk::Format::eB8G8R8A8Srgb},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    }
  );


  int width;
  int height;
  int channels;
  unsigned char* loaded = stbi_load(
    INFLIGHT_FRAMES_SHADERS_ROOT "../../../../resources/textures/test_tex_1.png",
    &width,
    &height,
    &channels,
    4);

  image = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{(unsigned int)width, (unsigned int)height, 1},
    .name = "test_tex_1.png",
    .format = vk::Format::eR8G8B8A8Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst});

  sampler = etna::Sampler(etna::Sampler::CreateInfo{
    .addressMode = vk::SamplerAddressMode::eRepeat,
    .name = "sampler"}
  );

  etna::BlockingTransferHelper(etna::BlockingTransferHelper::CreateInfo{
      .stagingSize = static_cast<std::uint32_t>(width * height),
  }).uploadImage(
    *etna::get_context().createOneShotCmdMgr(),
    image,
    0, 0,
    std::span<const std::byte>(reinterpret_cast<const std::byte*>(loaded), width * height * 4)
  );



  constants = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = sizeof(UniformParams),
    .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "constants",
  });
  constants.map();
}

App::~App()
{
  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::run()
{
  while (!osWindow->isBeingClosed())
  {
    windowing.poll();

    update();
    drawFrame();

    FrameMark;
  }

  // We need to wait for the GPU to execute the last frame before destroying
  // all resources and closing the application.
  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::update() {
  ZoneScoped;
  uniformParams.res = glm::vec2{resolution.x, resolution.y};
  uniformParams.cursor = glm::vec2{osWindow->mouse.freePos.x, osWindow->mouse.freePos.y};
  uniformParams.time = (std::chrono::system_clock::now().time_since_epoch().count() % 1'000'000'000'000ll) / 1'000'000'000.0;
  std::memcpy(constants.data(), &uniformParams, sizeof(uniformParams));
}

void App::drawFrame()
{
  ZoneScoped;
  // First, get a command buffer to write GPU commands into.
  auto currentCmdBuf = commandManager->acquireNext();

  // Next, tell Etna that we are going to start processing the next frame.
  etna::begin_frame();

  // And now get the image we should be rendering the picture into.
  auto nextSwapchainImage = vkWindow->acquireNext();

  // When window is minimized, we can't render anything in Windows
  // because it kills the swapchain, so we skip frames in this case.
  if (nextSwapchainImage)
  {
    auto [backbuffer, backbufferView, backbufferAvailableSem] = *nextSwapchainImage;

    ETNA_CHECK_VK_RESULT(currentCmdBuf.begin(vk::CommandBufferBeginInfo{}));
    {
      // First of all, we need to "initialize" th "backbuffer", aka the current swapchain
      // image, into a state that is appropriate for us working with it. The initial state
      // is considered to be "undefined" (aka "I contain trash memory"), by the way.
      // "Transfer" in vulkanese means "copy or blit".
      // Note that Etna sometimes calls this for you to make life simpler, read Etna's code!
      etna::set_state(
        currentCmdBuf,
        backbuffer,
        // We are going to use the texture at the transfer stage...
        vk::PipelineStageFlagBits2::eTransfer,
        // ...to transfer-write stuff into it...
        vk::AccessFlagBits2::eTransferWrite,
        // ...and want it to have the appropriate layout.
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageAspectFlagBits::eColor);
      // The set_state doesn't actually record any commands, they are deferred to
      // the moment you call flush_barriers.
      // As with set_state, Etna sometimes flushes on it's own.
      // Usually, flushes should be placed before "action", i.e. compute dispatches
      // and blit/copy operations.
      etna::flush_barriers(currentCmdBuf);


      // TODO: Record your commands here!
      ETNA_PROFILE_GPU(currentCmdBuf, renderLocalShadertoy2);

      {
        ETNA_PROFILE_GPU(currentCmdBuf, renderTexture);
        auto simpleComputeInfo = etna::get_shader_program("local_shadertoy2_texture");
        auto set = etna::create_descriptor_set(
          simpleComputeInfo.getDescriptorLayoutId(0),
          currentCmdBuf,
          {
            etna::Binding{0, computeImage.genBinding(computeSampler.get(), vk::ImageLayout::eGeneral)},
            etna::Binding{1, constants.genBinding()}
          });
        vk::DescriptorSet vkSet = set.getVkSet();
        currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.getVkPipeline());
        currentCmdBuf.bindDescriptorSets(
          vk::PipelineBindPoint::eCompute, pipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, nullptr);
        /*currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(
          computePipeline.getVkPipelineLayout(),
          vk::ShaderStageFlagBits::eCompute,
          0,
          sizeof(constants),
          &constants
        );*/
        etna::flush_barriers(currentCmdBuf);
        currentCmdBuf.dispatch((resolution.x + 31) / 16, (resolution.y + 31) / 16, 1);
      }

      etna::set_state(
        currentCmdBuf,
        computeImage.get(),
        // We are going to use the texture at the transfer stage...
        vk::PipelineStageFlagBits2::eTransfer,
        // ...to transfer-read stuff from it...
        vk::AccessFlagBits2::eTransferRead,
        // ...and want it to have the appropriate layout.
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::flush_barriers(currentCmdBuf);

      std::this_thread::sleep_for(std::chrono::milliseconds(6));

      {
        ETNA_PROFILE_GPU(currentCmdBuf, renderShader);
        auto simpleMaterialInfo = etna::get_shader_program("shader");
        auto set2 = etna::create_descriptor_set(
          simpleMaterialInfo.getDescriptorLayoutId(0),
          currentCmdBuf,
          {
                etna::Binding{0, computeImage.genBinding(computeSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{1,
                  image.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{2, constants.genBinding()}
          });

        etna::RenderTargetState renderTargets{
          currentCmdBuf,
          {{0, 0}, {resolution.x, resolution.y}},
          {{.image = backbuffer, .view = backbufferView}},
          {}
        };

        vk::DescriptorSet vkSet2 = set2.getVkSet();
        currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
        currentCmdBuf.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          pipeline.getVkPipelineLayout(),
          0, 1,
          &vkSet2,
          0, 0);

        //currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

        currentCmdBuf.draw(3, 1, 0, 0);
      }



      // At the end of "rendering", we are required to change how the pixels of the
      // swpchain image are laid out in memory to something that is appropriate
      // for presenting to the window (while preserving the content of the pixels!).
      etna::set_state(
        currentCmdBuf,
        backbuffer,
        // This looks weird, but is correct. Ask about it later.
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageAspectFlagBits::eColor);
      // And of course flush the layout transition.
      etna::flush_barriers(currentCmdBuf);
      ETNA_READ_BACK_GPU_PROFILING(currentCmdBuf);
    }
    ETNA_CHECK_VK_RESULT(currentCmdBuf.end());

    // We are done recording GPU commands now and we can send them to be executed by the GPU.
    // Note that the GPU won't start executing our commands before the semaphore is
    // signalled, which will happen when the OS says that the next swapchain image is ready.
    auto renderingDone =
      commandManager->submit(std::move(currentCmdBuf), std::move(backbufferAvailableSem));

    // Finally, present the backbuffer the screen, but only after the GPU tells the OS
    // that it is done executing the command buffer via the renderingDone semaphore.
    const bool presented = vkWindow->present(std::move(renderingDone), backbufferView);

    if (!presented)
      nextSwapchainImage = std::nullopt;
  }

  etna::end_frame();

  // After a window us un-minimized, we need to restore the swapchain to continue rendering.
  if (!nextSwapchainImage && osWindow->getResolution() != glm::uvec2{0, 0})
  {
    auto [w, h] = vkWindow->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });
    ETNA_VERIFY((resolution == glm::uvec2{w, h}));
  }
}
