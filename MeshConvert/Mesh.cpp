#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Mesh.h"

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

HRESULT Mesh::LoadFromObj(const char *inputFile)
{
	return S_OK;
}

HRESULT Mesh::LoadFromSDKMesh(const char *inputFile)
{
	using namespace DXUT;

	HRESULT hr = E_NOTIMPL;

	ScopedHandle hFile(safe_handle(CreateFile(inputFile, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL)));

	if (!hFile)
		return HRESULT_FROM_WIN32(GetLastError());

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

	assert(meshHeader.NumFrameInfluences == 1);

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

	UINT nMaterials = static_cast<UINT>(header.NumMaterials);
	std::vector<SDKMESH_MATERIAL> mats(nMaterials);
	bytesToRead = sizeof(SDKMESH_MATERIAL);
	for (i = 0; i < nMaterials; ++i)
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