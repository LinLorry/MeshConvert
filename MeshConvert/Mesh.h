#pragma once

#ifndef MESH_CONVERT_MESH_CLASS
#define MESH_CONVERT_MESH_CLASS

#include <d3d11_1.h>
#include <windows.h>
#include <directxmath.h>

#include "DirectXMesh.h"

class Mesh
{
private:
	enum Decl
	{
		SV_Position = 1,
		NORMAL,
		COLOR,
		TANGENT,
		BINORMAL,
		TEXCOORD,
		BLENDINDICES,
		BLENDWEIGHT
	};
public:
	Mesh();
	~Mesh();

	void Clear();

	HRESULT LoadFromObj(const char *inputFile);

	HRESULT LoadFromSDKMesh(const char *inputFile);

	HRESULT ExportToObj();

	HRESULT ExportToSDKMesh();

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

private:

	HRESULT OutputVertexBuffer(_Out_ DirectX::VBReader &reader);

private:
	DWORD										declOption;
	size_t                                      mnFaces;
	size_t                                      mnVerts;
	size_t										mnMaterials;
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
	std::unique_ptr<Material[]>					mMaterials;
};

#endif // !MESH_CONVERT_MESH_CLASS


