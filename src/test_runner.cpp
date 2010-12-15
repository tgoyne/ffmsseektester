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


test_runner::test_runner(bool verbose, bool spawn_children, string log_path, b::function<string (fs::path)> test_function)
: verbose(verbose)
, spawn_children(spawn_children)
, log_path(log_path)
, test_function(test_function)
{

}


void test_runner::run_regression() {
	fs::fstream log_file(log_path);
	if (!log_file.good()) {
		(*s_err) << "could not open log file " << log_path << endl;
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
			check_regression(files[i].first, files[i].second);
		}
	}
	else {
		for (size_t i = 0; i < files.size(); ++i) {
			check_regression(files[i].first, files[i].second);
		}
	}
}

void test_runner::check_regression(int expected_err_code, string const& file_path) {
	string res = test_function(file_path);

	if (res.empty()) {
		if (expected_err_code == 1)
			(*s_verbose) << file_path << " crashed as expected" << endl;
		else
			(*s_err) << file_path << " crashed!" << endl;
	}
	else {
		int actual_err_code = b::lexical_cast<int>(res);
		if (actual_err_code == expected_err_code)
			(*s_verbose) << file_path << " behaved as expected" << endl;
		else
			(*s_err) << file_path << " expected " << expected_err_code << ", got " << actual_err_code;
	}
}

void test_runner::run_test(fs::path path) {
	if (!fs::exists(path)) {
		(*s_err) << path << "not found." << endl;
		return;
	}
	if (!fs::is_regular_file(path)) return;

	test_result result = test_function(path);
	/*(*verbose) << path << ": ";
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
	}*/
}
