
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
		_data_end = .;
	}
	
	.bss ALIGN(4096) :
	{
		_bss_start = .;
		*(.bss)
		*(.bss.*)
		_bss_end = .;
	}
}
