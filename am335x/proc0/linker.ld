
SECTIONS
{
	.text ALIGN(4096) :
	{
		_text_start = .;
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		_text_end = .;
	}
	
	.data ALIGN(4096) :
	{
		_data_start = .;
		*(.data)
		*(.data.*)
		*(.bss)
		*(.bss.*)
	
		. = ALIGN(4096);
		
		_proc_uart_start = .;
		base/uart.bo(*)
		
		_data_end = .;
	}
}
