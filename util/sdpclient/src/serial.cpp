/* Copyright (c) 2016, Kulshan Concepts
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "serial.h"

#define MODULE "ClientSerial"

Serial::Serial(const std::string& device, Logger& logger) : logger(logger), abortFlag(false) {
    fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (-1 == fd) {
        throw SerialException("Could not open device");
    }

    if (!isatty(fd)) {
        close(fd);
        throw SerialException("Specified device is not a TTY");
    }

    termios termAttr;
    if (-1 == tcgetattr(fd, &termAttr)) {
        close(fd);
        throw SerialException("Could not get device attributes");
    }

    termAttr.c_cc[VTIME] = 0;
    termAttr.c_cc[VMIN] = 0;

    termAttr.c_iflag = 0;
    termAttr.c_oflag = 0;
    termAttr.c_cflag = CS8 | CREAD | CLOCAL;
    termAttr.c_lflag = 0;

    if (0 > cfsetispeed(&termAttr, B115200) ||
        0 > cfsetospeed(&termAttr, B115200)) {

        close(fd);
        throw SerialException("Could not set device baud rate");
    }

    if (-1 == tcsetattr(fd, TCSAFLUSH, &termAttr)) {
        close(fd);
        throw SerialException("Could not set device attributes");
    }
}

Serial::~Serial() {
    if (-1 != fd) {
        close(fd);
    }
}

bool Serial::isOpen() const {
    return -1 != fd;
}

void Serial::write(const char* data, size_t length) {
    ssize_t bw = 0;
    logger.debug(MODULE, "Writing %u bytes", length);
    while (length > 0) {
        waitToWrite();
        bw  = ::write(fd, data, length);
        if (-1 == bw) {
            throw SerialException("Could not write data");
        }

        length -= bw;
        data += bw;
    }
}

size_t Serial::read(char* data, size_t length) {
    waitForData();
    logger.debug(MODULE, "Reading data");
    ssize_t br = ::read(fd, data, length);
    logger.debug(MODULE, "Read %u bytes", br);
    if (br == -1) {
        throw SerialException("Could not read data");
    }
    return br;
}

void Serial::waitForData() {
    fd_set readset;
    int result;

    logger.debug(MODULE, "Waiting for data");

    do {
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        timeval timeout = {0, 100};
        result = select(fd + 1, &readset, nullptr, nullptr, &timeout);
    } while (result == 0 && !abortFlag);

    logger.debug(MODULE, "Got some data? %d %d %s", result, errno, abortFlag ? "true" : "false");

    if (abortFlag) {
        throw SerialException("Aborted by client request");
    }

    if (result < 0) {
        throw SerialException("Lost serial connection during select");
    }
}

void Serial::waitToWrite() {
    fd_set writeset;
    int result;

    logger.debug(MODULE, "Waiting to write");

    do {
        FD_ZERO(&writeset);
        FD_SET(fd, &writeset);
        timeval timeout = {0, 100};
        result = select(fd + 1, nullptr, &writeset, nullptr, &timeout);
    } while (result == 0 && !abortFlag);

    if (abortFlag) {
        throw SerialException("Aborted by client request");
    }

    if (result < 0) {
        throw SerialException("Lost serial connection during select");
    }
}
