#include "pre.h"

#include "progress.h"

progress::progress(size_t total) : total(total), current(0.) {

}
progress& progress::operator++() {
	if (total == 0) return *this;

	b::lock_guard<b::mutex> lock(mutex);
	++current;

	double percent = current / total;
	double sqpercent = pow(percent, 1.5);
	double eta = timer.elapsed() / sqpercent * (1 - sqpercent);
	cout << "\r                                                      " << flush;
	cout << "\r" << current << "/" << total << ": " << percent * 100 << "% " << eta << flush;

	return *this;
}
