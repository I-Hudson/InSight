#include "Graphics/RHI/RHI_Texture.h"
#include "Graphics/GraphicsManager.h"

#include "Graphics/RHI/Vulkan/RHI_Texture_Vulkan.h"
#include "Graphics/RHI/DX12/RHI_Texture_DX12.h"

#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Insight
{
	namespace Graphics
	{
		RHI_Texture* RHI_Texture::New()
		{
#if defined(IS_VULKAN_ENABLED)
			if (GraphicsManager::IsVulkan()) { return NewTracked(RHI::Vulkan::RHI_Texture_Vulkan); }
#endif
#if defined(IS_DX12_ENABLED)
			else if (GraphicsManager::IsDX12()) { return NewTracked(RHI::DX12::RHI_Texture_DX12); }
#endif	
			return nullptr;
		}

		void RHI_Texture::LoadFromFile(std::string filePath)
		{
			IS_PROFILE_FUNCTION();
			if (filePath.empty() || !std::filesystem::exists(filePath))
			{
				return;
			}

			int width, height, channels;
			Byte* data = nullptr;
			{
				IS_PROFILE_SCOPE("stbi_load");
				data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			}
			if (!data)
			{
				return;
			}

			RHI_TextureCreateInfo createInfo = { };
			createInfo.TextureType = TextureType::Tex2D;
			createInfo.Width = width;
			createInfo.Height = height;
			createInfo.Depth = 1;
			createInfo.Format = PixelFormat::R8G8B8A8_UNorm;
			createInfo.ImageUsage = ImageUsageFlagsBits::Sampled | ImageUsageFlagsBits::TransferDst;

			Create(GraphicsManager::Instance().GetRenderContext(), createInfo);
			const u64 textureSize = width * height * STBI_rgb_alpha;
			Upload(data, (int)textureSize);

			stbi_image_free(data);
		}

		void RHI_Texture::LoadFromData(Byte* data, u32 width, u32 height, u32 depth, u32 channels)
		{
			const u64 size_in_bytes = width * height * depth * channels;
			if (data == nullptr || size_in_bytes == 0)
			{
				return;
			}

			RHI_TextureCreateInfo createInfo = { };
			createInfo.TextureType = TextureType::Tex2D;
			createInfo.Width = width;
			createInfo.Height = height;
			createInfo.Depth = depth;
			createInfo.Format = PixelFormat::R8G8B8A8_UNorm;
			createInfo.ImageUsage = ImageUsageFlagsBits::Sampled | ImageUsageFlagsBits::TransferDst;

			Create(GraphicsManager::Instance().GetRenderContext(), createInfo);
			Upload(data, static_cast<int>(size_in_bytes));
		}
	}
}