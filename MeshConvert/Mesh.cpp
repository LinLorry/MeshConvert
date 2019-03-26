#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

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

	typedef std::unordered_map<XMFLOAT2, size_t, Mesh::XMFLOATHash, Mesh::XMFLOATCompare> XMFLOAT2Map;
	typedef std::unordered_map<XMFLOAT3, size_t, Mesh::XMFLOATHash, Mesh::XMFLOATCompare> XMFLOAT3Map;
	typedef std::unordered_map<XMFLOAT4, size_t, Mesh::XMFLOATHash, Mesh::XMFLOATCompare> XMFLOAT4Map;

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

HRESULT Mesh::SetVertexData(_Inout_ DirectX::VBReader& reader, _In_ size_t nVerts)
{
	if (!nVerts)
		return E_INVALIDARG;

	// Release vertex data
	mnVerts = 0;
	mPositions.reset();
	mNormals.reset();
	mTangents.reset();
	mBiTangents.reset();
	mTexCoords.reset();
	mColors.reset();
	mBlendIndices.reset();
	mBlendWeights.reset();

	// Load positions (required)
	std::unique_ptr<XMFLOAT3[]> pos(new (std::nothrow) XMFLOAT3[nVerts]);
	if (!pos)
		return E_OUTOFMEMORY;

	HRESULT hr = reader.Read(pos.get(), "SV_Position", 0, nVerts);
	if (FAILED(hr))
		return hr;

	// Load normals
	std::unique_ptr<XMFLOAT3[]> norms;
	auto e = reader.GetElement11("NORMAL", 0);
	if (e)
	{
		norms.reset(new (std::nothrow) XMFLOAT3[nVerts]);
		if (!norms)
			return E_OUTOFMEMORY;

		hr = reader.Read(norms.get(), "NORMAL", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load tangents
	std::unique_ptr<XMFLOAT4[]> tans1;
	e = reader.GetElement11("TANGENT", 0);
	if (e)
	{
		tans1.reset(new (std::nothrow) XMFLOAT4[nVerts]);
		if (!tans1)
			return E_OUTOFMEMORY;

		hr = reader.Read(tans1.get(), "TANGENT", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load bi-tangents
	std::unique_ptr<XMFLOAT3[]> tans2;
	e = reader.GetElement11("BINORMAL", 0);
	if (e)
	{
		tans2.reset(new (std::nothrow) XMFLOAT3[nVerts]);
		if (!tans2)
			return E_OUTOFMEMORY;

		hr = reader.Read(tans2.get(), "BINORMAL", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load texture coordinates
	std::unique_ptr<XMFLOAT2[]> texcoord;
	e = reader.GetElement11("TEXCOORD", 0);
	if (e)
	{
		texcoord.reset(new (std::nothrow) XMFLOAT2[nVerts]);
		if (!texcoord)
			return E_OUTOFMEMORY;

		hr = reader.Read(texcoord.get(), "TEXCOORD", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load vertex colors
	std::unique_ptr<XMFLOAT4[]> colors;
	e = reader.GetElement11("COLOR", 0);
	if (e)
	{
		colors.reset(new (std::nothrow) XMFLOAT4[nVerts]);
		if (!colors)
			return E_OUTOFMEMORY;

		hr = reader.Read(colors.get(), "COLOR", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load skinning bone indices
	std::unique_ptr<XMFLOAT4[]> blendIndices;
	e = reader.GetElement11("BLENDINDICES", 0);
	if (e)
	{
		blendIndices.reset(new (std::nothrow) XMFLOAT4[nVerts]);
		if (!blendIndices)
			return E_OUTOFMEMORY;

		hr = reader.Read(blendIndices.get(), "BLENDINDICES", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Load skinning bone weights
	std::unique_ptr<XMFLOAT4[]> blendWeights;
	e = reader.GetElement11("BLENDWEIGHT", 0);
	if (e)
	{
		blendWeights.reset(new (std::nothrow) XMFLOAT4[nVerts]);
		if (!blendWeights)
			return E_OUTOFMEMORY;

		hr = reader.Read(blendWeights.get(), "BLENDWEIGHT", 0, nVerts);
		if (FAILED(hr))
			return hr;
	}

	// Return values
	mPositions.swap(pos);
	mNormals.swap(norms);
	mTangents.swap(tans1);
	mBiTangents.swap(tans2);
	mTexCoords.swap(texcoord);
	mColors.swap(colors);
	mBlendIndices.swap(blendIndices);
	mBlendWeights.swap(blendWeights);
	mnVerts = nVerts;

	return S_OK;
}

HRESULT Mesh::GetVertexBuffer(_Inout_ DirectX::VBWriter& writer) const
{
	if (!mnVerts || !mPositions)
		return E_UNEXPECTED;

	HRESULT hr = writer.Write(mPositions.get(), "SV_Position", 0, mnVerts);
	if (FAILED(hr))
		return hr;

	if (mNormals)
	{
		auto e = writer.GetElement11("NORMAL", 0);
		if (e)
		{
			hr = writer.Write(mNormals.get(), "NORMAL", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mTangents)
	{
		auto e = writer.GetElement11("TANGENT", 0);
		if (e)
		{
			hr = writer.Write(mTangents.get(), "TANGENT", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mBiTangents)
	{
		auto e = writer.GetElement11("BINORMAL", 0);
		if (e)
		{
			hr = writer.Write(mBiTangents.get(), "BINORMAL", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mTexCoords)
	{
		auto e = writer.GetElement11("TEXCOORD", 0);
		if (e)
		{
			hr = writer.Write(mTexCoords.get(), "TEXCOORD", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mColors)
	{
		auto e = writer.GetElement11("COLOR", 0);
		if (e)
		{
			hr = writer.Write(mColors.get(), "COLOR", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mBlendIndices)
	{
		auto e = writer.GetElement11("BLENDINDICES", 0);
		if (e)
		{
			hr = writer.Write(mBlendIndices.get(), "BLENDINDICES", 0, mnVerts);
			if (FAILED(hr))
				return hr;
		}
	}

	if (mBlendWeights)
	{
		auto e = writer.GetElement11("BLENDWEIGHT", 0);
		if (e)
		{
			hr = writer.Write(mBlendWeights.get(), "BLENDWEIGHT", 0, mnVerts);
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
		if(!ReadFile(hFile.get(), &submeshes[i], bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;
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
		if (!ReadFile(hFile.get(), &mats[i], bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;
	}

	UINT nSubsets = static_cast<UINT>(meshHeader.NumSubsets);
	std::vector<UINT> subsetArray(nSubsets);
	bytesToRead = sizeof(UINT);
	for (i = 0; i < nSubsets; ++i)
	{
		if (!ReadFile(hFile.get(), &subsetArray[i], bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (bytesRead != bytesToRead)
			return E_FAIL;
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
	if (ibHeader.IndexType == IT_16BIT)
	{
		std::unique_ptr<uint16_t[]>
			ib16(new (std::nothrow) uint16_t[bytesToRead / sizeof(uint16_t)]);

		if (!ReadFile(hFile.get(), ib16.get(), bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		SetIndexBuffer32(ib16);
	}
	else
	{
		if (!ReadFile(hFile.get(), mIndices.get(), bytesToRead, &bytesRead, nullptr))
			return HRESULT_FROM_WIN32(GetLastError());
	}
	
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
		assert(
			(header.Version == SDKMESH_FILE_VERSION || header.Version == SDKMESH_FILE_VERSION_V2) &&
			(header.NumTotalSubsets == static_cast<UINT>(submeshes.size())) &&
			(header.HeaderSize == headerSize) &&
			(meshHeader.NumFrameInfluences == 1) &&
			(header.NonBufferDataSize == nonBufferDataSize) &&
			(header.VertexStreamHeadersOffset == sizeof(SDKMESH_HEADER)) &&
			(header.IndexStreamHeadersOffset == header.VertexStreamHeadersOffset + sizeof(SDKMESH_VERTEX_BUFFER_HEADER)) &&
			(header.MeshDataOffset == header.IndexStreamHeadersOffset + sizeof(SDKMESH_INDEX_BUFFER_HEADER)) &&
			(header.SubsetDataOffset == header.MeshDataOffset + sizeof(SDKMESH_MESH)) &&
			(header.FrameDataOffset == header.SubsetDataOffset + uint64_t(header.NumTotalSubsets) * sizeof(SDKMESH_SUBSET)) &&
			(header.MaterialDataOffset == header.FrameDataOffset + sizeof(SDKMESH_FRAME))
		);

		//assert(header.Version == SDKMESH_FILE_VERSION || header.Version == SDKMESH_FILE_VERSION_V2);
		//assert(header.NumTotalSubsets == static_cast<UINT>(submeshes.size()));
		//assert(header.HeaderSize == headerSize);
		//assert(meshHeader.NumFrameInfluences == 1);
		//assert(header.NonBufferDataSize == nonBufferDataSize);
		//assert(header.VertexStreamHeadersOffset == sizeof(SDKMESH_HEADER));
		//assert(header.IndexStreamHeadersOffset == header.VertexStreamHeadersOffset + sizeof(SDKMESH_VERTEX_BUFFER_HEADER));
		//assert(header.MeshDataOffset == header.IndexStreamHeadersOffset + sizeof(SDKMESH_INDEX_BUFFER_HEADER));
		//assert(header.SubsetDataOffset == header.MeshDataOffset + sizeof(SDKMESH_MESH));
		//assert(header.FrameDataOffset == header.SubsetDataOffset + uint64_t(header.NumTotalSubsets) * sizeof(SDKMESH_SUBSET));
		//assert(header.MaterialDataOffset == header.FrameDataOffset + sizeof(SDKMESH_FRAME));

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

	size_t nVerts = static_cast<uint64_t>(vbHeader.NumVertices);
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

		hr = reader.AddStream(vb.get(), nVerts, 0, stride);
		if (FAILED(hr))
			return hr;

		hr = SetVertexData(reader, nVerts);
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

			memset(m0, 0, sizeof(Material));

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

			memset(m0, 0, sizeof(Material));

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
		mAttributes[i] = submeshes[i].MaterialID;
	}

	return S_OK;
}

HRESULT Mesh::ExportToObj(const char *outputFile)
{
	using std::vector;

	ScopedHandle hFile(safe_handle(CreateFile(outputFile, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL)));

	XMFLOAT3Map positionMap;
	XMFLOAT2Map textCoordMap;
	XMFLOAT3Map normalMap;

	vector<XMFLOAT3> positions;
	vector<XMFLOAT2> textCoords;
	vector<XMFLOAT3> normals;
	vector<std::vector<FaceIndex>> faces;

	size_t imIndice = 0;

	for (;;)
	{
		if (imIndice >= mnFaces)
		{
			break;
		}
		std::vector<uint32_t> i;

		uint32_t i0 = mIndices[imIndice];
		uint32_t i1 = mIndices[imIndice + 1];
		uint32_t i2 = mIndices[imIndice + 2];

		i.emplace_back(i0);
		i.emplace_back(i1);
		i.emplace_back(i2);

		for (size_t j = imIndice+3; (mIndices[j] == i0) && (mIndices[j+1] == i2); j+=3, imIndice+=3)
		{
			if (j >= mnFaces)
				break;

			i2 = mIndices[j + 2];
			i.emplace_back(i2);
		}

		vector<FaceIndex> faceIndexs(i.size());

		for (size_t j = 0; j < i.size(); j++)
		{
			XMFLOAT3 *position = &mPositions[i[j]];
			XMFLOAT2 *textCoord = &mTexCoords[i[j]];
			XMFLOAT3 *normal = &mNormals[i[j]];

			FaceIndex faceIndex = { 0, 0, 0 };

			if (positionMap.find(*position) == positionMap.end())
			{
				faceIndex.positionIndex = positionMap[*position] = positions.size();
				positions.emplace_back(*position);
			}

			if (textCoordMap.find(*textCoord) == textCoordMap.end())
			{
				faceIndex.textCoordIndex = textCoordMap[*textCoord] = textCoords.size();
				textCoords.emplace_back(*textCoord);
			}

			if (normalMap.find(*normal) == normalMap.end())
			{
				faceIndex.normalIndex = normalMap[*normal] = normals.size();
				normals.emplace_back(*normal);
			}

			faceIndexs[j] = faceIndex;
		}
		faces.emplace_back(faceIndexs);
		imIndice += 3;
	}

	//
	std::ofstream outFile(outputFile, std::ios::out | std::ios::binary);

	for (auto iter = positions.begin(); iter < positions.end(); ++iter)
	{
		outFile << "v " << iter->x << ' ' << iter->y << ' ' << iter->z << '\n';
	}

	for (auto iter = textCoords.begin(); iter < textCoords.end(); ++iter)
	{
		outFile << "vt " << iter->x << " " << iter->y << '\n';
	}

	for (auto iter = normals.begin(); iter < normals.end(); ++iter)
	{
		outFile << "vn " << iter->x << " " << iter->y << " " << iter->z << '\n';
	}

	for (auto iter = faces.begin(); iter < faces.end(); ++iter)
	{
		outFile << "f ";

		for (auto fIter = iter->begin(); fIter < iter->end(); ++fIter)
		{
			FaceIndex face = *fIter;
			outFile << face.positionIndex;
			if (face.textCoordIndex)
			{
				outFile << '/' << face.textCoordIndex;
				if (face.normalIndex)
				{
					outFile << '/' << face.normalIndex;
				}
			}
			outFile << ' ';
		}
		outFile << '\n';
	}

	outFile.close();

	return S_OK;
}

HRESULT Mesh::ExportToSDKMesh(const char *outputFile)
{
	return S_OK;
}

HRESULT Mesh::SetIndexBuffer32(const std::unique_ptr<uint16_t[]> &ib16)
{
	if (!mnFaces)
		return E_FAIL;

	if ((uint64_t(mnFaces) * 3) >= UINT16_MAX)
		return E_FAIL;

	size_t count = mnFaces * 3;

	mIndices.reset(new (std::nothrow) uint32_t[count]);
	if (!mIndices)
		return E_FAIL;

	const uint16_t* iptr = ib16.get();
	for (size_t j = 0; j < count; ++j)
	{
		uint16_t index = *(iptr++);
		if (index == uint32_t(-1))
		{
			mIndices[j] = uint32_t(-1);
		}
		else if (index >= UINT32_MAX)
		{
			mIndices.reset();
			return E_FAIL;
		}
		else
		{
			mIndices[j] = static_cast<uint32_t>(index);
		}
	}

	return S_OK;
}
