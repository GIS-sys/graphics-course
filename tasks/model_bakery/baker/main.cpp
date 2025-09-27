#include "Bakery.hpp"


int main()
{
  if (argc != 2) {
    return 1;
  }
  Bakery().selectScene(std::filesystem::path(argv[1]));
  return 0;
}
