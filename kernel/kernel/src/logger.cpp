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

#include "logger.h"
#include "print.h"

Logger* Logger::instance = nullptr;

Logger::Logger(SdpServer& sdpServer)
        : sdpServer(sdpServer) {
    instance = this;
}

void Logger::log(LogLevel level, const char* module, const char* format, va_list args) {
    char buffer[512];
    vsnprintf(buffer, 512, format, args);
    sdpServer.log(level, module, buffer);
}

void Logger::log(LogLevel level, const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(level, module, format, args);
    va_end(args);
}

void Logger::fatal(const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOGLEVEL_FATAL, module, format, args);
    va_end(args);
}

void Logger::error(const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOGLEVEL_ERROR, module, format, args);
    va_end(args);
}

void Logger::warning(const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOGLEVEL_WARNING, module, format, args);
    va_end(args);
}

void Logger::info(const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOGLEVEL_INFO, module, format, args);
    va_end(args);
}

void Logger::debug(const char* module, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOGLEVEL_DEBUG, module, format, args);
    va_end(args);
}
