#pragma once

#include "libcurl/curl.h"
#include "./FortniteGame/Public/FortGameMode.h"
#include "framework.h"
#include "pch.h"
#include <iostream>
#include "./Erbium/Public/Configuration.h"

inline void Platinum(const char* url)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        std::cout << "[Platinum] failed to start curl" << '\n';
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK)
    {
        std::cout << "[Platinum] posted to matchmaker <3" << '\n';
    }
    else
    {
        std::cout << "[Platinum] post failed: " << curl_easy_strerror(res) << '\n';
    }

    curl_easy_cleanup(curl);
}

// include post.h and initialize it by doing PlatinumPOST(GameMode); after GameMode-bWorldIsReady
// still very experimenta since it crashes the gs after posting mostly
inline void PlatinumPOST(AFortGameMode* Gamemode) 
{
 if (FConfiguration::bAPI)
 {
    if (Gamemode->bWorldIsReady)
    {
        Platinum("http://ip:6767/api/match");
    }
    else
    {
        std::cout << "[Platinum] API disabled" << '\n';
    }
 }
}