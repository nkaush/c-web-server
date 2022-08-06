#include "format.h"
#include <assert.h>

int main(int argc, char** argv) {
    time_t expires = time(NULL) + 691200;
    char buf[TIME_BUFFER_SIZE] = { 0 };
    format_time(buf, expires);

    LOG("%s", buf);

    time_t parsed = parse_time_str(buf);

    struct tm gmt;
    strptime(buf, TIME_FMT_TZ, &gmt);
    time_t gmt_parsed = mktime(&gmt);

    LOG("original = %ld / parsed1 = %ld / parsed2 = %ld", expires, parsed, gmt_parsed);
    assert(expires == parsed);
    assert(expires == gmt_parsed);
}
