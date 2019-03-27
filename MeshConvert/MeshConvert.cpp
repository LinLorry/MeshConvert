#include <windows.h>
#include <iostream>
#include <fstream>
#include <list>
#include <stdlib.h>

#include "Mesh.h"

enum OPTIONS
{
	OPT_INPUT = 1,
	OPT_OUTPUT,
	OPT_OUT_OBJ,
	OPT_OUT_SDKMESH,
	OPT_IN_OBJ,
	OPT_IN_SDKMESH
};

struct SConversion
{
	wchar_t szSrc[MAX_PATH];
};

struct SValue
{
	const char *pName;
	DWORD dwValue;
};

const SValue g_pOptions[] =
{
	{ "i",			OPT_INPUT },
	{ "o",			OPT_OUTPUT },
	{ "obj",		OPT_OUT_OBJ },
	{ "sdkmesh",	OPT_OUT_SDKMESH }
};

namespace
{
	inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

	struct find_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

	typedef std::unique_ptr<void, find_closer> ScopedFindHandle;

	DWORD LookupByName(const char *pName, const SValue *pArray)
	{
		while (pArray->pName)
		{
			if (!strcmp(pName, pArray->pName))
				return pArray->dwValue;

			pArray++;
		}

		return 0;
	}

	void SearchForFiles(const wchar_t* path, std::list<SConversion>& files, bool recursive)
	{
		// Process files
		WIN32_FIND_DATAW findData = {};
		ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path,
			FindExInfoBasic, &findData,
			FindExSearchNameMatch, nullptr,
			FIND_FIRST_EX_LARGE_FETCH)));
		if (hFile)
		{
			for (;;)
			{
				if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
				{
					wchar_t drive[_MAX_DRIVE] = {};
					wchar_t dir[_MAX_DIR] = {};
					_wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

					SConversion conv;
					_wmakepath_s(conv.szSrc, drive, dir, findData.cFileName, nullptr);
					files.push_back(conv);
				}

				if (!FindNextFileW(hFile.get(), &findData))
					break;
			}
		}

		// Process directories
		if (recursive)
		{
			wchar_t searchDir[MAX_PATH] = {};
			{
				wchar_t drive[_MAX_DRIVE] = {};
				wchar_t dir[_MAX_DIR] = {};
				_wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
				_wmakepath_s(searchDir, drive, dir, L"*", nullptr);
			}

			hFile.reset(safe_handle(FindFirstFileExW(searchDir,
				FindExInfoBasic, &findData,
				FindExSearchLimitToDirectories, nullptr,
				FIND_FIRST_EX_LARGE_FETCH)));
			if (!hFile)
				return;

			for (;;)
			{
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (findData.cFileName[0] != L'.')
					{
						wchar_t subdir[MAX_PATH] = {};

						{
							wchar_t drive[_MAX_DRIVE] = {};
							wchar_t dir[_MAX_DIR] = {};
							wchar_t fname[_MAX_FNAME] = {};
							wchar_t ext[_MAX_FNAME] = {};
							_wsplitpath_s(path, drive, dir, fname, ext);
							wcscat_s(dir, findData.cFileName);
							_wmakepath_s(subdir, drive, dir, fname, ext);
						}

						SearchForFiles(subdir, files, recursive);
					}
				}

				if (!FindNextFileW(hFile.get(), &findData))
					break;
			}
		}
	}

	void PrintUsage()
	{
		using std::cout;

		cout << "Usage meshtransform <option> <file> \n"
			<< "\n"
			<< "	-i			Input file\n"
			<< "	-o			Output file\n"
			<< "	-obj		Format outfile Obj\n"
			<< "	-sdkmesh	Format outfile Sdkmesh\n"
			<< "Example: meshtransform -i test.obj -o test.sdkmesh\n"
			<< "		  meshtransform -i test.sdkmesh -obj\n\n";
	}
}

