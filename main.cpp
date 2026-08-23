// Sun Aug 23 8:00 AM
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

// Game States so the window doesn't INSTANTLY close when u DIE
enum class GameState{ MENU, PLAYING, GAMEOVER };

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
    GameState state = GameState::MENU;

    // [8/20/26] - SCREEN SHAKE
    int shakeTimer{};
    float shakeStrength{8.0f};


    // BIRD Variables
    float birdY{320.0f};
    float velocity{0.0f};
    float gravity{0.5f};
    float jumpPow{-8.5f};

    // PIPE Variables
    // pipeX starts OFFSCREEN
    float pipeX{400};
    float pipeSpeed{4.0f};
    float pipeWidth{60.0f};
    float gapSize{160.0f};
    float topPipeHeight{200.0f};

    // [8/23/26] - COLORED PIPES
    Uint8 pipeR{255};
    Uint8 pipeG{255};
    Uint8 pipeB{255};

    // SCORE Variables
    int score{};
    bool scoredThisPipe{false};

    // Random Num Gen for Pipe Heights
    std::random_device rd;
    std::mt19937 gen(rd());
    // Give buffer of 50 pixels minimum
    // for the Top and Bottom pipes
    std::uniform_real_distribution<float> pipeDist(
        50.0f, 640.0f - gapSize - 50.f);

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
                    if (state==GameState::MENU) {
                        state = GameState::PLAYING;
                        velocity = jumpPow;
                        // SOUND EFFECT
                        MIX_PlayTrack(flapTrack,0);
                    }
                    else if (state==GameState::PLAYING) {
                        velocity=jumpPow;
                        MIX_PlayTrack(flapTrack,0);
                    }
                    else if (state==GameState::GAMEOVER) {
                        // RESET EVERYTHING
                        state=GameState::MENU;
                        birdY = 320.0f;
                        velocity=0.0f;
                        pipeX=400.0f;
                        topPipeHeight=pipeDist(gen);
                        // RESET score
                        score=0;
                        scoredThisPipe=false;

                        // Cycle the catSkin on RESET
                        // [OLD]: currentCat = (currentCat+1) % catSkins.size();
                        currentCat = skinDist(gen);

                    }
                }
            }
        }

        // UPDATE LOGIC
        if (state==GameState::PLAYING) {
            velocity=velocity+gravity;
            birdY=birdY+velocity;
            pipeX=pipeX-pipeSpeed;

            // Score Check: If bird passes on the right edge of the pipe
            if (pipeX+pipeWidth < 100.0f && !scoredThisPipe) {
                score++;
                scoredThisPipe=true;
                // SOUND EFFECT
                MIX_PlayTrack(scoreTrack,0);
            }

            // Reset pipe when it goes off-screen & randomize Height
            if (pipeX < -pipeWidth) {
                pipeX=360.0f;
                topPipeHeight=pipeDist(gen);
                // Ready for the next pipe
                scoredThisPipe=false;

                // [8/23/26] - RGB + GOLDEN PIPE CHANCE
                pipeR = rand() % 256;
                pipeG = rand() % 256;
                pipeB = rand() % 256;

                if (rand()%10==0) {
                    pipeR=255;
                    pipeG=215;
                    pipeB=0;
                }

            }

            // Floor/Ceil Collision
            if (birdY > 640.0f-50.0f || birdY < 0.0f) {
                MIX_PlayTrack(crashTrack, 0);
                state = GameState::GAMEOVER;
                // [8/20/26] - Trigger Shake when cat dies
                shakeTimer=20;
            }
        }

        // [8/20/26] - Generate a camera offset each frame
        float shakeX{};
        float shakeY{};

        if (shakeTimer>0) {
            shakeX = ( rand() % static_cast<int>(shakeStrength*2) ) - shakeStrength;

            shakeY = ( rand() % static_cast<int>(shakeStrength*2) ) - shakeStrength;

            --shakeTimer;
        }

        // DRAWING

        SDL_RenderClear(renderer);

        // 1. Bring back the Rects (Positions & Hitboxes)
        SDL_FRect topPipe{ pipeX+shakeX , shakeY, pipeWidth, topPipeHeight};

        SDL_FRect botPipe{pipeX, topPipeHeight + gapSize, pipeWidth, 640.0f - (topPipeHeight + gapSize)};
        SDL_FRect bird{100.0f+shakeX, birdY+shakeY, 60.0f,50.0f};

        // 2. Draw Background
        // [8/20/26] - Offset draw rectangles
        SDL_FRect bgRect{shakeX,shakeY,360,640};

        SDL_RenderTexture(renderer, bgTex, nullptr, &bgRect);

        // 3. Draw Pipes (Using pipedown.png for the top!)

        // [8/23/26] - RGB Pipes
        SDL_SetTextureColorMod(pipeTex, pipeR, pipeG, pipeB);
        SDL_SetTextureColorMod(pipeDownText, pipeR, pipeG, pipeB);

        SDL_RenderTexture(renderer, pipeDownText, nullptr, &topPipe);
        SDL_RenderTexture(renderer, pipeTex, nullptr, &botPipe);

        // 4. Draw Bird
        // [8/16] : Replaced birdTex with catSkins[currentCat] to allow multiple cat skins

        if (state == GameState::GAMEOVER) {
            // Tints the PNG red when you die instead of drawing a block over it
            SDL_SetTextureColorMod(catSkins[currentCat], 255, 100, 100);
        } else {
            SDL_SetTextureColorMod(catSkins[currentCat], 255, 255, 255); // Normal
        }
        SDL_RenderTexture(renderer,catSkins[currentCat], nullptr, &bird);

        // CHECK PIPE COLLISION
        if (state==GameState::PLAYING) {
            if (SDL_HasRectIntersectionFloat(&bird,&topPipe) ||
                SDL_HasRectIntersectionFloat(&bird,&botPipe)) {

                MIX_PlayTrack(crashTrack,0);

                state=GameState::GAMEOVER;

                // [8/20/26] - Trigger shake
                shakeTimer=20;
                }
        }


        // Draw Score
        if ( (state==GameState::PLAYING || state==GameState::GAMEOVER) && font!=nullptr) {
            std::string scoreText = std::to_string(score);
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
    SDL_DestroyTexture(bgTex);
    // IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    MIX_DestroyTrack(flapTrack);
    MIX_DestroyTrack(crashTrack);
    MIX_DestroyTrack(scoreTrack);
    // 8/20/26
    MIX_DestroyTrack(natureTrack);

    // There are levels to ts

    MIX_DestroyAudio(flapSfx);
    MIX_DestroyAudio(crashSfx);
    MIX_DestroyAudio(scoreSfx);
    // 8/20/26
    MIX_DestroyAudio(natureSfx);

    MIX_DestroyMixer(mixer);
    MIX_Quit();
    SDL_Quit();
    SDL_DestroyTexture(pipeDownText);

    return {};
}

