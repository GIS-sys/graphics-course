
add_executable(imgui
  main.cpp
  App.cpp
)

target_link_libraries(imgui
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(imgui
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
)

