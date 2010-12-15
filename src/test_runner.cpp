#include "pre.h"

#include "audio_tester.h"
#include "format.h"
#include "test_runner.h"

#include "ffms.h"

static void test(fs::path const& test_file) {
	audio_tester tester(test_file);

	// verify that there is no unnecessary and broken seeking going on
	// all files that can be indexed should pass this
	for (size_t start_sample = 0;
		start_sample < tester.num_samples;
		start_sample += tester.test(ERR_INITIAL_DECODE, 0, start_sample)) ;

	// try seeking to beginning and decoding again; if this fails seeking
	// is probably completely broken
	for (size_t start_sample = 0;
		start_sample < tester.num_samples;
		start_sample += tester.test(ERR_SEEK, -1, start_sample)) ;

	// test random seeking
	for (int iterations = 0; iterations < 10000; ++iterations) {
		tester.test(ERR_SEEK, iterations, (rand() * RAND_MAX + rand()) % tester.num_samples);
	}
}

static void write_log(ostream *log, string str) {
	static b::mutex mutex;
	b::lock_guard<b::mutex> lock(mutex);
	(*log) << str << flush;
}

static void write_log(ostream *log, int err, fs::path path) {
	write_log(log, format("%d %s\n") % err % path);
}

void test_runner::run_test(fs::path path) {
	if (!fs::exists(path)) {
		(*error) << path << "not found." << endl;
		return;
	}
	if (!fs::is_regular_file(path)) return;

	(*verbose) << path << ": ";
	try {
		test(path);
		(*verbose) << "passed" << endl;
		write_log(log, -1, path);
	}
	catch (::error const& e) {
		(*verbose) << e.what() << endl;
		write_log(log, e.type, path);
	}
	catch (exception const& e) {
		(*verbose) << e.what() << endl;
		write_log(log, 0, path);
	}
}
void test_runner::launch_tester(fs::path path) {
	vector<string> args;
	args.push_back("--log=-");
	args.push_back(b::erase_all_copy(path.string(), "\""));
	if (disable_haali)
		args.push_back("--disable-haali");
	p::context ctx;
	ctx.streams[p::stdout_id] = p::behavior::pipe();
	p::child c = p::create_child(self.string(), args, ctx);
	c.wait();
	stringstream a;
	a << p::pistream(c.get_handle(p::stdout_id)).rdbuf();
	write_log(log, a.str());
}

static void check_regression(test_runner &out, int expected_err_code, string const& file_path) {
	stringstream ss;
	out.log = &ss;

	out.launch_tester(file_path);

	if (ss.str().empty()) {
		if (expected_err_code == 1)
			(*out.verbose) << file_path << " crashed as expected" << endl;
		else
			(*out.error) << file_path << " crashed!" << endl;
	}
	else {
		int actual_err_code;
		ss >> actual_err_code;
		if (actual_err_code == expected_err_code)
			(*out.verbose) << file_path << " behaved as expected" << endl;
		else
			(*out.error) << file_path << " expected " << expected_err_code << ", got " << actual_err_code;
	}
}

void test_runner::check_regressions(fs::path log_path) {
	fs::fstream log_file(log_path);
	if (!log_file.good()) {
		(*error) << "could not open log file " << log_path << endl;
		return;
	}
	vector<pair<int, string>> files;
	while (log_file.good()) {
		string file_path;
		int expected_err_code;
		log_file >> expected_err_code;
		getline(log_file, file_path);
		b::trim(file_path);
		files.push_back(make_pair(expected_err_code, file_path));
	}
	if (spawn_children) {
		#pragma omp parallel for
		for (int i = 0; i < (int)files.size(); ++i) {
			check_regression(*this, files[i].first, files[i].second);
		}
	}
	else {
		for (size_t i = 0; i < files.size(); ++i) {
			check_regression(*this, files[i].first, files[i].second);
		}
	}
}
