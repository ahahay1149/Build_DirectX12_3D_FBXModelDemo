#include <d3d12.h>
#include "ComPtr.h"

class VertexBuffer
{
public:
	VertexBuffer(size_t size, size_t stride, const void* pInitData);	
	D3D12_VERTEX_BUFFER_VIEW view() const;	
	bool IsValid();	

private:
	bool m_IsValid = false;	
	ComPtr<ID3D12Resource> m_pBuffer = nullptr;	
	D3D12_VERTEX_BUFFER_VIEW m_View = {};	

	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};

