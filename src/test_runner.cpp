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

	size_t file_count = files.size();
	size_t files_per_thread = files.size() / 8 + 1;
	b::thread threads[8];
	size_t start = 0;
	foreach (b::thread &t, threads) {
		t = b::move(b::thread(b::bind(&test_runner::check_regression, this, files, start, files_per_thread)));
		start += files_per_thread;
	}
	b::for_each(threads, b::bind(&b::thread::join, _1));
}

void test_runner::check_regression(vector<test_result> files, size_t start, size_t count) {
	for (size_t i = start; i < start+ count && i + files.size(); ++i) {
		test_result &expected = files[i];
		test_result actual = test_function(expected.path);

		static b::mutex mut;
		if (actual == expected) {
			if (verbose) {
				b::lock_guard<b::mutex> lock(mut);
				cerr << expected.path << " behaved as expected" << endl;
			}
		}
		else {
			b::lock_guard<b::mutex> lock(mut);
			if (expected.errcode == ERR_CRASH) {
				cerr << expected.path << " IMPROVEMENT: expected a crash, got " << actual.errcode << endl;
			}
			else if (actual.errcode == ERR_SUCCESS) {
				cerr << expected.path << " IMPROVEMENT: succeeded; expected a failure (" << expected.errcode << ")" << endl;
			}
			else if (expected.errcode == ERR_SUCCESS) {
				cerr << expected.path << " REGRESSION: expected success, got " << actual.errcode << endl;
			}
			else if (expected.errcode == ERR_INITIAL_DECODE && actual.errcode == ERR_SEEK) {
				cerr << expected.path << " IMPROVEMENT: expected initial decode failure, got seek failure" << endl;
			}
			else {
				cerr << expected.path << " CHANGED: expected " << expected.errcode << " got " << actual.errcode << endl;
			}
		}
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
