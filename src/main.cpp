#include "pre.h"

#include "audio_tester.h"
#include "test_runner.h"
#include "test_result.h"

#include "ffms.h"

static bool extension_filter(fs::path path) {
	static set<string> bad_ext;
	if (bad_ext.empty()) {
		bad_ext += ".ffindex", ".txt", ".exe", ".doc", ".EXE", ".rar", ".zip", ".dll", ".DOC", ".ini", ".zip";
	}
	return find(bad_ext.begin(), bad_ext.end(), path.extension().string()) == bad_ext.end();
}

int _tmain(int argc, _TCHAR *argv[]) {
	po::options_description opt("Allowed options");
	opt.add_options()
		("help", "")
		("verbose,v", "")
		("log", po::value<string>()->default_value(""), "")
		("run-regression-test", "")
		("spawn-children", "spawn child processes for each test to better handle crashes")
		("disable-haali", "don't use Haali's splitters even if ffms2 was built with them")
		("path", po::value<vector<string>>(), "path to files to test");

	po::positional_options_description p;
	p.add("path", -1);

	po::variables_map vm;
	try {
		po::store(command_line_parser(argc, argv).options(opt).positional(p).run(), vm);
		po::notify(vm);
	}
	catch (exception const& e) {
		cerr << e.what() << endl;
		return 1;
	}

	if (vm.count("help") ||
		(!vm.count("path") && !vm.count("run-regression-test")) ||
		(vm.count("run-regression-test") && !vm.count("log"))) {
			cerr << "Usage: ffmstest [options] input-file...\n" << opt << endl;
			return 1;
	}

	bool disable_haali = !!vm.count("disable-haali");
	bool spawn_children = !!vm.count("spawn-children");

	b::function<test_result (fs::path)> test_function;
	if (!spawn_children) {
		test_function = run_audio_test;
	}
	else {
		test_function = test_spawner(argv[0], disable_haali);
	}

	test_runner tester(
		!!vm.count("verbose"),
		spawn_children,
		vm["log"].as<string>(),
		test_function);

#ifdef _WIN32
#ifndef _DEBUG
	SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
#endif
	if (!disable_haali)
		CoInitializeEx(0, COINIT_APARTMENTTHREADED);
#endif

	FFMS_Init(0, true);

	if (vm.count("run-regression-test")) {
		tester.run_regression();
	}
	else {
		vector<fs::path> paths;
		foreach (string path, vm["path"].as<vector<string>>() | a::filtered(extension_filter)) {
			if (fs::is_directory(path))
				b::copy(
				b::make_iterator_range(fs::recursive_directory_iterator(path), fs::recursive_directory_iterator()) | a::filtered(extension_filter),
				back_inserter(paths));
			else
				paths.push_back(path);
		}

		if (!spawn_children)
			b::for_each(paths, b::bind(&test_runner::run_test, &tester, _1));
		else {
			#pragma omp parallel for
			for (int i = 0; i < (int)paths.size(); ++i) {
				tester.run_test(paths[i]);
			}
		}
	}

#ifdef _WIN32
	CoUninitialize();
#endif

	return 0;
}