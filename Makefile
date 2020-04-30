CXX=g++
CXXFLAGS=-std=c++14 -Wextra -Wpedantic -Wall -Werror=vla -MMD -g
OBJECTS=asm.o scanner.o parse_error.o
DEPENDS=${OBJECTS:.o=.d}
EXEC=program
${EXEC}: ${OBJECTS}
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC}
-include ${DEPENDS}
clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}
.PHONY: clean
