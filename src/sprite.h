#pragma once

#include <DirectXMath.h>

namespace Sprite {

	struct Vertex {
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 tex;
	};

	struct Instance {
		DirectX::XMFLOAT3X3 model;
	};

	class Data {
		DirectX::XMFLOAT2 _position = { 0.0f, 0.0f };
		float			  _rotation = 0.0f;
		DirectX::XMFLOAT2 _scale = { 1.0f, 1.0f };

	public:
		Data(const DirectX::XMFLOAT2& _pos, float _rot, const DirectX::XMFLOAT2& _scl)
			: _position(_pos), _rotation(_rot), _scale(_scl) {}

		DirectX::XMFLOAT3X3 getWorldMatrix();

		DirectX::XMVECTOR getPosition();
		Data& setPosition(DirectX::XMFLOAT2 other);
		Data& setPosition(DirectX::XMVECTOR other);

		float getRotation();
		Data& setRotation(float other);

		DirectX::XMVECTOR getScale();
		Data& setScale(DirectX::XMFLOAT2 other);
		Data& setScale(DirectX::XMVECTOR other);
	};

}