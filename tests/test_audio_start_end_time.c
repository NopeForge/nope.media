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

    int i = 0, smp = 0;
    struct nmd_ctx *s = nmd_create(filename);

    if (!s)
        return -1;

    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);
    nmd_set_option(s, "avselect", NMD_SELECT_AUDIO);
    nmd_set_option(s, "audio_texture", 0);
    nmd_set_option(s, "start_time", 30.0);
    nmd_set_option(s, "end_time", 90.0);

    for (int r = 0; r < 2; r++) {
        printf("run #%d\n", r+1);
        for (;;) {
            struct nmd_frame *frame;
            int ret = nmd_get_next_frame(s, &frame);
            if (ret != NMD_RET_NEWFRAME) {
                printf("null frame\n");
                break;
            }
            printf("frame #%d / data:%p ts:%f nb_samples:%d nmdsmpfmt:%d\n",
                   i++, frame->datap[0], frame->ts, frame->nb_samples, frame->pix_fmt);
            smp += frame->nb_samples;

            nmd_frame_releasep(&frame);
        }
    }

    nmd_freep(&s);

    if (smp != 5299200) {
        fprintf(stderr, "decoded %d/5299200 expected samples\n", smp);
        return -1;
    }

    return 0;
}
