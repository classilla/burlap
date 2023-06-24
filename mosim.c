/*
 * A simple modem simulator:
 * - Respond to Hayes '302, TIES or TIES-like +++ sequences with OK.
 * - Respond to ATD... sequences with CONNECT (and terminate, leaving the
 *   connection open).
 * - Respond to all other AT commands with OK.
 *
 * Copyright (C) 2023 Cameron Kaiser. All rights reserved.
 * Floodgap Free Software License.
 * http://oldvcr.blogspot.com/
 *
 * Usage: mosim <path to port> <port speed>
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int port, pid = 0;
struct termios tty, tty_saved, tty_saved_in, tty_saved_out, tty_saved_err;
struct sigaction sig;

speed_t
baudr(int baud) {
	switch (baud) {
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		default: return B0;
	}
	return B0; /* not reached */
}

void
cleanup()
{
	fprintf(stderr, "restoring terminal settings\n");
	(void)tcsetattr(port, TCSAFLUSH, &tty_saved);
	(void)close(port);
	(void)tcsetattr(STDIN_FILENO, TCSANOW, &tty_saved_in);
	(void)tcsetattr(STDOUT_FILENO, TCSANOW, &tty_saved_out);
	(void)tcsetattr(STDERR_FILENO, TCSANOW, &tty_saved_err);
}

void
deadme()
{
	fprintf(stderr, "terminating on user signal\n");
	exit(0);
}

int
main(int argc, char **argv)
{
	int port, speed, infd, outfd, rv, bytes;
	fd_set master_rfd, rfd, master_wfd, wfd;
	struct timeval tv;
	unsigned char c;
	unsigned char buf[256];
	speed_t rate;

	/* it's not enough to just restore the serial port settings */
	if (tcgetattr(STDIN_FILENO, &tty_saved_in) ||
			tcgetattr(STDOUT_FILENO, &tty_saved_out) ||
			tcgetattr(STDERR_FILENO, &tty_saved_err)) {
		perror("tcgetattr (tty)");
		return 1;
	}

	if (argc < 2) {
		fprintf(stderr, "missing path to serial port (e.g., /dev/cu.usbserial)\n");
		return 1;
	}
	if (argc < 3) {
		fprintf(stderr, "missing serial port speed\n");
		return 1;
	}
	speed = atoi(argv[2]);
	if ((speed == 0) || ((rate = baudr(speed)) == B0)) {
		fprintf(stderr, "unsupported baud rate %d\n", speed);
		return 1;
	}

	fprintf(stderr, "opening %s\n", argv[1]);
	port = open(argv[1], O_RDWR | O_NOCTTY);
	if (!isatty(port)) {
		fprintf(stderr, "warning: isatty returned false for this path\n");
	}
	fprintf(stderr, "setting up for serial access\n");
	if (tcgetattr(port, &tty)) {
		perror("tcgetattr");
		close(port);
		return 1;
	}
	tty_saved = tty;

	fprintf(stderr, "setting flags on serial port fd=%d\n", port);
	memset(&tty, 0, sizeof(tty));
	tty.c_cflag |= (CS8 | CREAD | CLOCAL);
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 1;
	(void)cfsetospeed(&tty, rate);
	(void)cfsetispeed(&tty, rate);
	if (tcsetattr(port, TCSAFLUSH, &tty)) {
		perror("tcsetattr (tty)");
		close(port);
		return 1;
	}
	atexit(cleanup);
	sig.sa_handler = deadme;
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGHUP, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL); /* yolo */

/* all blocking I/O */

#define READ read(port, &c, 1);fputc(c, stderr);
#define NL fputc(10, stderr);
#define WRITE(x) sprintf(buf, x "\r\n");write(port, &buf, strlen(buf));fprintf(stderr, "%s", buf);
	for(;;) {
		READ

		if (c == '+') {
			READ
			READ
			NL
			WRITE("OK");
			continue;
		}
		if (c == 'A') {
			unsigned char d = 0;

			READ
			READ
			d = c;
			for(;;) {
				READ
				if (c == 13) break;
			}
			NL
			if (d == 'D') {
				WRITE("CONNECT");
				break;
			} else {
				WRITE("OK");
				continue;
			}
		}
	}
	exit(0);
	return 255; /* not reached */
}
