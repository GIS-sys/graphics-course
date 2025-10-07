
add_executable(ssao
  main.cpp
  Renderer.cpp
  WorldRenderer.cpp
  App.cpp
)

target_link_libraries(ssao
  PRIVATE glfw etna glm::glm wsi gui scene render_utils)

target_add_shaders(ssao
  shaders/simple.vert
  shaders/simple_shadow.frag
)
