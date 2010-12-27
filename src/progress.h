class progress : b::noncopyable {
	b::timer timer;
	size_t total;
	double current;
	b::mutex mutex;

public:
	progress(size_t total);
	progress& operator++();
};
