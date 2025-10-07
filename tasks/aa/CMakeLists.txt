
add_executable(aa
  main.cpp
  Renderer.cpp
  WorldRenderer.cpp
  App.cpp
)

target_link_libraries(aa
  PRIVATE glfw etna glm::glm wsi gui scene render_utils)

target_add_shaders(aa
  shaders/simple.vert
  shaders/simple_shadow.frag
)
