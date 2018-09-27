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
class LuaStateManager;

}

namespace nc = ncine;

/// The particle editor loader/saver
class LuaLoader :
	public nc::IAppEventHandler,
	public nc::IInputEventHandler
{
  public:
	static const unsigned int MaxFilenameLength = 128;

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
			int layer;
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

		struct BackgroundProperties
		{
			nc::Colorf color;
			nctl::String imageName = nctl::String(MaxFilenameLength);
			nc::Vector2f imageNormalizedPosition;
			float imageScale;
			int imageLayer;
			nc::Colorf imageColor;
			nc::Recti imageRect;
		};

		nc::Vector2f normalizedAbsPosition;
		BackgroundProperties background;
		nctl::Array<ParticleSystem> systems;
	};

	struct Config
	{
		int width = 1280;
		int height = 720;
		bool fullscreen = false;
		unsigned long vboSize = 256 * 1024;
		unsigned long iboSize = 32 * 1024;
		bool batching = true;
		bool culling = true;
		unsigned int saveFileMaxSize = 8 * 1024;
		unsigned int logMaxSize = 4 * 1024;

		nctl::String scriptsPath = nctl::String(MaxFilenameLength);
		nctl::String texturesPath = nctl::String(MaxFilenameLength);
		nctl::String backgroundsPath = nctl::String(MaxFilenameLength);

		float maxBackgroundImageScale = 5.0f;
		int maxRenderingLayer = 16;
		int maxNumParticles = 256;
		float systemPositionRange = 200.0f;
		float minParticleScale = 0.0f;
		float maxParticleScale = 2.0f;
		float minParticleAngle = -360.0f;
		float maxParticleAngle = 360.0f;
		float positionRange = 5.0f;
		float velocityRange = 5.0f;
		float maxRandomLife = 5.0f;
		float randomPositionRange = 100.0f;
		float randomVelocityRange = 200.0f;
		float maxDelay = 5.0f;
	};

	LuaLoader() { }
	inline const Config &config() const { return config_; }
	inline Config &config() { return config_; }
	void sanitizeInitValues();
	void sanitizeGuiLimits();
	bool loadConfig(const char *filename);
	bool saveConfig(const char *filename);
	bool load(const char *filename, State &state);
	void save(const char *filename, const State &state);

  private:
	nctl::UniquePtr<nc::LuaStateManager> luaState_;
	Config config_;

	void createNewState();
};

#endif
