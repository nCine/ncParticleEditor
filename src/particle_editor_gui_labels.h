#ifndef GUI_LABELS
#define GUI_LABELS

#ifdef WITH_FONTAWESOME
#include "IconsFontAwesome5.h"
#endif

#define TEXT_LOADINGERROR "Loading error!"
#define TEXT_FILEEXISTS "File exists!"
#define TEXT_OK "Ok"
#define TEXT_CANCEL "Cancel"

#define TEXT_MENU_FILE_NEW "New"
#define TEXT_MENU_FILE_OPEN "Open"
#define TEXT_MENU_FILE_OPENRECENT "Open Recent"
#define TEXT_MENU_FILE_CLEARRECENTS "Clear Recents"
#define TEXT_MENU_FILE_OPENBUNDLED "Open Bundled"
#define TEXT_MENU_FILE_SAVE "Save"
#define TEXT_MENU_FILE_SAVEAS "Save as..."
#define TEXT_MENU_FILE_QUIT "Quit"
#define TEXT_MENU_VIEW_MAIN "Main"
#define TEXT_MENU_VIEW_CONFIG "Config"
#define TEXT_MENU_VIEW_LOG "Log"
#define TEXT_MENU_ABOUT "About"

#define TEXT_HEADER_BACKGROUND "Background"
#define TEXT_HEADER_TEXTURES "Textures"
#define TEXT_HEADER_MANAGE_SYSTEMS "Manage Systems"
#define TEXT_HEADER_PARTICLE_SYSTEM "Particle System"
#define TEXT_HEADER_COLOR_AFFECTOR "Color Affector"
#define TEXT_HEADER_SIZE_AFFECTOR "Size Affector"
#define TEXT_HEADER_ROTATION_AFFECTOR "Rotation Affector"
#define TEXT_HEADER_POSITION_AFFECTOR "Position Affector"
#define TEXT_HEADER_VELOCITY_AFFECTOR "Velocity Affector"
#define TEXT_HEADER_EMISSION "Emission"

#define TEXT_LOAD "Load"
#define TEXT_DELETE "Delete"
#define TEXT_CLONE "Clone"
#define TEXT_RESET "Reset"
#define TEXT_ASSIGN "Assign"
#define TEXT_RETRIEVE "Retrieve"
#define TEXT_CLEAR "Clear"
#define TEXT_APPLY "Apply"
#define TEXT_CURRENT "Current"
#define TEXT_LOCK "Lock"

#define TEXT_EMIT "Emit"
#define TEXT_KILL "Kill"

#define TEXT_ADD "Add"
#define TEXT_REMOVE "Remove"
#define TEXT_REMOVEALL "Remove All"

#ifndef WITH_FONTAWESOME
namespace Labels {
	static const char *HELP_MARKER = "(?)";

	static const char *LoadingError = TEXT_LOADINGERROR;
	static const char *FileExists = TEXT_FILEEXISTS;
	static const char *Ok = TEXT_OK;
	static const char *Cancel = TEXT_CANCEL;

	static const char *New = TEXT_MENU_FILE_NEW;
	static const char *Open = TEXT_MENU_FILE_OPEN;
	static const char *OpenRecent = TEXT_MENU_FILE_OPENRECENT;
	static const char *ClearRecents = TEXT_MENU_FILE_CLEARRECENTS;
	static const char *OpenBundled = TEXT_MENU_FILE_OPENBUNDLED;
	static const char *Save = TEXT_MENU_FILE_SAVE;
	static const char *SaveAs = TEXT_MENU_FILE_SAVEAS;
	static const char *Quit = TEXT_MENU_FILE_QUIT;
	static const char *Main = TEXT_MENU_VIEW_MAIN;
	static const char *Config = TEXT_MENU_VIEW_CONFIG;
	static const char *Log = TEXT_MENU_VIEW_LOG;
	static const char *About = TEXT_MENU_ABOUT;

	static const char *Background = TEXT_HEADER_BACKGROUND;
	static const char *Textures = TEXT_HEADER_TEXTURES;
	static const char *ManageSystems = TEXT_HEADER_MANAGE_SYSTEMS;
	static const char *ParticleSystem = TEXT_HEADER_PARTICLE_SYSTEM;
	static const char *ColorAffector = TEXT_HEADER_COLOR_AFFECTOR;
	static const char *SizeAffector = TEXT_HEADER_SIZE_AFFECTOR;
	static const char *RotationAffector = TEXT_HEADER_ROTATION_AFFECTOR;
	static const char *PositionAffector = TEXT_HEADER_POSITION_AFFECTOR;
	static const char *VelocityAffector = TEXT_HEADER_VELOCITY_AFFECTOR;
	static const char *Emission = TEXT_HEADER_EMISSION;

	static const char *Load = TEXT_LOAD;
	static const char *Delete = TEXT_DELETE;
	static const char *Clone = TEXT_CLONE;
	static const char *Reset = TEXT_RESET;
	static const char *Assign = TEXT_ASSIGN;
	static const char *Retrieve = TEXT_RETRIEVE;
	static const char *Clear = TEXT_CLEAR;
	static const char *Apply = TEXT_APPLY;
	static const char *Current = TEXT_CURRENT;
	static const char *Lock = TEXT_LOCK;

	static const char *Emit = TEXT_EMIT;
	static const char *Kill = TEXT_KILL;

