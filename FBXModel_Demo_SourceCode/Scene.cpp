#include "Scene.h"
#include "Engine.h"
#include "App.h"
#include <d3dx12.h>
#include "SharedStruct.h"
#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "IndexBuffer.h"
#include "AssimpLoader.h"
#include "DescriptorHeap.h"
#include "Texture2D.h"

Scene* g_Scene;

using namespace DirectX;

VertexBuffer* vertexBuffer;
ConstantBuffer* constantBuffer[Engine::FRAME_BUFFER_COUNT];
RootSignature* rootSignature;
PipelineState* pipelineState;
IndexBuffer* indexBuffer;

const wchar_t* modelFile = L"Assets/Alicia/FBX/Alicia_solid_Unity.FBX";
std::vector<Mesh> meshes;
std::vector<VertexBuffer*> vertexBuffers;	
std::vector<IndexBuffer*> indexBuffers;		

#include <filesystem>
namespace fs = std::filesystem;
std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

DescriptorHeap* descriptorHeap;
std::vector<DescriptorHandle*> materialHandles = {};  

bool Scene::Init()
{
	descriptorHeap = new DescriptorHeap();

	ImportSettings importSettings = 
	{
		modelFile,
		meshes,
		false,
		true	
	};

	AssimpLoader loader;
	if (!loader.Load(importSettings))
	{
		return false;
	}

	vertexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(Vertex) * meshes[i].Vertices.size();
		auto stride = sizeof(Vertex);
		auto vertices = meshes[i].Vertices.data();
		auto pVB = new VertexBuffer(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("���_�o�b�t�@�̐����Ɏ��s\n");
			return false;
		}

		vertexBuffers.push_back(pVB);
	}

	indexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(uint32_t) * meshes[i].Indices.size();
		auto indices = meshes[i].Indices.data();
		auto pIB = new IndexBuffer(size, indices);
		if (!pIB->IsValid())
		{
			printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s\n");
			return false;
		}

		indexBuffers.push_back(pIB);
	}

	auto eyePos = XMVectorSet(0.0f, 120.0f, 75.0f, 0.0f);	
	auto targetPos = XMVectorSet(0.0f,120.0f,0.0f,0.0f);	
	auto upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	
	auto fov = XMConvertToRadians(60);	
	auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

	for (size_t i = 0; i < Engine::FRAME_BUFFER_COUNT; i++)
	{
		constantBuffer[i] = new ConstantBuffer(sizeof(Transform));
		if (!constantBuffer[i]->IsValid())
		{
			printf("�ϊ��s��p�萔�o�b�t�@�̐����Ɏ��s\n");
			return false;
		}

		auto ptr = constantBuffer[i]->GetPtr<Transform>();
		ptr->World = XMMatrixIdentity();
		ptr->View = XMMatrixLookAtRH(eyePos, targetPos, upward);
		ptr->Proj = XMMatrixPerspectiveFovRH(fov, aspect, 0.3f, 1000.0f);
	}

	materialHandles.clear();

	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto texPath = ReplaceExtension(meshes[i].DiffuseMap, "tga");  
		auto mainTex = Texture2D::Get(texPath);
		auto handle = descriptorHeap->Register(mainTex);
		materialHandles.push_back(handle);
	}

	rootSignature = new RootSignature();
	if (!rootSignature->IsValid())
	{
		printf("���[�g�V�O�l�`���̐����Ɏ��s\n");
		return false;
	}

	pipelineState = new PipelineState();
	pipelineState->SetInputLayout(Vertex::InputLayout);
	pipelineState->SetRootSignature(rootSignature->Get());
	pipelineState->SetVS(L"../x64/Debug/SimpleVS.cso");
	pipelineState->SetPS(L"../x64/Debug/SimplePS.cso");
	pipelineState->Create();
	if (!pipelineState->IsValid())
	{
		printf("�p�C�v���C���X�e�[�g�̐����Ɏ��s\n");
		return false;
	}

	printf("�V�[���̏������ɐ���\n");
	return true;
}

void Scene::Update()
{
}

void Scene::Draw()
{
	auto currentIndex = g_Engine->CurrentBackBufferIndex();	
	auto commandList = g_Engine->CommandList();				
	auto materialHeap = descriptorHeap->GetHeap();			

	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto vbView = vertexBuffers[i]->view();	
		auto ibView = indexBuffers[i]->View();	

		commandList->SetGraphicsRootSignature(rootSignature->Get());
		commandList->SetPipelineState(pipelineState->Get());
		commandList->SetGraphicsRootConstantBufferView(0, constantBuffer[currentIndex]->GetAddress());	

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
		commandList->IASetVertexBuffers(0, 1, &vbView);	
		commandList->IASetIndexBuffer(&ibView);	

		commandList->SetDescriptorHeaps(1, &materialHeap);  
		commandList->SetGraphicsRootDescriptorTable(1, materialHandles[i]->HandleGPU);  

		commandList->DrawIndexedInstanced(meshes[i].Indices.size(), 1, 0, 0, 0);	
	}
}