#include "pre.h"

#include "audio_tester.h"
#include "ffms.h"

class error : public exception {
	string msg;
public:
	int type;
	error(int type, const char *msg) : msg(msg), type(type) { }
	template<class T> error(int type, T msg) : msg(b::str(msg)), type(type) { }
	const char *what() const { return msg.c_str(); }
};

enum ERROR_TYPE {
	ERR_INDEXER = 2,
	ERR_INDEX,
	ERR_NO_AUDIO,
	ERR_INITIAL_DECODE,
	ERR_SEEK
};

struct format : public b::format {
public:
	format(const std::string &s) : b::format(s) { }
	operator std::string() { return b::str( *this ); }
	template <class T> format& operator%(T v) {
		*static_cast<boost::format*>(this) % v;
		return *this;
	}
};

static void read_pcm(fs::path file, vector<uint8_t> &pcm) {
	// don't bother reading any of the format information, as we already know
	// it from opening the source file that produced the index
	file = file.native() + _T(".ffindex");
	fs::ifstream pf(file, ios::binary);
	pf.seekg(0, ios::end);
	size_t length = pf.tellg();
	pf.seekg(112, ios::beg);
	pcm.resize(length - 112);
	pf.read(reinterpret_cast<char *>(&pcm[0]), length - 112);
}

int FFMS_CC dump_file_name(const char *source_file, int, const FFMS_AudioProperties *, char *out, int out_size, void *first) {
	if (!*static_cast<bool*>(first)) return 0;
	if (out) {
		strcpy(out, source_file);
		strcat(out, ".ffindex");
		*static_cast<bool*>(first) = false;
	}
	return strlen(source_file) + sizeof(".ffindex") + 1;
}

static FFMS_AudioSource *init_ffms(fs::path const& file) {
	string sfile = file.string();
	FFMS_Indexer *indexer = FFMS_CreateIndexer(sfile.c_str(), NULL);
	if (!indexer) throw error(ERR_INDEXER, "failed to create indexer; file is not in a supported format");

	bool first = true;
	b::shared_ptr<FFMS_Index> index(FFMS_DoIndexing(indexer, -1, -1, dump_file_name, &first, FFMS_IEH_IGNORE, NULL, NULL, NULL), FFMS_DestroyIndex);
	if (!index) throw error(ERR_INDEX, "failed to create index");

	int track = FFMS_GetFirstTrackOfType(index.get(), FFMS_TYPE_AUDIO, NULL);
	if (track == -1) throw error(ERR_NO_AUDIO, "no audio tracks found");

	return FFMS_CreateAudioSource(sfile.c_str(), track, index.get(), NULL);
}

class decode_tester {
	FFMS_AudioSource *audio_source;
	vector<uint8_t> pcm;
	uint8_t zero_buffer[100];
	uint8_t decode_buffer[15100];

public:
	int bytes_per_sample;
	size_t num_samples;

	decode_tester(fs::path const& test_file) {
		audio_source = init_ffms(test_file);
		const FFMS_AudioProperties *audio_properties = FFMS_GetAudioProperties(audio_source);
		bytes_per_sample = audio_properties->BitsPerSample / 8 * audio_properties->Channels;
		// if NumSamples doesn't fit in size_t, we won't be able to load the decoded file anyway
		num_samples = static_cast<size_t>(audio_properties->NumSamples);
		if (num_samples == 0)
			throw error(ERR_NO_AUDIO, "file has zero audio samples");
		read_pcm(test_file, pcm);
	}
	~decode_tester() {
		 FFMS_DestroyAudioSource(audio_source);
	}
	size_t test(int err_code, int iterations, size_t start_sample) {
		memset(zero_buffer, 68, 100);
		memset(decode_buffer + 15000, 68, 100);

		if (start_sample >= num_samples) assert(false);

		size_t sample_count = min((sizeof(decode_buffer) - 100) / bytes_per_sample, size_t(num_samples - start_sample));

		if (FFMS_GetAudio(audio_source, decode_buffer, start_sample, sample_count, NULL))
			throw error(err_code, format("(%1%) %2%-%3% decode failed") % iterations % start_sample % sample_count);
		if (memcmp(zero_buffer, decode_buffer + 15000, 100))
			throw error(err_code, format("(%1%) %2%-%3% overflow") % iterations % start_sample % sample_count);

		if (!memcmp(decode_buffer, &pcm[start_sample * bytes_per_sample], sample_count * bytes_per_sample))
			return sample_count;

		// either the decoder barfed or it seeked to the wrong place, so check
		// if what we got is some other part of the file
		for (size_t i = 0; i < pcm.size() - sample_count * bytes_per_sample; ++i) {
			if (!memcmp(decode_buffer, &pcm[i], sample_count * bytes_per_sample))
				throw error(err_code, format("(%1%) asked for %2%, got %3%") % iterations % start_sample % (i / bytes_per_sample));
		}

		throw error(err_code, format("(%1%) asked for %2%, got garbage") % iterations % start_sample);
	}
};

