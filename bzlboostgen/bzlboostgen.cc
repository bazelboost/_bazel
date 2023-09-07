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

		{"boost/memory_order.hpp", "atomic"},

		{"boost/is_placeholder.hpp", "bind"},
		{"boost/mem_fn.hpp", "bind"},

		{"boost/circular_buffer_fwd.hpp", "circular_buffer"},

		{"boost/concept/*", "concept_check"},
		{"boost/concept_archetype.hpp", "concept_check"},

		{"boost/functional/hash/*", "container_hash"},
		{"boost/functional/hash.hpp", "container_hash"},
		{"boost/functional/hash_fwd.hpp", "container_hash"},

		{"boost/contract_macro.hpp", "contract"},

		{"boost/make_default.hpp", "convert"},

		{"boost/cstdint.hpp"sv, "config"sv},
		{"boost/cxx11_char_types.hpp"sv, "config"sv},
		{"boost/limits.hpp"sv, "config"sv},
		{"boost/version.hpp"sv, "config"sv},
		{"boost/detail/workaround.hpp"sv, "config"sv},

		{"boost/detail/iterator.hpp", "core"},
		{"boost/detail/lightweight_test.hpp", "core"},
		{"boost/detail/no_exceptions_support.hpp", "core"},
		{"boost/detail/scoped_enum_emulation", "core"},
		{"boost/detail/sp_typeinfo.hpp", "core"},
		{"boost/utility/addressof.hpp", "core"},
		{"boost/utility/enable_if.hpp", "core"},
		{"boost/utility/explicit_operator_bool.hpp", "core"},
		{"boost/utility/swap.hpp", "core"},
		{"boost/check_delete.hpp", "core"},
		{"boost/get_pointer.hpp", "core"},
		{"boost/iterator.hpp", "core"},
		{"boost/non_type.hpp", "core"},
		{"boost/noncopyable.hpp"sv, "core"sv},
		{"boost/ref.hpp", "core"},
		{"boost/swap.hpp", "core"},
		{"boost/type.hpp", "core"},
		{"boost/visit_each.hpp", "core"},

		{"boost/dynamic_bitset_fwd.hpp", "dynamic_bitset"},

		{"boost/exception_ptr.hpp", "exception"},

		{"boost/foeach_fwd.hpp", "foreach"},

		{"boost/function_equal.hpp", "function"},

		{"boost/detail/algorithm", "graph"},
		{"boost/pending/detail/disjoint_sets.hpp", "graph"},
		{"boost/pending/detail/property.hpp", "graph"},
		{"boost/pending/bucket_sorter.hpp", "graph"},
		{"boost/pending/container_traits.hpp", "graph"},
		{"boost/pending/disjoint_sets.hpp", "graph"},
		{"boost/pending/fenced_priority_queue.hpp", "graph"},
		{"boost/pending/fibonacci_heap.hpp", "graph"},
		{"boost/pending/indirect_cmp.hpp", "graph"},
		{"boost/pending/is_heap.hpp", "graph"},
		{"boost/pending/mutable_heap.hpp", "graph"},
		{"boost/pending/mutable_queue.hpp", "graph"},
		{"boost/pending/property.hpp", "graph"},
		{"boost/pending/property_serialize.hpp", "graph"},
		{"boost/pending/queue.hpp", "graph"},
		{"boost/pending/relaxed_heap.hpp", "graph"},
		{"boost/pending/stringtok.hpp", "graph"},

		{"boost/graph/parallel/*", "graph_parallel"},
		{"boost/graph/distributed/*", "graph_parallel"},
		{"boost/graph/accounting.hpp", "graph_parallel"},

		{"boost/pending/integer_log2.hpp", "integer"},
		{"boost/integer_fwd.hpp", "integer"},
		{"boost/integer_traits.hpp", "integer"},

		{"boost/io_fwd.hpp", "io"},

		{"boost/pending/detail/int_iterator.hpp", "iterator"},
		{"boost/pending/iterator_adaptors.hpp", "iterator"},
		{"boost/pending/iterator_tests.hpp", "iterator"},
		{"boost/function_output_iterator.hpp", "iterator"},
		{"boost/generator_iterator.hpp", "iterator"},
		{"boost/indirect_reference.hpp", "iterator"},
		{"boost/iterator_adaptors.hpp", "iterator"},
		{"boost/next_prior.hpp", "iterator"},
		{"boost/pointee.hpp", "iterator"},
		{"boost/shared_container_iterator.hpp", "iterator"},

		{"boost/detail/basic_pointerbuf.hpp", "lexical_cast"},
		{"boost/detail/lcast_precision.hpp", "lexical_cast"},

		{"boost/math_fwd.hpp", "math"},
		{"boost/cstdfloat.hpp", "math"},

		{"boost/multi_index_container.hpp", "multi_index"},
		{"boost/multi_index_container_fwd.hpp", "multi_index"},

		{"boost/numeric/conversion/*", "numeric_conversion"},
		{"boost/cast.hpp", "numeric_conversion"},

		{"boost/predef.h", "predef"},

		{"boost/qvm_lite.hpp", "qvm"},

		{"boost/nondet_random.hpp", "random"},

		{"boost/cregex.hpp", "regex"},
		{"boost/regex.h", "regex"},
		{"boost/regex_fwd.hpp", "regex"},

		{"boost/archive/*", "serialization"},

		{"boost/detail/atomic_count.hpp", "smart_ptr"},
		{"boost/detail/lightweight_mutex.hpp", "smart_ptr"},
		{"boost/detail/lightweight_thread.hpp", "smart_ptr"},
		{"boost/detail/quick_allocator.hpp", "smart_ptr"},
		{"boost/enable_shared_from_this.hpp", "smart_ptr"},
		{"boost/intrusive_ptr.hpp", "smart_ptr"},
		{"boost/make_shared.hpp", "smart_ptr"},
		{"boost/make_unique.hpp", "smart_ptr"},
		{"boost/pointer_cast.hpp", "smart_ptr"},
		{"boost/pointer_to_other.hpp", "smart_ptr"},
		{"boost/scoped_array.hpp", "smart_ptr"},
		{"boost/shared_array.hpp", "smart_ptr"},
		{"boost/shared_ptr.hpp", "smart_ptr"},
		{"boost/smart_ptr.hpp", "smart_ptr"},
		{"boost/weak_ptr.hpp", "smart_ptr"},

		{"boost/cerrno.hpp", "system"},

		{"boost/progress.hpp", "timer"},

		{"boost/token_functions.hpp", "tokenizer"},
		{"boost/token_iterator.hpp", "tokenizer"},

		{"boost/utility/declval.hpp", "type_traits"},
		{"boost/aligned_storage.hpp", "type_traits"},

		{"boost/unordered_map.hpp", "unordered"},
		{"boost/unordered_set.hpp", "unordered"},

		{"boost/detail/call_traits.hpp", "utility"},
		{"boost/detail/compressed_pair.hpp", "utility"},
		{"boost/detail/ob_compressed_pair.hpp", "utility"},
		{"boost/call_traits.hpp", "utility"},
		{"boost/compressed_pair.hpp", "utility"},
		{"boost/operators.hpp", "utility"},
		{"boost/operators_v1.hpp", "utility"},

		{"boost/detail/winapi/*", "winapi"},
		{"boost/detail/interlocked.hpp", "winapi"},

		{"boost/exception/exception.hpp", "throw_exception"},

		{"boost/blank.hpp", "detail"},
		{"boost/blank_fwd.hpp", "detail"},
		{"boost/cstdlib.hpp", "detail"},

		{"boost/none.hpp", "optional"},
		{"boost/none_t.hpp", "optional"},

		{"boost/implicit_cast.hpp", "conversion"},
		{"boost/polymorphic_cast.hpp", "conversion"},
		{"boost/polymorphic_pointer_cast.hpp", "conversion"},

		{"boost/parameter/python.hpp", "parameter_python"},
		{"boost/parameter/aux_/python/invoker.hpp", "parameter_python"},
		{"boost/parameter/aux_/python/invoker_iterate.hpp", "parameter_python"},

		{"boost/cast.hpp", "numeric_conversion"},

		{"boost/property_map/parallel/*", "property_map.parallel"},
	};

