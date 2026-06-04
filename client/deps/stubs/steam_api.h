#pragma once
// Steam stripped (LAN/local build). Client only calls SteamAPI_Init/Shutdown (Main.cpp).
inline bool SteamAPI_Init() { return true; }   // pretend Steam is up
inline void SteamAPI_Shutdown() {}
