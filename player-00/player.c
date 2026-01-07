#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define MAX_PATH_LEN 512
#define MAX_PLAYLIST_SIZE 256

typedef struct {
    char filenames[MAX_PLAYLIST_SIZE][MAX_PATH_LEN];
    int indices[MAX_PLAYLIST_SIZE];
    int count;
    bool shuffled;
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
            playlist->indices[playlist->count] = playlist->count;
            playlist->count++;
            if (playlist->count >= MAX_PLAYLIST_SIZE) break;
        }
    }
    fclose(file);
    return playlist->count > 0;
}

void shuffle_playlist(Playlist* playlist) {
    for (int i = playlist->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = playlist->indices[i];
        playlist->indices[i] = playlist->indices[j];
        playlist->indices[j] = temp;
    }
    playlist->shuffled = true;
    printf("Playlist shuffled\n");
}

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
    bool shuffle = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) shuffle = true;
    }
    
    if (argc < 2 || (argc == 2 && strcmp(argv[1], "-s") == 0)) {
        printf("Usage: %s [-s] <playlist.txt>\n", argv[0]);
        printf("  -s : shuffle mode\n");
        printf("Controls: N=Next, P=Prev, Esc=Quit\n");
        return -1;
    }
    
    srand(time(NULL));
    
    Playlist playlist = {0};
    char* playlist_file = shuffle ? argv[argc-1] : argv[1];
    if (!load_playlist(playlist_file, &playlist)) {
        printf("No valid tracks found\n");
        return -1;
    }
    
    if (shuffle) shuffle_playlist(&playlist);
    
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
    
    printf("Playlist: %s (%d tracks)\n", playlist_file, playlist.count);
    if (playlist.shuffled) printf("SHUFFLE MODE\n");
    printf("N=Next, P=Prev, Esc=Quit\n\n");
    
    while (true) {
        int track_idx = playlist.indices[current_track];
        printf("[%d/%d] %s\n", current_track+1, playlist.count, 
               playlist.filenames[track_idx]);
        
        result = ma_sound_init_from_file(&engine, playlist.filenames[track_idx], 
                                       0, NULL, NULL, &sound);
        if (result != MA_SUCCESS) {
            printf("Skip: %s\n", playlist.filenames[track_idx]);
            goto next_song;
        }
        sound_initialized = true;
        
        ma_sound_start(&sound);
        
        while (ma_sound_is_playing(&sound)) {
            char key = getch();
            if (key == 'n' || key == 'N') {
                printf("\n--- Next ---\n");
                goto next_song;
            }
            if (key == 'p' || key == 'P') {
                printf("\n--- Previous ---\n");
                current_track = (current_track - 1 + playlist.count) % playlist.count;
                goto prev_song;
            }
            if (key == 27) {  // ESC key (ASCII 27)
                goto cleanup;
            }
            ma_sleep(10);
        }
        
    next_song:
        if (sound_initialized) {
            ma_sound_uninit(&sound);
            sound_initialized = false;
        }
        current_track = (current_track + 1) % playlist.count;
        continue;
        
    prev_song:
        if (sound_initialized) {
            ma_sound_uninit(&sound);
            sound_initialized = false;
        }
        continue;
    }
    
cleanup:
    if (sound_initialized) {
        ma_sound_uninit(&sound);
    }
    ma_engine_uninit(&engine);
    printf("\nPlayer stopped\n");
    return 0;
}

