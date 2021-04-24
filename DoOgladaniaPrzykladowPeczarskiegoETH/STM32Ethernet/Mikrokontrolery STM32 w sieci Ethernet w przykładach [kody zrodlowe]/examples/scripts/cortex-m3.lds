SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        _etext = .;
        _sidata = _etext;
    } >FLASH =0xff

    .data : AT(_sidata)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } >RAM =0

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    /* Dodane, żeby się kompilowało za pomocą arm-none-eabi-gcc. */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
    }

    /* Stos umieszczamy na końcu RAM i wyrównujemy do 4 bajtów.
       W ARM rejestr SP wskazuje na wierzchołek stosu. */
    _estack = (ORIGIN(RAM) + LENGTH(RAM)) & 0xFFFFFFFC;

    /* Linker zgłosi błąd, gdy w RAM zostanie za mało miejsca
       na stos i stertę. */
    ASSERT(_ebss + _minimum_stack_and_heap_size <= _estack,
           "There is not enough space in RAM for stack and heap.")

    /* Początkowy koniec sterty, patrz funkcja sbrk w libc. */
    PROVIDE(end = _ebss);
    /* PROVIDE(_end = _ebss); */

    /* Teraz zostały już tylko informacje dla debuggera. */

    /* Usuń informacje dla debuggera ze standardowych bibliotek. */
    DISCARD :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    /* Zostaw wszystkie pozostałe informacje dla debuggera. */
    .stab            0 : {*(.stab)}
    .stabstr         0 : {*(.stabstr)}
    .stab.excl       0 : {*(.stab.excl)}
    .stab.exclstr    0 : {*(.stab.exclstr)}
    .stab.index      0 : {*(.stab.index)}
    .stab.indexstr   0 : {*(.stab.indexstr)}
    .comment         0 : {*(.comment)}
    .debug           0 : {*(.debug)}
    .line            0 : {*(.line)}
    .debug_srcinfo   0 : {*(.debug_srcinfo)}
    .debug_sfnames   0 : {*(.debug_sfnames)}
    .debug_aranges   0 : {*(.debug_aranges)}
    .debug_pubnames  0 : {*(.debug_pubnames)}
    .debug_info      0 : {*(.debug_info .gnu.linkonce.wi.*)}
    .debug_abbrev    0 : {*(.debug_abbrev)}
    .debug_line      0 : {*(.debug_line)}
    .debug_frame     0 : {*(.debug_frame)}
    .debug_str       0 : {*(.debug_str)}
    .debug_loc       0 : {*(.debug_loc)}
    .debug_macinfo   0 : {*(.debug_macinfo)}
    .debug_weaknames 0 : {*(.debug_weaknames)}
    .debug_funcnames 0 : {*(.debug_funcnames)}
    .debug_typenames 0 : {*(.debug_typenames)}
    .debug_varnames  0 : {*(.debug_varnames)}
}
