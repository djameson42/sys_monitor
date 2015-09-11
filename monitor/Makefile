RM=rm
CC=gcc
PROG=sys_monitor
CFLAGS=-Wall
OBJECTS=${PROG} ${PROG}.o hw_monitor.o

${PROG}: ${PROG}.o hw_info.o
	${CC} ${CFLAGS} ${PROG}.o hw_monitor.o -o ${PROG} -lpthread

${PROG}.o: ${PROG}.c
	${CC} ${CFLAGS} -c ${PROG}.c

hw_info.o: hw_monitor.c hw_monitor.h
	${CC} ${CFLAGS} -c hw_monitor.c -lpthread

.PHONY: clean
clean:
	${RM} -f ${OBJECTS}
