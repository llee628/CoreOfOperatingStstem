#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "idt.h"
#include "rtc.h"
#include "file_sys.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	if ((idt[SYSCALL_IDX].offset_15_00 == NULL) &&
		(idt[SYSCALL_IDX].offset_31_16 == NULL)){
		assertion_failure();
		result = FAIL;
	}

	return result;
}

/* Function: test_dvb;
 * Inputs: none
 * Return Value: FAIL if exception was not thrown
 * Function: Checks if Divide by Zero exception is properly initialized in IDT
 */
int test_dvb(){
	TEST_HEADER;
	int i = 10, k = 0;
	printf("Testing Divide by Zero exception \n");
	i = i / k;
	assertion_failure();
	return FAIL;
}

/* Function: test_df;
 * Inputs: none
 * Return Value: FAIL if exception was not thrown
 * Function: Checks if Double Fault exception is properly initialized in IDT
 */
int test_df(){
	TEST_HEADER;
	printf("Testing Double Fault exception \n");
	asm volatile("int $8");
	return FAIL;
}

/* Function: test_br;
 * Inputs: none
 * Return Value: FAIL if exception was not thrown
 * Function: Checks if Bound Range exception is properly initialized in IDT
 */

int test_br(){
	TEST_HEADER;
	printf("Testing Bound Range exception \n");
	asm volatile("int $5");
	return FAIL;
}

/* Function: test_while_loop_page;
 * Inputs: none
 * Return Value: FAIL or crash if paging is not initialized correctly
 * Function: Checks if kernel can be in a while loop with paging enabled
 */

int test_while_loop_page(){
	TEST_HEADER;
	while(1);
	assertion_failure();
	return FAIL;
}

/* Function: test_deref_null;
 * Inputs: none
 * Return Value: FAIL if page fault exception was not thrown
 * Function: Checks if page fault exception is properly initialized in IDT
 *					and paging has been initialized properly
 */
int test_deref_null(){
	TEST_HEADER;
	int k;
	int* i = NULL;
	printf("Testing Page Fault exception \n");
	k = *i;
	assertion_failure();
	return FAIL;
}

/* Function: test_deref_below_vid_mem;
 * Inputs: none
 * Return Value: FAIL if page fault exception was not thrown
 * Function: Checks if page fault exception is properly initialized in IDT
 *					and paging has been initialized properly
 */
int test_deref_below_vid_mem(){
	int k;
	int* i = (int*)(0xB8000 - 1);
	printf("Testing deref below vid_mem\n");
	k = *i;
	assertion_failure();
	return FAIL;
}

/* Function: test_deref_vid_mem;
 * Inputs: none
 * Return Value: Pass if vid_mem is properly paged
 * Function: Checks if vid mem is paged correctly
 */
int test_deref_vid_mem(){
	int k;
	int* i = (int*)(0xB8000);
	printf("Testing deref vid_mem\n");
	k = *i;
	return PASS;
}

/* Function: test_deref_above_vid_mem;
 * Inputs: none
 * Return Value: FAIL if page fault exception was not thrown
 * Function: Checks if page fault exception is properly initialized in IDT
 *					and paging has been initialized properly
 */
int test_deref_above_vid_mem(){
	int k;
	int* i = (int*)(0xB8000 + 0x4000 + 1);
	printf("Testing deref above vid_mem\n");
	k = *i;
	assertion_failure();
	return FAIL;
}

/* Function: test_deref_below_kernel;
 * Inputs: none
 * Return Value: FAIL if page fault exception was not thrown
 * Function: Checks if page fault exception is properly initialized in IDT
 *					and paging has been initialized properly
 */
int test_deref_below_kernel(){
	int k;
	int* i = (int*)(0x400000 - 1);
	printf("Testing deref below kernel\n");
	k = *i;
	assertion_failure();
	return FAIL;
}

/* Function: test_deref_kernel;
 * Inputs: none
 * Return Value: Pass if kernel code is properly paged
 * Function: Checks if kernel is paged correctly
 */
int test_deref_kernel(){
	int k;
	int* i = (int*)(0x400000);
	printf("Testing deref kernel\n");
	k = *i;
	return PASS;
}

/* Function: test_deref_above_kernel;
 * Inputs: none
 * Return Value: FAIL if page fault exception was not thrown
 * Function: Checks if page fault exception is properly initialized in IDT
 *					and paging has been initialized properly
 */
int test_deref_above_kernel(){
	int k;
	int* i = (int*)(0x800000 + 0x4000 + 1);
	printf("Testing deref above kernel\n");
	k = *i;
	assertion_failure();
	return FAIL;
}

/* Checkpoint 2 tests */

int test_dir_close(){
		uint8_t* filename = (uint8_t*)(".");
    if ( fs_dir_close(filename) != 0){ return FAIL;}
    printf("dir_close_value = %d\n",fs_dir_close(filename));
    return PASS;
}

