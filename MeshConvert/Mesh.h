#pragma once

#ifndef MESH_CONVERT_MESH_CLASS
#define MESH_CONVERT_MESH_CLASS

#include <d3d11_1.h>
#include <directxmath.h>
#include <windows.h>

#include "SDKMesh.h"

class Mesh
{
public:
	Mesh();
	~Mesh();

	HRESULT LoadFromObj(const char *inputFile);

	HRESULT LoadFromSDKMesh(const char *inputFile);

	HRESULT ExportToObj();

	HRESULT ExportToSDKMesh();

private:
	size_t                                      mnFaces;
	size_t                                      mnVerts;
	std::unique_ptr<uint32_t[]>                 mIndices;
	std::unique_ptr<uint32_t[]>                 mAttributes;
	std::unique_ptr<uint32_t[]>                 mAdjacency;
	std::unique_ptr<DirectX::XMFLOAT3[]>        mPositions;
	std::unique_ptr<DirectX::XMFLOAT3[]>        mNormals;
	std::unique_ptr<DirectX::XMFLOAT4[]>        mTangents;
	std::unique_ptr<DirectX::XMFLOAT3[]>        mBiTangents;
	std::unique_ptr<DirectX::XMFLOAT2[]>        mTexCoords;
	std::unique_ptr<DirectX::XMFLOAT4[]>        mColors;
	std::unique_ptr<DirectX::XMFLOAT4[]>        mBlendIndices;
	std::unique_ptr<DirectX::XMFLOAT4[]>        mBlendWeights;
};

#endif // !MESH_CONVERT_MESH_CLASS



