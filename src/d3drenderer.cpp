#include <d3drenderer.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <DDSTextureLoader.h>
#include <DX.h>

#include <span>
#include <array>
#include <vector>
#include <chrono>
#include <string>

#include <cube.h>
#include <font.h>
#include <sprite.h>
#include <window.h>

#define HR(fn) DX::ThrowIfFailed(fn, __FILE__, __LINE__, __func__)

void D3DRenderer::init() {
	deviceSetup();
	shaderSetup();
}

void D3DRenderer::deviceSetup() {
	std::array driverTypes = { 
		D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_SOFTWARE
	};

	std::array featureLevels = {
		D3D_FEATURE_LEVEL_11_1, 
		D3D_FEATURE_LEVEL_11_0, 
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	DXGI_MODE_DESC displayDesc = {
		.Width = static_cast<unsigned int>(_viewWidth),
		.Height = static_cast<unsigned int>(_viewHeight),
		.RefreshRate = { .Numerator = 60, .Denominator = 1, },
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {
		.BufferDesc = displayDesc,
		.SampleDesc = { .Count = 1, .Quality = 0, },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 1,
		.OutputWindow = _sysWin.sysWMInfo.info.win.window,
		.Windowed = true,
	};

	unsigned int creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT res = E_FAIL;
	for(auto& driver : driverTypes) {
		res = D3D11CreateDeviceAndSwapChain(
			nullptr,
			driver, 
			nullptr,
			creationFlags,
			featureLevels.data(), featureLevels.size(),
			D3D11_SDK_VERSION, 
			&swapChainDesc, &_swapChain,
			&_device, 
			&_featureLevel, 
			&_context
		);

		if(SUCCEEDED(res)) {
			_driverType = driver;
			break;
		}
	}
	HR(res);

	ID3D11Texture2D* bBufferTex = nullptr;
	HR(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bBufferTex)));

	try {
		HR(_device->CreateRenderTargetView(bBufferTex, nullptr, &_bBufferTarget));
	}
	catch (DX::com_exception e) {
		retire(bBufferTex);
		throw e;
	}
	retire(bBufferTex);

	D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc = {
		.BlendEnable = true,
		.SrcBlend = D3D11_BLEND_SRC_ALPHA,
		.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
		.BlendOp = D3D11_BLEND_OP_ADD,
		.SrcBlendAlpha = D3D11_BLEND_ZERO,
		.DestBlendAlpha = D3D11_BLEND_ZERO,
		.BlendOpAlpha = D3D11_BLEND_OP_ADD,
		.RenderTargetWriteMask = 0x0F,
	};

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0] = targetBlendDesc;

	HR(_device->CreateBlendState(&blendDesc, &_blendState));

	D3D11_TEXTURE2D_DESC depthTexDesc = {
		.Width = static_cast<unsigned int>(_viewWidth),
		.Height = static_cast<unsigned int>(_viewHeight),
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.SampleDesc = {.Count = 1, .Quality = 0, },
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
	};

	HR(_device->CreateTexture2D(&depthTexDesc, nullptr, &_depthTex));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
		.Format = depthTexDesc.Format,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
		.Texture2D = {.MipSlice = 0, },
	};

	HR(_device->CreateDepthStencilView(_depthTex, &depthViewDesc, &_depthTexView));

	_viewport = {
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = _viewWidth,
		.Height = _viewHeight,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};

	D3D11_SAMPLER_DESC texSamplerDesc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
		.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
		.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
		.MaxLOD = D3D11_FLOAT32_MAX,
	};

	HR(_device->CreateSamplerState(&texSamplerDesc, &_texSampler));
}

