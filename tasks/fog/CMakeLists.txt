
add_executable(fog
  main.cpp
  App.cpp
)

target_link_libraries(fog
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(fog
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
)

