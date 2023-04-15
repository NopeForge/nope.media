#include <stdio.h>
#include <stdlib.h>

#include <nopemd.h>

#define FLAG_SKIP          (1<<0)
#define FLAG_END_TIME      (1<<1)
#define FLAG_AUDIO         (1<<2)

int main(int ac, char **av)
{
    if (ac < 3) {
        fprintf(stderr, "Usage: %s <media.mkv> <flags> [<use_pkt_duration>]\n", av[0]);
        return -1;
    }

    const char *filename = av[1];
    const int flags = atoi(av[2]);
    const int use_pkt_duration = ac > 3 ? atoi(av[3]) : 0;

    int ret = 0, nb_frames = 0;
    struct nmd_ctx *s = nmd_create(filename);
    struct nmd_frame *frame = NULL;
    static const float ts[] = { 0.0, 0.5, 7.65 };

    if (!s)
        return -1;

    const double skip     = (flags & FLAG_SKIP)     ? 60.0 : 0.0;
    const double end_time = (flags & FLAG_END_TIME) ? 70.0 : -1.0;
    const int avselect    = (flags & FLAG_AUDIO)    ? NMD_SELECT_AUDIO : NMD_SELECT_VIDEO;

    nmd_set_option(s, "auto_hwaccel", 0);
    nmd_set_option(s, "avselect", avselect);
    nmd_set_option(s, "audio_texture", 0);
    nmd_set_option(s, "start_time", skip);
    nmd_set_option(s, "end_time", end_time);
    nmd_set_option(s, "use_pkt_duration", use_pkt_duration);

    printf("run #1 (avselect=%d end_time=%f)\n", avselect, end_time);
    for (;;) {
        frame = nmd_get_next_frame(s);
        if (!frame) {
            break;
        }
        nmd_release_frame(frame);
        nb_frames++;
    }

    nmd_freep(&s);

    for (int k = 0; k < 4; k++) {
        for (int j = 0; j < sizeof(ts)/sizeof(*ts); j++) {
            s = nmd_create(filename);
            if (!s)
                return -1;

            nmd_set_option(s, "auto_hwaccel", 0);
            nmd_set_option(s, "avselect", avselect);
            nmd_set_option(s, "audio_texture", 0);
            nmd_set_option(s, "start_time", skip);
            nmd_set_option(s, "end_time", end_time);

            for (int i = 0; i < nb_frames; i++) {
                frame = nmd_get_next_frame(s);
                if (!frame) {
                    fprintf(stderr, "unexpected null frame before EOS\n");
                    ret = -1;
                    goto done;
                }
                nmd_release_frame(frame);
            }

            if (k == 0) {
                nmd_seek(s, ts[j]);
                frame = nmd_get_next_frame(s);
                if (!frame) {
                    fprintf(stderr, "unexpected null frame from nmd_get_next_frame() after seeking at %f\n", ts[j]);
                    ret = -1;
                    goto done;
                }
            } else if (k == 1) {
                frame = nmd_get_frame(s, ts[j]);
                if (!frame) {
                    fprintf(stderr, "unexpected null frame from nmd_get_frame() after seeking at %f\n", ts[j]);
                    ret = -1;
                    goto done;
                }
            } else if (k == 2) {
                frame = nmd_get_frame(s, 1000.0);
                if (frame) {
                    fprintf(stderr, "unexpected frame at %f with ts=%f\n", 1000.0f, frame->ts);
                    ret = -1;
                    nmd_release_frame(frame);
                    goto done;
                }
            } else if (k == 3) {
                frame = NULL;
            }
            nmd_release_frame(frame);
            nmd_freep(&s);
        }
    }

done:
    nmd_freep(&s);

    return ret;
}
