#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: %s <media.mkv> [<use_pkt_duration>]\n", av[0]);
        return -1;
    }

    const char *filename = av[1];
    const int use_pkt_duration = ac > 2 ? atoi(av[2]) : 0;

    int i = 0;
    struct nmd_ctx *s = nmd_create(filename);

    if (!s)
        return -1;

    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);

    for (int r = 0; r < 2; r++) {
        printf("run #%d\n", r+1);
        for (;;) {
            struct nmd_frame *frame;

            int ret = nmd_get_next_frame(s, &frame);
            if (ret != NMD_RET_NEWFRAME) {
                printf("null frame\n");
                break;
            }
            printf("frame #%d / data:%p ts:%f %dx%d lz:%d nmdpixfmt:%d\n",
                   i++, frame->datap[0], frame->ts, frame->width, frame->height,
                   frame->linesizep[0], frame->pix_fmt);

            nmd_frame_releasep(&frame);
        }
    }

    nmd_freep(&s);

    if (i != 8192) {
        fprintf(stderr, "decoded %d/8192 expected frames\n", i);
        return -1;
    }

    return 0;
}
