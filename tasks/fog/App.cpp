#include "App.hpp"

#include <etna/Etna.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/Profiling.hpp>
#include <etna/RenderTargetStates.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include <imgui.h>
#include "gui/ImGuiRenderer.hpp"


void App::InitEmitters() {
    particleSystem = std::make_unique<ParticleSystem>();

    // Create 3 emitters at different positions
    emitterParams.resetN(0);
    particleSystem->addEmitter(emitterParams);
    emitterParams.resetN(1);
    particleSystem->addEmitter(emitterParams);
    emitterParams.resetN(2);
    particleSystem->addEmitter(emitterParams);
    emitterParams.resetN(0);

    lastFrameTime = std::chrono::steady_clock::now();
}

void App::initParticlePipeline() {
    etna::create_program(
        "particle",
        {
            FOG_SHADERS_ROOT "particle.vert.spv",
            FOG_SHADERS_ROOT "particle.frag.spv"
        }
    );

    particlePipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
        "particle",
        etna::GraphicsPipeline::CreateInfo{
            .fragmentShaderOutput = {
                .colorAttachmentFormats = {vkWindow->getCurrentFormat()}
            }
        });
}

App::App()
  : resolution{1280, 720}
  , useVsync{true}
{
  InitEmitters();

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

  commandManager = etna::get_context().createPerFrameCmdMgr();

  // Init shaders
  etna::create_program("local_shadertoy2_texture",
    {
      FOG_SHADERS_ROOT "toy.vert.spv",
      FOG_SHADERS_ROOT "texture.frag.spv",
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

  // Add this - create main render image
  mainRenderImage = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "main_render_image",
    .format = vkWindow->getCurrentFormat(),
    .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
  });
  // Add this - create sampler for main render image
  mainRenderSampler = etna::Sampler(etna::Sampler::CreateInfo{
    .addressMode = vk::SamplerAddressMode::eClampToEdge,
    .name = "mainRenderSampler"
  });


  etna::create_program(
    "shader",
    {
      FOG_SHADERS_ROOT "toy.vert.spv",
      FOG_SHADERS_ROOT "toy.frag.spv"
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
    FOG_SHADERS_ROOT "../../../../resources/textures/test_tex_1.png",
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

  fogTextureResolution = resolution / 2u;
  initFogTexturePipeline();

  initParticlePipeline();

  // In the constructor, after creating other resources:
  particleBuffer = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = MAX_PARTICLES * sizeof(ParticleData),
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "particle_buffer"
  });
  particleData.reserve(MAX_PARTICLES);

  // Init ImGUI
  ImGuiRenderer::enableImGuiForWindow(osWindow->native());
}

void App::updateCamera(float deltaTime) {
    // Simple orbiting camera for testing
    static float cameraAngle = 0.0f;
    cameraAngle += deltaTime * 0.5f;

    cameraPosition = glm::vec3(
        sin(cameraAngle) * 5.0f,
        2.0f,
        cos(cameraAngle) * 5.0f
    );
}