ID3DBlob* D3DRenderer::compileShader(const wchar_t* filename, 
		const char* entry, const char* version) {
	ID3DBlob* sBuffer = nullptr;
	ID3DBlob* errorBuffer = nullptr;

	unsigned int shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3DCOMPILE_DEBUG;
#endif

	try {
		HR(
			D3DCompileFromFile(
				filename,
				0,
				0,
				entry,
				version,
				shaderFlags,
				0,
				&sBuffer,
				&errorBuffer
			)
		);
	}
	catch (DX::com_exception e) {
		if (errorBuffer != nullptr) {
			_sysWin.shout(
				reinterpret_cast<char*>(errorBuffer->GetBufferPointer()), 
				"Shader error"
			);
			retire(errorBuffer);
		}
		retire(sBuffer);
		throw e;
	}

	retire(errorBuffer);
	return sBuffer;
}

void D3DRenderer::shaderSetup() {
	ID3DBlob* sBuffer = nullptr;
	sBuffer = compileShader(L"sampleShader.hlsl", "spriteVS", "vs_4_0");

	try {
		HR(
			_device->CreateVertexShader(
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				0,
				&_spriteVS
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}

	std::array spriteILDesc = {
		D3D11_INPUT_ELEMENT_DESC { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,
			0, offsetof(Sprite::Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
			0, offsetof(Sprite::Vertex, tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC { "MODEL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			1, offsetof(Sprite::Instance, model), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		D3D11_INPUT_ELEMENT_DESC { "MODEL", 1, DXGI_FORMAT_R32G32B32_FLOAT,
			1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		D3D11_INPUT_ELEMENT_DESC { "MODEL", 2, DXGI_FORMAT_R32G32B32_FLOAT,
			1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	try {
		HR(
			_device->CreateInputLayout(
				spriteILDesc.data(),
				spriteILDesc.size(),
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				&_spriteIL
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}
	retire(sBuffer);

	sBuffer = compileShader(L"sampleShader.hlsl", "fontVS", "vs_4_0");

	try {
		HR(
			_device->CreateVertexShader(
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				0,
				&_fontVS
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}
	retire(sBuffer);

	sBuffer = compileShader(L"sampleShader.hlsl", "cubeVS", "vs_4_0");

	try {
		HR(
			_device->CreateVertexShader(
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				0,
				&_cubeVS
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}

	std::array cubeILDesc = {
		D3D11_INPUT_ELEMENT_DESC { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, offsetof(Cube::Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
			0, offsetof(Cube::Vertex, tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	try {
		HR(
			_device->CreateInputLayout(
				cubeILDesc.data(),
				cubeILDesc.size(),
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				&_cubeIL
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}
	retire(sBuffer);

	sBuffer = compileShader(L"sampleShader.hlsl", "spritePS", "ps_4_0");

	try {
		HR(
			_device->CreatePixelShader(
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				0,
				&_PS
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}
	retire(sBuffer);

	sBuffer = compileShader(L"sampleShader.hlsl", "combiPS", "ps_4_0");

	try {
		HR(
			_device->CreatePixelShader(
				sBuffer->GetBufferPointer(),
				sBuffer->GetBufferSize(),
				0,
				&_combiPS
			)
		);
	}
	catch (DX::com_exception e) {
		retire(sBuffer);
		throw e;
	}
	retire(sBuffer);
}

void D3DRenderer::populateVRAM(unsigned int reserve_sprites, unsigned int reserve_letters) {
	using namespace DirectX;

	// Vertex Sprite buffer
	{
		std::array spriteVert = {
			Sprite::Vertex { XMFLOAT2(_viewHeight / 2,  _viewHeight / 2), XMFLOAT2(1.0, 1.0) },
			Sprite::Vertex { XMFLOAT2(_viewHeight / 2, -_viewHeight / 2), XMFLOAT2(1.0, 0.0) },
			Sprite::Vertex { XMFLOAT2(-_viewHeight / 2, -_viewHeight / 2), XMFLOAT2(0.0, 0.0) },

			Sprite::Vertex { XMFLOAT2(-_viewHeight / 2, -_viewHeight / 2), XMFLOAT2(0.0, 0.0) },
			Sprite::Vertex { XMFLOAT2(-_viewHeight / 2,  _viewHeight / 2), XMFLOAT2(0.0, 1.0) },
			Sprite::Vertex { XMFLOAT2(_viewHeight / 2,  _viewHeight / 2), XMFLOAT2(1.0, 1.0) },
		};

		D3D11_BUFFER_DESC spriteVertDesc = {
			.ByteWidth = sizeof(spriteVert),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA spriteVertResData = {
			.pSysMem = spriteVert.data(),
		};

		HR(_device->CreateBuffer(&spriteVertDesc, &spriteVertResData, &_spriteVertBuf));
	}

	// Instance Sprite buffer
	{
		D3D11_BUFFER_DESC instBufDesc = {
			.ByteWidth = reserve_sprites * sizeof(Sprite::Instance),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};

		HR(_device->CreateBuffer(&instBufDesc, nullptr, &_spriteInstBuf));
	}

	// Vertex Font buffer
	{
		D3D11_BUFFER_DESC instFontBufDesc = {
			.ByteWidth = reserve_letters * 6 * sizeof(Sprite::Vertex),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};

		HR(_device->CreateBuffer(&instFontBufDesc, nullptr, &_fontVertBuf));
	}

	// Projection buffer
	{
		XMMATRIX ortho = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(
			0.0f, static_cast<float>(_viewWidth), 0.0f, static_cast<float>(_viewHeight), 0.1f, 100.0f
		));

		XMMATRIX pers = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV4, 
			static_cast<float>(_viewWidth) / static_cast<float>(_viewHeight), 0.01f, 100.0f
		));

		std::array<XMFLOAT4X4, 2> vpMatrix;
		XMStoreFloat4x4(&vpMatrix[0], pers);
		XMStoreFloat4x4(&vpMatrix[1], ortho);

		D3D11_BUFFER_DESC projBufDesc = {
			.ByteWidth = sizeof(vpMatrix),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA projResData = {
			.pSysMem = &vpMatrix,
		};

		HR(_device->CreateBuffer(&projBufDesc, &projResData, &_projBuf));
	}

	HR(CreateDDSTextureFromFile(
			_device,
			L"res/wood/Wood066_1K_Color.dds",
			nullptr,
			&_woodTexView));

	HR(CreateDDSTextureFromFile(
			_device,
			L"res/font/Hack.dds",
			nullptr,
			&_fontTexView));

	HR(CreateDDSTextureFromFile(
			_device,
			L"res/heart/heart.dds",
			nullptr,
			&_heartTexView));

	// Cube section

	// Cube model matrix
	{
		D3D11_BUFFER_DESC cubeModelBufDesc = {
			.ByteWidth = sizeof(XMFLOAT4X4),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};

		HR(_device->CreateBuffer(&cubeModelBufDesc, nullptr, &_cubeModelBuf));
	}

	// Cube vertices
	{
		std::array<Cube::Vertex, 24> cubeVert = {
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },

			Cube::Vertex { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
			Cube::Vertex { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			Cube::Vertex { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		};

		D3D11_BUFFER_DESC cubeVertDesc = {
			.ByteWidth = sizeof(cubeVert),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA cubeVertResData = {
			.pSysMem = cubeVert.data(),
		};

		HR(_device->CreateBuffer(&cubeVertDesc, &cubeVertResData, &_cubeVertBuf));
	}

	// Cube indices
	{
		std::array<unsigned short, 36> cubeIdx = {
			3, 1, 0, 2, 1, 3,
			6, 4, 5, 7, 4, 6,
			11, 9, 8, 10, 9, 11,
			14, 12, 13, 15, 12, 14,
			19, 17, 16, 18, 17, 19,
			22, 20, 21, 23, 20, 22
		};

		D3D11_BUFFER_DESC cubeIdxDesc = {
			.ByteWidth = sizeof(cubeIdx),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA cubeIdxResData = {
			.pSysMem = cubeIdx.data(),
		};

		HR(_device->CreateBuffer(&cubeIdxDesc, &cubeIdxResData, &_cubeIdxBuf));
	}
}

void D3DRenderer::clrScr(const std::array<float, 4>& color) {
	_context->ClearRenderTargetView(_bBufferTarget, color.data());
	_context->ClearDepthStencilView(_depthTexView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void D3DRenderer::renderCube(const std::span<Cube::Data> cubes) {
	if(_context == nullptr) return;

	unsigned int stride = sizeof(Cube::Vertex);
	unsigned int offset = 0;

	_context->IASetInputLayout(_cubeIL);
	_context->IASetVertexBuffers(0, 1, &_cubeVertBuf, &stride, &offset);
	_context->IASetIndexBuffer(_cubeIdxBuf, DXGI_FORMAT_R16_UINT, 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::array cbuffers = { _projBuf, _cubeModelBuf, };

	_context->VSSetShader(_cubeVS, 0, 0);
	_context->VSSetConstantBuffers(0, cbuffers.size(), cbuffers.data());

	_context->RSSetViewports(1, &_viewport);

	std::array textures = {
		_woodTexView, _heartTexView
	};

	_context->PSSetShader(_combiPS, 0, 0);
	_context->PSSetShaderResources(0, textures.size(), textures.data());
	_context->PSSetSamplers(0, 1, &_texSampler);

	_context->OMSetRenderTargets(1, &_bBufferTarget, _depthTexView);
	_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	DirectX::XMFLOAT4X4 model;
	for (size_t iter = 0; iter < cubes.size(); iter++) {
		model = cubes[iter].getWorldMatrix();

		D3D11_MAPPED_SUBRESOURCE mappedModelBuf = {};
		_context->Map(_cubeModelBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedModelBuf);
		memcpy(mappedModelBuf.pData, &model, sizeof(model));
		_context->Unmap(_cubeModelBuf, 0);

		_context->DrawIndexed(36, 0, 0);
	}
}

void D3DRenderer::renderSprites(const std::span<Sprite::Data> sprites) {
	if(_context == nullptr) return;
	
	// Update instances
	_spriteInstMem.reserve(sprites.size());
	_spriteInstMem.clear();
	for (size_t iter = 0; iter < sprites.size(); iter++)
		_spriteInstMem.push_back({ sprites[iter].getWorldMatrix() });

	D3D11_MAPPED_SUBRESOURCE mappedInstBuf = {};
	_context->Map(_spriteInstBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedInstBuf);
	memcpy(mappedInstBuf.pData, _spriteInstMem.data(), _spriteInstMem.size() * sizeof(Sprite::Instance));
	_context->Unmap(_spriteInstBuf, 0);

	// Start drawing
	unsigned int stride[] = { sizeof(Sprite::Vertex), sizeof(Sprite::Instance) };
	unsigned int offset[] = { 0, 0 };
	ID3D11Buffer* vertBufs[] = { _spriteVertBuf, _spriteInstBuf };

	_context->IASetInputLayout(_spriteIL);
	_context->IASetVertexBuffers(0, 2, vertBufs, stride, offset);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_context->VSSetShader(_spriteVS, 0, 0);
	_context->VSSetConstantBuffers(0, 1, &_projBuf);

	_context->RSSetViewports(1, &_viewport);

	_context->PSSetShader(_PS, 0, 0);
	_context->PSSetShaderResources(0, 1, &_woodTexView);
	_context->PSSetSamplers(0, 1, &_texSampler);

	_context->OMSetRenderTargets(1, &_bBufferTarget, nullptr);
	_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	_context->DrawInstanced(6, sprites.size(), 0, 0);
}

void D3DRenderer::renderString(const std::span<Font::String> strings) {
	using namespace DirectX;

	if(_context == nullptr) return;

	// Generated vertices for string
	size_t overall_size = 0;
	for (auto& str : strings) overall_size += str.data.size();

	_fontVertMem.reserve(overall_size * 6);
	_fontVertMem.clear();

	const float lWidth = 2002.0f / 26.0f, lHeight = 150.0f;
	const float lAspect = lWidth / lHeight;

	for (auto& str : strings) {
		const float offX = static_cast<float>(str.pxOffset[0]);
		const float offY = static_cast<float>(str.pxOffset[1]);

		const float lVWidth = str.fontSize * lAspect;
		const float lVHeight = str.fontSize;

		for (size_t i = 0; i < str.data.size(); i++) {
			const int idx = str.data[i] - 'A';
			if (idx < 0 || idx > 25) continue;

			const float tStartX = static_cast<float>(idx) * 1.0f / 26.0f;
			const float tEndX = static_cast<float>(idx + 1) * 1.0f / 26.0f;
			const float iOffX = offX + lVWidth * static_cast<float>(i);

			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX + lVWidth, offY + lVHeight), XMFLOAT2(tEndX, 0.0f) });
			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX + lVWidth, offY			  ), XMFLOAT2(tEndX, 1.0f) });
			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX			 , offY			  ), XMFLOAT2(tStartX, 1.0f) });
			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX			 , offY			  ), XMFLOAT2(tStartX, 1.0f) });
			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX			 , offY + lVHeight), XMFLOAT2(tStartX, 0.0f) });
			_fontVertMem.push_back(Sprite::Vertex{ XMFLOAT2(iOffX + lVWidth, offY + lVHeight), XMFLOAT2(tEndX, 0.0f) });
		}
	}

	D3D11_MAPPED_SUBRESOURCE mappedVertBuf = {};
	_context->Map(_fontVertBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertBuf);
	memcpy(mappedVertBuf.pData, _fontVertMem.data(), _fontVertMem.size() * sizeof(Sprite::Vertex));
	_context->Unmap(_fontVertBuf, 0);

	// Draw string
	unsigned int stride[] = { sizeof(Sprite::Vertex) };
	unsigned int offset[] = { 0 };
	ID3D11Buffer* vertBufs[] = { _fontVertBuf };

	_context->IASetInputLayout(_spriteIL);
	_context->IASetVertexBuffers(0, 1, vertBufs, stride, offset);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_context->VSSetShader(_fontVS, 0, 0);
	_context->VSSetConstantBuffers(0, 1, &_projBuf);

	_context->RSSetViewports(1, &_viewport);

	_context->PSSetShader(_PS, 0, 0);
	_context->PSSetShaderResources(0, 1, &_fontTexView);
	_context->PSSetSamplers(0, 1, &_texSampler);

	_context->OMSetRenderTargets(1, &_bBufferTarget, nullptr);
	_context->OMSetBlendState(_blendState, nullptr, 0xFFFFFFFF);

	_context->Draw(_fontVertMem.size(), 0);
}

void D3DRenderer::present() {
	_swapChain->Present(1, 0);
}

void D3DRenderer::cleanUp() {
	retire(_texSampler);
	retire(_fontTexView);
	retire(_woodTexView);
	retire(_heartTexView);
	retire(_blendState);
	retire(_projBuf);
	retire(_spriteInstBuf);
	retire(_spriteVertBuf);
	retire(_fontVertBuf);
	retire(_cubeModelBuf);
	retire(_cubeIdxBuf);
	retire(_cubeVertBuf);
	retire(_spriteIL);
	retire(_cubeIL);
	retire(_cubeVS);
	retire(_fontVS);
	retire(_spriteVS);
	retire(_combiPS);
	retire(_PS);
	retire(_depthTexView);
	retire(_depthTex);
	retire(_bBufferTarget);
	retire(_swapChain);
	retire(_context);
	retire(_device);
}
