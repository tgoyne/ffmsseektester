// because ("%d" % foo).str() is annoying
struct format : public b::format {
public:
	format(const std::string &s) : b::format(s) { }
	operator std::string() { return b::str( *this ); }
	template <class T> format& operator%(T v) {
		*static_cast<boost::format*>(this) % v;
		return *this;
	}
};