void App::updateParticleBuffer() {
    particleData.clear();

    for (auto& emitter : particleSystem->getEmitters()) {
        for (const auto& particle : emitter->getParticles()) {
            if (particleData.size() < MAX_PARTICLES) {
                ParticleData data;
                data.position = particle.position;
                data.size = particle.size;
                data.color = particle.color;
                particleData.push_back(data);
            }
        }
    }

    // Update the buffer with current particle data
    if (!particleData.empty()) {
        void* mapped = particleBuffer.map();
        memcpy(mapped, particleData.data(), particleData.size() * sizeof(ParticleData));
        particleBuffer.unmap();
    }
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

    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    updateCamera(deltaTime);
    particleSystem->update(deltaTime, cameraPosition);
    updateParticleBuffer(); // Update particle buffer each frame

    // Debug: print particle counts
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        int totalParticles = 0;
        for (auto& emitter : particleSystem->getEmitters()) {
            totalParticles += emitter->getParticles().size();
        }
        std::cout << "Total particles: " << totalParticles << std::endl;
    }

    drawFrame();
  }

  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::drawGui() {
    ZoneScopedN("drawGui");
    guiRenderer->nextFrame();
    ImGui::NewFrame();
    ImGui::Begin("Simple render settings");

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

    if (ImGui::CollapsingHeader("Light")) {
        ImGui::SliderFloat3("Ambient Light", &ambientLight[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Diffuse Val", &diffuseVal, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular Power", &specPow, 0.0f, 50.0f);
        ImGui::SliderFloat("Specular Value", &specVal, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Particle System")) {
        ImGui::SliderFloat3("Emitter Position", &emitterParams.position.x, -50.0f, 50.0f);
        ImGui::SliderFloat("Spawn Rate", &emitterParams.spawnRate, 1.0f, 100.0f);
        ImGui::SliderFloat("Particle Lifetime", &emitterParams.particleLifetime, 0.1f, 5.0f);
        ImGui::SliderFloat("Initial Speed", &emitterParams.initialSpeed, 0.1f, 10.0f);
        ImGui::SliderFloat3("Velocity Variation", &emitterParams.velocityVariation.x, 0.0f, 2.0f);
        ImGui::SliderFloat("Start Size", &emitterParams.startSize, 0.01f, 1.0f);
        ImGui::SliderFloat("End Size", &emitterParams.endSize, 0.01f, 1.0f);
        ImGui::ColorEdit4("Start Color", &emitterParams.startColor.x);
        ImGui::ColorEdit4("End Color", &emitterParams.endColor.x);
        ImGui::Checkbox("Gravity Enabled", &emitterParams.gravityEnabled);
        if (ImGui::Button("Update All Emitters")) {
            int i = 0;
            for (auto& emitter : particleSystem->getEmitters()) {
                emitter->setParams(emitterParams.as(i));
                ++i;
            }
        }
    }

    if (ImGui::CollapsingHeader("Volumetric Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled", &fogEnabled);
        ImGui::SliderFloat("General Density", &fogGeneralDensity, 0.01f, 0.9f);
        ImGui::SliderInt("Ray Steps", &fogDivisions, 8, 128);
        ImGui::SliderFloat("Wind Strength", &fogWindStrength, 0.01, 30.0);
        ImGui::SliderFloat("Wind Speed", &fogWindSpeed, 0.01, 30.0);
        ImGui::Text("Quality: %dx%d", fogTextureResolution.x, fogTextureResolution.y);
        ImGui::Text("Performance: Higher steps = better quality but slower");
    }

    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Moving Light Enabled", &lightMovementEnabled);
        ImGui::SliderFloat("Hole Radius", &holeRadius, 0.01f, 10.0f);
        ImGui::SliderFloat("Hole Border Length", &holeBorderLength, 10.0, 1000.0);
        ImGui::SliderFloat("Hole Border Width", &holeBorderWidth, 5.0, 1000.0);
        ImGui::SliderFloat3("Hole Pos Delta", &holeDelta[0], -70.0, 160.0);
    }

    ImGui::NewLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press 'B' to recompile and reload shaders");

    ImGui::End();
    ImGui::Render();
}

void App::drawFrame()
{
    drawGui();

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
        specificDrawFrameMain(currentCmdBuf, backbuffer, backbufferView);
        specificDrawFrameParticles(currentCmdBuf, backbuffer, backbufferView); // Particles come after main rendering
        specificDrawFrameImGUI(currentCmdBuf, backbuffer, backbufferView);
        ETNA_CHECK_VK_RESULT(currentCmdBuf.end());

        // We are done recording GPU commands now and we can send them to be executed by the GPU.
        auto renderingDone =
            commandManager->submit(std::move(currentCmdBuf), std::move(backbufferAvailableSem));
        static_cast<void>(etna::get_context().getDevice().waitIdle());

        // Finally, present the backbuffer the screen
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

void App::specificDrawFrameMain(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView&)
{
  // Initialize backbuffer
  etna::set_state(
    currentCmdBuf,
    backbuffer,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::AccessFlagBits2::eTransferWrite,
    vk::ImageLayout::eTransferDstOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::flush_barriers(currentCmdBuf);

  glm::uvec2 res {resolution.x, resolution.y};
  glm::uvec2 cursor {osWindow->mouse.freePos.x, osWindow->mouse.freePos.y};
  double time = (std::chrono::system_clock::now().time_since_epoch().count() % 1'000'000'000'000ll) / 1'000'000'000.0;

  Constants constants {
    .ambientLight = {ambientLight, 1.0},
    .holeDelta = holeDelta,
    .res = res,
    .cursor = cursor,
    .time = (float)time,
    .fogGeneralDensity = fogGeneralDensity,
    .diffuseVal = diffuseVal,
    .specPow = specPow,
    .specVal = specVal,
    .fogWindStrength = fogWindStrength,
    .fogWindSpeed = fogWindSpeed,
    .holeRadius = holeRadius,
    .holeBorderLength = holeBorderLength,
    .holeBorderWidth = holeBorderWidth,
    .objectsAmount = objectsAmount,
    .mouseControlType = mouseControlType,
    .particleCount = (int)particleData.size(),
    .fogDivisions = fogDivisions,
    .fogEnabled = fogEnabled,
    .lightMovementEnabled = lightMovementEnabled,
  };

  ETNA_PROFILE_GPU(currentCmdBuf, renderLocalShadertoy2);

  // First, render the texture to computeImage
  {
    etna::set_state(
      currentCmdBuf,
      computeImage.get(),
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageAspectFlagBits::eColor);
    etna::flush_barriers(currentCmdBuf);

    etna::RenderTargetState textureState{
      currentCmdBuf,
      {{0, 0}, {resolution.x, resolution.y}},
      {{computeImage.get(), computeImage.getView({})}},
      {}
    };

    currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, computePipeline.getVkPipeline());
    currentCmdBuf.draw(3, 1, 0, 0);
  }

  updateFogTexture(currentCmdBuf);

  // Transition images for main shader
  etna::set_state(
    currentCmdBuf,
    computeImage.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::set_state(
    currentCmdBuf,
    fogTextureImage.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::set_state(
    currentCmdBuf,
    image.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);

  // Prepare mainRenderImage for rendering
  etna::set_state(
    currentCmdBuf,
    mainRenderImage.get(),
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::flush_barriers(currentCmdBuf);

  // Now render the main scene to mainRenderImage
  {
    auto simpleMaterialInfo = etna::get_shader_program("shader");
    auto set2 = etna::create_descriptor_set(
        simpleMaterialInfo.getDescriptorLayoutId(0),
        currentCmdBuf,
        {
            etna::Binding{0, computeImage.genBinding(computeSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{1, image.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{2, particleBuffer.genBinding()},
            etna::Binding{3, fogTextureImage.genBinding(fogTextureSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        });

    etna::RenderTargetState renderTargets{
      currentCmdBuf,
      {{0, 0}, {resolution.x, resolution.y}},
      {{mainRenderImage.get(), mainRenderImage.getView({})}},
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

    currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

    currentCmdBuf.draw(3, 1, 0, 0);
  }

  // Transition mainRenderImage to shader read for particle stage
  etna::set_state(
    currentCmdBuf,
    mainRenderImage.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::flush_barriers(currentCmdBuf);
}

void App::specificDrawFrameImGUI(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView)
{
  // Render ImGui on top of everything
  ImDrawData* pDrawData = ImGui::GetDrawData();
  if (pDrawData && pDrawData->CmdListsCount > 0) {
    guiRenderer->render(
      currentCmdBuf, {{0, 0}, {resolution.x, resolution.y}}, backbuffer, backbufferView, pDrawData);
  }
}

void App::specificDrawFrameParticles(vk::CommandBuffer& currentCmdBuf, vk::Image& backbuffer, vk::ImageView& backbufferView)
{
    ETNA_PROFILE_GPU(currentCmdBuf, renderParticles);

    // Ensure backbuffer is ready for writing
    etna::set_state(
        currentCmdBuf,
        backbuffer,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
    etna::flush_barriers(currentCmdBuf);

    // Set up constants for particle shader
    glm::uvec2 res {resolution.x, resolution.y};
    glm::uvec2 cursor {osWindow->mouse.freePos.x, osWindow->mouse.freePos.y};
    double time = (std::chrono::system_clock::now().time_since_epoch().count() % 1'000'000'000'000ll) / 1'000'000'000.0;

    Constants constants {
        .ambientLight = {ambientLight, 1.0},
        .holeDelta = holeDelta,
        .res = res,
        .cursor = cursor,
        .time = (float)time,
        .fogGeneralDensity = fogGeneralDensity,
        .diffuseVal = diffuseVal,
        .specPow = specPow,
        .specVal = specVal,
        .fogWindStrength = fogWindStrength,
        .fogWindSpeed = fogWindSpeed,
        .holeRadius = holeRadius,
        .holeBorderLength = holeBorderLength,
        .holeBorderWidth = holeBorderWidth,
        .objectsAmount = objectsAmount,
        .mouseControlType = mouseControlType,
        .particleCount = (int)particleData.size(),
        .fogDivisions = fogDivisions,
        .fogEnabled = fogEnabled,
        .lightMovementEnabled = lightMovementEnabled,
    };

    // Create descriptor set for particle shader - use mainRenderImage as input
    auto particleMaterialInfo = etna::get_shader_program("particle");
    auto particleSet = etna::create_descriptor_set(
        particleMaterialInfo.getDescriptorLayoutId(0),
        currentCmdBuf,
        {
            etna::Binding{0, mainRenderImage.genBinding(mainRenderSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
        });

    // Render fullscreen quad with particle shader to backbuffer
    etna::RenderTargetState renderTargets{
        currentCmdBuf,
        {{0, 0}, {resolution.x, resolution.y}},
        {{.image = backbuffer, .view = backbufferView}},
        {}
    };

    vk::DescriptorSet vkParticleSet = particleSet.getVkSet();
    currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, particlePipeline.getVkPipeline());
    currentCmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        particlePipeline.getVkPipelineLayout(),
        0, 1,
        &vkParticleSet,
        0, 0);

    currentCmdBuf.pushConstants<vk::DispatchLoaderDynamic>(
        particlePipeline.getVkPipelineLayout(),
        vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
        0, sizeof(constants), &constants);

    currentCmdBuf.draw(3, 1, 0, 0);

    // Transition backbuffer to present layout
    etna::set_state(
        currentCmdBuf,
        backbuffer,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageAspectFlagBits::eColor);
    etna::flush_barriers(currentCmdBuf);
}

void App::initFogTexturePipeline() {
    etna::create_program(
        "fog_texture",
        {
            FOG_SHADERS_ROOT "fog_texture.vert.spv",
            FOG_SHADERS_ROOT "fog_texture.frag.spv"
        }
    );

    fogTexturePipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
        "fog_texture",
        etna::GraphicsPipeline::CreateInfo{
            .fragmentShaderOutput = {
                .colorAttachmentFormats = {vkWindow->getCurrentFormat()}
            }
        });

    // Create fog texture image (half resolution)
    fogTextureImage = etna::get_context().createImage(etna::Image::CreateInfo{
        .extent = vk::Extent3D{fogTextureResolution.x, fogTextureResolution.y, 1},
        .name = "fog_texture",
        .format = vkWindow->getCurrentFormat(),
        .imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
    });

    fogTextureSampler = etna::Sampler(etna::Sampler::CreateInfo{
        .addressMode = vk::SamplerAddressMode::eClampToEdge,
        .name = "fogTextureSampler"
    });
}

void App::updateFogTexture(vk::CommandBuffer& cmdBuf) {
    ETNA_PROFILE_GPU(cmdBuf, updateFogTexture);

    // Transition fog texture for writing
    etna::set_state(
        cmdBuf,
        fogTextureImage.get(),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
    etna::flush_barriers(cmdBuf);

    // Render fog texture
    etna::RenderTargetState fogState{
        cmdBuf,
        {{0, 0}, {fogTextureResolution.x, fogTextureResolution.y}},
        {{fogTextureImage.get(), fogTextureImage.getView({})}},
        {}
    };

    glm::uvec2 res {fogTextureResolution.x, fogTextureResolution.y};
    glm::uvec2 cursor {osWindow->mouse.freePos.x, osWindow->mouse.freePos.y};
    double time = (std::chrono::system_clock::now().time_since_epoch().count() % 1'000'000'000'000ll) / 1'000'000'000.0;

    Constants fogConstants {
        .ambientLight = {ambientLight, 1.0},
        .holeDelta = holeDelta,
        .res = res,
        .cursor = cursor,
        .time = (float)time,
        .fogGeneralDensity = fogGeneralDensity,
        .diffuseVal = diffuseVal,
        .specPow = specPow,
        .specVal = specVal,
        .fogWindStrength = fogWindStrength,
        .fogWindSpeed = fogWindSpeed,
        .holeRadius = holeRadius,
        .holeBorderLength = holeBorderLength,
        .holeBorderWidth = holeBorderWidth,
        .objectsAmount = objectsAmount,
        .mouseControlType = mouseControlType,
        .particleCount = (int)particleData.size(),
        .fogDivisions = fogDivisions,
        .fogEnabled = fogEnabled,
        .lightMovementEnabled = lightMovementEnabled,
    };

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, fogTexturePipeline.getVkPipeline());
    cmdBuf.pushConstants<vk::DispatchLoaderDynamic>(
        fogTexturePipeline.getVkPipelineLayout(),
        vk::ShaderStageFlagBits::eFragment,
        0, sizeof(fogConstants), &fogConstants);

    cmdBuf.draw(3, 1, 0, 0);

    // Transition back to shader read
    etna::set_state(
        cmdBuf,
        fogTextureImage.get(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);
    etna::flush_barriers(cmdBuf);
}

