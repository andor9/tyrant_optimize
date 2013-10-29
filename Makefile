MAIN := tyrant_optimize
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,obj/%.o,$(SRCS))

CPPFLAGS := -Wall -Werror -std=gnu++11 -O3
LDFLAGS := -lboost_system -lboost_thread -lboost_filesystem

MKDIR_P = mkdir -p

.PHONY: directories

all: directories $(MAIN)

obj/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -o $@ -c $<

$(MAIN): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

directories: obj/

obj/:
	${MKDIR_P} $@

clean:
	rm -f $(MAIN) obj/*.o
