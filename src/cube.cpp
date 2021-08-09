#include <cube.h>

#include <DirectXMath.h>

DirectX::XMFLOAT4X4 Cube::Data::getWorldMatrix() {
	DirectX::XMVECTOR translateVec = DirectX::XMLoadFloat3(&_position);
	DirectX::XMMATRIX translate = DirectX::XMMatrixTranslationFromVector(translateVec);

	DirectX::XMVECTOR rotateVec = DirectX::XMLoadFloat3(&_rotation);
	DirectX::XMMATRIX rotate = DirectX::XMMatrixRotationRollPitchYawFromVector(rotateVec);

	DirectX::XMVECTOR scaleVec = DirectX::XMLoadFloat3(&_scale);
	DirectX::XMMATRIX scale = DirectX::XMMatrixScalingFromVector(scaleVec);

	DirectX::XMMATRIX world = DirectX::XMMatrixMultiplyTranspose(
		DirectX::XMMatrixMultiply(scale, rotate), translate
	);

	DirectX::XMFLOAT4X4 ret;
	DirectX::XMStoreFloat4x4(&ret, world);

	return ret;
}

DirectX::XMVECTOR Cube::Data::getPosition() {
	return DirectX::XMLoadFloat3(&_position);
}

Cube::Data& Cube::Data::setPosition(DirectX::XMFLOAT3 other) {
	_position = other;
	return *this;
}

Cube::Data& Cube::Data::setPosition(DirectX::XMVECTOR other) {
	DirectX::XMStoreFloat3(&_position, other);
	return *this;
}

DirectX::XMVECTOR Cube::Data::getRotation() {
	return DirectX::XMLoadFloat3(&_rotation);
}

Cube::Data& Cube::Data::setRotation(DirectX::XMFLOAT3 other) {
	_rotation = other;
	return *this;
}

Cube::Data& Cube::Data::setRotation(DirectX::XMVECTOR other) {
	DirectX::XMStoreFloat3(&_rotation, other);
	return *this;
}

DirectX::XMVECTOR Cube::Data::getScale() {
	return DirectX::XMLoadFloat3(&_scale);
}

Cube::Data& Cube::Data::setScale(DirectX::XMFLOAT3 other) {
	_scale = other;
	return *this;
}

Cube::Data& Cube::Data::setScale(DirectX::XMVECTOR other) {
	DirectX::XMStoreFloat3(&_scale, other);
	return *this;
}
