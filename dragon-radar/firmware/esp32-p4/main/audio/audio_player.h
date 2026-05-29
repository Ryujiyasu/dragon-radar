#pragma once

typedef enum {
    AUDIO_CUE_PROXIMITY = 0,  /* ball entered reach: short blip */
    AUDIO_CUE_COLLECTED,      /* ball collected: ascending chime */
    AUDIO_CUE_SUMMON,         /* all collected: rising fanfare */
} audio_cue_t;

void audio_player_init(void);

/* Non-blocking: queues a synthesized cue, played by the audio task. */
void audio_player_cue(audio_cue_t cue);

/* Phase 4 later: play a WAV file from SD/flash. */
void audio_player_play_wav(const char *path);
