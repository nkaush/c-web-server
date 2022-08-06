#pragma D option quiet

pid$target:server-debug::entry
/address[probefunc] == 0/
{
    address[probefunc]=uregs[R_PC];
}

pid$target:server-debug::
/address[probefunc] != 0/
{
    @a[probefunc,uregs[R_PC]]=count();
}

END
{
    printa("%s+%#x:\t%@d\n", @a);
}