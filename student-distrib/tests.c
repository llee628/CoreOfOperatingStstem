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

FILE f;     // TODO Temporary fix

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
    if ( fs_dir_close(&f) != 0){ return FAIL;}
    printf("dir_close_value = %d\n",fs_dir_close(&f));
    return PASS;
}

int test_dir_write(){
		uint8_t buf[MAX_NAME_LENGTH];
    if ( fs_dir_write(buf, MAX_NAME_LENGTH, &f) != -1){ return FAIL;}
    printf("dir_write_value = %d\n", fs_dir_write(buf, MAX_NAME_LENGTH, &f));
    return PASS;
}

int test_file_close(){
		uint8_t* filename = (uint8_t*)(".");
    if ( fs_file_close(&f) != 0){ return FAIL;}
		printf("file_close_value = %d\n",fs_file_close(&f));
    return PASS;
}

int test_file_write(){
		uint8_t buf[MAX_NAME_LENGTH];
    if ( fs_file_write(buf, MAX_NAME_LENGTH, &f) != -1){ return FAIL;}
		printf("file_write_value = %d\n", fs_file_write(buf, MAX_NAME_LENGTH, &f));
    return PASS;
}

int test_dir_read(){
		uint8_t* filename = (uint8_t*)(".");
		fs_dir_open(filename, &f);
		clear();
		while(1){
			uint8_t buf[MAX_NAME_LENGTH];
			if(!fs_dir_read(buf, 32, &f))
				break;
			printf("File name: ");
			modified_puts((int8_t*)buf, 32);
			printf(", File type: ");
			fs_file_open(buf, &f);
			printf("%d", get_type());
			printf(", File size: ");
			printf("%d", get_size());
			printf("\n");
			fs_file_close(&f);
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
		fs_file_open(filename, &f);

		while(1){
			uint8_t buf[1000];
			int cur_bytes_read = fs_file_read(buf, 1000, &f);
			if(!cur_bytes_read)
				break;
			bytes_read = bytes_read + cur_bytes_read;
			modified_puts((int8_t*)buf, 1000);
		}

		fs_file_close(&f);

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
		fs_file_open(filename, &f);

		uint8_t buf[4];
		int cur_bytes_read = fs_file_read(buf, 4, &f);
		bytes_read = bytes_read + cur_bytes_read;
		printf("First four bytes are: ");
		modified_puts((int8_t*)buf, 4);
		printf("\n");

		fs_file_close(&f);

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
		fs_file_open(filename, &f);

		while(1){
			uint8_t buf[1000];
			int cur_bytes_read = fs_file_read(buf, 1000, &f);
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

		fs_file_close(&f);

		if(bytes_read == 5277)
			return PASS;
		else{
			assertion_failure();
			return FAIL;
		}
}

/* Function: test_rtc_read;
 * Inputs: none
 * Return Value: FAIL if the rtc_read do not return the appropriate value
 * Side effect: none
 * Description: Show the program can return from rtc_read appropriately showing that
 *	RTC interrupt handler is called normally
 */

// int test_rtc_read(){
//     if ( rtc_read() != 0){ return FAIL;}
//     printf("rtc_read_rvalue = %d\n", rtc_read());
//     return PASS;
// }

/* Function: test_rtc_close;
 * Inputs: none
 * Return Value: FAIL if the rtc_close do not return the appropriate value
 * Side effect: none
 * Description: not doing special things so far
 */

// int test_rtc_close(){
//     if ( rtc_close() != 0){ return FAIL;}
//     return PASS;
// }

/* Function: test_rtc;
 * Inputs: none
 * Return Value: FAIL if the rtc_write and rtc_open do not return the appropriate value
 * Side effect: Change the RTC frequency
 * Description: Show rtc_write works by print '1' in different pace under different
 *	frequency changed by rtc_write and rtc_open
 */

int test_rtc(){
    int i = 0;
	rtc_info_t rtc;

	// Test open/close
	{
		if( rtc_open(&rtc)!=0 ){ return FAIL;}
		if( rtc_close(&rtc) != 0){ return FAIL;}
		if( rtc_open(NULL)==0 ){return FAIL;}
		if( rtc_close(NULL)==0){return FAIL;}
	}

    // Test invalid arguments
    {
		if( rtc_open(&rtc)!=0 ){ return FAIL;}

		// test NULL argument
		if( rtc_write_usr(NULL, 128) != -1){ return FAIL;}

		// test argument out of range
        if( rtc_write_usr(&rtc, -1) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, -100) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, RTC_USR_MX_FREQ<<1) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, RTC_USR_MX_FREQ<<2) != -1){ return FAIL;}

        // test not power of 2
        if( rtc_write_usr(&rtc, 0) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, 87) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, 199) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, 878) != -1){ return FAIL;}
        if( rtc_write_usr(&rtc, 2049) != -1){ return FAIL;}


		if( rtc_close(&rtc) != 0){ return FAIL;}
	}

	int dot_count = 256;

	// Test NUL read
	{
		if( rtc_open(&rtc)!=0 ){ return FAIL;}
		if (rtc_write_usr(&rtc, 64) != 0){ return FAIL;}
		if( rtc_read(NULL) == 0){ return FAIL;}
		if( rtc_close(&rtc) != 0){ return FAIL;}
	}

    // Test Different frequency
	{
		if( rtc_open(&rtc)!=0 ){ return FAIL;}

		printf(" RTC frequency = 128HZ:\n");
		if (rtc_write_usr(&rtc, 128) != 0){ return FAIL;}
		for( i=0; i<dot_count; i++){
			rtc_read(&rtc);
			printf("1");
		}
		printf("\n");



		printf(" RTC frequency = 32HZ:\n");
		if (rtc_write_usr(&rtc, 32) != 0){ return FAIL;}
		for( i=0; i<(dot_count/4); i++){
			rtc_read(&rtc);
			printf("1");
		}
		printf("\n");


		printf(" RTC frequency = 4HZ:\n");
		if (rtc_write_usr(&rtc, 4) != 0){ return FAIL;}
		for( i=0; i<(dot_count/8); i++){
			rtc_read(&rtc);
			printf("1");
		}
		printf("\n");

		if( rtc_close(&rtc) != 0){ return FAIL;}
	}

	// Test RTC open default frequency
	{
		if( rtc_open(&rtc)!=0 ){ return FAIL;}

		printf(" RTC frequency after run rtc_open() (RTC freq = 2HZ)\n");
		for( i=0; i<8; i++){
			rtc_read(&rtc);
			printf("1");
		}
		printf("\n");

		if( rtc_close(&rtc) != 0){ return FAIL;}
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
	TEST_OUTPUT("test_rtc", test_rtc());

	//TEST_OUTPUT("test_dir_close", test_dir_close());
	//TEST_OUTPUT("test_dir_write", test_dir_write());
	//TEST_OUTPUT("test_file_close", test_file_close());
	//TEST_OUTPUT("test_file_write", test_file_write());
	//TEST_OUTPUT("test_dir_read", test_dir_read());
	//TEST_OUTPUT("test_frame0_file", test_frame0_file());
	//TEST_OUTPUT("test_nontext_file", test_nontext_file());
	//TEST_OUTPUT("test_large_file", test_large_file());

}
