#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: %s <media> [<use_pkt_duration>]\n", av[0]);
        return -1;
    }

    const char *filename = av[1];
    const int use_pkt_duration = ac > 2 ? atoi(av[2]) : 0;

    struct nmd_ctx *s = nmd_create(filename);
    struct nmd_frame *f;

    if (!s)
        return -1;

    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);
    nmd_seek(s, 12.7);
    nmd_seek(s, 21.0);
    nmd_seek(s, 5.3);
    nmd_start(s);
    nmd_start(s);
    nmd_seek(s, 15.3);
    nmd_stop(s);
    nmd_start(s);
    nmd_stop(s);
    nmd_start(s);
    nmd_seek(s, 7.2);
    nmd_start(s);
    nmd_stop(s);
    nmd_seek(s, 82.9);
    f = nmd_get_frame(s, 83.5);
    if (!f) {
        nmd_free(&s);
        return -1;
    }
    nmd_release_frame(f);
    nmd_stop(s);
    f = nmd_get_frame(s, 83.5);
    if (!f) {
        nmd_free(&s);
        return -1;
    }
    nmd_free(&s);
    nmd_release_frame(f);
    return 0;
}
