#include <stdint.h>

class error : public exception {
	string msg;
public:
	int type;
	error(int type, const char *msg) : msg(msg), type(type) { }
	template<class T> error(int type, T msg) : msg(b::str(msg)), type(type) { }
	const char *what() const { return msg.c_str(); }
};

enum ERROR_TYPE {
	ERR_SUCCESS = 0,
	ERR_UNKNOWN,
	ERR_CRASH,
	ERR_INDEXER,
	ERR_INDEX,
	ERR_NO_AUDIO,
	ERR_INITIAL_DECODE,
	ERR_SEEK
};

string run_audio_test(fs::path const& path);
