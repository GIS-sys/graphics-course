
add_executable(particles
  main.cpp
  App.cpp
)

target_link_libraries(particles
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(particles
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
)