int test_dir_write(){
		uint8_t buf[MAX_NAME_LENGTH];
    if ( fs_dir_write(buf, MAX_NAME_LENGTH) != -1){ return FAIL;}
    printf("dir_write_value = %d\n", fs_dir_write(buf, MAX_NAME_LENGTH));
    return PASS;
}

int test_file_close(){
		uint8_t* filename = (uint8_t*)(".");
    if ( fs_file_close(filename) != 0){ return FAIL;}
		printf("file_close_value = %d\n",fs_file_close(filename));
    return PASS;
}

int test_file_write(){
		uint8_t buf[MAX_NAME_LENGTH];
    if ( fs_file_write(buf, MAX_NAME_LENGTH) != -1){ return FAIL;}
		printf("file_write_value = %d\n", fs_file_write(buf, MAX_NAME_LENGTH));
    return PASS;
}

int test_dir_read(){
		uint8_t* filename = (uint8_t*)(".");
		fs_dir_open(filename);
		clear();
		while(1){
			uint8_t buf[MAX_NAME_LENGTH];
			if(!fs_dir_read(buf))
				break;
			printf("File name: ");
			modified_puts((int8_t*)buf, 32);
			printf(", File type: ");
			fs_file_open(buf);
			printf("%d", get_type());
			printf(", File size: ");
			printf("%d", get_size());
			printf("\n");
			fs_file_close(buf);
		}
		return 1;
}


int test_frame0_file(){
		int bytes_read = 0;
		uint8_t* filename = (uint8_t*)("frame0.txt");
		clear();
		printf("Input file name is ");
		puts((int8_t*)filename);
		printf("\n");
		fs_file_open(filename);

		while(1){
			uint8_t buf[1000];
			int cur_bytes_read = fs_file_read(buf, 1000);
			if(!cur_bytes_read)
				break;
			bytes_read = bytes_read + cur_bytes_read;
			modified_puts((int8_t*)buf, 1000);
		}

		fs_file_close(filename);

		if(bytes_read == 187)
			return PASS;
		else{
			assertion_failure();
			return FAIL;
		}
}

int test_nontext_file(){
		int bytes_read = 0;
		uint8_t* filename = (uint8_t*)("cat");
		clear();
		printf("Input file name is ");
		puts((int8_t*)filename);
		printf("\n");
		printf("Should show a weird character and ELF if reading the first 4 bytes correctly\n");
		fs_file_open(filename);

		uint8_t buf[4];
		int cur_bytes_read = fs_file_read(buf, 4);
		bytes_read = bytes_read + cur_bytes_read;
		printf("First four bytes are: ");
		modified_puts((int8_t*)buf, 4);
		printf("\n");

		fs_file_close(filename);

		if(bytes_read == 4)
			return PASS;
		else{
			assertion_failure();
			return FAIL;
		}
}


int test_large_file(){
		int bytes_read = 0;
		int flag = 1;
		uint8_t* filename = (uint8_t*)("verylargetextwithverylongname.txt");
		clear();
		printf("Input file name is ");
		puts((int8_t*)filename);
		printf("\n");
		fs_file_open(filename);

		while(1){
			uint8_t buf[1000];
			int cur_bytes_read = fs_file_read(buf, 1000);
			if(flag){
				printf("Only printing first 42 characters of file, test will read entire file though\n");
				printf("First 42 characters: ");
				modified_puts((int8_t*)buf, 42);
				printf("\n");
				flag = 0;
			}
			bytes_read = bytes_read + cur_bytes_read;
			if(!cur_bytes_read)
				break;
		}
		printf("Reading entire file should return 5277\n");
		printf("Bytes read is %d\n", bytes_read);

		fs_file_close(filename);

		if(bytes_read == 5277)
			return PASS;
		else{
			assertion_failure();
			return FAIL;
		}
}

/* Function: test_deref_kernel;
 * Inputs: none
 * Return Value: Pass if argument is checked. Failed if
 * Function: Checks if kernel is paged correctly
 */
int test_rtc_set_pi_freq(){
	int i =0;
	// Test invalid arguments
	{
		// test argument out of range
		if( rtc_set_pi_freq(-1) != -1){ return FAIL;}
		if( rtc_set_pi_freq(-100) != -1){ return FAIL;}
		if( rtc_set_pi_freq(16384) != -1){ return FAIL;}
		if( rtc_set_pi_freq(4096) != -1){ return FAIL;}
		if( rtc_set_pi_freq(8192) != -1){ return FAIL;}

		// test not power of 2
		if( rtc_set_pi_freq(0) != -1){ return FAIL;}
		if( rtc_set_pi_freq(87) != -1){ return FAIL;}
		if( rtc_set_pi_freq(199) != -1){ return FAIL;}
		if( rtc_set_pi_freq(878) != -1){ return FAIL;}
		if( rtc_set_pi_freq(2049) != -1){ return FAIL;}
	}

	// system should have the ability to set freq more than 1024Hz.
	// test maximum user freq
	// not done yet

	// test valid arguments
	{
		printf(" Test RTC in different frequency,the dot should always\
		pace in the same speed, regarless of the frequency.\n");
		test_interrupt_freq(1,2);
		if( rtc_set_pi_freq(2) != 0){ return FAIL;}
		while( test_interrupt_freq(2,0) != 0 ){
			; // waiting test to be finshed
		}

		test_interrupt_freq(1,256);
		if( rtc_set_pi_freq(256) != 0){ return FAIL;}
		while( test_interrupt_freq(2,0) != 0 ){
			; // waiting test to be finshed
		}

		test_interrupt_freq(1,1024);
		if( rtc_set_pi_freq(1024) != 0){ return FAIL;}
		while( test_interrupt_freq(2,0) != 0 ){
			; // waiting test to be finshed
		}
		// use a loop to prevent race condition in printing result.
		i=0;
		while(++i<1000){
			;
		}
	}
	return PASS;
}

