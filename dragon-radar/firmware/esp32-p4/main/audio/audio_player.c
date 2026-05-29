#include "audio_player.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SR        22050
#define VOL       80

static const char *TAG = "audio";
static esp_codec_dev_handle_t s_spk;
static QueueHandle_t s_queue;

/* Synthesize a sine note with a linear attack/release envelope to avoid clicks,
 * streaming it to the codec in small chunks. */
static void play_note(float freq, int ms, float amp)
{
    const int total = SR * ms / 1000;
    const int fade  = total / 8 + 1;
    int16_t buf[256];
    double phase = 0.0, dphase = 2.0 * M_PI * freq / SR;

    int done = 0;
    while (done < total) {
        int n = (total - done < 256) ? (total - done) : 256;
        for (int i = 0; i < n; i++) {
            int pos = done + i;
            float env = 1.0f;
            if (pos < fade)              env = (float)pos / fade;
            else if (pos > total - fade) env = (float)(total - pos) / fade;
            buf[i] = (int16_t)(amp * env * 32767.0f * sin(phase));
            phase += dphase;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        esp_codec_dev_write(s_spk, buf, n * (int)sizeof(int16_t));
        done += n;
    }
}

static void play_cue(audio_cue_t cue)
{
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel         = 1,
        .channel_mask    = 1,
        .sample_rate     = SR,
    };
    if (esp_codec_dev_open(s_spk, &fs) != 0) {
        ESP_LOGW(TAG, "codec open failed");
        return;
    }
    switch (cue) {
    case AUDIO_CUE_PROXIMITY:
        play_note(1200, 70, 0.3f);
        break;
    case AUDIO_CUE_COLLECTED:
        play_note(880, 90, 0.4f);
        play_note(1320, 150, 0.4f);
        break;
    case AUDIO_CUE_SUMMON:
        play_note(523, 120, 0.4f);
        play_note(659, 120, 0.4f);
        play_note(784, 120, 0.4f);
        play_note(1047, 320, 0.5f);
        break;
    }
    esp_codec_dev_close(s_spk);
}

static void audio_task(void *arg)
{
    (void)arg;
    audio_cue_t cue;
    while (1) {
        if (xQueueReceive(s_queue, &cue, portMAX_DELAY)) {
            play_cue(cue);
        }
    }
}

void audio_player_init(void)
{
    if (bsp_audio_init(NULL) != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_init failed");
        return;
    }
    s_spk = bsp_audio_codec_speaker_init();
    if (!s_spk) {
        ESP_LOGE(TAG, "speaker init failed");
        return;
    }
    esp_codec_dev_set_out_vol(s_spk, VOL);

    s_queue = xQueueCreate(4, sizeof(audio_cue_t));
    xTaskCreate(audio_task, "audio", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "audio ready (codec %p)", s_spk);
}

void audio_player_cue(audio_cue_t cue)
{
    if (s_queue) xQueueSend(s_queue, &cue, 0);  /* non-blocking, drop if full */
}

void audio_player_play_wav(const char *path) { (void)path; }
