/************************************************************************
*					Portable v34 SDK by InUrFace						*
*						15. 03. 2014									*
*					Not for commercial purposes						*
*																		*
*																		*
************************************************************************/

#include "Main.h"

CVMTHook::CVMTHook(void* instance)
{
	// NOTE: never CloseHandle() a handle returned by GetProcessHeap() -
	// it is a process-lifetime pseudo handle; closing it corrupts the CRT heap.
	m_iNumIndices = 0;
	m_pOriginalVTable = NULL;
	m_pNewVTable = NULL;
	m_pInstance = NULL;

	if( instance )
	{
		m_pInstance = (void***) instance;
		m_pOriginalVTable = *m_pInstance;

		//Count number of pointers in the table.
		//Bounded scan: vtables are not guaranteed to be NULL-terminated,
		//an unbounded walk can run past the table into unmapped memory.
		const size_t kMaxIndices = 4096;
		m_iNumIndices = 0;
		while( m_iNumIndices < kMaxIndices && m_pOriginalVTable[ m_iNumIndices ] )
		{
			m_iNumIndices++;
		}

		//Allocate memory on the heap for our own copy of the table
		HANDLE hProcessHeap = GetProcessHeap();

		if( hProcessHeap && m_iNumIndices > 0 )
		{
			m_pNewVTable = (void**) HeapAlloc(hProcessHeap, 0, sizeof(void*) * m_iNumIndices);
			if( m_pNewVTable )
			{
				memcpy(m_pNewVTable, m_pOriginalVTable, sizeof(void*) * m_iNumIndices);
				SetHookEnabled();
			}
		}
	}
}

CVMTHook::~CVMTHook()
{
	//Reset the VTable pointer
	if( m_pInstance && m_pNewVTable && *m_pInstance == m_pNewVTable )
	{
		*m_pInstance = m_pOriginalVTable;
	}

	//Free our copy of the VTable (never CloseHandle the process heap)
	if( m_pNewVTable )
	{
		HANDLE hProcessHeap = GetProcessHeap();
		if( hProcessHeap )
		{
			HeapFree(hProcessHeap, 0, m_pNewVTable);
			m_pNewVTable = NULL;
		}
	}
}

void* CVMTHook::GetOriginalFunction(size_t iIndex)
{
	return m_pOriginalVTable[iIndex];
}

void* CVMTHook::HookFunction(size_t iIndex, void* pfnHook)
{
	//Valid index?
	if( !m_pNewVTable || iIndex >= m_iNumIndices )
		return NULL;

	//Write new pointer
	m_pNewVTable[iIndex] = pfnHook;

	//And return pointer to original function
	return m_pOriginalVTable[iIndex];
}

void CVMTHook::SetHookEnabled(bool bEnabled)
{
	if( !m_pInstance || !m_pNewVTable )
		return;

	if(bEnabled)
	{
		//Point to our copy of the VTable
		*m_pInstance = m_pNewVTable;
	}
	else
	{
		//Point to the original VTable
		*m_pInstance = m_pOriginalVTable;
	}
}
