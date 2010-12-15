#include <stdint.h>

struct test_result;
class error : public exception {
	string msg;
public:
	int type;
	error(int type, const char *msg) : msg(msg), type(type) { }
	template<class T> error(int type, T msg) : msg(b::str(msg)), type(type) { }
	const char *what() const { return msg.c_str(); }
};

test_result run_audio_test(fs::path const& path);
