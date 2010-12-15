enum error_type {
	ERR_SUCCESS = 0,
	ERR_UNKNOWN,
	ERR_CRASH,
	ERR_INDEXER,
	ERR_INDEX,
	ERR_NO_AUDIO,
	ERR_INITIAL_DECODE,
	ERR_SEEK
};

struct test_result {
	error_type errcode;
	fs::path path;
	string msg;

	test_result(string);
	test_result(int errcode, fs::path path, string msg = "");

	operator string();
};
bool operator==(test_result const& lft, test_result const& rgt);