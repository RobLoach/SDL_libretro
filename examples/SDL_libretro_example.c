#include <SDL2/SDL.h>

int main() {
	SDL_Init(SDL_INIT_EVERYTHING);

    const int screenWidth = 800;
    const int screenHeight = 450;
	SDL_Window* window = SDL_CreateWindow("SDL_libretro: Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 450, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    int quit = 0;
    SDL_Event event;

    while (quit == 0) {
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            quit = 1;
                        break;
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 102, 191, 255, 255);
		SDL_RenderClear(renderer);
		//SDL_RenderCopy(renderer, tex, NULL, &destination);
        SDL_SetRenderDrawColor(renderer, 230, 41, 55, 255);
        SDL_Rect rect = {100, 100, 100, 100};
        SDL_RenderFillRect(renderer, &rect);
        
		SDL_RenderPresent(renderer);
    }

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}