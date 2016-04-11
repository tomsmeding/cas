#include "bt.h"
#include <sstream>
#include <execinfo.h>

using namespace std;

string makebacktrace(){
	void *callstack[128];
	int nframes=backtrace(callstack,128);
	char **tracelist=backtrace_symbols(callstack,nframes);
	stringstream ss;
	for(int i=0;i<nframes;i++)ss<<tracelist[i]<<'\n';
	free(tracelist);
	return ss.str();
}
