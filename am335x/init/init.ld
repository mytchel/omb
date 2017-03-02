ENTRY(_start)

SECTIONS
{
	.text ALIGN(4096) :
	{
		init/c.o(.text)
		*(.text)
		*(.text.*)
		. = ALIGN(4096);
		*(.rodata)
		*(.rodata.*)
	}
	
	.data ALIGN(4096) :
	{
		*(.data)
		*(.data.*)
		. = ALIGN(4096);
		*(.bss)
		*(.bss.*)
	}
}
