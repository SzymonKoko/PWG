#ifndef SRC_SCENE_ISCENE_H_
#define SRC_SCENE_ISCENE_H_

#include <memory>

namespace pwg
{
	class IScene
	{
	public:
		virtual ~IScene() = default;

		virtual void Update(const float& dt) = 0;
		virtual void Draw() = 0;

	protected:

	};
}

#endif // !SRC_SCENE_ISCENE_H_
