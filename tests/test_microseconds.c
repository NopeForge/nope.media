#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <nopemd.h>

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: %s <media.mkv> [<use_pkt_duration>]\n", av[0]);
        return -1;
    }

    const char *filename = av[1];
    const int use_pkt_duration = ac > 2 ? atoi(av[2]) : 0;

    struct nmd_ctx *s1 = nmd_create(filename);
    struct nmd_ctx *s2 = nmd_create(filename);
    struct nmd_frame *f1, *f2;

    if (!s1 || !s2)
        return -1;

    nmd_set_option(s1, "auto_hwaccel", 0);
    nmd_set_option(s1, "use_pkt_duration", use_pkt_duration);
    nmd_set_option(s2, "auto_hwaccel", 0);
    nmd_set_option(s2, "use_pkt_duration", use_pkt_duration);
    f1 = nmd_get_frame(s1, 3.0);
    f2 = nmd_get_frame_ms(s2, 3*1000000);
    if (!f1 || !f2)
        return -1;

    if (f1->ts != f2->ts) {
        fprintf(stderr, "%g != %g\n", f1->ts, f2->ts);
        return -1;
    }

    if (f1->ms != f2->ms) {
        fprintf(stderr, "%"PRId64" != %"PRId64"\n", f1->ms, f2->ms);
        return -1;
    }

    nmd_release_frame(f1);
    nmd_release_frame(f2);
    nmd_free(&s1);
    nmd_free(&s2);
    return 0;
}
