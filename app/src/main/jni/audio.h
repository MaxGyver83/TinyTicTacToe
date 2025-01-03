#ifndef AUDIO_H
#define AUDIO_H

void create_audio_engine(void);
void play_audio(const char *asset_path);
void destroy_audio_player(void);
void destroy_audio_engine(void);

#endif // AUDIO_H
