#include "pre.h"

#include "test_runner.h"
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
		("log", po::value<string>(), "")
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

	fstream dummy_ofstream;
	test_runner tester = {
		&dummy_ofstream,
		&cerr,
		&dummy_ofstream,
		fs::path(argv[0]),
		!!vm.count("spawn-children"),
		!!vm.count("disable-haali")
	};

#ifdef _WIN32
#ifndef _DEBUG
	SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
#endif
	if (!tester.disable_haali)
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif

	fs::ofstream log_stream;

	if (vm.count("verbose"))
		tester.verbose = &cout;

	if (vm.count("log")) {
		if (vm["log"].as<string>() == "-")
			tester.log = &cout;
		else if (!vm.count("run-regression-test")) {
			log_stream.open(vm["log"].as<string>());
			tester.log = &log_stream;
		}
	}

	FFMS_Init(0, true);

	if (vm.count("run-regression-test")) {
		tester.check_regressions(vm["log"].as<string>());
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

		if (!tester.spawn_children)
			b::for_each(paths, b::bind(&test_runner::run_test, &tester, _1));
		else {
			#pragma omp parallel for
			for (int i = 0; i < (int)paths.size(); ++i) {
				//write_log(out.verbose, i, paths[i]);
				tester.launch_tester(paths[i]);
			}
		}
	}

#ifdef _WIN32
	CoUninitialize();
#endif

	return 0;
}