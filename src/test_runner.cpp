#include "pre.h"

#include "audio_tester.h"
#include "format.h"
#include "test_runner.h"
#include "test_result.h"

#include "ffms.h"

test_result test_spawner::operator()(fs::path path) {
	vector<string> args;
	args.push_back("--log=-");
	args.push_back(b::erase_all_copy(path.string(), "\""));
	if (disable_haali)
		args.push_back("--disable-haali");
	p::context ctx;
	ctx.streams[p::stdout_id] = p::behavior::pipe();
	p::child c = p::create_child(self.string(), args, ctx);
	c.wait();
	stringstream ss;
	ss << p::pistream(c.get_handle(p::stdout_id)).rdbuf();
	return ss.str();
}


test_runner::test_runner(bool verbose, bool spawn_children, string log_path, b::function<test_result (fs::path)> test_function)
: verbose(verbose)
, spawn_children(spawn_children)
, log_path(log_path)
, test_function(test_function)
{
	if (log_path == "-") {
		log = &cout;
	}
	else {
		log_out_file.open(log_path);
		log = &log_out_file;
	}
}


void test_runner::run_regression() {
	fs::fstream log_file(log_path);
	if (!log_file.good()) {
		cerr << "could not open log file " << log_path << endl;
		return;
	}

	vector<test_result> files;
	while (!log_file.eof() && log_file.good()) {
		string line;
		getline(log_file, line);
		files.push_back(line);
	}

	if (spawn_children) {
		#pragma omp parallel for
		for (int i = 0; i < (int)files.size(); ++i) {
			check_regression(files[i]);
		}
	}
	else {
		for (size_t i = 0; i < files.size(); ++i) {
			check_regression(files[i]);
		}
	}
}

void test_runner::check_regression(test_result expected) {
	test_result actual = test_function(expected.path);

	if (actual == expected) {
		if (verbose) cerr << expected.path << " behaved as expected" << endl;
	}
	else {
		cerr << expected.path << " expected " << expected.errcode << " got " << actual.errcode << endl;
	}
}

void test_runner::run_test(fs::path path) {
	if (!fs::exists(path)) {
		cerr << path << "not found." << endl;
		return;
	}
	if (!fs::is_regular_file(path)) return;

	if (verbose) cerr << path << ": ";
	test_result result = test_function(path);
	if (verbose) {
		if (result.errcode == ERR_SUCCESS)
			cerr << "passed" << endl;
		else
			cerr << result.msg << endl;
	}
	(*log) << (string)result << endl;
}
