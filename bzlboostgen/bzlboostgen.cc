#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <iostream>
#include <format>
#include <ranges>
#include <cstdlib>
#include <future>
#include <stacktrace>
#include <unordered_map>
#include "absl/strings/str_split.h"

using absl::ByAnyChar;
using absl::SkipWhitespace;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

constexpr auto cpp_extensions = std::array{
	".hpp"sv,
	".hh"sv,
	".h"sv,
	".hxx"sv,
	".cpp"sv,
	".cc"sv,
	".cxx"sv,
};

// Some boost headers don't follow the <boost/MODULE_NAME.hpp> format
// Those non-standard (mostly deprecated) headers are listed here.
const auto non_standard_boost_headers =
	std::unordered_map<std::string_view, std::string_view>{
		{"boost/current_function.hpp", "assert"},

		{"boost/cstdint.hpp"sv, "config"sv},
		{"boost/cxx11_char_types.hpp"sv, "config"sv},
		{"boost/limits.hpp"sv, "config"sv},
		{"boost/version.hpp"sv, "config"sv},
		{"boost/detail/workaround.hpp"sv, "config"sv},

		{"boost/noncopyable.hpp"sv, "core"sv},

		{"boost/make_shared.hpp"sv, "smart_ptr"sv},

		{"boost/exception/exception.hpp", "throw_exception"},

		{"boost/blank.hpp", "detail"},
		{"boost/blank_fwd.hpp", "detail"},
		{"boost/cstdlib.hpp", "detail"},

		{"boost/none.hpp", "optional"},
		{"boost/none_t.hpp", "optional"},
	};

constexpr auto BZLMOD_MODULE = R"starlark(
module(
    name = "{}",
    version = "{}",
    compatibility_level = {},
)
)starlark";

constexpr auto BZLMOD_BAZEL_DEP = R"starlark(
bazel_dep(name = "{}", version = "{}")
)starlark";

constexpr auto BAZELRC_DEFAULT = R"bazelrc(
common --enable_bzlmod
build --incompatible_use_platforms_repo_for_constraints
build --incompatible_enable_cc_toolchain_resolution
build --incompatible_strict_action_env
build --enable_runfiles
build --registry=https://raw.githubusercontent.com/bazelboost/registry/main
build --registry=https://bcr.bazel.build
)bazelrc";

constexpr auto BZLMOD_GITHUB_WORKFLOW = R"yaml(name: Bzlmod Archive

on:
  release:
    types: [published]

jobs:
  bzlmod-archive:
    uses: bazelboost/registry/.github/workflows/bzlmod-archive.yml@main
    secrets: inherit
    permissions:
      contents: write
)yaml";

constexpr auto BUILD_FILE_DEFAULT = R"starlark(
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "{}",
    includes = ["include"],
    hdrs = glob(["include/**/*.hpp", "include/**/*.h"]),
)
)starlark";

constexpr auto BUILD_FILE_DEFAULT_TEST = R"starlark(
load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "{}",
    srcs = glob(["**/*.hpp", "**/*.h", "**/*.cpp"]),
)
)starlark";

auto is_cpp_extension(const fs::path& p) noexcept -> bool {
	auto ext = p.extension();

	for(auto cpp_ext : ext) {
		if(ext == cpp_ext) {
			return true;
		}
	}

	return false;
}

auto find_boost_deps(fs::path dir) noexcept -> std::set<std::string> {
	auto deps = std::set<std::string>{};

	for(auto entry : fs::recursive_directory_iterator(dir)) {
		const auto& p = entry.path();

		if(!is_cpp_extension(p)) {
			continue;
		}

		auto hdr_file = std::ifstream{p};
		auto line = std::string{};
		while(std::getline(hdr_file, line)) {
			std::vector<std::string_view> split =
				absl::StrSplit(line, ByAnyChar("# \t"), SkipWhitespace());
			if(split.size() < 2 || split[0] != "include") {
				continue;
			}

			auto inc_path_str =
				std::string_view{split[1].data() + 1, split[1].size() - 2};

			if(auto itr = non_standard_boost_headers.find(inc_path_str);
				 itr != non_standard_boost_headers.end()) {
				auto module_name = std::string{itr->second};
				if(!deps.contains(module_name)) {
					deps.insert(module_name);
				}
				continue;
			}

			std::vector<std::string_view> inc_path_parts =
				absl::StrSplit(inc_path_str, '/');
			if(inc_path_parts[0] != "boost") {
				continue;
			}

			auto module_name = std::string{inc_path_parts[1]};
			if(module_name.ends_with(".hpp")) {
				module_name =
					module_name.substr(0, module_name.size() - ".hpp"sv.size());
			}

			if(!deps.contains(module_name)) {
				deps.insert(module_name);
			}
		}
	}

	return deps;
}

