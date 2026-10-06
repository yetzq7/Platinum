#pragma once

struct FConfiguration
{
 //static inline auto Playlist = L"/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo";
 // static inline auto Playlist = L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo";
 static inline auto Playlist = L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo";

    static inline auto MaxTickRate = 120;
    static inline auto bLateGame = false;
    static inline auto SiphonAmount = 50;
    static inline auto LateGameZone = 4;
    static inline auto bLateGameLongZone = false;
    static inline auto GameStartTime = 110.f;

    static inline auto PregameMats = false; // pretty ass for now

    static inline constexpr auto ApiKey = "platinum"; // backend api key (reload/phoenix/better reload or any reload fork ig)
    static inline constexpr auto VbucksAPI = "http://127.0.0.1:6767/api/skid/vbucks"; // for vb on kills n wins

    // tournament stuff
    // only if enabletournaments is set to true on Better Phoenix
    // sends data back to better phoenix! (public soon)
    static inline auto bTournament = false;
    static inline constexpr auto TournamentDataURL = "http://127.0.0.1:6767/datarouter/api/v1/public/data";

    // post.h
    // matchmaker stuff for phoenix mm/astrid
    // dont use
    static inline auto bAPI = false;

    static inline auto CreativeTerrainPath = L"/Game/Playgrounds/Items/Plots/Tropical_Medium.Tropical_Medium";

    static inline auto bEnableCheats = false;
    static inline auto bInfiniteMats = false;
    static inline auto bInfiniteAmmo = false;
    static inline auto bForceRespawns = false; // build your client with this too!
    static inline auto bJoinInProgress = false;
    static inline auto bAutoRestart = false;
    static inline auto bKeepInventory = false;
    static inline auto Port = 7777;
    static inline auto bEnableIris = true;
    static inline constexpr auto bGUI = false;
    static inline constexpr auto bCustomCrashReporter = true;
    static inline constexpr auto bUseStdoutLog = true;
    static inline constexpr auto WebhookURL = ""; // fill in if you want status to send to a webhook
};

class Bosses
{
    public:
   // soon
  //  constexpr static bool bS15 = false;
  //  constexpr static bool bS13 = false;

    private:
};

class LategameLoot
{
};