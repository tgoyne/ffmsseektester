class audio_tester {
public:
	ostream *verbose;
	ostream *error;
	ostream *log;
	fs::path self;
	bool spawn_children;
	bool disable_haali;

	void check_regressions(fs::path log_path);
	void run_test(fs::path path);
	void launch_tester(fs::path path);
};
