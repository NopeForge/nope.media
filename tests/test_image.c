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

    struct nmd_info info;
    struct nmd_ctx *s = nmd_create(filename);
    struct nmd_frame *f;

    if (!s)
        return -1;
    nmd_set_option(s, "start_time", 3.0);
    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);
    int ret = nmd_get_frame(s, 53.0, &f);
    if (ret != NMD_RET_NEWFRAME) {
        fprintf(stderr, "didn't get an image\n");
        return -1;
    }
    nmd_frame_releasep(&f);

    if (nmd_get_info(s, &info) < 0) {
        fprintf(stderr, "can not fetch image info\n");
    }
    if (info.width != 480 || info.height != 640) {
        fprintf(stderr, "image isn't the expected size\n");
        return -1;
    }

    ret = nmd_get_frame(s, 12.3, &f);
    if (ret == NMD_RET_NEWFRAME) {
        nmd_frame_releasep(&f);
        fprintf(stderr, "we got a new frame even though the source is an image\n");
        return -1;
    }

    nmd_freep(&s);
    return 0;
}
