#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

static const char *fake_filename = "/i/do/not/exist";

static void log_callback(void *arg, int level, const char *filename, int ln,
                         const char *fn, const char *fmt, va_list vl)
{
    if (arg != fake_filename)
        abort();
    printf("level=%d filename=%s ln=%d fn=%s fmt=%s\n", level, filename, ln, fn, fmt);
}

int main(int ac, char **av)
{
    const int use_pkt_duration = ac > 1 ? atoi(av[1]) : 0;

    struct nmd_ctx *s = nmd_create(fake_filename);

    if (!s)
        return -1;
    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);
    nmd_set_log_callback(s, (void*)fake_filename, log_callback);
    nmd_release_frame(nmd_get_frame(s, -1));
    nmd_release_frame(nmd_get_frame(s, 1.0));
    nmd_release_frame(nmd_get_frame(s, 3.0));
    nmd_free(&s);
    return 0;
}
