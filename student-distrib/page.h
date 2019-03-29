#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

#define MAX_ENTRIES       1024
#define ADDRESS_SHIFT       12
#define KERNEL_ADDR   0x400000
#define VID_MEM_ADDR      0xB8
#define USER_PAGE_START 0x08000000
// 4 MB = 4 * 1024 * 1024
#define USER_PAGE_INDEX (USER_PAGE_START >> (2 + 10 + 10))

/* Structure for a page table entry */
typedef struct __attribute__ ((packed)) PTE_t{
    uint8_t present : 1;
    uint8_t read_write : 1;
    uint8_t user_super : 1;
    uint8_t pwt : 1;
    uint8_t pcd : 1;
    uint8_t accessed : 1;
    uint8_t dirty : 1;
    uint8_t pat : 1;
    uint8_t global : 1;
    uint8_t available :3;
    uint32_t page_addr : 20;
} PTE_t;

/* Union of two types of Page directory entry we need
* first entry is a 4 MB page, second entry is a page table */
typedef union PDE_t {
    struct{
        uint8_t present : 1;
        uint8_t read_write : 1;
        uint8_t user_super : 1;
        uint8_t pwt : 1;
        uint8_t pcd : 1;
        uint8_t accessed : 1;
        uint8_t dirty : 1;
        uint8_t page_size : 1;
        uint8_t global : 1;
        uint8_t available :3;                       // 3 bits are marked as available
        uint8_t pat : 1;
        uint16_t reserved  : 9;                     // 9 bits are marked as resered
        uint16_t page_addr : 10;                    // 10 bits are marked for the page address
      } __attribute__ ((packed)) page_PDE;
    struct{
        uint8_t present : 1;
        uint8_t read_write : 1;
        uint8_t user_super : 1;
        uint8_t pwt : 1;
        uint8_t pcd : 1;
        uint8_t accessed : 1;
        uint8_t reserved  : 1;
        uint8_t page_size : 1;
        uint8_t global : 1;
        uint8_t available :3;                       // 3 bits are marked as available
        uint32_t table_addr : 20;                   // 20 bits are marked for the page table address
    } __attribute__ ((packed)) table_PDE;
} PDE_t;

/* initializes the page directory and enables paging */
void init_page(void);

PDE_t page_directory[MAX_ENTRIES];
PTE_t vidmem_page_table[MAX_ENTRIES];

#endif
