#include "riolib.hpp"
#include <iostream>
#include <string>

int main() {
  std::string str = "dfasf+fkdsjf%25d";
  std::cout << riolib::PercentageDecoding::decode(str) << std::endl;
}
