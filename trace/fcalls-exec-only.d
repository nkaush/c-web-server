
#pragma D option quiet
#pragma D option aggsortkey=1
#pragma D option aggsortkeypos=0

BEGIN {
    printf("\n======================================================================\n");
    printf("                    Function Call Count Statistics\n");
    printf("======================================================================\n");
}

pid$target:server-debug::entry {
    @call_counts_per_function[probefunc] = count();
}

END {
    printf("%-40s %12s\n\n", "Function name", "Count");
    printa("%-40s %@12lu\n", @call_counts_per_function);
}