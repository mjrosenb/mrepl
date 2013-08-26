LDFLAGS=`pkg-config --libs ncursesw` -lreadline -lstdc++
LD=$(CXX)
CXXFLAGS=-g -Wall
CC=clang
CXX=clang++
SRCS=MREPL.cc FrontEnd.cc Line.cc MiddleEnd.cc BackEnd.cc ParseOutput.cc Machine_$(shell uname -m).cc
OBJS=$(patsubst %.cc,%.o,$(SRCS))
DEPS=$(patsubst %.cc,%.d,$(SRCS))
# CXX=clang++

mrepl: $(OBJS)
	$(CXX) $(LDFLAGS)  $^ -o $@
#.objs/%.o: %.cc
#	$(CXX) $(CXXFLAGS) -c $< -o $@
%.d: %.cc
	$(CXX) -MM $(CXXFLAGS) $< -MF $@

.PHONY : clean
clean:
	rm $(OBJS) $(DEPS) mrepl


check-syntax:
	$(CXX) -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

ifeq (0,$(words $(filter %clean,$(MAKECMDGOALS))))
# ifeq (,$(findstring check-syntax,$(MAKECMDGOALS)))
-include $(DEPS)
#endif
endif
