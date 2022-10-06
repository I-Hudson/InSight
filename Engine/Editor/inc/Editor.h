#pragma once

#include "Runtime/Engine.h"

namespace Insight
{
	namespace Editor
	{
		class Editor : public Insight::App::Engine
		{
		public:

			virtual void OnInit() override;
			virtual void OnDestroy() override;

		private:

		};
	}
}