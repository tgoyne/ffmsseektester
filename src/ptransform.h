template<class Value, class OutputIterator, class MutexType>
void locked_apply(Value const& value, OutputIterator out, MutexType *mutex) {
	b::lock_guard<MutexType> lock(*mutex);
	*out = value;
}

template<class InputIterator, class OutputIterator, class UnaryOperation, class MutexType>
void ptransform_wrapper(b::function<InputIterator()> itsource, InputIterator end, OutputIterator out, UnaryOperation func, MutexType *mutex) {
	InputIterator it;
	while ((it = itsource()) != end) {
		locked_apply(func(*it), out, mutex);
	}
}

template<class InputIterator, class MutexType>
InputIterator ptransform_next(InputIterator *cur, InputIterator const& end, MutexType *mutex) {
	b::lock_guard<MutexType> lock(*mutex);
	if (*cur == end) return end;
	return (*cur)++;
}

template<class ForwardRange, class OutputIterator, class UnaryOperation>
void ptransform(ForwardRange const& rng, OutputIterator out, UnaryOperation func) {
	typedef b::range_const_iterator<ForwardRange>::type InputIterator;
	unsigned thread_count = b::thread::hardware_concurrency() * 2;
	b::thread_group threads;

	InputIterator it = b::begin(rng);
	InputIterator end = b::end(rng);
	b::mutex mutex;
	for (unsigned i = 0; i < thread_count; ++i) {
		threads.create_thread(
			b::bind(
				ptransform_wrapper<InputIterator, OutputIterator, UnaryOperation, b::mutex>,
				b::protect(b::bind(ptransform_next<InputIterator, b::mutex>, &it, end, &mutex)),
				end, out, func, &mutex));
	}
	threads.join_all();
}
