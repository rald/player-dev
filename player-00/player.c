#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    ma_engine engine;
    ma_sound* playlist;
    int playlist_count;
    int current_index;
    int shuffle_mode;
    int shuffle_indices[1024];  // Simple shuffle buffer
} player_t;

player_t player = {0};

void load_playlist(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open playlist file '%s'\n", filename);
        return;
    }
    
    char path[1024];
    int capacity = 64;
    player.playlist = malloc(capacity * sizeof(ma_sound));
    player.playlist_count = 0;
    
    while (fgets(path, sizeof(path), file) && player.playlist_count < capacity) {
        // Remove newline
        path[strcspn(path, "\n")] = 0;
        if (strlen(path) == 0) continue;
        
        ma_result result = ma_sound_init_from_file(&player.engine, path, 0, NULL, NULL, 
                                                  &player.playlist[player.playlist_count]);
        if (result == MA_SUCCESS) {
            printf("Loaded: %s\n", path);
            player.playlist_count++;
        } else {
            printf("Failed to load: %s\n", path);
        }
    }
    
    fclose(file);
    player.current_index = 0;
    
    // Initialize shuffle indices
    for (int i = 0; i < player.playlist_count; i++) {
        player.shuffle_indices[i] = i;
    }
}

void stop_current() {
    if (player.current_index >= 0 && player.current_index < player.playlist_count) {
        ma_sound_stop(&player.playlist[player.current_index]);
    }
}

void play_index(int index) {
    stop_current();
    if (index >= 0 && index < player.playlist_count) {
        player.current_index = index;
        ma_sound_start(&player.playlist[player.current_index]);
        printf("Playing track %d/%d\n", index + 1, player.playlist_count);
    }
}

void next_track() {
    int next_idx = (player.current_index + 1) % player.playlist_count;
    play_index(player.shuffle_mode ? player.shuffle_indices[next_idx] : next_idx);
}

void prev_track() {
    int prev_idx = (player.current_index - 1 + player.playlist_count) % player.playlist_count;
    play_index(player.shuffle_mode ? player.shuffle_indices[prev_idx] : prev_idx);
}

void toggle_shuffle() {
    player.shuffle_mode = !player.shuffle_mode;
    printf("Shuffle %s\n", player.shuffle_mode ? "ON" : "OFF");
    
    if (player.shuffle_mode) {
        // Fisher-Yates shuffle
        for (int i = player.playlist_count - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = player.shuffle_indices[i];
            player.shuffle_indices[i] = player.shuffle_indices[j];
            player.shuffle_indices[j] = temp;
        }
    }
}

void print_help() {
    printf("\nControls:\n");
    printf("  n    - Next track\n");
    printf("  p    - Previous track\n");
    printf("  s    - Toggle shuffle\n");
    printf("  q    - Quit\n\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <playlist.txt>\n", argv[0]);
        printf("Playlist format: one MP3 path per line\n");
        return 1;
    }
    
    srand(time(NULL));
    
    // Initialize engine
    ma_result result = ma_engine_init(NULL, &player.engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio engine\n");
        return 1;
    }
    
    load_playlist(argv[1]);
    if (player.playlist_count == 0) {
        ma_engine_uninit(&player.engine);
        return 1;
    }
    
    play_index(0);
    print_help();
    
    char cmd;
    printf("> ");
    while ((cmd = getchar()) != 'q' && cmd != EOF) {
        while (getchar() != '\n');  // Clear input buffer
        
        switch (tolower(cmd)) {
            case 'n': next_track(); break;
            case 'p': prev_track(); break;
            case 's': toggle_shuffle(); break;
            default: print_help(); break;
        }
        printf("> ");
    }
    
    // Cleanup
    stop_current();
    for (int i = 0; i < player.playlist_count; i++) {
        ma_sound_uninit(&player.playlist[i]);
    }
    free(player.playlist);
    ma_engine_uninit(&player.engine);
    
    return 0;
}