auto print_non_boost_dir_error(auto&& msg, auto&& path) noexcept {
	std::cerr << std::format(
		"[ERROR] {}\n"
		"        Are you sure '{}' is a boost module directory?",
		msg,
		fs::relative(path).generic_string()
	);
}

auto run_command( //
	std::string               cmd,
	std::ranges::range auto&& valid_exit_codes
) -> void {
	auto exit_code = std::system(cmd.c_str());
	auto is_success_error_code = false;
	for(auto valid_exit_code : valid_exit_codes) {
		if(exit_code == valid_exit_code) {
			is_success_error_code = true;
			break;
		}
	}

	if(!is_success_error_code) {
		std::cerr << std::format(
			"[ERROR] Exit code {} from running command:\n"
			"        cd {}\n"
			"        {}",
			exit_code,
			fs::current_path().generic_string(),
			cmd
		);

		std::exit(exit_code);
	}
}

auto run_buildifier(const fs::path& path) -> void {
	run_command(
		std::format("buildifier \"{}\"", path.string()).c_str(),
		std::array{0}
	);
}

struct bzlmod_info {
	std::string name;
	std::string version;
	int         compatibility_level;
};

auto write_bzlmod_files(
	auto&&                    dir,
	std::ranges::range auto&& target_source_dirs,
	bzlmod_info               info
) noexcept -> std::set<std::string> {
	auto deps = std::set<std::string>{};

	for(const fs::path& src_dir : target_source_dirs) {
		if(!fs::exists(src_dir)) {
			continue;
		}

		auto src_dir_deps = find_boost_deps(src_dir);

		for(auto&& src_dir_dep : src_dir_deps) {
			if(!deps.contains(src_dir_dep)) {
				deps.insert(src_dir_dep);
			}
		}
	}

	deps.erase(info.name.substr("boost."sv.size()));

	const auto bzlmod_file_path = dir / "MODULE.bazel";
	auto       bzlmod_file = std::ofstream{bzlmod_file_path};

	bzlmod_file << std::format(
		BZLMOD_MODULE,
		info.name,
		info.version,
		info.compatibility_level
	);

	bzlmod_file << "\n";

	for(auto&& dep : deps) {
		bzlmod_file //
			<< "\n"
			<< std::format(BZLMOD_BAZEL_DEP, "boost." + dep, info.version);
	}

	bzlmod_file << "\n";

	bzlmod_file.flush();
	bzlmod_file.close();

	run_buildifier(bzlmod_file_path);

	return deps;
}

auto write_file_contents(const fs::path& p, auto&& contents) -> void {
	if(p.has_parent_path()) {
		auto ec = std::error_code{};
		fs::create_directories(p.parent_path(), ec);
	}

	auto f = std::ofstream{p};
	f << contents;
	f.flush();
	f.close();
}

auto add_deps(
	fs::path                  dir,
	std::string_view          label,
	std::ranges::range auto&& deps
) -> void {
	if(std::size(deps) == 0) {
		return;
	}

	auto cmd =
		std::format("buildozer -root_dir=\"{}\"", fs::relative(dir).string());

	cmd += " \"set deps";
	for(std::string dep : deps) {
		cmd += std::format(" @boost.{}", dep);
	}

	cmd += std::format("\" {}", label);

	run_command(cmd, std::array{0, 3 /* no changes */});
}

