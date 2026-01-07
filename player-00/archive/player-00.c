#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#define MAX_PATH_LEN 512
#define MAX_PLAYLIST_SIZE 256

typedef struct {
    char filenames[MAX_PLAYLIST_SIZE][MAX_PATH_LEN];
    int count;
} Playlist;

bool load_playlist(const char* playlist_file, Playlist* playlist) {
    FILE* file = fopen(playlist_file, "r");
    if (!file) {
        printf("Failed to open playlist: %s\n", playlist_file);
        return false;
    }
    
    playlist->count = 0;
    while (fgets(playlist->filenames[playlist->count], MAX_PATH_LEN, file)) {
        size_t len = strlen(playlist->filenames[playlist->count]);
        if (len > 0 && playlist->filenames[playlist->count][len-1] == '\n')
            playlist->filenames[playlist->count][len-1] = '\0';
        
        if (strlen(playlist->filenames[playlist->count]) > 0) {
            playlist->count++;
            if (playlist->count >= MAX_PLAYLIST_SIZE) break;
        }
    }
    
    fclose(file);
    return playlist->count > 0;
}

// Non-blocking keyboard input
char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <playlist.txt>\n", argv[0]);
        printf("Space=Next, Enter=Quit\n");
        return -1;
    }
    
    Playlist playlist = {0};
    if (!load_playlist(argv[1], &playlist)) {
        printf("No valid tracks found\n");
        return -1;
    }
    
    ma_result result;
    ma_engine engine;
    ma_sound sound;
    int current_track = 0;
    bool sound_initialized = false;
    
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize engine\n");
        return -1;
    }
    
    printf("Playlist: %s (%d tracks)\n", argv[1], playlist.count);
    printf("Space=Next Track, Enter=Quit\n\n");
    
    while (current_track < playlist.count) {
        printf("[%d/%d] %s\n", current_track+1, playlist.count, 
               playlist.filenames[current_track]);
        
        result = ma_sound_init_from_file(&engine, playlist.filenames[current_track], 
                                       0, NULL, NULL, &sound);
        if (result != MA_SUCCESS) {
            printf("Skip: %s\n", playlist.filenames[current_track]);
            current_track++;
            continue;
        }
        sound_initialized = true;
        
        ma_sound_start(&sound);
        
        // Poll with key checking
        while (ma_sound_is_playing(&sound)) {
            char key = getch();
            if (key == ' ') {  // Spacebar = next
                printf("\n--- Next Track ---\n");
                goto next_track;
            }
            if (key == '\n' || key == '\r') {  // Enter = quit
                goto cleanup;
            }
            ma_sleep(10);
        }
        
    next_track:
        ma_sound_uninit(&sound);
        sound_initialized = false;
        current_track++;
    }
    
cleanup:
    if (sound_initialized) {
        ma_sound_uninit(&sound);
    }
    ma_engine_uninit(&engine);
    printf("\nPlaylist complete\n");
    return 0;
}

