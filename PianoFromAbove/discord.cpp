#define _CRT_SECURE_NO_WARNINGS /* thanks Microsoft */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <discord_rpc.h>

static const char* APPLICATION_ID = "668327430733299713";
static int64_t StartTime;
static int SendPresence = 1;

static void updateDiscordPresence(const char* state, const char* details)
{
  DiscordRichPresence discordPresence;
  memset(&discordPresence, 0, sizeof(discordPresence));
  discordPresence.state = state;
  discordPresence.details = details;
  discordPresence.largeImageKey = "bpfa_icon";
  discordPresence.instance = 0;
  Discord_UpdatePresence(&discordPresence);
}

static void handleDiscordReady(const DiscordUser* connectedUser)
{
  printf("\nDiscord: connected to user %s#%s - %s\n",
    connectedUser->username,
    connectedUser->discriminator,
    connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
  printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
  printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

static void discordInit()
{
  DiscordEventHandlers handlers;
  memset(&handlers, 0, sizeof(handlers));
  handlers.ready = handleDiscordReady;
  handlers.disconnected = handleDiscordDisconnected;
  handlers.errored = handleDiscordError;
  Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}