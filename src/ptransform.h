#undef min

template<class InputIterator, class OutputIterator, class UnaryOperation, class MutexType>
void ptransform_wrapper(InputIterator begin, InputIterator end, OutputIterator out, UnaryOperation func, MutexType &mutex) {
	for (; begin != end; ++begin, ++out) {
		BOOST_AUTO_TPL(res, func(*begin));
		b::lock_guard<MutexType> lock(mutex);
		*out = res;
	}
}

template<class RandomAccessRange, class OutputIterator, class UnaryOperation>
void ptransform(RandomAccessRange const& rng, OutputIterator out, UnaryOperation func) {
	typedef b::range_difference<RandomAccessRange>::type size_type;
	typedef b::range_const_iterator<RandomAccessRange>::type InputIterator;
	size_type count = b::distance(rng);
	unsigned thread_count = b::thread::hardware_concurrency();
	b::thread_group threads;

	InputIterator begin = b::begin(rng);
	b::mutex mutex;
	for (unsigned i = 0; i < thread_count; ++i) {
		InputIterator end = begin + min<size_type>(count / thread_count + 1, distance(begin, b::end(rng)));
		threads.create_thread(
			b::bind(
				ptransform_wrapper<InputIterator, OutputIterator, UnaryOperation, b::mutex>,
				begin, end, out, func, b::ref(mutex)));
		begin = end;
	}
	threads.join_all();
}
