#pragma D option quiet

pid$target:::entry {
    @all_calls[probefunc,probemod] = count();
}

END {
    printa(@all_calls);
}