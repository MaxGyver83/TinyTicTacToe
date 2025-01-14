#include "audio.h"
#include <SLES/OpenSLES.h>            // for SLObjectItf_, SL_RESULT_SUCCESS
#ifndef X11
#include <SLES/OpenSLES_Android.h>    // for SLDataLocator_AndroidFD, SL_DAT...
#endif
#include <android/asset_manager.h>    // for AAssetManager_open, AAsset_close
#include <stdbool.h>                  // for bool, false, true
#include <stdlib.h>                   // for NULL
#include <string.h>                   // for memcpy
#include <sys/types.h>                // for off_t
#include <unistd.h>                   // for close
#include <android_native_app_glue.h>  // for android_app
#include "init.h"                     // for g_app
#include "utils.h"                    // for debug, error, info

#define MAX_PLAYERS 5
#define MAX_USES 100
#define SL_PLAYSTATE_UNKNOWN 999

typedef struct {
	SLObjectItf player_object;
	SLPlayItf player_play;
	SLSeekItf player_seek;
	int fd; // file descriptor
	int uses;
	char asset_path[256];
} AudioPlayer;

static AudioPlayer players[MAX_PLAYERS] = { { NULL, NULL, NULL, -1, 0, { 0 } } };
static SLObjectItf engine_object = NULL;
static SLEngineItf engine_engine;
static SLObjectItf output_mix_object = NULL;
static int current_player_index = 0;

static void
reset_player(AudioPlayer *player)
{
	memset(player, 0, sizeof(AudioPlayer));
	player->fd = -1;
}

void
create_audio_engine()
{
	SLresult result;

	// create engine
	result = slCreateEngine(&engine_object, 0, NULL, 0, NULL, NULL);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to create engine: %d", result);
		return;
	}

	// init engine
	result = (*engine_object)->Realize(engine_object, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to realize engine: %d", result);
		return;
	}

	// get interface engine
	result = (*engine_object)->GetInterface(engine_object, SL_IID_ENGINE, &engine_engine);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to get engine interface: %d", result);
		return;
	}

	// create output sound
	const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean req[1] = { SL_BOOLEAN_FALSE };
	result = (*engine_engine)->CreateOutputMix(engine_engine, &output_mix_object, 1, ids, req);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to create output mix: %d", result);
		return;
	}

	// init output
	result = (*output_mix_object)->Realize(output_mix_object, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to realize output mix: %d", result);
		return;
	}
}

static bool
create_audio_player(AudioPlayer *player, const char *asset_path)
{
	SLresult result;

	AAsset *audio_asset = AAssetManager_open(g_app->activity->assetManager, asset_path, AASSET_MODE_BUFFER);
	if (!audio_asset) {
		error("Failed to open asset file: %s", asset_path);
		return false;
	}

	off_t start, length;
	int fd = AAsset_openFileDescriptor(audio_asset, &start, &length);
	AAsset_close(audio_asset);
	if (fd < 0) {
		error("Failed to get file descriptor for asset: %s", asset_path);
		return false;
	}

	// create source data
	SLDataLocator_AndroidFD loc_fd = { SL_DATALOCATOR_ANDROIDFD, fd, start, length };
	SLDataFormat_MIME format_mime = { SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_MP3 };
	SLDataSource audioSrc = { &loc_fd, &format_mime };

	// setting receive data
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, output_mix_object };
	SLDataSink audio_snk = { &loc_outmix, NULL };

	// create player
	const SLInterfaceID ids[3] = { SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME };
	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
	result = (*engine_engine)->CreateAudioPlayer(engine_engine, &player->player_object, &audioSrc, &audio_snk, 3, ids, req);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to create audio player: %d", result);
		close(fd);
		return false;
	}

	// init player
	result = (*player->player_object)->Realize(player->player_object, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to realize player: %d", result);
		close(fd);
		return false;
	}

	// get interface player
	result = (*player->player_object)->GetInterface(player->player_object, SL_IID_PLAY, &player->player_play);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to get play interface: %d", result);
		close(fd);
		return false;
	}

	// get interface seek
	result = (*player->player_object)->GetInterface(player->player_object, SL_IID_SEEK, &player->player_seek);
	if (result != SL_RESULT_SUCCESS) {
		error("Failed to get seek interface: %d", result);
		close(fd);
		return false;
	}

	player->fd = fd;
	player->uses = 0;
	strncpy(player->asset_path, asset_path, sizeof(player->asset_path));
	debug("Successfully created audio player for asset: %s", asset_path);
	return true;
}

