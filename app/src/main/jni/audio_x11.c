#include "audio.h"
#include <alsa/asoundlib.h>
#include <errno.h>              // for EPIPE
#include <stdio.h>              // for fprintf, stderr, snprintf
#include <stdlib.h>             // for exit
#include <string.h>             // for strstr
#include <vorbis/codec.h>       // for vorbis_info
#include <vorbis/vorbisfile.h>  // for ov_clear, ov_fopen, ov_info, ov_read
#include "utils.h"              // for error

static snd_pcm_t *pcm_handle = NULL;

static bool
init_audio(unsigned int rate, int channels)
{
	// Configure ALSA
	snd_pcm_hw_params_t *params;

	if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		error("Error opening PCM device.");
		return false;
	}

	snd_pcm_hw_params_malloc(&params);
	snd_pcm_hw_params_any(pcm_handle, params);
	snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);

	if (snd_pcm_hw_params(pcm_handle, params) < 0) {
		error("Error setting PCM parameters.");
		snd_pcm_close(pcm_handle);
		exit(1);
	}

	snd_pcm_hw_params_free(params);
	return true;
}

static void
play_ogg(const char *filename)
{
	OggVorbis_File vf = {0};
	int current_section;
	char buffer[4096];
	int bytes_read;

	// Open the .ogg file
	if (vf.datasource == NULL) {
		if (ov_fopen(filename, &vf) < 0) {
			error("Error opening OGG file: %s", filename);
			return;
		}
	}

	// Retrieve audio information
	vorbis_info *vi = ov_info(&vf, -1);
	if (!vi) {
		error("Error retrieving Vorbis information.");
		ov_clear(&vf);
		return;
	}

	if (pcm_handle == NULL && init_audio(vi->rate, vi->channels) == false) {
		ov_clear(&vf);
		return;
	}

	// Decode OGG data and play
	while ((bytes_read = ov_read(&vf, buffer, sizeof(buffer), 0, 2, 1, &current_section)) > 0) {
		int frames = bytes_read / (2 * vi->channels); // 2 bytes per sample
		if (snd_pcm_writei(pcm_handle, buffer, frames) == -EPIPE) {
			snd_pcm_prepare(pcm_handle);
		}
	}

	ov_clear(&vf);
}

void
play_audio(const char *filename)
{
	if (!endswith(".ogg", filename)) {
		error("Only ogg files are supported!");
		return;
	}

	char adapted_path[256];
	snprintf(adapted_path, sizeof(adapted_path), "../assets/%s", filename);
	play_ogg(adapted_path);
}

void
destroy_audio_player()
{
	if (pcm_handle) {
		snd_pcm_drain(pcm_handle);
		snd_pcm_close(pcm_handle);
	}
}

void create_audio_engine() {}
void destroy_audio_engine() {}
