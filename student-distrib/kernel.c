/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "idt.h"
#include "debug.h"
#include "tests.h"
#include "rtc.h"
#include "kb.h"
#include "page.h"
#include "term.h"
#include "file_sys.h"
#include "signals.h"
#include "scheduling.h"

extern int32_t do_syscall(int32_t a, int32_t b, int32_t c, int32_t d);

/* #define RUN_TESTS */

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear(terms);

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        /* printf(terms, "Invalid magic number: 0x%#x\n", (unsigned)magic); */
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    /* printf(terms, "flags = 0x%#x\n", (unsigned)mbi->flags); */

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        /* printf(terms, "mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper); */

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        /* printf(terms, "boot_device = 0x%#x\n", (unsigned)mbi->boot_device); */

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        /* printf(terms, "cmdline = %s\n", (char *)mbi->cmdline); */

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            /* printf(terms, "Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start); */
            /* printf(terms, "Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end); */
            /* printf(terms, "First few bytes of module:\n"); */
            for (i = 0; i < 16; i++) {
                /* printf(terms, "0x%x ", *((char*)(mod->mod_start+i))); */
            }
            /* printf(terms, "\n"); */
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        /* printf(terms, "Both bits 4 and 5 are set.\n"); */
        return;
    }

    /* /1* Is the section header table of ELF valid? *1/ */
    /* if (CHECK_FLAG(mbi->flags, 5)) { */
    /*     elf_section_header_table_t *elf_sec = &(mbi->elf_sec); */
    /*     printf(terms, "elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n", */
    /*             (unsigned)elf_sec->num, (unsigned)elf_sec->size, */
    /*             (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx); */
    /* } */

    /* /1* Are mmap_* valid? *1/ */
    /* if (CHECK_FLAG(mbi->flags, 6)) { */
    /*     memory_map_t *mmap; */
    /*     printf(terms, "mmap_addr = 0x%#x, mmap_length = 0x%x\n", */
    /*             (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length); */
    /*     for (mmap = (memory_map_t *)mbi->mmap_addr; */
    /*             (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length; */
    /*             mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size))) */
    /*         printf(terms, "    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n", */
    /*                 (unsigned)mmap->size, */
    /*                 (unsigned)mmap->base_addr_high, */
    /*                 (unsigned)mmap->base_addr_low, */
    /*                 (unsigned)mmap->type, */
    /*                 (unsigned)mmap->length_high, */
    /*                 (unsigned)mmap->length_low); */
    /* } */

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    /* Getting the File System boot block address */
   module_t* mod = (module_t*)mbi->mods_addr;
   uint32_t bblock_addr = (uint32_t)mod->mod_start;

    /* Init the IDT */
    idt_init();
    /* Init Paging */
    init_page();
    /* Init the PIC */
    i8259_init();
    /* Init the RTC */
    init_rtc();
    /* Init the PIT */
    /* Init the keyboard */
	init_kb();
    /* Init the File System */
    fs_init(bblock_addr);
    init_term();
	init_signals();
    init_pit();

/*	int i;
	for (i = 0; i < 16; i ++) {
		setattr(i);
		printf(terms, "%x ", i);
		setattr(i << 4);
		printf(terms, "%x ", i);
	}
	setattr(DEF_ATTR);
	putc('\n');
*/
#ifdef RUN_TESTS
    /* Run tests */
    launch_tests();
#else
	//_syscall_execute((int32_t) (uint8_t *) "shell", 0);
	 //do_syscall(2, (int32_t) (uint8_t *) "shell", 0, 0);
#endif
	while (1) {
		asm volatile ("hlt;");
	}
}
