#pragma once

#include "Graphics/CommandList.h"
#include "Graphics/RHI/DX12/RHI_PhysicalDevice_DX12.h"

namespace Insight
{
	namespace Graphics
	{
		namespace RHI::DX12
		{
			class RenderContext_DX12;
			class CommandAllocator_DX12;
			struct FrameResourceDX12;

			class CommandList_DX12
			{
			public:
				CommandList_DX12();
				CommandList_DX12(RenderContext_DX12* context);
				~CommandList_DX12();

				void Record(CommandList& cmdList, FrameResourceDX12& frameResouces);
				void Reset();
				void Close();
				ID3D12GraphicsCommandList* GetCommandBuffer() const { return m_commandList.Get(); }
				
				bool operator==(const CommandList_DX12& other) const { return m_commandList == other.m_commandList; }

				void Release();

			private:
				bool CanDraw(CommandList& cmdList);
				bool BindDescriptorSets();
				ComPtr<ID3D12GraphicsCommandList>& GetCommandBufferInteral() { return m_commandList; }

			private:
				ComPtr<ID3D12GraphicsCommandList> m_commandList;
				RenderContext_DX12* m_context;

				bool m_activeRenderpass = false;
				PipelineStateObject m_pso;
				PipelineStateObject m_activePSO;

				FrameResourceDX12* m_frameResouces = nullptr;

				friend class CommandAllocator_DX12;
				friend class RenderContext_DX12;
			};

			class CommandAllocator_DX12
			{
			public:
				CommandAllocator_DX12();
				~CommandAllocator_DX12() { }

				void Init(RenderContext_DX12* context);
				void Update();
				void Destroy();

				CommandList_DX12* GetCommandList();
				void ReturnCommandList(CommandList_DX12& cmdList);

			private:
				RenderContext_DX12* m_context{ nullptr };

				std::vector<CommandList_DX12*> m_allocLists;
				std::vector<CommandList_DX12*> m_freeLists;

				ComPtr<ID3D12CommandAllocator> m_allocator{ nullptr };
			};
		}
	}
}