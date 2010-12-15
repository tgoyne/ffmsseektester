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

class FFMS_AudioSource;
class audio_tester {
	FFMS_AudioSource *audio_source;
	vector<uint8_t> pcm;
	uint8_t zero_buffer[100];
	uint8_t decode_buffer[15100];

public:
	int bytes_per_sample;
	size_t num_samples;

	audio_tester(fs::path const& test_file);
	~audio_tester();
	size_t test(int err_code, int iterations, size_t start_sample);
};
