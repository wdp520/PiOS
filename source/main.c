#include "hardware/emmc.h"
#include "hardware/interrupts.h"
#include "hardware/timer.h"
#include "hardware/mmu.h"
#include "hardware/uart.h"
#include "types/string.h"
#include "util/utilities.h"
#include "terminal.h"
#include "taskScheduler.h"

// Windows doesn't have __attribute__ :(
#ifdef _MSC_VER
#define __attribute__(a)
#define asm 
#endif

// user_code gets relocated to a section in memory where user mode has access
#define user_code __attribute__((section(".user"))) __attribute__ ((noinline))
#define SVC_INSTRUCTION(number) asm volatile("svc %0" : : "I" (number))
#define FINAL_USER_START_VA 0x00F00000
#define task_main_func int(*)(void)

// This variable makes sure we have something in the .data section,
// Because we place .bss before data, this will make sure that the compiler
// Zeroes out the .bss region for us, as it pads it with 0's
volatile unsigned int dataVarForPadding = 42;

extern void enable_irq(void);
extern void branchTo(unsigned int*);

volatile extern Emmc* gEmmc;

void system_initialize_serial(void)
{
	Uart_Initialize();

	Uart_EnableInterrupts();

	Arm_InterruptInitialize();

	Arm_IrqDisableall();
	Arm_IrqEnable(interrupt_source_uart);

	enable_irq();
}

unsigned int system_initialize(void)
{
	unsigned int result = 0;

    system_initialize_serial();

    Uart_SendString("Welcome to PiOS!\n\r");
	
    // Initialize terminal first so we can print error messages if any (Hah, unlikely!)
	if ((result = Terminal_Initialize()) != 0)
	{
		Uart_SendString("Failed to initialize terminal.\n");
	}

	unsigned int* basePageTable = (unsigned int *)0x00A08000;

	Mmu_Initialize(basePageTable);
	
	//taskScheduler_Init();
	
	printf("System initialization complete, result: %d\n", result);

    return result;
}

int cmain(void)
{
    if (system_initialize() != 0)
    {
        printf("\n * * * System Halting * * *\n");

        while (1);
    }

	Terminal_PrintWelcome();
	Terminal_PrintPrompt();

	printf("Testing translation fault by accessing unmapped memory.\n");
	*((unsigned int*)0x10E00000) = 2;
	printf("If you can see this, the data abort was successful.\n");
                
    //printf("Calling loaded program img, should trigger SVC!\n");
    //branchTo((unsigned int *)(FINAL_USER_START_VA));
    printf("It worked!? :-)\n");

    // Timer temporarily disabled as it messes with execution of relocated code
	// Enable timer intterrupts and set up timer
    /*timer_sp_clearmatch();
    timer_sp_setinterval(TASK_SCHEDULER_TICK_MS);
	arm_irq_enable(interrupt_source_system_timer); */
    while (1)
    {
        Terminal_Update();

        wait(200);
    }
}
