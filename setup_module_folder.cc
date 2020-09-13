#include <filesystem>
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include "absl/strings/str_replace.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	const auto name = std::string(argv[1]);

	if(name.empty()) {
		std::cerr << "missing fork name" << std::endl;
		return 1;
	}

	std::string cmd;
	auto workspace_dir = fs::path(std::getenv("BUILD_WORKSPACE_DIRECTORY"));
	auto module_template_dir = workspace_dir / "module_template";
	auto module_path = workspace_dir / ".." / "boost" / name;

	if(!fs::exists(module_path)) {
		fs::create_directory(module_path);
	}

	for(auto entry : fs::recursive_directory_iterator(module_template_dir)) {
		auto full_path = entry.path();
		auto path = fs::relative(full_path, module_template_dir);
		auto new_path = module_path / path;

		if(entry.is_directory()) {
			if(!fs::exists(new_path)) {
				fs::create_directory(new_path);
			}
			continue;
		}

		if(fs::exists(new_path)) {
			std::cout
				<< "skipping " << new_path.generic_string()
				<< " because it already exists" << std::endl;
			continue;
		}

		std::string file_contents =
			(std::stringstream() << std::ifstream(full_path).rdbuf()).str();
		absl::StrReplaceAll({{"{NAME}", name}}, &file_contents);

		std::ofstream file_stream(module_path / path);
		file_stream << file_contents;
	}

	return 0;
}
