#include <windows.h>
#include <iostream>
#include <stdlib.h>

#include "MeshLoadFile.h"

enum OPTIONS
{
	OPT_INPUT = 1,
	OPT_OUTPUT,
	OPT_OBJ,
	OPT_SDKMESH
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
	{ "obj",		OPT_OBJ },
	{ "sdkmesh",	OPT_SDKMESH }
};

namespace
{
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
	DWORD dwOptions = 0;

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


		}
	}
}