#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: %s <image.jpg> [<use_pkt_duration>]\n", av[0]);
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
    nmd_seek(s, 10.2);
    int ret = nmd_get_frame(s, 10.5, &f);
    if (ret != NMD_RET_NEWFRAME) {
        fprintf(stderr, "didn't get first image\n");
        return -1;
    }
    nmd_frame_releasep(&f);

    nmd_freep(&s);
    return 0;
}
