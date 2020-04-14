#pragma once

#include "datagram.h"
#include "datagramIterator.h"
#include "luse.h"

class CToonDNA
{
public:
	enum
	{
		GENDER_BOY,
		GENDER_GIRL,
		GENDER_FLUID,
	};

	enum
	{
		ANIMAL_DOG,
		ANIMAL_CAT,
		ANIMAL_DUCK,
		ANIMAL_RABBIT,
		ANIMAL_MOUSE,
		ANIMAL_HORSE,
		ANIMAL_MONKEY,
		ANIMAL_BEAR,
		ANIMAL_PIG,
	};

	enum
	{
		HEAD_LONGSHORT,
		HEAD_SHORTSHORT,
		HEAD_SHORTLONG,
		HEAD_LONGLONG,
	};

	enum
	{
		TORSO_SHORT,
		TORSO_MEDIUM,
		TORSO_TALL,
	};

	enum
	{
		LEGS_SHORT,
		LEGS_MEDIUM,
		LEGS_TALL,
	};

	enum
	{
		BODYCOLOR_WHITE,
		BODYCOLOR_PEACH,
		BODYCOLOR_BRIGHTRED,
		BODYCOLOR_RED,
		BODYCOLOR_MAROON,
		BODYCOLOR_SIENNA,
		BODYCOLOR_BROWN,
		BODYCOLOR_TAN,
		BODYCOLOR_CORAL,
		BODYCOLOR_ORANGE,
		BODYCOLOR_YELLOW,
		BODYCOLOR_CREAM,
		BODYCOLOR_CITRINE,
		BODYCOLOR_LIMEGREEN,
		BODYCOLOR_GREEN,
		BODYCOLOR_SEAGREEN,
		BODYCOLOR_LIGHTBLUE,
		BODYCOLOR_AQUA,
		BODYCOLOR_BLUE,
		BODYCOLOR_ROYALBLUE,
		BODYCOLOR_SLATEBLUE,
		BODYCOLOR_PERIWINKLE,
		BODYCOLOR_PURPLE,
		BODYCOLOR_LAVENDER,
		BODYCOLOR_PINK,
		BODYCOLOR_GRAY,
		BODYCOLOR_BLACK,

		BODYCOLOR_COUNT,
	};

	// Don't know the names of the colors so...
	enum
	{
		CLOTHESCOLOR_00,
		CLOTHESCOLOR_01,
		CLOTHESCOLOR_02,
		CLOTHESCOLOR_03,
		CLOTHESCOLOR_04,
		CLOTHESCOLOR_05,
		CLOTHESCOLOR_06,
		CLOTHESCOLOR_07,
		CLOTHESCOLOR_08,
		CLOTHESCOLOR_09,
		CLOTHESCOLOR_10,
		CLOTHESCOLOR_11,
		CLOTHESCOLOR_12,
		CLOTHESCOLOR_13,
		CLOTHESCOLOR_14,
		CLOTHESCOLOR_15,
		CLOTHESCOLOR_16,
		CLOTHESCOLOR_17,
		CLOTHESCOLOR_18,
		CLOTHESCOLOR_19,
		CLOTHESCOLOR_20,
		CLOTHESCOLOR_21,
		CLOTHESCOLOR_22,
		CLOTHESCOLOR_23,
		CLOTHESCOLOR_24,
		CLOTHESCOLOR_25,
		CLOTHESCOLOR_26,
		CLOTHESCOLOR_27,
		CLOTHESCOLOR_28,
		CLOTHESCOLOR_29,
		CLOTHESCOLOR_30,
		CLOTHESCOLOR_31,

		CLOTHESCOLOR_COUNT,
	};

	static LColorf body_colors[BODYCOLOR_COUNT];
	static LColorf clothes_colors[CLOTHESCOLOR_COUNT];

	int gender;
	int animal;
	int head;
	int headcolor;
	int torso;
	int torsocolor;
	int legs;
	int legscolor;
	int top;
	int topcolor;
	int sleeve;
	int sleevecolor;
	int bottom;
	int bottomcolor;
	int glovecolor;

	static const char *model_for_legs( int legs );
	static const char *model_for_gender_torso( int gender, int torso );
	static const char *model_for_animal_head( int animal, int head );
	static bool is_head_animated( int animal );
	static float scale_for_animal( int animal );
	static bool has_eyelashes( int gender );

	CToonDNA();

	void write_datagram( Datagram &dg );
	void read_datagram( DatagramIterator &dgi );
};

inline CToonDNA::CToonDNA()
{
	memset( this, 0, sizeof( CToonDNA ) );
	glovecolor = CLOTHESCOLOR_27;
	topcolor = CLOTHESCOLOR_27;
	sleevecolor = CLOTHESCOLOR_27;
	bottomcolor = CLOTHESCOLOR_27;
}