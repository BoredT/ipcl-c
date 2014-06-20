all: ipcl-c

OBJS = parser.o  \
       codegen.o \
       main.o    \
       tokens.o  \
       corefn.o  \

CPPFLAGS = `llvm-config --cppflags`
LDFLAGS = `llvm-config --ldflags`
LIBS = `llvm-config --libs`

clean:
	$(RM) -rf parser.cpp parser.hpp ipcl-c tokens.cpp $(OBJS)

parser.cpp: parser.y
	bison -d -o $@ $^
	
parser.hpp: parser.cpp

tokens.cpp: tokens.l parser.hpp
	flex -o $@ $^

%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $@ $<


ipcl-c: $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

compile:
	./ipcl-c $(source) && llc main.bc && g++ main.s && ./a.out