static void test(fs::path const& test_file) {
	decode_tester tester(test_file);

	// verify that there is no unnecessary and broken seeking going on
	// all files that can be indexed should pass this
	for (size_t start_sample = 0;
		start_sample < tester.num_samples;
		start_sample += tester.test(ERR_INITIAL_DECODE, 0, start_sample)) ;

	// try seeking to beginning and decoding again; if this fails seeking
	// is probably completely broken
	for (size_t start_sample = 0;
		start_sample < tester.num_samples;
		start_sample += tester.test(ERR_SEEK, -1, start_sample)) ;

	// test random seeking
	for (int iterations = 0; iterations < 10000; ++iterations) {
		tester.test(ERR_SEEK, iterations, (rand() * RAND_MAX + rand()) % tester.num_samples);
	}
}

static void write_log(ostream *log, string str) {
	static b::mutex mutex;
	b::lock_guard<b::mutex> lock(mutex);
	(*log) << str << flush;
}

static void write_log(ostream *log, int err, fs::path path) {
	write_log(log, format("%d %s\n") % err % path);
}

void audio_tester::run_test(fs::path path) {
	if (!fs::exists(path)) {
		(*error) << path << "not found." << endl;
		return;
	}
	if (!fs::is_regular_file(path)) return;

	(*verbose) << path << ": ";
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
	}
}
void audio_tester::launch_tester(fs::path path) {
	vector<string> args;
	args.push_back("--log=-");
	args.push_back(b::erase_all_copy(path.string(), "\""));
	if (disable_haali)
		args.push_back("--disable-haali");
	p::context ctx;
	ctx.streams[p::stdout_id] = p::behavior::pipe();
	p::child c = p::create_child(self.string(), args, ctx);
	c.wait();
	stringstream a;
	a << p::pistream(c.get_handle(p::stdout_id)).rdbuf();
	write_log(log, a.str());
}

static void check_regression(audio_tester &out, int expected_err_code, string const& file_path) {
	stringstream ss;
	out.log = &ss;

	out.launch_tester(file_path);

	if (ss.str().empty()) {
		if (expected_err_code == 1)
			(*out.verbose) << file_path << " crashed as expected" << endl;
		else
			(*out.error) << file_path << " crashed!" << endl;
	}
	else {
		int actual_err_code;
		ss >> actual_err_code;
		if (actual_err_code == expected_err_code)
			(*out.verbose) << file_path << " behaved as expected" << endl;
		else
			(*out.error) << file_path << " expected " << expected_err_code << ", got " << actual_err_code;
	}
}

void audio_tester::check_regressions(fs::path log_path) {
	fs::fstream log_file(log_path);
	if (!log_file.good()) {
		(*error) << "could not open log file " << log_path << endl;
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
			check_regression(*this, files[i].first, files[i].second);
		}
	}
	else {
		for (size_t i = 0; i < files.size(); ++i) {
			check_regression(*this, files[i].first, files[i].second);
		}
	}
}
