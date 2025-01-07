#include <iostream>
#include <moloader/moloader.hpp>

int main() {
  moloader::load("/usr/share/locale/be/LC_MESSAGES/inkscape.mo");
  std::cout << "Black Stroke " << moloader::getstring("Black stroke") << std::endl;
  return 0;
}