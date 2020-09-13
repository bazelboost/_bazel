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
	auto fork_template_dir = workspace_dir / "fork_template";
	auto fork_path = workspace_dir / ".." / name;

	if(!fs::exists(fork_path)) {
		fs::create_directory(fork_path);
		fs::current_path(fork_path);
		cmd = "git clone https://github.com/bazelboost/" + name + " .";
		std::cout << cmd << std::endl;
		std::system(cmd.c_str());

		cmd = "git remote add upstream https://github.com/boostorg/" + name;
		std::cout << cmd << std::endl;
		std::system(cmd.c_str());

		cmd = "git fetch upstream develop";
		std::cout << cmd << std::endl;
		std::system(cmd.c_str());

		cmd = "git rebase upstream/develop";
		std::cout << cmd << std::endl;
		std::system(cmd.c_str());

		cmd = "git push";
		std::cout << cmd << std::endl;
		std::system(cmd.c_str());

		fs::current_path(workspace_dir);
	}

	for(auto entry : fs::recursive_directory_iterator(fork_template_dir)) {
		auto full_path = entry.path();
		auto path = fs::relative(full_path, fork_template_dir);
		auto new_path = fork_path / path;

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

		std::ofstream file_stream(fork_path / path);
		file_stream << file_contents;
	}

	fs::current_path(fork_path);
	cmd = "code " + fork_path.generic_string();
	std::cout << cmd << std::endl;
	std::system(cmd.c_str());

	return 0;
}
