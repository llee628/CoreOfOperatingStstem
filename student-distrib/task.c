#include "task.h"
#include "file_sys.h"
#include "lib.h"
#include "page.h"

// Open the specified program file, check for ELF format,
// and then load the task in the a set address
// returns the entry point in the pointer
int32_t load_task(uint8_t *filename, int32_t task_count, int32_t *entry_addr) {
    if (fs_file_open(filename) == -1) {
        return -1;
    }

    uint8_t buf[BUF_SIZE];
    int32_t read_size = fs_file_read(buf, BUF_SIZE);
    // Check for ELF magic
    if (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        fs_file_close(filename);
        return -1;
    }

    *entry_addr = *((int32_t *) (buf + 24));
    // Check that the entry address is after the image starting point
    if (*entry_addr < TASK_IMG_START_ADDR) {
        fs_file_close(filename);
        return -1;
    }

    // Setup paging
    // Set task's target page address
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(task_count);
    // Reload the TLB
    asm volatile(
        "movl %%cr3, %%eax;"
        " movl %0, %%cr3; "
        :
        : "r"(page_directory)
        :"%eax"
    );

    // Copy the rest of the image
    uint8_t *task_img_cur = (uint8_t *) TASK_IMG_START_ADDR;
    memcpy(task_img_cur, buf, read_size);
    task_img_cur += read_size;
    while ((read_size = fs_file_read(buf, BUF_SIZE)) > 0) {
        memcpy(task_img_cur, buf, read_size);
        task_img_cur += read_size;
    }

    fs_file_close(filename);
    return 0;
}
