#pragma once

#include "Core/ReferencePtr.h"
#include "Asset/Asset.h"

namespace Insight
{
    namespace Runtime
    {
        class IAssetImporter
        {
        public:
            IAssetImporter() = delete;
            IAssetImporter(std::vector<const char*> validFileExtensions);
            virtual ~IAssetImporter();

            bool IsValidImporterForFileExtension(const char* fileExtension) const;
            virtual Ref<Asset> Import(const AssetInfo* assetInfo, const std::string_view path) const = 0;

        private:
            std::vector<const char*> m_validFileExtensions;
        };
    }
}