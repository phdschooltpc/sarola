#include <interpow/interpow.h>
#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>

#include "fann.h"
#include "thyroid_test.h"
#include "profiler.h"
/*Intermittent Tester*/
#include <tester.h>
//#include <noise.h>

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

//#pragma PERSISTENT(TASK_FIND_MIN)
//NewTask(TASK_FIND_MIN, task_find_min_f, 1) // with self-field
//
//#pragma PERSISTENT(TASK_SUB_MIN)
//NewTask(TASK_SUB_MIN, task_sub_min_f, 1) // with self-field

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
//// This field connects the two tasks, informing TASK_SUB_MIN about the position
//// of each array's minimum
//#pragma PERSISTENT(PersField(TASK_FIND_MIN, TASK_SUB_MIN, f_min_index))
//NewField(TASK_FIND_MIN, TASK_SUB_MIN, f_min_index, UINT8, NUM_OF_ARRAYS)
//
//// This self-field helps TASK_SUB_MIN keep track of the array whose average
//// has to be computed
//#pragma PERSISTENT(PersSField0(TASK_FIND_MIN, sf_array_index))
//#pragma PERSISTENT(PersSField1(TASK_FIND_MIN, sf_array_index))
//NewSelfField(TASK_SUB_MIN, sf_array_index, UINT8, 1, SELF_FIELD_CODE_1)
//
//// This self-field helps TASK_SUB_MIN keep track of each array's average,
//// after an array has been subtracted its minimum
//#pragma PERSISTENT(PersSField0(TASK_SUB_MIN, sf_avg))
//#pragma PERSISTENT(PersSField1(TASK_SUB_MIN, sf_avg))
//NewSelfField(TASK_SUB_MIN, sf_avg, FLOAT32, NUM_OF_ARRAYS, SELF_FIELD_CODE_2)

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
/*
    uint8_t i = num_data;
    // num_data from thyroid_test.h: 100 -> uint8_t is sufficient
    for (; i > 0; i--) {
        fann_test(ann, input[i], output[i]);
    }
   */
   //uint16_t i;
   /* Debug variable. */
    /*
   fann_type *calc_out;
   for (i = 0; i < num_data; i++) {
           calc_out = fann_test(ann, input[i], output[i]);
           __no_operation();
   }
   */

    fann_test(ann, input[test_index], output[test_index]);

    //puts("bar\n");

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

//
//uint8_t find_min_pos(uint16_t* array)
//{
//    uint8_t i, min_pos;
//    uint16_t min = array[0];
//
//    min_pos = 0;
//    for (i = 1; i < SIZE; i++) {
//        if (array[i] < min) {
//            min = array[i];
//            min_pos = i;
//        }
//    }
//
//    return min_pos;
//}
//
//
//float sub_min_and_avg(uint16_t* array, uint8_t min_pos)
//{
//    uint8_t i;
//    float avg = 0;
//
//    for (i = 0; i < SIZE; i++) {
//        avg += array[i] - array[min_pos];
//    }
//
//    return avg / SIZE;
//}
//
//
//void task_find_min_f()
//{
//    uint8_t array_index;
//    uint8_t min_pos;
//
//    ReadSelfField_U8(TASK_FIND_MIN, sf_array_index, &array_index); // initially sf_array_index = 0
//
//    switch (array_index) {
//    case 0:
//        min_pos = find_min_pos(a);
//        break;
//    case 1:
//        min_pos = find_min_pos(b);
//        break;
//    case 2:
//        min_pos = find_min_pos(c);
//        break;
//    case 3:
//        min_pos = find_min_pos(d);
//        break;
//    }
//
//    WriteFieldElement_U8(TASK_FIND_MIN, TASK_SUB_MIN, f_min_index, &min_pos, array_index);
//
//    if (++array_index == NUM_OF_ARRAYS) {
//        array_index = 0;
//        WriteSelfField_U8(TASK_FIND_MIN, sf_array_index, &array_index);
//        StartTask(TASK_SUB_MIN);
//    }
//    else {
//        WriteSelfField_U8(TASK_FIND_MIN, sf_array_index, &array_index);
//        StartTask(TASK_FIND_MIN);
//    }
//}
//
//
//void task_sub_min_f()
//{
//    uint8_t array_index;
//    uint8_t min_pos[NUM_OF_ARRAYS];
//    float avg[NUM_OF_ARRAYS];
//
//    ReadSelfField_U8(TASK_SUB_MIN, sf_array_index, &array_index); // initially sf_array_index = 0
//    ReadField_U8(TASK_FIND_MIN, TASK_SUB_MIN, f_min_index, min_pos);
//    ReadSelfField_F32(TASK_SUB_MIN, sf_avg, avg);
//
//    switch (array_index) {
//    case 0:
//        avg[array_index] = sub_min_and_avg(a, min_pos[array_index]);
//        break;
//    case 1:
//        avg[array_index] = sub_min_and_avg(b, min_pos[array_index]);
//        break;
//    case 2:
//        avg[array_index] = sub_min_and_avg(c, min_pos[array_index]);
//        break;
//    case 3:
//        avg[array_index] = sub_min_and_avg(d, min_pos[array_index]);
//        break;
//    }
//
//    WriteSelfField_F32(TASK_SUB_MIN, sf_avg, avg);
//
//    if (++array_index == NUM_OF_ARRAYS) {
//        array_index = 0;
//        WriteSelfField_U8(TASK_SUB_MIN, sf_array_index, &array_index);
//        while(1); // END OF PROGRAM, breakpoint here to check result
//    }
//    else {
//        WriteSelfField_U8(TASK_SUB_MIN, sf_array_index, &array_index);
//        StartTask(TASK_SUB_MIN);
//    }
//}
