#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: %s <media.mkv> [use_pkt_duration]\n", av[0]);
        return -1;
    }

    const char *filename = av[1];
    const int use_pkt_duration = ac > 2 ? atoi(av[2]) : 0;

    struct nmd_ctx *s = nmd_create(filename);
    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);
    struct nmd_frame *f;
    const double t = 1/60.;

    if (!s)
        return -1;

    f = nmd_get_frame(s, 0.);
    if (!f)
        return -1;
    nmd_frame_releasep(&f);
    f = nmd_get_frame(s, t);
    if (f && f->ts > t) {
        fprintf(stderr, "unexpected frame at %f with ts=%f\n", t, f->ts);
        nmd_frame_releasep(&f);
        return -1;
    }
    nmd_freep(&s);
    return 0;
}
