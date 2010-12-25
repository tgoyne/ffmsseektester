class test_runner {
	fs::ofstream log_out_file;
	ostream *log;

	bool verbose;
	bool spawn_children;
	fs::path log_path;
	b::function<test_result (fs::path)> test_function;

	string check_regression(test_result file, progress &prog);
public:
	test_runner(bool verbose, bool spawn_children, string log_path, b::function<test_result (fs::path)> test_function);

	void run_regression(bool show_progress);
	void run_test(fs::path path, progress &prog);
};

class test_spawner {
	fs::path self;
	bool disable_haali;
public:
	test_spawner(fs::path self, bool disable_haali) : self(self), disable_haali(disable_haali) { }
	test_result operator()(fs::path path);
};
