#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <stdbool.h>

int main(int argc,char **argv) {
    ma_result result;
    ma_engine engine;
    ma_sound sound;

    // Initialize engine
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize engine\n");
        return -1;
    }

    // Load and play MP3 asynchronously (decodes on-the-fly)
    result = ma_sound_init_from_file(&engine, argv[1], 0, NULL, NULL, &sound);
    if (result != MA_SUCCESS) {
        printf("Failed to load MP3\n");
        ma_engine_uninit(&engine);
        return -1;
    }

    ma_sound_start(&sound);  // Non-blocking start [web:1][web:5]

    printf("Playing: %s\n",argv[1]);

    // Poll until end (integrate this into your game loop)
    while (ma_sound_is_playing(&sound)) {
        if (getchar() == '\n') break;  // Early exit
        ma_sleep(10);  // 100 FPS polling, non-blocking
    }

    // Cleanup
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
    printf("Playback finished\n");
    return 0;
}
