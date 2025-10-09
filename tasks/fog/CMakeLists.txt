
add_executable(fog
  main.cpp
  App.cpp
  ParticleEmitter.cpp
  ParticleSystem.cpp
)

target_link_libraries(fog
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(fog
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
  shaders/particle.frag
  shaders/particle.vert
  shaders/fog_texture.vert
  shaders/fog_texture.frag
)

