ENTRY(_kstart);

SECTIONS
{
	. = 0x80000000;
	_ram_start = .;

	.text ALIGN(4096) :
	{
		_kernel_start = .;
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
	
	.proc0 ALIGN(4096) :
	{
		_proc0_text_start = .;
		*(.proc0.text)
		_proc0_text_end = .;
		. = ALIGN(4096);
	
		_proc0_data_start = .;
		*(.proc0.data)
		_proc0_data_end = .;
		. = ALIGN(4096);
	
		_proc0_bss_start = .;
		*(.proc0.bss)
		_proc0_bss_end = .;
	}
	
	.proc1 ALIGN(4096) :
	{	
		_proc1_text_start = .;
		*(.proc1.text)
		_proc1_text_end = .;
		. = ALIGN(4096);
	
		_proc1_data_start = .;
		*(.proc1.data)
		_proc1_data_end = .;
		. = ALIGN(4096);
	
		_proc1_bss_start = .;
		*(.proc1.bss)
		_proc1_bss_end = .;
		. = ALIGN(4096);
	}
		
	.bss ALIGN(4096) : 
	{
		_kernel_bss_start = .;
		*(.bss)
		*(.bss.*)
		_kernel_bss_end = .;
	}
	. = ALIGN(4096);

	. = . + 0x100;
	. = ALIGN(4096);
	
	_ex_stack_top = .;

	_kernel_end = .;

	. = 0xa0000000;
	_ram_end = .;
}
