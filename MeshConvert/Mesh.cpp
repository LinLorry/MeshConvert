#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#include "Mesh.h"
#include "SDKMesh.h"

using namespace DirectX;

namespace
{
	struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

	typedef std::unique_ptr<void, handle_closer> ScopedHandle;

	inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

	template<typename T> inline HRESULT read_file(HANDLE hFile, T& value)
	{
		DWORD bytesWritten;
		if (!ReadFile(hFile, &value, static_cast<DWORD>(sizeof(T)), &bytesWritten, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesWritten != sizeof(T))
			return E_FAIL;

		return S_OK;
	}

	inline UINT64 roundup4k(UINT64 value)
	{
		return ((value + 4095) / 4096) * 4096;
	}
}

Mesh::Mesh()
{
}


Mesh::~Mesh()
{
}

HRESULT Mesh::OutputVertexBuffer(DirectX::VBReader & reader)
{
	mPositions.reset(new (std::nothrow) XMFLOAT3[mnVerts]);
	mNormals.reset(new (std::nothrow) XMFLOAT3[mnVerts]);
	mTangents.reset(new (std::nothrow) XMFLOAT4[mnVerts]);
	mBiTangents.reset(new (std::nothrow) XMFLOAT3[mnVerts]);
	mTexCoords.reset(new (std::nothrow) XMFLOAT2[mnVerts]);
	mColors.reset(new (std::nothrow) XMFLOAT4[mnVerts]);
	mBlendIndices.reset(new (std::nothrow) XMFLOAT4[mnVerts]);
	mBlendWeights.reset(new (std::nothrow) XMFLOAT4[mnVerts]);

	HRESULT hr = reader.Read(mPositions.get(), "SV_Position", 0, mnVerts);
	if (FAILED(hr))
		return hr;

	if (declOption & (1 << NORMAL))
	{
		auto e = reader.GetElement11("NORMAL", 0);
		if (e)
		{
			hr = reader.Read(mNormals.get(), "NORMAL", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << TANGENT))
	{
		auto e = reader.GetElement11("TANGENT", 0);
		if (e)
		{
			hr = reader.Read(mTangents.get(), "TANGENT", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << BINORMAL))
	{
		auto e = reader.GetElement11("BINORMAL", 0);
		if (e)
		{
			hr = reader.Read(mBiTangents.get(), "BINORMAL", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << TEXCOORD))
	{
		auto e = reader.GetElement11("TEXCOORD", 0);
		if (e)
		{
			hr = reader.Read(mTexCoords.get(), "TEXCOORD", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << COLOR))
	{
		auto e = reader.GetElement11("COLOR", 0);
		if (e)
		{
			hr = reader.Read(mColors.get(), "COLOR", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << BLENDINDICES))
	{
		auto e = reader.GetElement11("BLENDINDICES", 0);
		if (e)
		{
			hr = reader.Read(mBlendIndices.get(), "BLENDINDICES", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (declOption & (1 << BLENDWEIGHT))
	{
		auto e = reader.GetElement11("BLENDWEIGHT", 0);
		if (e)
		{
			hr = reader.Read(mBlendWeights.get(), "BLENDWEIGHT", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	return S_OK;
}

void Mesh::Clear()
{
	mnFaces = mnVerts = 0;

	// Release face data
	mIndices.reset();
	mAttributes.reset();
	mAdjacency.reset();

	// Release vertex data
	mPositions.reset();
	mNormals.reset();
	mTangents.reset();
	mBiTangents.reset();
	mTexCoords.reset();
	mColors.reset();
	mBlendIndices.reset();
	mBlendWeights.reset();
}

HRESULT Mesh::LoadFromObj(const char *inputFile)
{
	return S_OK;
}

HRESULT Mesh::LoadFromSDKMesh(const char *inputFile)
{
	using namespace DXUT;

	Clear();

	HRESULT hr = E_NOTIMPL;

	ScopedHandle hFile(safe_handle(CreateFile(inputFile, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL)));

	if (!hFile)
		return HRESULT_FROM_WIN32(GetLastError());

	// Read File
	SDKMESH_HEADER header = {};
	hr = read_file(hFile.get(), header);
	if (FAILED(hr))
		return hr;

	SDKMESH_VERTEX_BUFFER_HEADER vbHeader = {};
	hr = read_file(hFile.get(), vbHeader);
	if (FAILED(hr))
		return hr;

	SDKMESH_INDEX_BUFFER_HEADER ibHeader = {};
	hr = read_file(hFile.get(), ibHeader);
	if (FAILED(hr))
		return hr;

	SDKMESH_MESH meshHeader = {};
	hr = read_file(hFile.get(), meshHeader);
	if (FAILED(hr))
		return hr;

	DWORD bytesToRead;
	DWORD bytesRead;
	UINT i;

	UINT nSubmeshes = static_cast<UINT>(meshHeader.NumSubsets);
	std::vector<SDKMESH_SUBSET> submeshes(nSubmeshes);
	bytesToRead = sizeof(SDKMESH_SUBSET);
	for (i = 0; i < nSubmeshes; ++i)
	{
		SDKMESH_SUBSET iSdkmeshSubset;
		
		if(!ReadFile(hFile.get(), &iSdkmeshSubset, bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;

		submeshes.emplace_back(iSdkmeshSubset);
	}

	SDKMESH_FRAME frame = {};
	hr = read_file(hFile.get(), frame);
	if (FAILED(hr))
		return hr;

	mnMaterials = static_cast<UINT>(header.NumMaterials);
	std::vector<SDKMESH_MATERIAL> mats(mnMaterials);
	bytesToRead = sizeof(SDKMESH_MATERIAL);
	for (i = 0; i < mnMaterials; ++i)
	{
		SDKMESH_MATERIAL iSdkmeshMaterial;

		if (!ReadFile(hFile.get(), &iSdkmeshMaterial, bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;

		mats.emplace_back(iSdkmeshMaterial);
	}

	UINT nSubsets = static_cast<UINT>(meshHeader.NumSubsets);
	std::vector<UINT> subsetArray(nSubsets);
	bytesToRead = sizeof(UINT);
	for (i = 0; i < nSubsets; ++i)
	{
		UINT j;

		if (!ReadFile(hFile.get(), &j, bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;

		subsetArray.emplace_back(j);
	}

	UINT frameIndex = 0;
	hr = read_file(hFile.get(), frameIndex);
	if (FAILED(hr))
		return hr;

	bytesToRead = static_cast<DWORD>(vbHeader.SizeBytes);
	std::unique_ptr<uint8_t> vb(new (std::nothrow) uint8_t[bytesToRead / sizeof(uint8_t)]);
	if (!ReadFile(hFile.get(), vb.get(), bytesToRead, &bytesRead, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());

	if (bytesRead != bytesToRead)
		return E_FAIL;

	bytesToRead = static_cast<DWORD>(roundup4k(vbHeader.SizeBytes) - vbHeader.SizeBytes);
	if (bytesToRead > 0)
	{
		uint8_t g_padding[4096] = {};
		assert(bytesToRead < sizeof(g_padding));

		if (!ReadFile(hFile.get(), g_padding, bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;
	}

	// 16 byte or 32 byte
	bytesToRead = static_cast<DWORD>(ibHeader.SizeBytes);
	std::unique_ptr<uint16_t[]> 
		ib16(new (std::nothrow) uint16_t[bytesToRead / sizeof(uint16_t)]);
	if (!ReadFile(hFile.get(), ib16.get(), bytesToRead, &bytesRead, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());

	if (bytesRead != bytesToRead)
		return E_FAIL;


	bytesToRead = static_cast<DWORD>(roundup4k(ibHeader.SizeBytes) - ibHeader.SizeBytes);
	if (bytesToRead > 0)
	{
		uint8_t g_padding[4096] = {};
		assert(bytesToRead < sizeof(g_padding));

		if (!ReadFile(hFile.get(), g_padding, bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;
	}

	//
	{
		UINT headerSize = sizeof(SDKMESH_HEADER) + sizeof(SDKMESH_VERTEX_BUFFER_HEADER) + sizeof(SDKMESH_INDEX_BUFFER_HEADER);
		size_t staticDataSize = sizeof(SDKMESH_MESH)
			+ header.NumTotalSubsets * sizeof(SDKMESH_SUBSET)
			+ sizeof(SDKMESH_FRAME)
			+ header.NumMaterials * sizeof(SDKMESH_MATERIAL);
		uint64_t nonBufferDataSize = uint64_t(staticDataSize) + uint64_t(subsetArray.size()) * sizeof(UINT) + sizeof(UINT);
		//assert(
		//	(header.Version == SDKMESH_FILE_VERSION || header.Version == SDKMESH_FILE_VERSION_V2) &&
		//	(header.NumTotalSubsets == static_cast<UINT>(submeshes.size())) &&
		//	(header.HeaderSize == headerSize) &&
		//	(meshHeader.NumFrameInfluences == 1) &&
		//	(header.NonBufferDataSize == nonBufferDataSize) &&
		//	(header.VertexStreamHeadersOffset == sizeof(SDKMESH_HEADER)) &&
		//	(header.IndexStreamHeadersOffset == header.VertexStreamHeadersOffset + sizeof(SDKMESH_VERTEX_BUFFER_HEADER)) &&
		//	(header.MeshDataOffset == header.IndexStreamHeadersOffset + sizeof(SDKMESH_INDEX_BUFFER_HEADER)) &&
		//	(header.SubsetDataOffset == header.MeshDataOffset + sizeof(SDKMESH_MESH)) &&
		//	(header.FrameDataOffset == header.SubsetDataOffset + uint64_t(header.NumTotalSubsets) * sizeof(SDKMESH_SUBSET)) &&
		//	(header.MaterialDataOffset == header.FrameDataOffset + sizeof(SDKMESH_FRAME))
		//);

		assert(header.Version == SDKMESH_FILE_VERSION || header.Version == SDKMESH_FILE_VERSION_V2);
		assert(header.NumTotalSubsets == static_cast<UINT>(submeshes.size()));
		assert(header.HeaderSize == headerSize);
		assert(meshHeader.NumFrameInfluences == 1);
		assert(header.NonBufferDataSize == nonBufferDataSize);
		assert(header.VertexStreamHeadersOffset == sizeof(SDKMESH_HEADER));
		assert(header.IndexStreamHeadersOffset == header.VertexStreamHeadersOffset + sizeof(SDKMESH_VERTEX_BUFFER_HEADER));
		assert(header.MeshDataOffset == header.IndexStreamHeadersOffset + sizeof(SDKMESH_INDEX_BUFFER_HEADER));
		assert(header.SubsetDataOffset == header.MeshDataOffset + sizeof(SDKMESH_MESH));
		assert(header.FrameDataOffset == header.SubsetDataOffset + uint64_t(header.NumTotalSubsets) * sizeof(SDKMESH_SUBSET));
		assert(header.MaterialDataOffset == header.FrameDataOffset + sizeof(SDKMESH_FRAME));

		UINT64 offset = header.HeaderSize + header.NonBufferDataSize;
		assert(vbHeader.DataOffset == offset);

		offset += roundup4k(vbHeader.SizeBytes);
		assert(ibHeader.DataOffset == offset);

		offset = header.HeaderSize + staticDataSize;
		assert(meshHeader.SubsetOffset == offset);

		offset += uint64_t(meshHeader.NumSubsets) * sizeof(UINT);
		assert(meshHeader.FrameInfluenceOffset == offset);
	}

	//
	static const D3D11_INPUT_ELEMENT_DESC s_elements[] =
	{
		{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 0
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 1
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 2
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 3
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 4
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 5
		{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 6
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 7
	};

	static const D3DVERTEXELEMENT9 s_decls[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 }, // 0
		{ 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0 }, // 1
		{ 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0 }, // 2
		{ 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT, 0 }, // 3
		{ 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BINORMAL, 0 }, // 4
		{ 0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 }, // 5
		{ 0, 0, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, // 6
		{ 0, 0, D3DDECLTYPE_UBYTE4N, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 }, // 7
		{ 0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0 },
	};

	mnVerts = static_cast<uint64_t>(vbHeader.NumVertices);
	size_t stride = static_cast<uint64_t>(vbHeader.StrideBytes);

	D3D11_INPUT_ELEMENT_DESC outputLayout[MAX_VERTEX_ELEMENTS] = {};
	outputLayout[0] = s_elements[0];

	size_t nDecl = 1;

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_BLENDWEIGHT)
	{
		outputLayout[nDecl] = s_elements[7];
		++nDecl;
	}

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_BLENDINDICES)
	{
		outputLayout[nDecl] = s_elements[6];
		++nDecl;
		declOption |= (1 << BLENDINDICES);
	}

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_NORMAL)
	{
		outputLayout[nDecl] = s_elements[1];
		++nDecl;
		declOption |= (1 << NORMAL);
	}

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_COLOR)
	{
		outputLayout[nDecl] = s_elements[2];
		++nDecl;
		declOption |= (1 << COLOR);
	}

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_TANGENT)
	{
		outputLayout[nDecl] = s_elements[3];
		++nDecl;
		declOption |= (1 << TANGENT);
	}

	if (vbHeader.Decl[nDecl].Usage == D3DDECLUSAGE_BINORMAL)
	{
		outputLayout[nDecl] = s_elements[4];
		++nDecl;
		declOption |= (1 << BINORMAL);
	}

	{
		DirectX::VBReader reader;

		hr = reader.Initialize(outputLayout, nDecl);
		if (FAILED(hr))
			return hr;

		hr = reader.AddStream(vb.get(), mnVerts, 0, stride);
		if (FAILED(hr))
			return hr;

		hr = OutputVertexBuffer(reader);
		if (FAILED(hr))
			return hr;
	}
	
	mnFaces = static_cast<uint64_t>(ibHeader.NumIndices / 3);
	mMaterials.reset(new (std::nothrow) Material[mnMaterials]);
	wchar_t buff[MAX_PATH];

	if (header.Version == SDKMESH_FILE_VERSION_V2)
	{
		for (i = 0; i < mnMaterials; ++i)
		{
			auto m0 = &mMaterials[i];
			auto m2 = reinterpret_cast<SDKMESH_MATERIAL_V2*>(&mats[i]);

			memset(m0, 0, sizeof(SDKMESH_MATERIAL_V2));

			if (*m2->Name)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m2->Name, -1,
					buff, MAX_MATERIAL_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			m0->alpha = m2->Alpha;

			if (*m2->AlbetoTexture)
			{
				wchar_t buff[MAX_MATERIAL_NAME];
				int result = MultiByteToWideChar(CP_ACP, 0,
					m2->AlbetoTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			if (*m2->NormalTexture)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m2->NormalTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			if (*m2->EmissiveTexture)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m2->EmissiveTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}
		}
	}
	else if (header.Version == SDKMESH_FILE_VERSION)
	{
		for (i = 0; i < mnMaterials; ++i)
		{
			auto m0 = &mMaterials[i];
			auto m = &mats[i];

			memset(m0, 0, sizeof(SDKMESH_MATERIAL));

			if (*m->Name)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m->Name, -1,
					buff, MAX_MATERIAL_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			if (*m->DiffuseTexture)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m->DiffuseTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			if (*m->NormalTexture)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m->NormalTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			if (*m->SpecularTexture)
			{
				int result = MultiByteToWideChar(CP_ACP, 0,
					m->SpecularTexture, -1,
					buff, MAX_TEXTURE_NAME);
				if (result)
					m0->name = std::wstring(buff);
			}

			m0->diffuseColor.x = m->Diffuse.x;
			m0->diffuseColor.y = m->Diffuse.y;
			m0->diffuseColor.z = m->Diffuse.z;
			m0->alpha = m->Diffuse.w;

			m0->ambientColor.x = m->Ambient.x;
			m0->ambientColor.x = m->Ambient.x;
			m0->ambientColor.x = m->Ambient.x;

			m0->specularColor.x = m->Specular.x;
			m0->specularColor.y = m->Specular.y;
			m0->specularColor.z = m->Specular.z;
			m0->specularPower = m->Power;

			m0->emissiveColor.x = m->Emissive.x;
			m0->emissiveColor.y = m->Emissive.y;
			m0->emissiveColor.z = m->Emissive.z;
		}
	}
	else
	{
		return E_FAIL;
	}

	mAttributes.reset(new (std::nothrow) uint32_t[nSubmeshes]);
	for (i = 0; i < nSubmeshes; i++)
	{
		mAdjacency[i] = submeshes[i].MaterialID;
	}

	return S_OK;
}

HRESULT Mesh::ExportToObj()
{
	return S_OK;
}

HRESULT Mesh::ExportToSDKMesh()
{
	return S_OK;
}
