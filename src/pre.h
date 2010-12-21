#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED
#define WIN32_LEAN_AND_MEAN

#include <boost/algorithm/string.hpp>
#include <boost/assign/std/set.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/iterator.hpp>
#include <boost/process.hpp>
#include <boost/progress.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/typeof/typeof.hpp>
#include <iostream>
#include <omp.h>
#include <vector>

using namespace std;
using namespace boost::assign;
namespace b = boost;
namespace a = b::adaptors;
namespace p = b::process;
namespace po = b::program_options;
namespace fs = b::filesystem;

#define foreach BOOST_FOREACH

#ifdef _WIN32
#include <objbase.h>
#include <tchar.h>
typedef po::wcommand_line_parser command_line_parser;
#else
#define _T(x) x
#define _tmain main
#define _TCHAR char
typedef po::command_line_parser command_line_parser;
#endif

#undef INTMAX_C
#undef UINTMAX_C
