all: syslog_lubuntu

syslog_lubuntu: syslog_lubuntu.o
	gcc -o syslog_lubuntu syslog_lubuntu.o

syslog_lubuntu.o: syslog_lubuntu.c
	gcc -c syslog_lubuntu.c

clean:
		rm -f syslog_lubuntu.o syslog_lubuntu
