#pragma once

#include <SDL.h>

#include <d3d11.h>
#include <directxmath.h>

#include <span>
#include <vector>
#include <string>

#include <font.h>
#include <sprite.h>
#include <cube.h>
#include <window.h>

struct D3DRenderer {
	Window& _sysWin;
	float _viewWidth = -1.0f, _viewHeight = -1.0f;

	ID3D11Device* _device = nullptr;
	D3D_DRIVER_TYPE _driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_11_0;

	ID3D11DeviceContext* _context = nullptr;
	IDXGISwapChain* _swapChain = nullptr;

	D3D11_VIEWPORT _viewport = {};
	ID3D11RenderTargetView* _bBufferTarget = nullptr;
	ID3D11BlendState* _blendState = nullptr;
	ID3D11Texture2D* _depthTex = nullptr;
	ID3D11DepthStencilView* _depthTexView = nullptr;

	std::vector<Sprite::Instance> _spriteInstMem;
	std::vector<Sprite::Vertex> _fontVertMem;

	ID3D11Buffer* _cubeVertBuf = nullptr;
	ID3D11Buffer* _cubeIdxBuf = nullptr;
	ID3D11Buffer* _cubeModelBuf = nullptr;

	ID3D11Buffer* _spriteVertBuf = nullptr;
	ID3D11Buffer* _fontVertBuf = nullptr;
	ID3D11Buffer* _spriteInstBuf = nullptr;
	ID3D11Buffer* _projBuf = nullptr;

	ID3D11VertexShader* _cubeVS = nullptr;
	ID3D11VertexShader* _spriteVS = nullptr;
	ID3D11VertexShader* _fontVS = nullptr;
	ID3D11PixelShader* _PS = nullptr;
	ID3D11PixelShader* _combiPS = nullptr;
	ID3D11InputLayout* _spriteIL = nullptr;
	ID3D11InputLayout* _cubeIL = nullptr;

	ID3D11ShaderResourceView* _heartTexView = nullptr;
	ID3D11ShaderResourceView* _woodTexView = nullptr;
	ID3D11ShaderResourceView* _fontTexView = nullptr;
	ID3D11SamplerState* _texSampler = nullptr;

	D3DRenderer(Window& _win) : _sysWin(_win) {
		int _width, _height;
		SDL_GetWindowSize(_sysWin.SDL, &_width, &_height);
		
		this->_viewWidth = static_cast<float>(_width);
		this->_viewHeight = static_cast<float>(_height);
	};

	void init();

	void populateVRAM(unsigned int reserve_sprites, unsigned int reserve_letters);

	void clrScr(const std::array<float, 4>&);
	void renderCube(const std::span<Cube::Data>);
	void renderSprites(const std::span<Sprite::Data>);
	void renderString(const std::span<Font::String>);
	void present();

	template<typename T> void retire(T& COMobj) {
		if (COMobj == nullptr)
			return;

		COMobj->Release();
		COMobj = nullptr;
	};

	void cleanUp();

	void deviceSetup();

	ID3DBlob* compileShader(const wchar_t*, const char*, const char*);
	void shaderSetup();
};
