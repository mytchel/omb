SECTIONS
{
	. = 0x80000000;
	ram_start = .;

	.text ALIGN(4096) :
	{
		kernel_start = .;
		*(.text)
	}
	
	.rodata ALIGN(4096) : 
	{
		*(.rodata)
		*(.rodata.*)
	}

	.data ALIGN(4096) : 
	{
		*(.data)
		*(.data.*)
	}
	
	.bss ALIGN(4096) : 
	{
		kernel_bss_start = .;
		*(.bss)
		*(.bss.*)
		kernel_bss_end = .;
	}

	. = ALIGN(4096);

	. = . + 0x100;
	ex_stack_top = .;

	kernel_end = .;

	.init ALIGN(4096) :
	{
		init_start = .;

		_init.o(*)

		. = ALIGN(4096);
		init_end = .;
	}

	. = 0xa0000000;
	ram_end = .;
}
