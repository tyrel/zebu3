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

#include "exception.h"
#include "mmio.h"
#include "thread.h"
#include "logger.h"

#define VECTOR_TABLE_SIZE 32

#define TIMER_BASE 0x00003000

#define TIMER_CONTROL_AND_STATUS TIMER_BASE
#define TIMER_COUNTER_LOW TIMER_BASE+0x04
#define TIMER_COUNTER_HIGH TIMER_BASE+0x08
#define TIMER_COMPARE_1 TIMER_BASE+0x10
#define TIMER_COMPARE_3 TIMER_BASE+0x18

#define CLEAR_TIMER_1 0x20 // write these to TIMER_CONTROL_AND_STATUS to clear interrupt
#define CLEAR_TIMER_3 0x80

#define PIC_BASE 0xB000

#define PIC_IRQ_BASIC_PENDING PIC_BASE+0x200
#define PIC_IRQ_PENDING_1 PIC_BASE+0x204
#define PIC_IRQ_PENDING_2 PIC_BASE+0x208
#define PIC_FIQ_CONTROL PIC_BASE+0x20C
#define PIC_ENABLE_IRQ_1 PIC_BASE+0x210
#define PIC_ENABLE_IRQ_2 PIC_BASE+0x214
#define PIC_ENABLE_BASIC_IRQ PIC_BASE+0x218
#define PIC_DISABLE_IRQ_1 PIC_BASE+0x21C
#define PIC_DISABLE_IRQ_2 PIC_BASE+0x220
#define PIC_DISABLE_BASIC_IRQ PIC_BASE+0x224

#define EX_OFFSET_UNDEFINED 1
#define EX_OFFSET_SVCCALL 2
#define EX_OFFSET_PREFETCH_ABORT 3
#define EX_OFFSET_DATA_ABORT 4
#define EX_OFFSET_IRQ 6
#define EX_OFFSET_FIQ 7

#define IRQ_TIMER_0 0x00
#define IRQ_TIMER_1 0x01
#define IRQ_TIMER_2 0x02
#define IRQ_TIMER_3 0x03

// TODO: this should probably be determined dynamically
#define KERNEL_EXCEPTION_STACK 0x7000

/** Get the CPSR register
 */
uint32_t getCpsr() {
	uint32_t r;
	asm("mrs %[ps], cpsr" : [ps]"=r" (r));
	return r;
}

/** Set the CPSR register
 */
void setCpsr(uint32_t r) {
	asm("msr cpsr, %[ps]" : : [ps]"r" (r));
}

/** Get the SPSR register
 */
uint32_t getSpsr() {
	uint32_t r;
	asm("mrs %[ps], spsr" : [ps]"=r" (r));
	return r;
}

void enableFIQ() {
	setCpsr(getCpsr() & ~(1 << 6));
}

void enableIRQ() {
	setCpsr(getCpsr() & ~(1 << 7));
}

static bool firstSwitch = true;

void exceptionHandler(uint32_t lr, uint32_t type) {

	Logger::getInstance()->info("Exception", "Handing %X %X", lr, type);

	if (type == EX_OFFSET_IRQ) {
		// TODO find out which IRQ it is?

		if (/*that number == */IRQ_TIMER_1) {

			// clear the IRQ
			mmio_write(TIMER_CONTROL_AND_STATUS, 1 << 1);

			Thread* thread;

			// The very first time we get this interrupt, we don't have registers
			// to store.
			if (!firstSwitch) {
				thread = Thread::getCurrentThread();
				thread->pc = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-1];
				thread->r12 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-2];
				thread->r11 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-3];
				thread->r10 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-4];
				thread->r9 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-5];
				thread->r8 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-6];
				thread->r7 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-7];
				thread->r6 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-8];
				thread->r5 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-9];
				thread->r4 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-10];
				thread->r3 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-11];
				thread->r2 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-12];
				thread->r1 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-13];
				thread->r0 = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-14];
				thread->cpsr = ((uint32_t*)KERNEL_EXCEPTION_STACK)[-15];

				// To get the sp and lr we need to go into system mode
				uint32_t savedSP;
				uint32_t savedLR;

				asm("mrs r0, cpsr \n"
					"bic r0, r0, #0x1f \n"
					"orr r0, r0, #0x1f \n"
					"msr cpsr, r0 \n" /* now in system mode */
					"mov %[sp], sp \n"
					"mov %[lr], lr \n"
					"bic r0, r0, #0x1f \n"
					"orr r0, r0, #0x12 \n"
					"msr cpsr, r0 \n" /* and back to exception mode */
					: [sp]"=r" (savedSP), [lr]"=r" (savedLR));

				thread->sp = savedSP;
				thread->lr = savedLR;
			}

			thread = Thread::getNextReady();

			firstSwitch = false;

			// Now reaturing to a thread. We need to restore its registers.
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-1] = thread->pc;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-2] = thread->r12;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-3] = thread->r11;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-4] = thread->r10;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-5] = thread->r9;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-6] = thread->r8;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-7] = thread->r7;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-8] = thread->r6;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-9] = thread->r5;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-10] = thread->r4;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-11] = thread->r3;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-12] = thread->r2;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-13] = thread->r1;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-14] = thread->r0;
			((uint32_t*)KERNEL_EXCEPTION_STACK)[-15] = thread->cpsr;

			// again, we need system mode to get sp and lr
			asm("mrs r0, cpsr \n"
				"bic r0, r0, #0x1f \n"
				"orr r0, r0, #0x1f \n"
				"msr cpsr, r0 \n" /* now in system mode */
				"mov sp, %[sp] \n"
				"mov lr, %[lr] \n"
				"bic r0, r0, #0x1f \n"
				"orr r0, r0, #0x12 \n"
				"msr cpsr, r0 \n" /* and back to exception mode */
				: [sp]"=r" (thread->sp), [lr]"=r" (thread->lr));
		} else {
			// unknown IRQ. do we care?
		}
	} else {
		Logger::getInstance()->fatal("Exception", "Unhandled exception");
		while (1) { }
	}
}

