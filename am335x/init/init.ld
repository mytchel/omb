
SECTIONS
{
	.text ALIGN(4096) :
	{
		*(.text)
		*(.text.*)
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
		*(.bss)
		*(.bss.*)
	}
}
