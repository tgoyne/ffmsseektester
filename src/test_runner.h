class test_runner {
	fs::ofstream log_out_file;
	ostream *log;

	bool verbose;
	bool spawn_children;
	b::function<test_result (fs::path)> test_function;
	bool enable_progress;

	string check_regression(test_result file, progress &prog);
	string run_test(fs::path path, progress &prog);
public:
	test_runner(bool verbose, bool spawn_children, string log_path, b::function<test_result (fs::path)> test_function, bool enable_progress);

	void run_regression(fs::path path);
	void run(vector<fs::path> const& paths);
};

class test_spawner {
	fs::path self;
	bool disable_haali;
public:
	test_spawner(fs::path self, bool disable_haali) : self(self), disable_haali(disable_haali) { }
	test_result operator()(fs::path path);
};
