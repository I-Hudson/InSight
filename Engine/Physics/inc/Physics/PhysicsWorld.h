#pragma once

#include "Physics/Defines.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Insight
{
    namespace Physics
    {
        class IS_PHYSICS PhysicsWorld
        {
        public:
            PhysicsWorld();
            ~PhysicsWorld();

            void Update(const float deltaTime);

        private:
            JPH::PhysicsSystem m_physicsSystem;
            JPH::JobSystemThreadPool m_jobSystem;
            JPH::TempAllocatorMalloc m_tempAllocatorMalloc;
            std::vector<JPH::BodyID> m_bodyIds;
        };
    }
}