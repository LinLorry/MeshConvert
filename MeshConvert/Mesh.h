#pragma once

#ifndef MESH_CONVERT_MESH_CLASS
#define MESH_CONVERT_MESH_CLASS

#include <d3d11_1.h>
#include <windows.h>
#include <directxmath.h>

#include "DirectXMesh.h"

class Mesh
{
public:
	Mesh() noexcept : mnFaces(0), mnVerts(0) {};

	void Clear();

	HRESULT LoadFromObj(const char *inputFile);

	HRESULT LoadFromSDKMesh(const char *inputFile);

	HRESULT ExportToObj(const char *outputFile);

	HRESULT ExportToSDKMesh(const char *outputFile);

	HRESULT SetIndexBuffer32(const std::unique_ptr<uint16_t[]> &ib16, const size_t nFaces);

	struct Material
	{
		std::wstring        name;
		bool                perVertexColor;
		float               specularPower;
		float               alpha;
		DirectX::XMFLOAT3   ambientColor;
		DirectX::XMFLOAT3   diffuseColor;
		DirectX::XMFLOAT3   specularColor;
		DirectX::XMFLOAT3   emissiveColor;
		std::wstring        texture;
		std::wstring        normalTexture;
		std::wstring        specularTexture;
		std::wstring        emissiveTexture;

		Material() noexcept :
			perVertexColor(false),
			specularPower(1.f),
			alpha(1.f),
			ambientColor{},
			diffuseColor{},
			specularColor{},
			emissiveColor{}
		{
		}

		Material(
			const wchar_t* iname,
			bool pvc,
			float power,
			float ialpha,
			const DirectX::XMFLOAT3& ambient,
			const DirectX::XMFLOAT3 diffuse,
			const DirectX::XMFLOAT3& specular,
			const DirectX::XMFLOAT3& emissive,
			const wchar_t* txtname) :
			name(iname),
			perVertexColor(pvc),
			specularPower(power),
			alpha(ialpha),
			ambientColor(ambient),
			diffuseColor(diffuse),
			specularColor(specular),
			emissiveColor(emissive),
			texture(txtname)
		{
		}
	};

	struct XMFLOATHash
	{
		size_t operator()(const DirectX::XMFLOAT2 &x) const
		{
			size_t x_x = *((size_t *)(&x.x));
			size_t x_y = *((size_t *)(&x.y));
			return static_cast<size_t>(x_x xor x_y);
		}

		size_t operator()(const DirectX::XMFLOAT3 &x) const
		{
			size_t x_x = *((size_t *)(&x.x));
			size_t x_y = *((size_t *)(&x.y));
			size_t x_z = *((size_t *)(&x.z));
			return static_cast<size_t>(x_x xor x_y xor x_z);
		}

		size_t operator()(const DirectX::XMFLOAT4 &x) const
		{
			size_t x_x = *((size_t *)(&x.x));
			size_t x_y = *((size_t *)(&x.y));
			size_t x_z = *((size_t *)(&x.z));
			size_t x_w = *((size_t *)(&x.w));
			return static_cast<size_t>(x_x xor x_y xor x_z xor x_w);
		}
	};

	struct XMFLOATCompare
	{
		inline bool operator()(const DirectX::XMFLOAT2 &x, const DirectX::XMFLOAT2 &y) const
		{
			return !memcmp(&x, &y, sizeof(DirectX::XMFLOAT2));
		}

		inline bool operator()(const DirectX::XMFLOAT3 &x, const DirectX::XMFLOAT3 &y) const
		{
			return !memcmp(&x, &y, sizeof(DirectX::XMFLOAT3));
		}

		inline bool operator()(const DirectX::XMFLOAT4 &x, const DirectX::XMFLOAT4 &y) const
		{
			return !memcmp(&x, &y, sizeof(DirectX::XMFLOAT4));
		}
	};

	struct FaceIndex
	{
		uint32_t positionIndex;
		uint32_t textCoordIndex;
		uint32_t normalIndex;
	};
private:

	HRESULT SetVertexData(_Inout_ DirectX::VBReader& reader, _In_ size_t nVerts);
	HRESULT GetVertexBuffer(_Inout_ DirectX::VBWriter& writer) const;

private:
	size_t                                      mnFaces;
	size_t                                      mnVerts;
	size_t										mnMaterials;
	std::unique_ptr<uint32_t[]>                 mIndices;
	std::unique_ptr<uint32_t[]>                 mAttributes;
	std::unique_ptr<uint32_t[]>                 mAdjacency;
	std::unique_ptr<DirectX::XMFLOAT3[]>		mPositions;
	std::unique_ptr<DirectX::XMFLOAT3[]>		mNormals;
	std::unique_ptr<DirectX::XMFLOAT4[]>		mTangents;
	std::unique_ptr<DirectX::XMFLOAT3[]>		mBiTangents;
	std::unique_ptr<DirectX::XMFLOAT2[]>		mTexCoords;
	std::unique_ptr<DirectX::XMFLOAT4[]>		mColors;
	std::unique_ptr<DirectX::XMFLOAT4[]>		mBlendIndices;
	std::unique_ptr<DirectX::XMFLOAT4[]>		mBlendWeights;
	std::unique_ptr<Material[]>					mMaterials;
};

#endif // !MESH_CONVERT_MESH_CLASS



