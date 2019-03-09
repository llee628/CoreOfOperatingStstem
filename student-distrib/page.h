#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

#define MAX_ENTRIES       1024
#define ADDRESS_SHIFT       12
#define KERNEL_ADDR   0x400000
#define VID_MEM_ADDR      0xB8

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
        uint8_t available :3;
        uint8_t pat : 1;
        uint16_t reserved  : 9;
        uint16_t page_addr : 10;
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
        uint8_t available :3;
        uint32_t table_addr : 20;
    } __attribute__ ((packed)) table_PDE;
} PDE_t;

void init_page(void);

#endif
