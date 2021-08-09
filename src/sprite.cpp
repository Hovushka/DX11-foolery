#include <sprite.h>

#include <DirectXMath.h>

DirectX::XMFLOAT3X3 Sprite::Data::getWorldMatrix() {
	DirectX::XMVECTOR zeroes = { 0.0f, 0.0f };
	DirectX::XMVECTOR simdScale = DirectX::XMLoadFloat2(&_scale);
	DirectX::XMVECTOR simdPosition = DirectX::XMLoadFloat2(&_position);

	DirectX::XMMATRIX simdTmp = DirectX::XMMatrixTransformation2D(
		zeroes, 0.0f, simdScale, zeroes, _rotation, simdPosition
	);

	DirectX::XMFLOAT4X4 tmp;
	DirectX::XMStoreFloat4x4(&tmp, DirectX::XMMatrixTranspose(simdTmp));

	DirectX::XMFLOAT3X3 ret(
		tmp._11, tmp._12, tmp._14, 
		tmp._21, tmp._22, tmp._24, 
		tmp._41, tmp._42, tmp._44
	);
	return ret;
}

DirectX::XMVECTOR Sprite::Data::getPosition() {
	return DirectX::XMLoadFloat2(&_position);
}

Sprite::Data& Sprite::Data::setPosition(DirectX::XMFLOAT2 other) {
	_position = other;
	return *this;
}

Sprite::Data& Sprite::Data::setPosition(DirectX::XMVECTOR other) {
	DirectX::XMStoreFloat2(&_position, other);
	return *this;
}

float Sprite::Data::getRotation() {
	return _rotation;
}

Sprite::Data& Sprite::Data::setRotation(float other) {
	_rotation = other;
	return *this;
}

DirectX::XMVECTOR Sprite::Data::getScale() {
	return DirectX::XMLoadFloat2(&_scale);
}

Sprite::Data& Sprite::Data::setScale(DirectX::XMFLOAT2 other) {
	_scale = other;
	return *this;
}

Sprite::Data& Sprite::Data::setScale(DirectX::XMVECTOR other) {
	DirectX::XMStoreFloat2(&_scale, other);
	return *this;
}
