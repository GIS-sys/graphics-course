
add_executable(model_bakery_baker
  main.cpp
)

target_link_libraries(model_bakery_baker
  PRIVATE tinygltf glfw etna glm::glm wsi gui scene render_utils)
