#include "pre.h"

#include "format.h"
#include "test_result.h"

test_result::test_result(string str)
{
	vector<string> chunks;
	b::split(chunks, str, b::is_any_of(":"));
	errcode = static_cast<error_type>(b::lexical_cast<int>(chunks[0]));
	path = chunks[1];
	msg = chunks[2];
}

test_result::test_result(int errcode, fs::path path, string msg)
: errcode(static_cast<error_type>(errcode))
, path(path)
, msg(msg)
{
}

test_result::operator string() {
	return format("%d:%s:%s") % errcode % path % msg;
}
bool operator==(test_result const& lft, test_result const& rgt) {
	return lft.errcode == rgt.errcode && lft.path == rgt.path;
}
