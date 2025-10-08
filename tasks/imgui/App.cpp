#include "App.hpp"

#include <etna/Etna.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/Profiling.hpp>
#include <etna/RenderTargetStates.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <chrono>

#include <imgui.h>
#include "gui/ImGuiRenderer.hpp"


App::App()
  : resolution{1280, 720}
  , useVsync{true}
{
  // Create window
  osWindow = windowing.createWindow(OsWindow::CreateInfo{
    .resolution = resolution,
  });

  // Init Vulkan
  {
    auto glfwInstExts = windowing.getRequiredVulkanInstanceExtensions();
    std::vector<const char*> instanceExtensions{glfwInstExts.begin(), glfwInstExts.end()};
    std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    etna::initialize(etna::InitParams{
      .applicationName = "Local Shadertoy",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = instanceExtensions,
      .deviceExtensions = deviceExtensions,
      .features = vk::PhysicalDeviceFeatures2{.features = {}},
      .physicalDeviceIndexOverride = {},
      .numFramesInFlight = 1,
    });
  }

  // Init window
  {
    auto surface = osWindow->createVkSurface(etna::get_context().getInstance());
    vkWindow = etna::get_context().createWindow(etna::Window::CreateInfo{
      .surface = std::move(surface),
    });
    auto [w, h] = vkWindow->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });
    resolution = {w, h};
    guiRenderer = std::make_unique<ImGuiRenderer>(vkWindow->getCurrentFormat());
  }

  // Init shaders
  commandManager = etna::get_context().createPerFrameCmdMgr();
  etna::create_program("local_shadertoy2_texture",
    {
      IMGUI_SHADERS_ROOT "toy.vert.spv",
      IMGUI_SHADERS_ROOT "texture.frag.spv",
    }
  );
  computePipeline = etna::get_context().getPipelineManager().createGraphicsPipeline("local_shadertoy2_texture", etna::GraphicsPipeline::CreateInfo {
      .fragmentShaderOutput = {
        .colorAttachmentFormats = {vkWindow->getCurrentFormat()}
      }
    });
  computeImage = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "local_shadertoy2_image",
    .format = vkWindow->getCurrentFormat(),//vk::Format::eB8G8R8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
  });
  computeSampler = etna::Sampler(etna::Sampler::CreateInfo{.addressMode = vk::SamplerAddressMode::eMirroredRepeat, .name = "computeSampler"});

  etna::create_program(
    "shader",
    {
      IMGUI_SHADERS_ROOT "toy.vert.spv",
      IMGUI_SHADERS_ROOT "toy.frag.spv"
    }
  );
  pipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vkWindow->getCurrentFormat()}, //{vk::Format::eB8G8R8A8Srgb},
        },
    }
  );

  int width;
  int height;
  int channels;
  unsigned char* loaded = stbi_load(
    IMGUI_SHADERS_ROOT "../../../../resources/textures/test_tex_1.png",
    &width,
    &height,
    &channels,
    STBI_rgb_alpha);
  channels = 4; // because we are using STBI flag

  image = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{(unsigned int)width, (unsigned int)height, 1},
    .name = "test_tex_1.png",
    .format = vkWindow->getCurrentFormat(), //vk::Format::eR8G8B8A8Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst});

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
    std::span<const std::byte>(reinterpret_cast<const std::byte*>(loaded), width * height * channels)
  );

  // Init ImGUI
  ImGuiRenderer::enableImGuiForWindow(osWindow->native());
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
    drawFrame();
  }

  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::drawFrame()
{
  {
    ZoneScopedN("drawGui");
    guiRenderer->nextFrame();
    ImGui::NewFrame();
    ImGui::Begin("Simple render settings");

    //float color[3]{uniformParams.baseColor.r, uniformParams.baseColor.g, uniformParams.baseColor.b};
    //ImGui::ColorEdit3(
    //  "Meshes base color", color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs);
    //uniformParams.baseColor = {color[0], color[1], color[2]};

    //float pos[3]{uniformParams.lightPos.x, uniformParams.lightPos.y, uniformParams.lightPos.z};
    //ImGui::SliderFloat3("Light source position", pos, -10.f, 10.f);
    //uniformParams.lightPos = {pos[0], pos[1], pos[2]};

    ImGui::Text(
      "Application average %.3f ms/frame (%.1f FPS)",
      1000.0f / ImGui::GetIO().Framerate,
      ImGui::GetIO().Framerate);

    ImGui::SliderInt("Objects Amount", &objectsAmount, 0, 256);

    static const char* items[] {
        "Nothing",
        "Orbit",
        "Look around",
    };
    ImGui::Combo("Mouse Control Type", &mouseControlType, items, IM_ARRAYSIZE(items));

    ImGui::NewLine();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press 'B' to recompile and reload shaders");
    ImGui::End();
    ImGui::Render();
  }

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

      glm::uvec2 res {resolution.x, resolution.y};
      glm::uvec2 cursor {osWindow->mouse.freePos.x, osWindow->mouse.freePos.y};
      double time = (std::chrono::system_clock::now().time_since_epoch().count() % 1'000'000'000'000ll) / 1'000'000'000.0;

      Constants constants {
        .res = res,
        .cursor = cursor,
        .time = (float)time,
        .objectsAmount = objectsAmount,
        .mouseControlType = mouseControlType,
      };



      ETNA_PROFILE_GPU(currentCmdBuf, renderLocalShadertoy2);

      {
        etna::RenderTargetState state{currentCmdBuf, {{0, 0}, {resolution.x, resolution.y}}, {{computeImage.get(), computeImage.getView({})}}, {}};
        auto simpleMaterialInfo = etna::get_shader_program("local_shadertoy2_texture");
        auto set = etna::create_descriptor_set(
          simpleMaterialInfo.getDescriptorLayoutId(0),
          currentCmdBuf,
          {
            etna::Binding{1, image.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
          });

        currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, computePipeline.getVkPipeline());
        vk::DescriptorSet vkSet = set.getVkSet();
        currentCmdBuf.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          computePipeline.getVkPipelineLayout(),
          0, 1,
          &vkSet,
          0, nullptr);
        currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(computePipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);
        currentCmdBuf.draw(3, 1, 0, 0);
      }

      etna::set_state(
        currentCmdBuf,
        computeImage.get(),
        // We are going to use the texture at the transfer stage...
        vk::PipelineStageFlagBits2::eFragmentShader,
        // ...to transfer-read stuff from it...
        vk::AccessFlagBits2::eShaderRead,
        // ...and want it to have the appropriate layout.
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::set_state(
        currentCmdBuf,
        image.get(),
        // We are going to use the texture at the transfer stage...
        vk::PipelineStageFlagBits2::eFragmentShader,
        // ...to transfer-read stuff from it...
        vk::AccessFlagBits2::eShaderRead,
        // ...and want it to have the appropriate layout.
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::flush_barriers(currentCmdBuf);



      {
      auto simpleMaterialInfo = etna::get_shader_program("shader");
      auto set2 = etna::create_descriptor_set(
        simpleMaterialInfo.getDescriptorLayoutId(0),
        currentCmdBuf,
        {
              etna::Binding{0, computeImage.genBinding(computeSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
              etna::Binding{1,
                image.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}});

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
        0, nullptr);

      currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

      currentCmdBuf.draw(3, 1, 0, 0);
      }

      {
        ImDrawData* pDrawData = ImGui::GetDrawData();
        guiRenderer->render(
          currentCmdBuf, {{0, 0}, {resolution.x, resolution.y}}, backbuffer, backbufferView, pDrawData);
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
