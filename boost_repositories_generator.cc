#include <filesystem>
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include "absl/strings/str_replace.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	std::string cmd;
	auto workspace_dir = fs::path(std::getenv("BUILD_WORKSPACE_DIRECTORY"));
	auto boost_dir = workspace_dir / ".." / "boost";



	return 0;
}
