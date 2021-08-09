#pragma once

#include <DirectXMath.h>

namespace Cube {
	struct Vertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 tex;
	};

	class Data {
		DirectX::XMFLOAT3 _position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 _rotation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 _scale = { 0.0f, 0.0f, 0.0f };

	public:
		Data(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT3& _rot, const DirectX::XMFLOAT3& _scl)
			: _position(_pos), _rotation(_rot), _scale(_scl) {}

		DirectX::XMFLOAT4X4 getWorldMatrix();

		DirectX::XMVECTOR getPosition();
		Data& setPosition(DirectX::XMFLOAT3 other);
		Data& setPosition(DirectX::XMVECTOR other);

		DirectX::XMVECTOR getRotation();
		Data& setRotation(DirectX::XMFLOAT3 other);
		Data& setRotation(DirectX::XMVECTOR other);

		DirectX::XMVECTOR getScale();
		Data& setScale(DirectX::XMFLOAT3 other);
		Data& setScale(DirectX::XMVECTOR other);
	};

}
