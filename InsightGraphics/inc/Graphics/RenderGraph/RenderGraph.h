#pragma once

#ifdef RENDER_GRAPH_ENABLED

#include "Core/Memory.h"
#include "Core/Singleton.h"
#include "Graphics/RenderGraph/RenderGraphPass.h"
#include "Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Graphics/RHI/RHI_CommandList.h"
#include "Graphics/RHI/RHI_Descriptor.h"

namespace Insight
{
	namespace Graphics
	{
		class RHI_Texture;
		class RenderContext;

		template<typename TValue>
		class FrameResource
		{
		public:
			void Setup()
			{
				m_values.clear();
				m_valuePtrs.clear();

				m_values.resize(RenderGraph::s_FarmeCount);
				for (size_t i = 0; i < RenderGraph::s_FarmeCount; ++i)
				{
					m_valuePtrs.push_back(&m_values.at(i));
				}
			}

			TValue* operator->() const
			{
				return Get();
			}

			TValue* Get() const
			{
				ASSERT(!m_valuePtrs.empty());
				return m_valuePtrs.at(RenderGraph::Instance().GetFrameIndex());
			}

			void ForEach(std::function<void(TValue& value)> func)
			{
				for (TValue& v : m_values)
				{
					func(v);
				}
			}

		private:
			std::vector<TValue> m_values;
			std::vector<TValue*> m_valuePtrs;
		};

		class RenderGraph : public Core::Singleton<RenderGraph>
		{
		public:
			RenderGraph();

			void Init(RenderContext* context);
			void Execute();

			RGTextureHandle CreateTexture(std::wstring textureName, RHI_TextureCreateInfo info);

			RGTextureHandle GetTexture(std::wstring textureName) const;
			RHI_Texture* GetRHITexture(RGTextureHandle handle) const;

			RenderpassDescription GetRenderpassDescription(std::wstring_view passName) const;
			PipelineStateObject GetPipelineStateObject(std::wstring_view passName) const;

			void Release();

			u32 GetFrameIndex() const { return m_frameIndex; }

			template<typename TData>
			void AddPass(std::wstring passName, typename RenderGraphPass<TData>::SetupFunc setupFunc
				, typename RenderGraphPass<TData>::ExecuteFunc executeFunc, TData initalData = { })
			{
				m_passes.emplace_back(MakeUPtr<RenderGraphPass<TData>>(std::move(passName), std::move(setupFunc), std::move(executeFunc), std::move(initalData)));
			}

			static u32 s_FarmeCount;

		private:
			void Build();
			void PlaceBarriers();
			void Render();
			void Clear();

			void PlaceBarriersInToPipeline(RenderGraphPassBase* pass, RHI_CommandList* cmdList);

		private:
			RenderContext* m_context = nullptr;
			std::vector<UPtr<RenderGraphPassBase>> m_passes;

			u32 m_frameIndex = 0;
			FrameResource<RHI_ResourceCache<RHI_Texture>> m_textureCaches;
			FrameResource<CommandListManager> m_commandListManager;
			FrameResource<DescriptorAllocator> m_descriptorManagers;
		};
	}
}
#endif //RENDER_GRAPH_ENABLED