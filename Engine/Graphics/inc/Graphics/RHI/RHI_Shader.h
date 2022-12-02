#pragma once

#include "Graphics/Defines.h"

#include "Graphics/PixelFormat.h"
#include "Graphics/RHI/RHI_Descriptor.h"
#include "Graphics/Enums.h"
#include "Graphics/ShaderDesc.h"

#include <vector>
#include <map>
#include <unordered_map>

struct IDxcBlob;
struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcResult;

namespace Insight
{
	namespace Graphics
	{
		class RHI_ShaderManager;
		class RenderContext;

		class IS_GRAPHICS RHI_Shader
		{
		public:
			virtual ~RHI_Shader() { }

			bool IsCompiled() const { return m_compiled; }
			std::vector<DescriptorSet> GetDescriptorSets() const { return m_descriptor_sets; }
			PushConstant GetPushConstant() const { return m_push_constant; }
			int GetShaderInputLayoutStride() const { return m_shaderInputLayputStride; }

		private:
			static RHI_Shader* New();
			virtual void Create(RenderContext* context, ShaderDesc desc) = 0;
			virtual void Destroy() = 0;

		protected:
			bool m_compiled = false;
			std::vector<DescriptorSet> m_descriptor_sets;
			PushConstant m_push_constant;
			std::vector<ShaderInputLayout> m_shaderInputLayout;
			int m_shaderInputLayputStride = 0;

			friend RHI_ShaderManager;
		};

		class RHI_ShaderManager
		{
		public:
			RHI_ShaderManager();
			~RHI_ShaderManager();

			void SetRenderContext(RenderContext* context) { m_context = context; }
			RHI_Shader* GetOrCreateShader(ShaderDesc desc);
			void Destroy();

		private:
			std::map<u64, RHI_Shader*> m_shaders;
			RenderContext* m_context{ nullptr };
		};

		enum class ShaderCompilerLanguage
		{
			Spirv,
			Hlsl
		};

		struct ShaderCompiler
		{
			ShaderCompiler();
			ShaderCompiler(const ShaderCompiler& other) = delete;
			ShaderCompiler(ShaderCompiler&& other) = delete;
			~ShaderCompiler();

			std::string StageToFuncName(ShaderStageFlagBits stage);
			std::string StageToProfileTarget(ShaderStageFlagBits stage);

			IDxcBlob* Compile(ShaderStageFlagBits stage, std::string_view filePath, ShaderCompilerLanguage languageToCompileTo);
			void GetDescriptorSets(ShaderStageFlagBits stage, std::vector<DescriptorSet>& descriptor_sets, PushConstant& push_constant);
			std::vector<ShaderInputLayout> GetInputLayout();

			DescriptorType SpvReflectDescriptorTypeToDescriptorType(u32 type);
			DescriptorResourceType SpvReflectDescriptorResourceTypeToDescriptorResourceType(u32 type);

			ShaderCompilerLanguage m_languageToCompileTo;

			IDxcUtils* DXUtils;
			IDxcCompiler3* DXCompiler;

			IDxcResult* ShaderCompileResults;
			IDxcResult* ShaderReflectionResults;
		};
	}
}