//==========================================================
// Project:	Memory Patching Classes
// Coder:	Keytonic 2006.10.23
//==========================================================

#ifndef PATCHES_H
#define PATCHES_H

#include <vector>

using namespace std;

class Patch
{
private:

	HANDLE Process;
	DWORDLONG BaseAddress;
	BYTE *OldData;
	BYTE *NewData;
	size_t Size;
	bool IsPatched;

public:

	Patch(HANDLE hProcess,void *lpBaseAddress,void *lpBuffer,size_t size)
	:Process(hProcess)
	,BaseAddress((DWORDLONG)lpBaseAddress)
	,OldData(0)
	,NewData(0)
	,Size(size)
	,IsPatched(0)
	{
		OldData = new BYTE[size];
		NewData = new BYTE[size];
		memcpy(NewData,lpBuffer,size);
	}

	~Patch()
	{
		delete [] OldData;
		delete [] NewData;
	}


	void Toggle()
	{
			DWORD prot = PAGE_EXECUTE_READWRITE;

			VirtualProtectEx(Process,(void*)BaseAddress,Size,prot,&prot);

			if(!IsPatched)
				ReadProcessMemory(Process,(void*)BaseAddress,OldData,Size,0);
				//NtReadVirtualMemory(Process,(void*)BaseAddress,OldData,Size,0);

			//NtWriteVirtualMemory(Process,(void*)BaseAddress,IsPatched?OldData:NewData,Size,0);
			WriteProcessMemory(Process,(void*)BaseAddress,IsPatched?OldData:NewData,Size,0);

			VirtualProtectEx(Process,(void*)BaseAddress,Size,prot,&prot);

			IsPatched = !IsPatched;
	}
	bool GetStatus()
	{
		return IsPatched;
	}
};

class Patches
{

private:

	vector<Patch*> patches;

public:

	Patches()
	{
	}

	~Patches()
	{
	}

	size_t GetSize()
	{
		return patches.size();
	}

	void AddPatch(HANDLE hProcess,void *lpBaseAddress,void *lpBuffer,size_t size)
	{
		patches.push_back(new Patch(hProcess,lpBaseAddress,lpBuffer,size));
	}

	void TogglePatches(bool status)
	{
		for(unsigned int i = 0; i < patches.size(); i++)
		{
			if(patches.at(i)->GetStatus() != status)
				patches.at(i)->Toggle();
		}
	}
};

#endif