#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char** files;
    int count;
    int current;
    bool shuffle;
} Playlist;

Playlist playlist = {0};
ma_engine engine;
ma_sound* current_sound = NULL;
bool is_playing = false;

void load_playlist(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open playlist file '%s'\n", filename);
        exit(1);
    }
    
    // Count lines first
    char line[1024];
    int count = 0;
    while (fgets(line, sizeof(line), file)) {
        // Skip empty lines and comments
        if (line[0] != '\n' && line[0] != '#' && line[0] != '\r') {
            count++;
        }
    }
    rewind(file);
    
    if (count == 0) {
        printf("Error: Playlist is empty\n");
        fclose(file);
        exit(1);
    }
    
    playlist.files = malloc(count * sizeof(char*));
    playlist.count = count;
    playlist.current = 0;
    
    // Read actual file paths
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < count) {
        // Remove newline and trim
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] != '\n' && line[0] != '#' && line[0] != '\r' && strlen(line) > 0) {
            playlist.files[i] = strdup(line);
            i++;
        }
    }
    
    fclose(file);
    printf("Loaded %d songs from '%s'\n", playlist.count, filename);
}

int get_next_index() {
    if (playlist.shuffle) {
        return rand() % playlist.count;
    }
    return (playlist.current + 1) % playlist.count;
}

int get_prev_index() {
    return (playlist.current - 1 + playlist.count) % playlist.count;
}

void play_index(int index) {
    if (current_sound) {
        ma_sound_stop(current_sound);
        ma_sound_uninit(current_sound);
        free(current_sound);
        current_sound = NULL;
    }
    
    current_sound = malloc(sizeof(ma_sound));
    ma_result result = ma_sound_init_from_file(&engine, playlist.files[index], 
                                             MA_SOUND_FLAG_DECODE, NULL, NULL, 
                                             current_sound);
    
    if (result != MA_SUCCESS) {
        printf("Failed to load %s\n", playlist.files[index]);
        return;
    }
    
    playlist.current = index;
    printf("Now playing: %s\n", playlist.files[index]);
    ma_sound_start(current_sound);
    is_playing = true;
}

void next_song() {
    int next = get_next_index();
    play_index(next);
}

void prev_song() {
    int prev = get_prev_index();
    play_index(prev);
}

void toggle_pause() {
    if (current_sound) {
        if (is_playing) {
            ma_sound_stop(current_sound);
        } else {
            ma_sound_start(current_sound);
        }
        is_playing = !is_playing;
        printf("Playback %s\n", is_playing ? "resumed" : "paused");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s playlist.m3u [--shuffle]\n", argv[0]);
        printf("Playlist format: one MP3 path per line (M3U compatible)\n");
        return 1;
    }
    
    srand(time(NULL));
    playlist.shuffle = (argc > 2 && strcmp(argv[2], "--shuffle") == 0);
    
    load_playlist(argv[1]);
    
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Failed to init audio engine\n");
        return 1;
    }
    
    // Start first song
    play_index(0);
    
    printf("Controls: n=next, p=prev, s=pause/resume, q=quit\n");
    
    char cmd;
    while ((cmd = getchar()) != 'q') {
        switch (cmd) {
            case 'n': case 'N': next_song(); break;
            case 'p': case 'P': prev_song(); break;
            case 's': case 'S': toggle_pause(); break;
        }
    }
    
    // Cleanup
    for (int i = 0; i < playlist.count; i++) {
        free(playlist.files[i]);
    }
    free(playlist.files);
    
    if (current_sound) {
        ma_sound_uninit(current_sound);
        free(current_sound);
    }
    ma_engine_uninit(&engine);
    
    return 0;
}
