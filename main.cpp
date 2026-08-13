// Wed Aug 12 9:11 PM
// Flappy Folk v 1.0
// by Moises Guillen
#include <iostream>
#include <random>
#include <string>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL.h>

// Game States so the window doesn't INSTANTLY close when u DIE
enum class GameState{ MENU, PLAYING, GAMEOVER };

int main(int argc, char *argv[]) {

    // AUDIO
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cout << "SDL Init Failed: " << SDL_GetError() << '\n';
        return 1;
    }
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL Init Failed: " << SDL_GetError() << '\n';
        return 1;
    }

    // Init TTF
    if (!TTF_Init()) {
        std::cout << "TTF Init Failed: " << SDL_GetError() << '\n';
        return 1;
    }

    // Init Image Lib
    //if (!IMG_Init(IMG_INIT_PNG)) {
    //    std::cout << "IMG Init Failed: " << SDL_GetError() << '\n';
    //}

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
        "Flappy Bird",360,640,0);

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
    SDL_Texture* birdTex = IMG_LoadTexture(renderer, "bird.png");
    SDL_Texture* pipeTex = IMG_LoadTexture(renderer, "pipe.png");
    SDL_Texture* pipeDownText = IMG_LoadTexture(renderer, "pipedown.png");

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
            }

            // Floor/Ceil Collision
            if (birdY > 640.0f-50.0f || birdY < 0.0f) {
                MIX_PlayTrack(crashTrack, 0);
                state = GameState::GAMEOVER;
            }
        }

        // DRAWING

        SDL_RenderClear(renderer);

        // 1. Bring back the Rects (Positions & Hitboxes)
        SDL_FRect topPipe{pipeX, 0, pipeWidth, topPipeHeight};
        SDL_FRect botPipe{pipeX, topPipeHeight + gapSize, pipeWidth, 640.0f - (topPipeHeight + gapSize)};
        SDL_FRect bird{100.0f, birdY, 60.0f, 50.0f};

        // 2. Draw Background
        SDL_FRect bgRect{0, 0, 360, 640};
        SDL_RenderTexture(renderer, bgTex, nullptr, &bgRect);

        // 3. Draw Pipes (Using pipedown.png for the top!)
        SDL_RenderTexture(renderer, pipeDownText, nullptr, &topPipe);
        SDL_RenderTexture(renderer, pipeTex, nullptr, &botPipe);

        // 4. Draw Bird
        if (state == GameState::GAMEOVER) {
            // Tints the PNG red when you die instead of drawing a block over it
            SDL_SetTextureColorMod(birdTex, 255, 100, 100);
        } else {
            SDL_SetTextureColorMod(birdTex, 255, 255, 255); // Normal
        }
        SDL_RenderTexture(renderer, birdTex, nullptr, &bird);

        // CHECK PIPE COLLISION
        if (state==GameState::PLAYING) {
            if (SDL_HasRectIntersectionFloat(&bird,&topPipe) ||
                SDL_HasRectIntersectionFloat(&bird,&botPipe)) {

                MIX_PlayTrack(crashTrack,0);

                state=GameState::GAMEOVER;
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
    SDL_DestroyTexture(birdTex);
    SDL_DestroyTexture(pipeTex);
    SDL_DestroyTexture(bgTex);
    // IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    MIX_DestroyTrack(flapTrack);
    MIX_DestroyTrack(crashTrack);
    MIX_DestroyTrack(scoreTrack);
    // This project taught me that it requires a lot of programming experience

    MIX_DestroyAudio(flapSfx);
    MIX_DestroyAudio(crashSfx);
    MIX_DestroyAudio(scoreSfx);

    MIX_DestroyMixer(mixer);
    MIX_Quit();
    SDL_Quit();
    SDL_DestroyTexture(pipeDownText);

    return 0;
}

