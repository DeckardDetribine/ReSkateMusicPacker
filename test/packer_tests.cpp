// The music packer library end to end: two generated tones in, a mod folder out. Needs ffmpeg
// on PATH and a game folder (argv[1], default F:\Games\ReSkate-1.0.0); without the game it skips.
#include "packer.h"
#include "Engine/Core/Json/json.h"
#include <Windows.h>
#include <cstdio>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
std::string slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}
void tone(const fs::path& file, int hertz, const char* artist, const char* title) {
    const auto command = L"ffmpeg -y -v error -f lavfi -i sine=frequency=" + std::to_wstring(hertz) +
                         L":duration=3 -metadata artist=\"" + std::wstring(artist, artist + strlen(artist)) +
                         L"\" -metadata title=\"" + std::wstring(title, title + strlen(title)) + L"\" \"" + file.wstring() + L"\"";
    if (_wsystem((L"\"" + command + L"\"").c_str()) != 0) { std::printf("FAIL: ffmpeg\n"); std::exit(1); }
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    const fs::path game = argc > 1 ? argv[1] : L"F:/Games/ReSkate-1.0.0";
    if (!fs::exists(game / L"Skate.exe")) { std::printf("SKIP: no game at %ls\n", game.c_str()); return 0; }

    const auto root = fs::temp_directory_path() / L"ReSkateMusicPackerTest";
    fs::remove_all(root);
    fs::create_directories(root / L"songs");
    tone(root / L"songs" / L"a.mp3", 440, "Test Artist", "Tone A");
    tone(root / L"songs" / L"b.flac", 880, "Test Artist", "Tone B");
    const std::vector<fs::path> files{root / L"songs" / L"a.mp3", root / L"songs" / L"b.flac"};

    auto songs = music::scan(files);
    check(songs.size() == 2, "scan returns one entry per file");
    check(songs[0].artist == "Test Artist" && songs[0].title == "Tone A", "scan reads the tags");
    check(songs[1].title == "Tone B", "scan reads the second file's tags");
    check(songs[0].seconds > 2.9 && songs[0].seconds < 3.2, "scan reads the length");
    check(songs[0].problems.empty(), "a tagged tone has no problems");
    const std::vector<fs::path> bad{root / L"missing.mp3"};
    check(!music::scan(bad)[0].problems.empty(), "an unreadable file is a problem, not a throw");

    songs[1].playlist = "Second Station";
    const auto cover = root / L"cover image.png";
    const auto coverCommand = L"ffmpeg -y -v error -f lavfi -i color=c=red:s=320x180 -frames:v 1 -update 1 \"" + cover.wstring() + L"\"";
    if (_wsystem((L"\"" + coverCommand + L"\"").c_str()) != 0) { std::printf("FAIL: artwork fixture\n"); return 1; }
    const auto embeddedFile = root / L"embedded cover.png";
    const auto embeddedPng = music::playlist_artwork_png("Embedded Album");
    {
        std::ofstream out(embeddedFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(embeddedPng.data()), static_cast<std::streamsize>(embeddedPng.size()));
    }
    const auto tagged = root / L"tagged.mp3";
    const auto attachCommand = L"ffmpeg -y -v error -i \"" + files[0].wstring() + L"\" -i \"" + embeddedFile.wstring() +
        L"\" -map 0:a -map 1:v -c copy -disposition:v attached_pic \"" + tagged.wstring() + L"\"";
    if (_wsystem((L"\"" + attachCommand + L"\"").c_str()) != 0) { std::printf("FAIL: attached artwork fixture\n"); return 1; }
    fs::remove(files[0]);
    fs::rename(tagged, files[0]);
    songs[0].artwork = cover;

    music::PackOptions options;
    options.game = game;
    options.output = root / L"mod";
    options.playlist = "Tones";
    options.playlist_artwork["Tones"] = cover;
    options.generated_playlist_artwork.insert("Tones"); // explicit image must still win
    options.generated_playlist_artwork.insert("Second Station");
    std::size_t encoded = 0, cached = 0;
    const auto result = music::pack(options, songs, [&](const music::Progress& step) {
        if (std::string(step.stage) == "encoded") {
            ++encoded;
            if (step.detail.find("(cached)") != std::string::npos) ++cached;
        }
    });
    check(result.songs == 2 && encoded == 2, "pack reports both songs");

    std::size_t reEncoded = 0, reCached = 0;
    options.output = root / L"mod2";
    const auto result2 = music::pack(options, songs, [&](const music::Progress& step) {
        if (std::string(step.stage) == "encoded") {
            ++reEncoded;
            if (step.detail.find("(cached)") != std::string::npos) ++reCached;
        }
    });
    check(result2.songs == 2 && reEncoded == 2 && reCached == 2, "repacking reuses cached encodes");
    options.output = root / L"mod";
    for (const auto* file : {L"layout.toc", L"manifest.json", L"reskate-music.json", L"reskate-build.json",
                             L"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",
                             L"Win32/levels/game/bam_levelroot/bam_levelroot.toc"})
        check(fs::exists(options.output / file), "a mod file is missing");
    const auto playlist = slurp(options.output / L"reskate-music.json");
    check(playlist.find("\"name\": \"Tones\"") != std::string::npos &&
          playlist.find("\"name\": \"Second Station\"") != std::string::npos, "reskate-music.json lists both playlists");
    check(playlist.find("\"Test Artist - Tone A\"") != std::string::npos &&
          playlist.find("\"Test Artist - Tone B\"") != std::string::npos, "reskate-music.json lists both songs");
    check(!fs::exists(root / L"mod.partial"), "no partial folder is left behind");
    const auto art = dingosdk::Json::parse(playlist);
    const auto songCover = art.at("song_artwork").at("Test Artist - Tone A").string();
    check(art.at("playlists").at(0).at("artwork").string() == songCover, "shared artwork is packaged once");
    const auto png = slurp(options.output / fs::path(songCover));
    check(png.size() > 24 && static_cast<unsigned char>(png[0]) == 137 && png.substr(1, 3) == "PNG", "artwork is a PNG");
    check(png.size() > 24 && png[16] == 0 && png[17] == 0 && png[18] == 2 && png[19] == 0 &&
          png[20] == 0 && png[21] == 0 && png[22] == 2 && png[23] == 0, "artwork is 512 by 512");
    const auto selectedPng = music::image_artwork_png(cover);
    check(png == std::string(reinterpret_cast<const char*>(selectedPng.data()), selectedPng.size()),
          "selected track image overrides its embedded album art");
    check(art.at("playlists").at(1).at("artwork").string() == "artwork/playlist-1.png" &&
          fs::is_regular_file(options.output / L"artwork/playlist-1.png"), "generated playlist cover is packaged");
    check(!art.at("song_artwork").contains("Test Artist - Tone B"), "track without embedded or chosen art has no cover");

    const auto project = music::load_project(options.output);
    check(project.name == options.name && project.playlist == "Tones" && project.bitrate == options.bitrate &&
          project.normalize == options.normalize, "the project keeps the mod's settings");
    check(project.songs.size() == 2 && fs::equivalent(project.songs[0].file, files[0]) &&
          project.songs[1].title == "Tone B" && project.songs[0].artist == "Test Artist" &&
          project.songs[1].playlist == "Second Station", "the project keeps the songs and custom playlist");
    check(fs::equivalent(project.songs[0].artwork, cover) && fs::equivalent(project.playlist_artwork.at("Tones"), cover),
          "project keeps track and playlist artwork choices");
    check(project.generated_playlist_artwork == options.generated_playlist_artwork, "project keeps generated cover choices");
    {
        const auto file = options.output / L"reskate-music-project.json";
        const auto original = slurp(file);
        auto legacy = dingosdk::Json::parse(original);
        legacy.erase("playlist_artwork");
        legacy.erase("generated_playlist_artwork");
        for (auto& song : legacy["songs"]) song.erase("artwork");
        { std::ofstream out(file); out << legacy.dump(2); }
        const auto old = music::load_project(options.output);
        check(old.playlist_artwork.empty() && old.generated_playlist_artwork.empty() && old.songs[0].artwork.empty(),
              "legacy projects still load without artwork");
        { std::ofstream out(file); out << original; }
    }
    try {
        music::load_project(root / L"songs");
        check(false, "a folder without a project throws");
    } catch (const std::runtime_error&) {
    }

    music::ThunderstoreOptions tsOpts;
    tsOpts.author = "TestAuthor";
    tsOpts.version = "1.2.3";
    tsOpts.description = "Test package description";
    const auto zip = music::export_thunderstore(options.output, tsOpts);
    check(fs::exists(zip), "thunderstore zip created");
    check(fs::file_size(zip) > 1000, "thunderstore zip has content");
    const auto zipBytes = slurp(zip);
    check(zipBytes.find("cover-0.png") != std::string::npos, "thunderstore zip contains artwork");
    fs::remove(zip);

    std::atomic<bool> cancel = true;
    options.output = root / L"cancelled";
    try {
        music::pack(options, songs, {}, &cancel);
        check(false, "cancel throws");
    } catch (const music::Cancelled&) {
    }
    check(!fs::exists(options.output) && !fs::exists(root / L"cancelled.partial"), "cancel leaves no output");

    options.output = root / L"auto-art";
    options.playlist_artwork.clear();
    options.generated_playlist_artwork.clear();
    songs[0].artwork.clear();
    music::pack(options, songs);
    const auto automatic = dingosdk::Json::parse(slurp(options.output / L"reskate-music.json"));
    const auto autoCover = automatic.at("song_artwork").at("Test Artist - Tone A").string();
    check(automatic.at("playlists").at(0).at("artwork").string() == autoCover,
          "playlist automatically uses its first available track cover");
    const auto extractedPng = music::image_artwork_png(music::embedded_artwork(files[0]));
    check(slurp(options.output / fs::path(autoCover)) == std::string(reinterpret_cast<const char*>(extractedPng.data()), extractedPng.size()),
          "packing automatically extracts the track's embedded album art");
    check(!automatic.at("playlists").at(1).contains("artwork"), "playlist without any art remains optional");

    options.output = root / L"bad-art";
    songs[0].artwork = root / L"missing-art.png";
    try { music::pack(options, songs); check(false, "missing artwork refuses build"); }
    catch (const std::runtime_error& error) {
        check(std::string(error.what()).find("Artwork file is missing") != std::string::npos, "missing artwork gives a specific error");
    }
    check(!fs::exists(options.output) && !fs::exists(root / L"bad-art.partial"), "bad artwork leaves no partial mod");

    fs::remove_all(root);
    std::printf(failures ? "%d failure(s)\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