int test_rtc_read(){
    if ( rtc_read() != 0){ return FAIL;}
    printf("rtc_read_rvalue = %d\n", rtc_read());
    return PASS;
}

int test_rtc_close(){
    if ( rtc_close() != 0){ return FAIL;}
    return PASS;
}

int test_rtc_write_open(){
    int i = 0;
    // Test invalid arguments
    {
        // test argument out of range
        if( rtc_set_pi_freq(-1) != -1){ return FAIL;}
        if( rtc_set_pi_freq(-100) != -1){ return FAIL;}
        if( rtc_set_pi_freq(16384) != -1){ return FAIL;}
        if( rtc_set_pi_freq(4096) != -1){ return FAIL;}
        if( rtc_set_pi_freq(8192) != -1){ return FAIL;}

        // test not power of 2
        if( rtc_set_pi_freq(0) != -1){ return FAIL;}
        if( rtc_set_pi_freq(87) != -1){ return FAIL;}
        if( rtc_set_pi_freq(199) != -1){ return FAIL;}
        if( rtc_set_pi_freq(878) != -1){ return FAIL;}
        if( rtc_set_pi_freq(2049) != -1){ return FAIL;}
    }


    // Test valid arguments
    printf(" RTC frequency = 16HZ:\n");
    if (rtc_set_pi_freq(16) != 0){ return FAIL;}
    test_rtc_freq(1);
    while (test_rtc_freq(2) != 0){
        ;//waiting test to be finished
    }

    printf(" RTC frequency = 8HZ:\n");
    if (rtc_set_pi_freq(8) != 0){ return FAIL;}
    test_rtc_freq(1);
    while (test_rtc_freq(2) != 0){
        ;//waiting test to be finished
    }

    printf(" RTC frequency = 4HZ:\n");
    if (rtc_set_pi_freq(4) != 0){ return FAIL;}
    test_rtc_freq(1);
    while (test_rtc_freq(2) != 0){
        ;//waiting test to be finished
    }

    printf(" RTC frequency after run rtc_open() (RTC freq = 2HZ)\n");
    if (rtc_open() != 0){ return FAIL;}
    test_rtc_freq(1);
    while ( test_rtc_freq(2) != 0){
        ;//wait test to be finished
    }

    // use a loop to prevent race condition in printing result.
    i = 0;
    while(++i<1000){
        ;
    }
    return PASS;


}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
  //TEST_OUTPUT("test_dvb", test_dvb());
	//TEST_OUTPUT("test_br", test_br());
	//TEST_OUTPUT("test_df", test_df());
	//TEST_OUTPUT("test_while_loop_page", test_while_loop_page());
	//TEST_OUTPUT("test_deref_null", test_deref_null());
	//TEST_OUTPUT("test_deref_below_vid_mem", test_deref_below_vid_mem());
	//TEST_OUTPUT("test_deref_vid_mem", test_deref_vid_mem());
	//TEST_OUTPUT("test_deref_above_vid_mem", test_deref_above_vid_mem());
	//TEST_OUTPUT("test_deref_below_kernel", test_deref_below_kernel());
	//TEST_OUTPUT("test_deref_kernel", test_deref_kernel());
	//TEST_OUTPUT("test_deref_above_kernel", test_deref_above_kernel());

	// ------ Check point 2
  //TEST_OUTPUT("test_rtc_close", test_rtc_close());
  //TEST_OUTPUT("test_rtc_write_open", test_rtc_write_open());
  //TEST_OUTPUT("test_rtc_read", test_rtc_read());
	//TEST_OUTPUT("test_rtc_set_pi_freq", test_rtc_set_pi_freq());
	//TEST_OUTPUT("test_dir_close", test_dir_close());
	//TEST_OUTPUT("test_dir_write", test_dir_write());
	//TEST_OUTPUT("test_file_close", test_file_close());
	//TEST_OUTPUT("test_file_write", test_file_write());
	//TEST_OUTPUT("test_dir_read", test_dir_read());
	//TEST_OUTPUT("test_frame0_file", test_frame0_file());
	//TEST_OUTPUT("test_nontext_file", test_nontext_file());
	//TEST_OUTPUT("test_large_file", test_large_file());

}
