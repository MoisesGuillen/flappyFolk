// Sun Sept 3 4:45 PM
// Flappy Folk v 1.0
// by Moises Guillen
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL.h>
#include <ctime>

// Game States so the window doesn't INSTANTLY close when u DIE
enum class GameState {
    MENU,
    PLAYING,
    GAMEOVER
};

// [8/24/26] - Refactoring w/ Structs
struct Bird {
    float y{320.0f};
    float velocity{0.0f};
    float gravity{0.5f};
    float jumpPow{-8.5f};
    std::vector<float> trail{}; // [8/25/26] - pseudo motion blur, last N y-positions

};

struct Pipe {
    float x{400};
    float speed{4.0f};
    float width{60.0f};
    float size{160.0f};
    float topHeight{200.0f};

    Uint8 r{255};
    Uint8 g{255};
    Uint8 b{255};

    bool scoredThisPipe{false};

};

struct CamShake {
    int timer{};
    float strength{8.0f};
};

struct Game {
    Bird bird{};
    Pipe pipe{};
    CamShake shake{};
    int score{};
    int highScore{};
    GameState state{GameState::MENU};

};


int main(int argc, char *argv[]) {
    // VIDEO + AUDIO CHECK
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cout << "SDL Init Failed: " << SDL_GetError() << '\n';
        return 1;
    }

    // Init TTF
    if (!TTF_Init()) {
        std::cout << "TTF Init Failed: " << SDL_GetError() << '\n';
        return 1;
    }

    // AUDIO
    if (!MIX_Init()) {
        std::cout << "Mixer Init Failed: " << SDL_GetError() << '\n';
    }

    MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mixer) {
        std::cout << "Mixer Device Failed: " << SDL_GetError() << '\n';
    }

    // ** 1. Changed to PORTRAIT Res (Mobile Aspect Ratio)
    SDL_Window* window = SDL_CreateWindow(
        "Flappy Folk",360,640,0);

    if (!window) {
        std::cout << "Window failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    // LOCK Framerate to MACBOOK DISPLAY
    SDL_SetRenderVSync(renderer,1);

    // LOAD flappy-font
    TTF_Font* font = TTF_OpenFont("flappy-font.ttf", 48);
    if (!font) {
        std::cout << "Make sure font is in the cmake-build-debug folder! "
        << SDL_GetError() << '\n';
    }

    bool running{true};

    Game game{};

    // Random Num Gen for Pipe Heights
    std::random_device rd;
    std::mt19937 gen(rd());

    std::srand(static_cast<unsigned>( time(nullptr)));

    // Give buffer of 50 pixels minimum
    // for the Top and Bottom pipes
    std::uniform_real_distribution<float> pipeDist(
        50.0f, 640.0f - game.pipe.size - 50.f);

    // Load textures
    SDL_Texture* bgTex = IMG_LoadTexture(renderer, "bg.png");

    // SDL_Texture* birdTex = IMG_LoadTexture(renderer, "bird.png");

    std::vector<SDL_Texture*> catSkins={
        IMG_LoadTexture(renderer, "bird.png"),
        IMG_LoadTexture(renderer, "bird2.png"),
        IMG_LoadTexture(renderer, "bird3.png"),
        IMG_LoadTexture(renderer, "bird4.png"),
        IMG_LoadTexture(renderer, "bird5.png"),
        IMG_LoadTexture(renderer,"bird6.png"),
        IMG_LoadTexture(renderer,"bird7.png"),
        IMG_LoadTexture(renderer,"bird8.png"),
        // [8/21/26] - 5 NEW SKINS
        IMG_LoadTexture(renderer,"bird9.png"),
        IMG_LoadTexture(renderer,"bird10.png"),
        IMG_LoadTexture(renderer,"bird11.png"),
        IMG_LoadTexture(renderer,"bird12.png"),
        IMG_LoadTexture(renderer,"bird13.png"),
    };

    // Null Check: make sure all skins load
    for (size_t i{}; i<catSkins.size(); ++i) {
        if (!catSkins[i]) {
            std::cout << "Failed to load skin " << i << ": " << SDL_GetError() << '\n';
        }
    }

    // New distribution for ints
    // self-adjusts to however many skins are in the vector
    std::uniform_int_distribution<int> skinDist(0, static_cast<int>(catSkins.size()) - 1);

    int currentCat{};

    SDL_Texture* pipeTex = IMG_LoadTexture(renderer, "pipe.png");
    SDL_Texture* pipeDownText = IMG_LoadTexture(renderer, "pipedown.png");

    // Null Check: make sure all textures load
    if (!bgTex){ std::cout << "Failed to load bg.png " << SDL_GetError() << ": " << '\n'; }
    if (!pipeTex){ std::cout << "Failed to load pipe.png " << SDL_GetError() << ": " << '\n'; }
    if (!pipeDownText){ std::cout << "Failed to load pipedown.png " << SDL_GetError() << ": " << '\n'; }

    // [8/21/26] - Folk Valley Noises "folkvalleybgSFX.mp3"
    MIX_Audio* natureSfx = MIX_LoadAudio(mixer,"folkvalleybgSFX.mp3",false);
    if (!natureSfx){ std::cout << "Failed to load folkvalleybgSFX.mp3: " << SDL_GetError() << '\n'; }
    MIX_Track* natureTrack = MIX_CreateTrack(mixer);
    MIX_SetTrackAudio(natureTrack, natureSfx);

    // [8/21/26] - Start bg ambience looping infinitely
    SDL_PropertiesID natureProps = SDL_CreateProperties();
    SDL_SetNumberProperty(natureProps, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(natureTrack,natureProps);
    SDL_DestroyProperties(natureProps);

    // LOAD wav files
    MIX_Audio* flapSfx = MIX_LoadAudio(mixer, "sfx_wing.wav", true);
    MIX_Audio* crashSfx = MIX_LoadAudio(mixer, "sfx_hit.wav", true);
    MIX_Audio* scoreSfx = MIX_LoadAudio(mixer, "sfx_point.wav", true);
    // Create a dedicated track for each sound
    MIX_Track* flapTrack = MIX_CreateTrack(mixer);
    MIX_Track* crashTrack = MIX_CreateTrack(mixer);
    MIX_Track* scoreTrack = MIX_CreateTrack(mixer);
    // Bind the audio files to the tracks
    MIX_SetTrackAudio(flapTrack, flapSfx);
    MIX_SetTrackAudio(crashTrack, crashSfx);
    MIX_SetTrackAudio(scoreTrack, scoreSfx);


    // [8/12/26]: Setup 4 variables before the loop
    const int targetFPS{60};
    const int frameDELAY{1000/targetFPS};
    Uint64 frameStart{};
    Uint32 frameTime{};


    while (running) {

        // [8/12/26]: Grab the start time
        frameStart=SDL_GetTicks();

        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running=false;
            }

            if (event.type==SDL_EVENT_KEY_DOWN) {
                if (event.key.key==SDLK_SPACE) {
                    if (game.state==GameState::MENU) {
                        game.state = GameState::PLAYING;
                        game.bird.velocity = game.bird.jumpPow;
                        // SOUND EFFECT
                        MIX_PlayTrack(flapTrack,0);
                    }
                    else if (game.state==GameState::PLAYING) {
                        game.bird.velocity=game.bird.jumpPow;
                        MIX_PlayTrack(flapTrack,0);
                    }
                    else if (game.state==GameState::GAMEOVER) {
                        // RESET EVERYTHING
                        game.state=GameState::MENU;
                        game.bird.y = 320.0f;
                        game.bird.velocity=0.0f;

                        game.bird.trail.clear();

                        game.pipe.x=400.0f;
                        game.pipe.topHeight=pipeDist(gen);
                        // RESET score
                        game.score=0;
                        game.pipe.scoredThisPipe=false;

                        // Cycle the catSkin on RESET
                        // [OLD]: currentCat = (currentCat+1) % catSkins.size();
                        currentCat = skinDist(gen);

                    }
                }
            }
        }

        // UPDATE LOGIC
        if (game.state==GameState::PLAYING) {
            game.bird.velocity=game.bird.velocity+game.bird.gravity;
            game.bird.y=game.bird.y+game.bird.velocity;

            // [8/25/26] - Push into it every PLAYING update tick
            game.bird.trail.push_back(game.bird.y);
            if (game.bird.trail.size() > 6) {
                game.bird.trail.erase(game.bird.trail.begin());
            }

            game.pipe.x=game.pipe.x-game.pipe.speed;

            // Score Check: If bird passes on the right edge of the pipe
            if (game.pipe.x+game.pipe.width < 100.0f && !game.pipe.scoredThisPipe) {
                game.score++;
                game.pipe.scoredThisPipe=true;
                // SOUND EFFECT
                MIX_PlayTrack(scoreTrack,0);
            }

            // Reset pipe when it goes off-screen & randomize Height
            if (game.pipe.x < -game.pipe.width) {
                game.pipe.x=360.0f;
                game.pipe.topHeight=pipeDist(gen);
                // Ready for the next pipe
                game.pipe.scoredThisPipe=false;

                // [8/23/26] - RGB + GOLDEN PIPE CHANCE
                game.pipe.r = rand() % 256;
                game.pipe.g = rand() % 256;
                game.pipe.b = rand() % 256;

                if (rand()%10==0) {
                    game.pipe.r=255;
                    game.pipe.g=215;
                    game.pipe.b=0;
                }

            }

            // Floor/Ceil Collision
            if (game.bird.y > 640.0f-50.0f || game.bird.y < 0.0f) {
                MIX_PlayTrack(crashTrack, 0);
                game.state = GameState::GAMEOVER;
                // [8/20/26] - Trigger Shake when cat dies
                game.shake.timer=20;
                // [8/20/26] - High Score UPDATE
                if (game.score > game.highScore) {
                    game.highScore = game.score;
                }
            }
        }

        // [8/20/26] - Generate a camera offset each frame
        float shakeX{};
        float shakeY{};

        if (game.shake.timer>0) {
            shakeX = ( rand() % static_cast<int>(game.shake.strength*2) ) - game.shake.strength;

            shakeY = ( rand() % static_cast<int>(game.shake.strength*2) ) - game.shake.strength;

            --game.shake.timer;
        }

        // DRAWING

        SDL_RenderClear(renderer);

        // 1. Bring back the Rects (Positions & Hitboxes)
        SDL_FRect topPipe{ game.pipe.x+shakeX , shakeY, game.pipe.width, game.pipe.topHeight};

        SDL_FRect botPipe{game.pipe.x, game.pipe.topHeight + game.pipe.size, game.pipe.width, 640.0f - (game.pipe.topHeight + game.pipe.size)};

        SDL_FRect birdRect{100.0f+shakeX, game.bird.y + shakeY, 60.0f,50.0f};

        // 2. Draw Background
        // [8/20/26] - Offset draw rectangles
        SDL_FRect bgRect{shakeX,shakeY,360,640};
        SDL_RenderTexture(renderer, bgTex, nullptr, &bgRect);

        // 3. Draw Pipes (Using pipedown.png for the top!)
        // [8/23/26] - RGB Pipes
        SDL_SetTextureColorMod(pipeTex, game.pipe.r, game.pipe.g, game.pipe.b);
        SDL_SetTextureColorMod(pipeDownText, game.pipe.r, game.pipe.g, game.pipe.b);
        SDL_RenderTexture(renderer, pipeDownText, nullptr, &topPipe);
        SDL_RenderTexture(renderer, pipeTex, nullptr, &botPipe);

        // 4. Draw Ghosts
        size_t trailCount = game.bird.trail.size();
        for (size_t i{}; i<trailCount; ++i) {
            // Uint8 alpha = static_cast<Uint8>( 15 + (i * 90 / std::max<size_t>(trailCount,1)));
            Uint8 alpha = static_cast<Uint8>( 8 + (i * 35 / std::max<size_t>(trailCount,1)));  // was 15 + 90
            SDL_FRect ghostRect{ 100.0f + shakeX, game.bird.trail[i] + shakeY, 60.0f, 50.0f};
            SDL_SetTextureAlphaMod(catSkins[currentCat],alpha);
            SDL_RenderTexture(renderer,catSkins[currentCat], nullptr, &ghostRect);
        }
        SDL_SetTextureAlphaMod(catSkins[currentCat],255); // reset before real bird

        // 4. Draw Bird
        // [8/16] : Replaced birdTex with catSkins[currentCat] to allow multiple cat skins

        if (game.state == GameState::GAMEOVER) {
            // Tints the PNG red when you die instead of drawing a block over it
            SDL_SetTextureColorMod(catSkins[currentCat], 255, 100, 100);
        } else {
            SDL_SetTextureColorMod(catSkins[currentCat], 255, 255, 255); // Normal
        }
        SDL_RenderTexture(renderer,catSkins[currentCat], nullptr, &birdRect);

        // CHECK PIPE COLLISION
        if (game.state==GameState::PLAYING) {
            if (SDL_HasRectIntersectionFloat(&birdRect,&topPipe) ||
                SDL_HasRectIntersectionFloat(&birdRect,&botPipe)) {

                MIX_PlayTrack(crashTrack,0);

                game.state=GameState::GAMEOVER;

                // [8/20/26] - Trigger shake
                game.shake.timer=20;

                // [8/30/26] - High Score
                if (game.score > game.highScore) {
                    game.highScore = game.score;
                }

                }
        }

        // Draw Score
        if ( (game.state==GameState::PLAYING || game.state==GameState::GAMEOVER) && font!=nullptr) {
            std::string scoreText = std::to_string(game.score);
            SDL_Color textColor = {255,255,255,255};

            SDL_Surface* textSurface = TTF_RenderText_Solid(font,scoreText.c_str(),
                scoreText.length(), textColor);
            if (textSurface) {
                SDL_Texture* textTexture=SDL_CreateTextureFromSurface(renderer,textSurface);
                SDL_FRect textRect = {180.0f - (textSurface->w / 2.0f),
                    50.0f,(float)textSurface->w, (float)textSurface->h};
                SDL_RenderTexture(renderer, textTexture, nullptr, &textRect);

                SDL_DestroyTexture(textTexture);
                SDL_DestroySurface(textSurface);
            }
        }

        // Draw High Score (top right, only on GAMEOVER)
        if (game.state == GameState::GAMEOVER || (game.state == GameState::MENU && font != nullptr)) {
            std::string hiText = "BEST " + std::to_string(game.highScore);
            SDL_Color hiColor = {255, 255, 255, 255};

            // Render smaller by using a separate small font, OR scale down the rect (see note below)
            SDL_Surface* hiSurface = TTF_RenderText_Solid(font, hiText.c_str(), hiText.length(), hiColor);
            if (hiSurface) {
                SDL_Texture* hiTexture = SDL_CreateTextureFromSurface(renderer, hiSurface);
                float hiW = hiSurface->w * 0.3f;   // shrink to ~40% size
                float hiH = hiSurface->h * 0.3f;
                SDL_FRect hiRect = {360.0f - hiW - 10.0f, 10.0f, hiW, hiH};  // top-right, 10px padding
                SDL_RenderTexture(renderer, hiTexture, nullptr, &hiRect);

                SDL_DestroyTexture(hiTexture);
                SDL_DestroySurface(hiSurface);
            }
        }

        SDL_RenderPresent(renderer);

        // [8/12/26]: Delay the frame at the end
        frameTime=SDL_GetTicks()-frameStart;
        if (frameDELAY > frameTime) {
            SDL_Delay(frameDELAY - frameTime);
        }
    }

    if (font) { TTF_CloseFont(font); }
    TTF_Quit();

    // [8/18]: Memory leak on cleanup
    // SDL_DestroyTexture(catSkins[currentCat]); BUG: only 1/8 variants eliminated
    for (auto* variants:catSkins) {
        SDL_DestroyTexture(variants);
    }

    SDL_DestroyTexture(pipeTex);
    SDL_DestroyTexture(pipeDownText);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    MIX_DestroyTrack(flapTrack);
    MIX_DestroyTrack(crashTrack);
    MIX_DestroyTrack(scoreTrack);
    MIX_DestroyTrack(natureTrack);

    // There are levels to ts
    MIX_DestroyAudio(flapSfx);
    MIX_DestroyAudio(crashSfx);
    MIX_DestroyAudio(scoreSfx);
    MIX_DestroyAudio(natureSfx);

    MIX_DestroyMixer(mixer);
    MIX_Quit();
    SDL_Quit();

    return {};
}

