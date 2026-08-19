#pragma once

#define DEBUG_SAVE_PCM      (1)
#define PCM_ONE_CHANNEL     (1)
#define FILE_SIZE (256000)
#define MAX_FILE_SIZE       (1*1024*1024)
#define RECORD_NAME         "/spiffs/record.wav"

typedef struct {
    uint8_t ChunkID[4];
    int32_t ChunkSize;
    uint8_t Format[4];
    uint8_t Subchunk1ID[4];
    int32_t Subchunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
    uint8_t Subchunk2ID[4];
    int32_t Subchunk2Size;
} wav_header_t;

typedef void (*audio_play_finish_cb_t)(void);

void sr_handler_task(void *pvParam);
extern uint8_t *audio_rx_buffer;

esp_err_t audio_play_task(void *filepath);
void audio_record_init(void);
void audio_record_save(int16_t *audio_buffer, int audio_chunksize);
void audio_register_play_finish_cb(audio_play_finish_cb_t cb);
