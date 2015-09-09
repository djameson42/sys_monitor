RM=rm
CC=gcc
PROG=sys_monitor
CFLAGS=-Wall -g
OBJECTS=${PROG} ${PROG}.o hw_monitor.o

${PROG}: ${PROG}.o hw_info.o
	${CC} ${CFLAGS} ${PROG}.o hw_monitor.o -o ${PROG}

${PROG}.o: ${PROG}.c
	${CC} ${CFLAGS} -c ${PROG}.c

hw_info.o: hw_monitor.c hw_monitor.h
	${CC} ${CFLAGS} -c hw_monitor.c

.PHONY: clean
clean:
	${RM} -f ${OBJECTS}