static SLuint32
get_play_state(SLPlayItf player_play)
{
	SLuint32 state;
	SLresult result = (*player_play)->GetPlayState(player_play, &state);
	if (result == SL_RESULT_SUCCESS)
		return state;
	error("Failed to get play state. result = %d", result);
	return SL_PLAYSTATE_UNKNOWN;
}

static SLuint32
stop_player(SLPlayItf player_play)
{
	SLresult result = (*player_play)->SetPlayState(player_play, SL_PLAYSTATE_STOPPED);
	if (result == SL_RESULT_SUCCESS)
		return get_play_state(player_play);
	error("Failed to set play state. result = %d", result);
	return SL_PLAYSTATE_UNKNOWN;
}

static int
find_available_player(const char *asset_path)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].player_play
				&& strncmp(players[i].asset_path, asset_path, sizeof(players[i].asset_path)) == 0) {
			SLuint32 state = get_play_state(players[i].player_play);
			if (state == SL_PLAYSTATE_PAUSED) {
				debug("Player %d is paused. Stopping it.", i);
				state = stop_player(players[i].player_play);
			}
			if (state == SL_PLAYSTATE_STOPPED) {
				debug("Reusing player %d for asset: %s", i, asset_path);
				return i;
			}
			if (state == SL_PLAYSTATE_UNKNOWN)
				return SL_PLAYSTATE_UNKNOWN;
			debug("Player %d is not available.", i);
		}
	}
	return -1;
}

void
play_audio(const char *asset_path)
{
	SLresult result;

	// find an available player
	SLPlayItf player_play = NULL;
	int available_index = find_available_player(asset_path);
	if (available_index == SL_PLAYSTATE_UNKNOWN)
		return;
	if (available_index >= 0)
		player_play = players[available_index].player_play;

	// if no available player, create a new one
	if (!player_play) {
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (!players[i].player_play) {
				if (!create_audio_player(&players[i], asset_path))
					return;
				player_play = players[i].player_play;
				available_index = i;
				debug("Created new player %d for asset: %s", i, asset_path);
				break;
			}
		}
	}

	// if still no available player, destroy the next player in the cycle and create a new one
	if (!player_play) {
		int oldest_index = current_player_index;
		current_player_index = (current_player_index + 1) % MAX_PLAYERS; // move to the next player in the cycle

		debug("Destroying player %d to create a new one for asset: %s", oldest_index, asset_path);
		(*players[oldest_index].player_object)->Destroy(players[oldest_index].player_object);
		close(players[oldest_index].fd);
		reset_player(&players[oldest_index]);

		if (!create_audio_player(&players[oldest_index], asset_path))
			return;
		player_play = players[oldest_index].player_play;
		available_index = oldest_index;
		debug("Created new player %d for asset: %s", oldest_index, asset_path);
	}

	// start playing
	if (player_play) {
		result = (*player_play)->SetPlayState(player_play, SL_PLAYSTATE_PLAYING);
		if (result != SL_RESULT_SUCCESS) {
			error("Failed to start playing: %d", result);
			return;
		}
		debug("Started playing asset: %s", asset_path);
		players[available_index].uses++;

		// check if the player has reached the maximum uses
		if (players[available_index].uses >= MAX_USES) {
			debug("Player %d has reached the maximum uses, destroying and creating a new one", available_index);
			(*players[available_index].player_object)->Destroy(players[available_index].player_object);
			close(players[available_index].fd);
			reset_player(&players[available_index]);

			if (!create_audio_player(&players[available_index], asset_path))
				return;
			player_play = players[available_index].player_play;
			debug("Created new player %d for asset: %s", available_index, asset_path);
		}
	} else {
		info("No available player to play asset: %s", asset_path);
	}
}

void
destroy_audio_player()
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].player_object) {
			(*players[i].player_object)->Destroy(players[i].player_object);
			close(players[i].fd);
			reset_player(&players[i]);
			debug("Destroyed player %d", i);
		}
	}
}

void
destroy_audio_engine()
{
	if (output_mix_object) {
		(*output_mix_object)->Destroy(output_mix_object);
		output_mix_object = NULL;
		info("Destroyed output mix object");
	}

	if (engine_object) {
		(*engine_object)->Destroy(engine_object);
		engine_object = NULL;
		engine_engine = NULL;
		info("Destroyed engine object");
	}
}
