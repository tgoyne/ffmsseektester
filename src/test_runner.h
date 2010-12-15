class test_runner {
	ostream *s_verbose;
	ostream *s_err;
	ostream *s_log;

	bool verbose;
	bool spawn_children;
	fs::path log_path;
	b::function<string (fs::path)> test_function;

	void check_regression(int expected_err_code, string const& file_path);
public:
	test_runner(bool verbose, bool spawn_children, string log_path, b::function<string (fs::path)> test_function);

	void run_regression();
	void run_test(fs::path path);
};

class test_spawner {
	fs::path self;
	bool disable_haali;
public:
	test_spawner(fs::path self, bool disable_haali) : self(self), disable_haali(disable_haali) { }
	string operator()(fs::path path);
};