	static const char *Add = TEXT_ADD;
	static const char *Remove = TEXT_REMOVE;
	static const char *RemoveAll = TEXT_REMOVEALL;
}
#else
#define FA5_SPACING " "

namespace Labels {
	static const char *HELP_MARKER = "(" ICON_FA_QUESTION ")";

	static const char *LoadingError = ICON_FA_EXCLAMATION_TRIANGLE FA5_SPACING TEXT_LOADINGERROR;
	static const char *FileExists = ICON_FA_EXCLAMATION_TRIANGLE FA5_SPACING TEXT_FILEEXISTS;
	static const char *Ok = ICON_FA_CHECK FA5_SPACING TEXT_OK;
	static const char *Cancel = ICON_FA_TIMES FA5_SPACING TEXT_CANCEL;

	static const char *New = ICON_FA_FILE FA5_SPACING TEXT_MENU_FILE_NEW;
	static const char *Open = ICON_FA_FOLDER_OPEN FA5_SPACING TEXT_MENU_FILE_OPEN;
	static const char *OpenRecent = ICON_FA_FOLDER_OPEN FA5_SPACING TEXT_MENU_FILE_OPENRECENT;
	static const char *ClearRecents = ICON_FA_BACKSPACE FA5_SPACING TEXT_MENU_FILE_CLEARRECENTS;
	static const char *OpenBundled = ICON_FA_FOLDER_OPEN FA5_SPACING TEXT_MENU_FILE_OPENBUNDLED;
	static const char *Save = ICON_FA_SAVE FA5_SPACING TEXT_MENU_FILE_SAVE;
	static const char *SaveAs = ICON_FA_SAVE FA5_SPACING TEXT_MENU_FILE_SAVEAS;
	static const char *Quit = ICON_FA_POWER_OFF FA5_SPACING TEXT_MENU_FILE_QUIT;
	static const char *Main = ICON_FA_WINDOW_MAXIMIZE FA5_SPACING TEXT_MENU_VIEW_MAIN;
	static const char *Config = ICON_FA_TOOLS FA5_SPACING TEXT_MENU_VIEW_CONFIG;
	static const char *Log = ICON_FA_CLIPBOARD_LIST FA5_SPACING TEXT_MENU_VIEW_LOG;
	static const char *About = ICON_FA_INFO_CIRCLE FA5_SPACING TEXT_MENU_ABOUT;

	static const char *Background = ICON_FA_PALETTE FA5_SPACING TEXT_HEADER_BACKGROUND;
	static const char *Textures = ICON_FA_IMAGES FA5_SPACING TEXT_HEADER_TEXTURES;
	static const char *ManageSystems = ICON_FA_SLIDERS_H FA5_SPACING TEXT_HEADER_MANAGE_SYSTEMS;
	static const char *ParticleSystem = ICON_FA_SLIDERS_H FA5_SPACING TEXT_HEADER_PARTICLE_SYSTEM;
	static const char *ColorAffector = ICON_FA_TINT FA5_SPACING TEXT_HEADER_COLOR_AFFECTOR;
	static const char *SizeAffector = ICON_FA_WEIGHT_HANGING FA5_SPACING TEXT_HEADER_SIZE_AFFECTOR;
	static const char *RotationAffector = ICON_FA_UNDO FA5_SPACING TEXT_HEADER_ROTATION_AFFECTOR;
	static const char *PositionAffector = ICON_FA_MAP_MARKER_ALT FA5_SPACING TEXT_HEADER_POSITION_AFFECTOR;
	static const char *VelocityAffector = ICON_FA_TACHOMETER_ALT FA5_SPACING TEXT_HEADER_VELOCITY_AFFECTOR;
	static const char *Emission = ICON_FA_SLIDERS_H FA5_SPACING TEXT_HEADER_EMISSION;

	static const char *Load = ICON_FA_FILE_UPLOAD FA5_SPACING TEXT_LOAD;
	static const char *Delete = ICON_FA_TRASH FA5_SPACING TEXT_DELETE;
	static const char *Clone = ICON_FA_CLONE FA5_SPACING TEXT_CLONE;
	static const char *Reset = ICON_FA_BACKSPACE FA5_SPACING TEXT_RESET;
	static const char *Assign = ICON_FA_CHECK_CIRCLE FA5_SPACING TEXT_ASSIGN;
	static const char *Retrieve = ICON_FA_SYNC FA5_SPACING TEXT_RETRIEVE;
	static const char *Clear = ICON_FA_BACKSPACE FA5_SPACING TEXT_CLEAR;
	static const char *Apply = ICON_FA_CHECK_CIRCLE FA5_SPACING TEXT_APPLY;
	static const char *Current = ICON_FA_SYNC FA5_SPACING TEXT_CURRENT;
	static const char *Lock = ICON_FA_LOCK;

	static const char *Emit = ICON_FA_FIRE FA5_SPACING TEXT_EMIT;
	static const char *Kill = ICON_FA_SKULL FA5_SPACING TEXT_KILL;

	static const char *Add = ICON_FA_PLUS FA5_SPACING TEXT_ADD;
	static const char *Remove = ICON_FA_MINUS FA5_SPACING TEXT_REMOVE;
	static const char *RemoveAll = ICON_FA_TRASH FA5_SPACING TEXT_REMOVEALL;
}
#endif

#endif
