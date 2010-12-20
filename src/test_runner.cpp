#include "pre.h"

#include "audio_tester.h"
#include "format.h"
#include "ptransform.h"
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
	test_result ret(ss.str());
	if (ret.path.empty()) ret.path = path;
	return ret;
}


test_runner::test_runner(bool verbose, bool spawn_children, string log_path, b::function<test_result (fs::path)> test_function)
: log(0)
, verbose(verbose)
, spawn_children(spawn_children)
, log_path(log_path)
, test_function(test_function)
{
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
		if (!line.empty())
			files.push_back(line);
	}

	if (spawn_children)
		ptransform(files, ostream_iterator<test_result>(cerr), b::protect(b::bind(&test_runner::check_regression, this, _1)));
	else
		transform(files, ostream_iterator<test_result>(cerr), b::protect(b::bind(&test_runner::check_regression, this, _1)));
}

string test_runner::check_regression(test_result expected) {
	test_result actual = test_function(expected.path);

	if (actual == expected) {
		if (verbose) {
			return format("%s behaved as expected\n") % expected.path;
		}
		return "";
	}
	else {
		if (expected.errcode == ERR_CRASH) {
			return format("IMPROVEMENT %s expected a crash, got %d\n") % expected.path % actual.errcode;
		}
		if (actual.errcode == ERR_SUCCESS) {
			return format("IMPROVEMENT %s expected a failure (%d), succeeded\n") % expected.path % expected.errcode;
		}
		if (expected.errcode == ERR_SUCCESS) {
			return format("REGRESSION %s expected success, got %d\n") % expected.path % actual.errcode;
			cerr << expected.path << " REGRESSION: expected success, got " << actual.errcode << endl;
		}
		if (expected.errcode == ERR_INITIAL_DECODE && actual.errcode == ERR_SEEK) {
			return format("IMPROVEMENT %s expected initial decode failure, got seek failure\n") % expected.path;
		}
		return format("CHANGED %s expected %d, got %d\n") % expected.path % expected.errcode % actual.errcode;
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

	#pragma omp critical
	{
		if (!log) {
			if (log_path == "-") {
				log = &cout;
			}
			else {
				log_out_file.open(log_path);
				log = &log_out_file;
			}
		}

		(*log) << (string)result << flush;
	}
}
