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

/* This is just a test kernel to figure out some problems I'm having.
 * It should be deleted once I've figured it out.
 */

#include "led.h"

void pause(int ms) {
    int whole = ms * 1200;
    while (whole > 0) { whole--; }
}

void sendNumber(RaspiLed& led, int number) {
    led.turnOff();
    pause(500);
    while (number > 0) {
        led.turnOn();
        pause(250);
        led.turnOff();
        pause(250);

        number--;
    }
    pause(750);
    led.turnOn();
}

#define MORSE_BASE 100

void sendLetter(RaspiLed& led, char letter) {
    static char codes[][6] = {
        { 1,2,0 }, // A
        { 2,1,1,1,0 }, // B
        { 2,1,2,1,0 }, // C
        { 2,1,1,0 }, // D
        { 1,0 }, // E
        { 1,1,2,1,0 }, // F
        { 2,2,1,0 }, // G
        { 1,1,1,1,0 }, // H
        { 1,1,0 }, // I
        { 1,2,2,2,0 }, // J
        { 2,1,2,0 }, // K
        { 1,2,1,1,0 }, // L
        { 2,2,0 }, // M
        { 2,1,0 }, // N
        { 2,2,2,0 }, // O
        { 1,2,2,1,0 }, // P
        { 2,2,1,2,0 }, // Q
        { 1,2,1,0 }, // R
        { 1,1,1,0 }, // S
        { 2,0 }, // T
        { 1,1,2,0 }, // U
        { 1,1,1,2,0 }, // V
        { 1,2,2,0 }, // W
        { 2,1,1,2,0 }, // X
        { 2,1,2,2,0 }, // Y
        { 2,2,1,1,0 }, // Z
        { 2,2,2,2,2,0 }, // 0
        { 1,2,2,2,2,0 }, // 1
        { 1,1,2,2,2,0 }, // 2
        { 1,1,1,2,2,0 }, // 3
        { 1,1,1,1,2,0 }, // 4
        { 1,1,1,1,1,0 }, // 5
        { 2,1,1,1,1,0 }, // 6
        { 2,2,1,1,1,0 }, // 7
        { 2,2,2,1,1,0 }, // 8
        { 2,2,2,2,1,0 } // 9
    };

    char offset;
    if (letter >= 'a' && letter <= 'z') {
        offset = letter - 'a';
    } else if (letter >= 'A' && letter <= 'Z') {
        offset = letter - 'A';
    } else if (letter >= '0' && letter <= '9') {
        offset = letter - '0' + 26;
    } else if (letter == ' ') {
        pause(MORSE_BASE*4); // 3 already done at end of letter
        return;
    } else {
        return;
    }

    char* morse = codes[(int)offset];

    while (*morse != 0) {
        led.turnOn();
        if (*morse == 1) {
            pause(MORSE_BASE);
        } else {
            pause(MORSE_BASE*3);
        }
        led.turnOff();
        pause(MORSE_BASE);
        morse++;
    }

    pause(MORSE_BASE*2); // already did 1 at the end of the letter
}

void sendText(RaspiLed& led, const char* message) {
    int offset = 0;

    while (message[offset]) {
        sendLetter(led, message[offset]);
        offset++;
    }

    pause(MORSE_BASE*10);
}

extern "C"
void kernel_main(uint32_t r0, uint32_t r1, uint32_t atagsAddress) {

    RaspiLed led;
    led.turnOff();

    while (1) {
        sendText(led, "SOS");
    }
}