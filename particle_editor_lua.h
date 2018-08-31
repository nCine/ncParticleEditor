#ifndef CLASS_LUALOADER
#define CLASS_LUALOADER

#include "IAppEventHandler.h"
#include "IInputEventHandler.h"
#include "nctl/UniquePtr.h"
#include "nctl/String.h"
#include "nctl/StaticArray.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Rect.h"
#include "ParticleAffectors.h"
#include "ParticleInitializer.h"
#include "Timer.h"
#include "LuaStateManager.h"

namespace ncine {

class Texture;
class ParticleSystem;

}

namespace nc = ncine;

/// The particle editor loader/saver
class LuaLoader :
	public nc::IAppEventHandler,
	public nc::IInputEventHandler
{
  public:
	static const unsigned int MaxFilenameLength = 64;

	struct State
	{
		struct ColorStep
		{
			float age;
			nc::Colorf color;
		};

		struct SizeStep
		{
			float age;
			float scale;
		};

		struct RotationStep
		{
			float age;
			float angle;
		};

		struct PositionStep
		{
			float age;
			nc::Vector2f position;
		};

		struct VelocityStep
		{
			float age;
			nc::Vector2f velocity;
		};

		struct ParticleSystem
		{
			nctl::String name = nctl::String(MaxFilenameLength);
			int numParticles;
			nctl::String textureName = nctl::String(MaxFilenameLength);
			nc::Recti texRect;
			nc::Vector2f position;
			bool inLocalSpace;
			bool active;
			nctl::Array<ColorStep> colorSteps;
			float sizeStepBaseScale;
			nctl::Array<SizeStep> sizeSteps;
			nctl::Array<RotationStep> rotationSteps;
			nctl::Array<PositionStep> positionSteps;
			nctl::Array<VelocityStep> velocitySteps;
			nc::ParticleInitializer init;
			float emitDelay;
		};

		nc::Vector2f normalizedAbsPosition;
		nctl::Array<ParticleSystem> systems;
	};

	struct Config
	{
		int width = 0;
		int height = 0;
		bool fullscreen = false;
		unsigned long vboSize = 0;
		unsigned long iboSize = 0;
		bool batching = false;
		bool culling = false;
	};

	LuaLoader();
	inline bool configLoaded() const { return configLoaded_; }
	inline const Config &config() const { return config_; }
	inline Config &config() { return config_; }
	bool loadConfig(const char *filename);
	bool saveConfig(const char *filename);
	bool load(const char *filename, State &state);
	void save(const char *filename, const State &state);

  private:
	nc::LuaStateManager luaState_;
	Config config_;
	bool configLoaded_;
};

#endif