int main(int argc, char* argv[])
{
	using std::cout;
	using std::endl;

	DWORD dwOptions = 0;

	char inputFile[MAX_PATH] = "";
	char outputFile[MAX_PATH] = "";

	Mesh mesh;

	for (int iArg = 1; iArg < argc; iArg++)
	{
		char *pArg = argv[iArg];

		if (('-' == pArg[0] || ('/' == pArg[0])))
		{
			pArg++;
			char *pValue;

			for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

			if (*pValue)
				*pValue++ = 0;

			DWORD  dwOption = LookupByName(pArg, g_pOptions);

			if (!dwOption || (dwOptions & (1 << dwOption)))
			{
				cout << "ERROR: unknown command-line option" << pArg << "\n\n";
				PrintUsage();
				return 1;
			}

			dwOptions |= (1 << dwOption);

			switch (dwOption)
			{
			case OPT_INPUT:
				if (++iArg >= argc)
				{
					cout << "ERROR: missing input file.\n\n";
					PrintUsage();
					return 1;
				}
				strcpy_s(inputFile, MAX_PATH, argv[iArg]);
				break;
			case OPT_OUTPUT:
				if (++iArg >= argc)
				{
					cout << "ERROR: missing output file.\n\n";
					PrintUsage();
					return 1;
				}
				strcpy_s(outputFile, MAX_PATH, argv[iArg]);
				break;
			case OPT_OUT_OBJ:
				dwOptions |= (1 << OPT_OUT_OBJ);
				break;
			case OPT_OUT_SDKMESH:
				dwOptions |= (1 << OPT_OUT_SDKMESH);
				break;
			}
		}
	}

	char iExt[_MAX_EXT];
	char ifName[_MAX_FNAME];

	_splitpath_s(inputFile, nullptr, 0, nullptr, 0, ifName, _MAX_FNAME, iExt, _MAX_EXT);

	cout << "Input File: " << inputFile << endl;
	fflush(stdout);

	HRESULT hr = E_NOTIMPL;
	if (strcmp(iExt, ".obj") == 0)
	{
		hr = mesh.LoadFromObj(inputFile);
		dwOptions |= (1 << OPT_IN_OBJ);
	}
	else if (strcmp(iExt, ".sdkmesh") == 0)
	{
		hr = mesh.LoadFromSDKMesh(inputFile);
		dwOptions |= (1 << OPT_IN_SDKMESH);
	}
	else
	{
		cout << "\nERROR: Importing files not supported\n";
		return 1;
	}

	if (FAILED(hr))
	{
		cout << "FAILED " << hr << endl;
		return 1;
	}
	
	cout << "Success Load File.\n";

	char oExt[_MAX_EXT];
	char ofName[_MAX_FNAME];

	if (*outputFile)
	{
		_splitpath_s(outputFile, nullptr, 0, nullptr, 0, ofName, _MAX_FNAME, oExt, _MAX_EXT);
	}
	else
	{
		if (dwOptions & (1 << OPT_OUT_OBJ))
		{
			strcpy_s(oExt, ".obj");
		}
		else if (dwOptions & (1 << OPT_OUT_SDKMESH))
		{
			strcpy_s(oExt, ".sdkmesh");
		}

		strcpy_s(ofName, ifName);
		_makepath_s(outputFile, nullptr, nullptr, ofName, oExt);
	}

	cout << "Output File: " << outputFile;

	if (!_stricmp(oExt, ".obj"))
	{
		hr = mesh.ExportToObj(outputFile);
	}
	else if (!_stricmp(oExt, ".sdkmesh"))
	{
		hr = mesh.ExportToSDKMesh(outputFile);
	}
	else
	{
		cout << "\nERROR: Unknown output file type " << oExt << endl;
		return 1;
	}

	if (FAILED(hr))
	{
		cout << "\nERROR: Failed write " << hr << "-> " << outputFile << endl;
		return 1;
	}

	cout << "Success Output File.\n";

	return 0;
}