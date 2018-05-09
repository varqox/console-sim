SRCS=main.cpp http/http.cpp objects/contests.cpp


CXX=clang++
LDLIBS=$(shell curl-config --libs)
CPPFLAGS=-std=c++14 -Wall -Wextra
RM=rm -f

OBJS=$(subst .cpp,.o,$(SRCS))

all: main

main: $(OBJS)
	$(CXX) $(CPPFLAGS) -o main $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)
	rm main
	rm .depend

include .depend