static uint32_t timer_last = 0;

static void timer_update() {
	// timer runs a 1 MHz, and we want to get a hit every 10 ms,
	// or 100 times per second, so the next hit will be 1 million / 100
	// units from now, or 10,000 ticks of the timer
	timer_last += 10000;
	mmio_write(TIMER_COMPARE_1, timer_last);
}

#define EXCEPTION_TOP_SWI \
	uint32_t lr; \
	asm("mov sp, %[ps]" : : [ps]"i" (KERNEL_EXCEPTION_STACK)); /* set sp(r13) */ \
	asm("push {lr}"); /* save lr(r14) */ \
	asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); /* save others */ \
	asm("mrs r0, spsr"); /* get spsr to save it */ \
	asm("push {r0}");  /* save spsr */ \
	asm("mov %[ps], lr" : [ps]"=r" (lr)); /* get new lr */

#define EXCEPTION_BOTTOM_SWI \
	asm("pop {r0}"); /* get old value of spsr */ \
	asm("msr spsr, r0"); /* set spsr back */ \
	asm("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); /* restore registers */ \
	asm("ldm sp!, {pc}^"); /* special mode to return from exception handlers */

#define EXCEPTION_TOP \
	uint32_t lr; \
	asm("mov sp, %[ps]" : : [ps]"i" (KERNEL_EXCEPTION_STACK)); /* set sp */ \
	asm("sub lr, lr, #4"); /* adjust as per Table B1-7 of the manual */ \
	asm("push {lr}"); /* save lr */ \
	asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); /* save others */ \
	asm("mrs r0, spsr"); /* get spsr to save it */ \
	asm("push {r0}");  /* save spsr */ \
	asm("mov %[ps], lr" : [ps]"=r" (lr)); /* get the new lr */

#define EXCEPTION_BOTTOM \
	asm("pop {r0}"); /* get old value of spsr */ \
	asm("msr spsr, r0"); /* set spsr back */ \
	asm("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); /* restore others */ \
	asm("ldm sp!, {pc}^"); /* special mode to return from exception handlers */

void __attribute__((naked)) exceptionIrqEntry() { EXCEPTION_TOP exceptionHandler(lr, EX_OFFSET_IRQ); EXCEPTION_BOTTOM }
void __attribute__((naked)) exceptionUnknownEntry() { EXCEPTION_TOP exceptionHandler(lr, 0xFFFFFFFF); EXCEPTION_BOTTOM }

void installExceptionHandler(uint32_t index, void(*address)()) {
	uint32_t* handlers = (uint32_t*)VECTOR_TABLE_SIZE;
	handlers[index] = (uint32_t)address;
}

Exceptions::Exceptions() {
	// fill vector table with "ldr pc, [pc, #24]" which will point to our table of ex handlers
	uint32_t* vectorTable = (uint32_t*)0;
	for (int i = 0; i < 8; i++) {
		vectorTable[i] = 0xe59ff018;
		installExceptionHandler(i, &exceptionUnknownEntry);
	}

	installExceptionHandler(EX_OFFSET_IRQ, &exceptionIrqEntry);
}

void Exceptions::enableExceptions() {
	Logger::getInstance()->info("Exception", "Enabling IRQs");

	enableIRQ();

	Logger::getInstance()->info("Exception", "Enabling timer IRQ");

	// enable timer IRQ
	mmio_write(PIC_ENABLE_BASIC_IRQ, 1 << IRQ_TIMER_1);

	Logger::getInstance()->info("Exception", "Initializing timer");

	// initialize timer
	timer_last = mmio_read(TIMER_COUNTER_LOW);

	Logger::getInstance()->info("Exception", "Timer update");

	timer_update();
}
