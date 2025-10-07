
add_executable(a_a
  main.cpp
  App.cpp
)

target_link_libraries(a_a
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(a_a
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
)