auto get_module_name_inc_path(std::string_view inc_path) -> std::string {
	for(auto&& [non_standard_boost_header, mod] : non_standard_boost_headers) {
		if(non_standard_boost_header.ends_with("*")) {
			auto prefix = non_standard_boost_header.substr(
				0,
				non_standard_boost_header.length() - 1
			);
			if(inc_path.starts_with(prefix)) {
				return std::string{mod};
			}
		}
		if(non_standard_boost_header == inc_path) {
			return std::string{mod};
		}
	}

	std::vector<std::string_view> inc_path_parts = absl::StrSplit(inc_path, '/');

	auto module_name = std::string{inc_path_parts[1]};
	if(module_name.ends_with(".hpp")) {
		module_name = module_name.substr(0, module_name.size() - ".hpp"sv.size());
	}

	return module_name;
}

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

			if(!inc_path_str.starts_with("boost/")) {
				continue;
			}

			auto module_name = get_module_name_inc_path(inc_path_str);

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

auto find_boost_module_name(fs::path inc_dir) -> std::string {
	auto module_names = std::set<std::string>{};
	
	for(auto entry : fs::recursive_directory_iterator(inc_dir)) {
		if(entry.is_directory()) {
			continue;
		}
		
		auto inc_path = fs::relative(entry.path(), inc_dir).generic_string();
		assert(inc_path.starts_with("boost/"));
		auto module_name = get_module_name_inc_path(inc_path);
		if(!module_names.contains(module_name)) {
			module_names.insert(module_name);
		}
	}

	if(module_names.empty()) {
		std::cerr << std::format("Failed to find module name in dir {}\n", inc_dir.generic_string());
		std::exit(1);
	}

	if(module_names.size() > 1) {
		std::cerr << "Module name is ambiguous. Found these names:\n";
		for(auto module_name : module_names) {
			std::cerr << " - " << module_name << "\n";
		}
		std::exit(1);
	}

	return *module_names.begin();
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
		.name = "boost." + find_boost_module_name(include_dir),
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
