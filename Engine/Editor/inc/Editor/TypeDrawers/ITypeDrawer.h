#pragma once

#include "Editor/Defines.h"
#include "Editor/TypeDrawers/TypeDrawerRegister.h"

#include <Reflect.h>

namespace Insight
{
    namespace Editor
    {
        class IS_EDITOR ITypeDrawer
        {
        public:
            virtual void Draw(void* data) const = 0;
        };

        template<typename T>
        struct RegisterTypeDrawer
        {
            RegisterTypeDrawer() { }
            RegisterTypeDrawer(const char* typeName)
            {
                TypeDrawerRegister::RegisterStaticDrawer<T>(typeName);
            }
        };
#define IS_REGISTER_TYPE_DRAWER_DEC(TypeDrawer) static RegisterTypeDrawer<TypeDrawer> s_registerTypeDrawer
#define IS_REGISTER_TYPE_DRAWER_DEF(TypeName, TypeDrawer) RegisterTypeDrawer<TypeDrawer> TypeDrawer::s_registerTypeDrawer = RegisterTypeDrawer<TypeDrawer>(TypeName)
    }
}