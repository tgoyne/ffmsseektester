#include "pre.h"

#include "audio_tester.h"
#include "progress.h"
#include "test_runner.h"
#include "test_result.h"

#include "ffms.h"

static bool extension_filter(fs::path path) {
	static set<string> bad_ext;
	if (bad_ext.empty()) {
		bad_ext += ".ffindex", ".txt", ".exe", ".doc", ".rar", ".zip", ".dll", ".ini", ".zip", ".log";
	}
	return bad_ext.find(b::to_lower_copy(path.extension().string())) != bad_ext.end();
}

int _tmain(int argc, _TCHAR *argv[]) {
	po::options_description opt("Allowed options");
	opt.add_options()
		("help", "")
		("verbose,v", "")
		("log", po::value<string>()->default_value(""), "")
		("run-regression-test", po::value<string>(), "")
		("spawn-children", "spawn child processes for each test to better handle crashes")
		("disable-haali", "don't use Haali's splitters even if ffms2 was built with them")
		("progress", "")
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
	bool enable_progress = !!vm.count("progress");

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
		test_function,
		enable_progress);

#ifdef _WIN32
	if (!spawn_children) SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
	if (!disable_haali)
		CoInitializeEx(0, COINIT_APARTMENTTHREADED);
#endif

	FFMS_Init(0, true);

	if (vm.count("run-regression-test")) {
		tester.run_regression(vm["run-regression-test"].as<string>());
	}
	else {
		vector<fs::path> paths;
		fs::recursive_directory_iterator end;
		foreach (string path, vm["path"].as<vector<string>>()) {
			if (fs::is_directory(path))
				copy(fs::recursive_directory_iterator(path), end, back_inserter(paths));
			else
				paths.push_back(path);
		}
		paths.erase(b::remove_if(paths, extension_filter), paths.end());
		tester.run(paths);

		if (enable_progress) cout << endl;
	}

#ifdef _WIN32
	CoUninitialize();
#endif

	return 0;
}