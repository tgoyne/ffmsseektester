enum error_type {
	ERR_SUCCESS = 0,
	ERR_SEEK,
	ERR_INITIAL_DECODE,
	ERR_NO_AUDIO,
	ERR_INDEX,
	ERR_INDEXER,
	ERR_UNKNOWN,
	ERR_CRASH,
};

struct test_result {
	error_type errcode;
	fs::path path;
	string msg;

	test_result(string);
	test_result(int errcode, fs::path path, string msg = "");

	test_result& operator=(test_result const& rgt);
	operator string() const;
};
bool operator==(test_result const& lft, test_result const& rgt);

template<class T>
T& operator<<(T& dst, test_result const& test) {
	return dst << (string)test;
}
