#include "page.h"
#include "lib.h"
#include "x86_desc.h"

static PDE_t __attribute__((aligned (4096))) page_directory[MAX_ENTRIES];
static PTE_t __attribute__((aligned (4096))) table[MAX_ENTRIES];

void init_page(void){
    /* for loop indices */
    int i, j;

    /* initialize the page table */
    page_directory[0].table_PDE.present = 0x1;
    page_directory[0].table_PDE.read_write = 0x1;
    page_directory[0].table_PDE.user_super = 0x0;
    page_directory[0].table_PDE.pwt = 0x0;
    page_directory[0].table_PDE.pcd = 0x0;
    page_directory[0].table_PDE.accessed = 0x0;
    page_directory[0].table_PDE.reserved = 0x0;
    page_directory[0].table_PDE.page_size = 0x0;
    page_directory[0].table_PDE.global = 0x0;
    page_directory[0].table_PDE.available = 0x0;
    page_directory[0].table_PDE.table_addr = (uint32_t)table >> ADDRESS_SHIFT;

    /* initialize the kernel page */
    page_directory[1].page_PDE.present = 0x1;
    page_directory[1].page_PDE.read_write = 0x1;
    page_directory[1].page_PDE.user_super = 0x1;
    page_directory[1].page_PDE.pwt = 0x0;
    page_directory[1].page_PDE.pcd = 0x0;
    page_directory[1].page_PDE.accessed = 0x0;
    page_directory[1].page_PDE.dirty = 0x0;
    page_directory[1].page_PDE.page_size = 0x1;
    page_directory[1].page_PDE.global = 0x1;
    page_directory[1].page_PDE.available = 0x0;
    page_directory[1].page_PDE.pat = 0x0;
    page_directory[1].page_PDE.reserved =0x0;
    page_directory[1].page_PDE.page_addr = 0x1;

    /* fill in the page table, mark everything that isnt vid_mem
    * as not present
    */
    for(i = 0; i < MAX_ENTRIES; i++){

      if(i == VID_MEM_ADDR){
        table[i].present = 0x1;
        table[i].user_super = 0x1;
      }
      else{
        table[i].present = 0x0;
        table[i].user_super = 0x0;
      }
      table[i].read_write = 0x1;
      table[i].pwt = 0x0;
      table[i].pcd = 0x0;
      table[i].accessed = 0x0;
      table[i].dirty = 0x0;
      table[i].pat = 0x0;
      table[i].global = 0x0;
      table[i].available = 0x0;
      table[i].page_addr = i;
    }

    /* mark the rest of the page directory as not present */
    for(j = 2; j < MAX_ENTRIES; j++){
      page_directory[j].page_PDE.present = 0x0;
      page_directory[j].page_PDE.read_write = 0x1;
      page_directory[j].page_PDE.user_super = 0x0;
      page_directory[j].page_PDE.pwt = 0x0;
      page_directory[j].page_PDE.pcd = 0x0;
      page_directory[j].page_PDE.accessed = 0x0;
      page_directory[j].page_PDE.dirty = 0x0;
      page_directory[j].page_PDE.page_size = 0x1;
      page_directory[j].page_PDE.global = 0x0;
      page_directory[j].page_PDE.available = 0x0;
      page_directory[j].page_PDE.pat = 0x0;
      page_directory[j].page_PDE.reserved =0x0;
      page_directory[j].page_PDE.page_addr = 0x0;
    }

    asm volatile(
      " movl %0, %%eax; "
      " movl %%eax, %%cr3; "
      " movl %%cr4, %%eax; "
      " orl $0x00000010, %%eax; "
      " movl %%eax, %%cr4; "
      " movl %%cr0, %%eax; "
      " orl $0x80000000, %%eax; "
      " movl %%eax, %%cr0; "
      :
      : "r"(page_directory)
      :"%eax"
    );

}
