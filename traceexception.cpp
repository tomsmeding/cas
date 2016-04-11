#include "traceexception.h"
#include <sstream>
#include <execinfo.h> //backtrace

TraceException::TraceException(const string &s){
	void *callstack[128];
	int nframes=backtrace(callstack,128);
	char **tracelist=backtrace_symbols(callstack,nframes);
	stringstream ss;
	for(int i=0;i<nframes;i++)ss<<tracelist[i]<<'\n';
	free(tracelist);
	tracestr=ss.str();

	size_t whatsize=s.size();
	size_t tracesize=tracestr.size();
	if(whatsize+1+tracesize>1023)tracesize=1023-whatsize-1;
	memcpy(buf,s.data(),whatsize);
	buf[whatsize]='\n';
	memcpy(buf+whatsize+1,tracestr.data(),tracesize);
	buf[whatsize+1+tracesize]='\0';
}

const char* TraceException::what() const noexcept {
	return buf;
}

const char* TraceException::trace() const noexcept {
	return tracestr.data();
}
