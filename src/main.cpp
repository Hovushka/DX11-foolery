// DXtest.cpp : Defines the entry point for the application.
//

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include <iostream>
#include <array>
#include <chrono>
#include <exception>

#include <window.h>
#include <d3drenderer.h>
#include <sprite.h>
#include <cube.h>
#include <DX.h>

const int WIDTH = 800, HEIGHT = 600;
const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

struct state {
    std::vector<Sprite::Data> sprites = {
        Sprite::Data { { WIDTH * 0.75f, HEIGHT * 0.75f }, 0.0f, { 0.5f,  0.5f} },
        Sprite::Data { { WIDTH * 0.25f, HEIGHT * 0.75f }, 0.0f, { 0.1f,  0.5f} },
        Sprite::Data { { WIDTH * 0.25f, HEIGHT * 0.25f }, 0.0f, {0.25f, 0.25f} },
        Sprite::Data { { WIDTH * 0.75f, HEIGHT * 0.25f }, 0.0f, { 0.5f,  0.5f} },
    };

    std::vector<Cube::Data> cubes = {
        Cube::Data { { 0.0f, 0.0f, 6.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
        Cube::Data { { -2.0f, -2.0f, 8.0f }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 1.0f, 1.0f } },
    };

    std::vector<Font::String> strings = {
		Font::String { "CHOP A WOOD", { 25, 250 }, 128 },
		Font::String { "MEW", { 700, 20 }, 16 },
    };

    void update();
} state;

void state::update() {
	static auto start = std::chrono::high_resolution_clock::now();
	auto current = std::chrono::high_resolution_clock::now();
	const float delta = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();

    for (auto& sprite : this->sprites)
        sprite.setRotation(DirectX::XM_PI * delta);

    bool flag = false;
    for (auto& cube : this->cubes) {
        cube.setRotation(DirectX::XMVECTOR{ 
            DirectX::XM_PIDIV4 * delta * ((flag) ? 0.0f : 1.0f), DirectX::XM_PIDIV4 * delta, 0 
        });

        flag = !flag;
    }
}

int main(int argc, char *argv[])
{
	Window window;
    SDL_Init(SDL_INIT_VIDEO);

    if ((window.SDL = SDL_CreateWindow(
			"DirectX 11 test", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			WIDTH, HEIGHT,
			0
		)) == nullptr) {

        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }

	SDL_VERSION(&window.sysWMInfo.version);
	SDL_GetWindowWMInfo(window.SDL, &window.sysWMInfo);

	D3DRenderer renderer(window);

    try {
        renderer.init();
        renderer.populateVRAM(4, 16);
    }
    catch (DX::com_exception e) {
		window.shout(e.what(), "DirectX 11 error");
		renderer.cleanUp();
		SDL_DestroyWindow(window.SDL);
		SDL_Quit();
        return 1;
    }
    
    SDL_Event windowEvent;
    while (true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (SDL_QUIT == windowEvent.type)
                break;
        }

        state.update();

        renderer.clrScr({ 0.0f, 0.0f, 0.25f, 1.0f });
		renderer.renderSprites(state.sprites);
        renderer.renderCube(state.cubes);
        renderer.renderString(state.strings);
        renderer.present();
    }
    
    renderer.cleanUp();
    SDL_DestroyWindow(window.SDL);
    SDL_Quit();
    
    return 0;
}
