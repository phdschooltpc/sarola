#include <interpow/interpow.h>
#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>

#include "fann.h"
#include "thyroid_test.h"
#include "profiler.h"
/*Intermittent Tester*/
#include <tester.h>

/*
 *******************************************************************************
 * Task functions declaration
 *******************************************************************************
 */

void task_fann_load(void);
void task_fann_test(void);
void task_result(void);

/*
 *******************************************************************************
 * Definition of tasks, preceded by the PERSISTENT declaration
 *******************************************************************************
 */

#pragma PERSISTENT(TASK_FANN_LOAD)
NewTask(TASK_FANN_LOAD, task_fann_load, 1) // with self-field

#pragma PERSISTENT(TASK_FANN_TEST)
NewTask(TASK_FANN_TEST, task_fann_test, 1) // with self-field

#pragma PERSISTENT(TASK_RESULT)
NewTask(TASK_RESULT, task_result, 1) // with self-field

/*
 *******************************************************************************
 * Inform the program about the task to execute on the first start of the
 * system, preceded by the PERSISTENT declaration
 *******************************************************************************
 */

#pragma PERSISTENT(PersState)
InitialTask(TASK_FANN_LOAD)

/*
 *******************************************************************************
 * Definition of fields, preceded by the PERSISTENT declaration
 *******************************************************************************
 */

//// This self-field helps TASK_FIND_MIN keep track of the array whose minimum
//// has to be found (array a to d)
#pragma PERSISTENT(PersSField0(TASK_FANN_TEST, sf_test_index))
#pragma PERSISTENT(PersSField1(TASK_FANN_TEST, sf_test_index))
/* task, name, type, len, code */
NewSelfField(TASK_FANN_TEST, sf_test_index, UINT8, 1, SELF_FIELD_CODE_1)
//

/*
 *******************************************************************************
 * main
 *******************************************************************************
 */

/// TODO(rh): This has to be persistent!
static struct fann *ann;

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    /* Prepare LED. */
    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
                          // to activate previously configured port settings
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    tester_notify_start();
    while(1) {
        Resume();
    }
}

void task_fann_load(void) {
    puts("load...\n");
#ifdef PROFILE
    /* Start counting clock cycles. */
    profiler_start();
#endif // PROFILE

    ann = fann_create_from_header();
    fann_reset_MSE(ann);
#ifdef PROFILE
    /* Stop counting clock cycles. */
     uint32_t clk_cycles = profiler_stop();

    /* Print profiling. */
    printf("ANN initialisation:\n"
           "-> execution cycles = %lu\n"
           "-> execution time = %.3f ms\n\n",
           clk_cycles, (float) clk_cycles / 8000);
#endif // PROFILE

    StartTask(TASK_FANN_TEST);
}

void task_fann_test(void) {
#ifdef PROFILE
    /* Start counting clock cycles. */
    profiler_start();
#endif // PROFILE

    uint8_t test_index;
    ReadSelfField_U8(TASK_FANN_TEST, sf_test_index, &test_index); // initially sf_array_index = 0

    fann_type* calc_out = fann_test(ann, input[test_index], output[test_index]);
    tester_send_data(test_index, calc_out, sizeof(calc_out));

#ifdef PROFILE
    /* Stop counting clock cycles. */
    uint32_t clk_cycles = profiler_stop();

    /* Print profiling. */
    printf("Run %u tests:\n"
           "-> execution cycles = %lu (%lu per test)\n"
           "-> execution time = %.3f ms (%.3f ms per test)\n\n",
           test_index,
           clk_cycles, clk_cycles / test_index,
           (float) clk_cycles / 8000, (float) clk_cycles / 8000 / test_index);
#endif // PROFILE

    /// All data processed? -> Done!
    if(++test_index == num_data) {
        StartTask(TASK_RESULT);
    } else {
        /// Some data left? -> update field and call task again
        WriteSelfField_U8(TASK_FANN_TEST, sf_test_index, &test_index);
        StartTask(TASK_FANN_TEST);
    }

}

void task_result(void) {
    /* Print error. */
    printf("MSE error on %d test data: %f\n\n", num_data, fann_get_MSE(ann));

    /* Clean-up. */
    fann_destroy(ann);
    __no_operation();

    puts("DONE\n");

    P1OUT |= BIT0;

    while(1);

    /*Report results*/
    /* You need to include that statement at the termination of your intermittent program*/
    //tester_send_data(0, string, 15);
}
