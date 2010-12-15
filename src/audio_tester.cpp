#include "pre.h"

#include "audio_tester.h"
#include "format.h"

#include "ffms.h"

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

audio_tester::audio_tester(fs::path const& test_file) {
	audio_source = init_ffms(test_file);
	const FFMS_AudioProperties *audio_properties = FFMS_GetAudioProperties(audio_source);
	bytes_per_sample = audio_properties->BitsPerSample / 8 * audio_properties->Channels;
	// if NumSamples doesn't fit in size_t, we won't be able to load the decoded file anyway
	num_samples = static_cast<size_t>(audio_properties->NumSamples);
	if (num_samples == 0)
		throw error(ERR_NO_AUDIO, "file has zero audio samples");
	read_pcm(test_file, pcm);
}

audio_tester::~audio_tester() {
	FFMS_DestroyAudioSource(audio_source);
}


size_t audio_tester::test(int err_code, int iterations, size_t start_sample) {
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