auto main(int argc, char* argv[]) -> int {
	std::set_terminate([]() {
		std::cerr << std::stacktrace::current();
		std::abort();
	});
	const auto dir = argc > 1 ? fs::path{argv[1]} : fs::current_path();
	const auto include_dir = dir / "include";
	const auto test_dir = dir / "test";

	fs::current_path(dir);

	if(!fs::exists(include_dir / "boost")) {
		print_non_boost_dir_error("Cannot find 'include/boost' dir", dir);
		return 1;
	}

	auto info = bzlmod_info{
		.name = "boost." + dir.filename().generic_string(),
		.version = "1.83.0",
		.compatibility_level = 108300,
	};

	auto jobs = std::array{
		std::async(
			std::launch::async,
			[&, info] {
				auto deps =
					write_bzlmod_files(dir, std::array{include_dir, dir / "src"}, info);

				const auto build_file_path = dir / "BUILD.bazel";
				if(!fs::exists(build_file_path)) {
					write_file_contents(
						build_file_path,
						std::format(BUILD_FILE_DEFAULT, info.name)
					);
				}

				add_deps(dir, "//:" + info.name, deps);
				run_buildifier(build_file_path);
			}
		),
		std::async(
			std::launch::async,
			[&, info]() mutable {
				if(!fs::exists(test_dir)) {
					return;
				}

				auto module_name = info.name;

				info.name += ".test";
				auto deps = write_bzlmod_files(test_dir, std::array{test_dir}, info);

				const auto build_file_path = test_dir / "BUILD.bazel";
				if(!fs::exists(build_file_path)) {
					write_file_contents(
						build_file_path,
						std::format(BUILD_FILE_DEFAULT_TEST, info.name)
					);
				}

				add_deps(test_dir, "//:" + info.name, deps);
				run_command(
					std::format(
						"buildozer -root_dir=\"{}\" -quiet \"remove version\" "
						"//MODULE.bazel:{}",
						fs::relative(test_dir).string(),
						module_name
					),
					std::array{0}
				);

				run_command(
					std::format(
						"buildozer -root_dir=\"{}\" -quiet \"new local_path_override _{}\" "
						"//MODULE.bazel:all",
						fs::relative(test_dir).string(),
						module_name
					),
					std::array{0}
				);

				run_command(
					std::format(
						"buildozer -root_dir=\"{}\" -quiet \"set path ..\" "
						"//MODULE.bazel:_{}",
						fs::relative(test_dir).string(),
						module_name
					),
					std::array{0}
				);

				run_command(
					std::format(
						"buildozer -root_dir=\"{0}\" -quiet \"set module_name {1}\" "
						"//MODULE.bazel:_{1}",
						fs::relative(test_dir).string(),
						module_name
					),
					std::array{0}
				);

				run_command(
					std::format(
						"buildozer -root_dir=\"{0}\" -quiet \"remove name\" "
						"//MODULE.bazel:_{1}",
						fs::relative(test_dir).string(),
						module_name
					),
					std::array{0}
				);

				run_buildifier(build_file_path);
			}
		),
		std::async(
			std::launch::async,
			[&, info] {
				const auto bazelrc_path = dir / ".bazelrc";
				if(!fs::exists(bazelrc_path)) {
					write_file_contents(bazelrc_path, BAZELRC_DEFAULT);
				}
			}
		),
		std::async(
			std::launch::async,
			[&, info] {
				const auto workflow_path =
					dir / ".github" / "workflows" / "bzlmod-archive.yml";
				if(!fs::exists(workflow_path)) {
					write_file_contents(workflow_path, BZLMOD_GITHUB_WORKFLOW);
				}
			}
		),
		std::async(
			std::launch::async,
			[&, info] {
				const auto workspace_file_path = dir / "WORKSPACE.bazel";
				if(!fs::exists(workspace_file_path)) {
					write_file_contents(workspace_file_path, "# SEE: MODULE.bazel\n");
				}
			}
		),

		std::async(
			std::launch::async,
			[&, info] {
				const auto workspace_file_path = test_dir / "WORKSPACE.bazel";
				if(!fs::exists(workspace_file_path)) {
					write_file_contents(workspace_file_path, "# SEE: MODULE.bazel\n");
				}
			}
		),
	};

	for(auto& job : jobs) {
		job.wait();
	}

	return 0;
}
