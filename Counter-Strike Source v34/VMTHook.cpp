/************************************************************************
*					Portable v34 SDK by InUrFace						*
*							15. 03. 2014								*
*					 Not for commercial purposes						*
*																		*
*																		*
************************************************************************/

#include "Main.h"

CVMTHook::CVMTHook(void* instance)
	: m_pInstance(nullptr), m_pOriginalVTable(nullptr), m_pNewVTable(nullptr), m_iNumIndices(0)
{
	if( !instance )
		return;

	m_pInstance = (void***) instance;
	m_pOriginalVTable = *m_pInstance;
	
	// Count number of pointers in the table - bounded and validated
	m_iNumIndices = 0;
	const size_t kMaxIndices = 150;
	while(m_iNumIndices < kMaxIndices && m_pOriginalVTable[m_iNumIndices])
	{
		MEMORY_BASIC_INFORMATION mbi;
		if( !VirtualQuery( m_pOriginalVTable[m_iNumIndices], &mbi, sizeof(mbi) ) )
			break;
		if( !(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) )
			break;
		m_iNumIndices++;
	}

	if( m_iNumIndices == 0 )
		return;

	// Allocate memory on the heap for our own copy of the table
	m_pNewVTable = (void**) HeapAlloc(GetProcessHeap(), 0, sizeof(void*) * m_iNumIndices);
	if( m_pNewVTable )
	{
		memcpy(m_pNewVTable, m_pOriginalVTable, sizeof(void*) * m_iNumIndices);
		SetHookEnabled(true);
	}
}

CVMTHook::~CVMTHook()
{
	// Reset the VTable pointer
	if( m_pInstance && m_pNewVTable && *m_pInstance == m_pNewVTable )
	{
		*m_pInstance = m_pOriginalVTable;
	}

	// Free our copy of the VTable
	if( m_pNewVTable )
	{
		HeapFree(GetProcessHeap(), 0, m_pNewVTable);
		m_pNewVTable = nullptr;
	}
}

void* CVMTHook::GetOriginalFunction(size_t iIndex)
{
	if( iIndex >= m_iNumIndices )
		return nullptr;
	return m_pOriginalVTable[iIndex];
}

void* CVMTHook::HookFunction(size_t iIndex, void* pfnHook)
{
	// Valid index?
	if(iIndex >= m_iNumIndices)
		return NULL;
	
	// Write new pointer
	m_pNewVTable[iIndex] = pfnHook;

	// And return pointer to original function
	return m_pOriginalVTable[iIndex];
}

void CVMTHook::SetHookEnabled(bool bEnabled)
{
	if( !m_pInstance || !m_pNewVTable || !m_pOriginalVTable )
		return;
	if(bEnabled)
	{
		// Point to our copy of the VTable
		*m_pInstance = m_pNewVTable;
	}
	else
	{
		// Point to the original VTable
		*m_pInstance = m_pOriginalVTable;
	}
}
