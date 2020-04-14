#include "toondna.h"

LColorf CToonDNA::body_colors[CToonDNA::BODYCOLOR_COUNT] =
{
	LColorf( 1.0, 1.0, 1.0, 1.0 ),
	LColorf( 0.96875, 0.691406, 0.699219, 1),
	LColorf( 0.933594, 0.265625, 0.28125, 1 ),
	LColorf( 0.863281, 0.40625, 0.417969, 1 ),
	LColorf( 0.710938, 0.234375, 0.4375, 1 ),
	LColorf( 0.570312, 0.449219, 0.164062, 1 ),
	LColorf( 0.640625, 0.355469, 0.269531, 1 ),
	LColorf( 0.996094, 0.695312, 0.511719, 1 ),
	LColorf( 0.832031, 0.5, 0.296875, 1 ),
	LColorf( 0.992188, 0.480469, 0.167969, 1 ),
	LColorf( 0.996094, 0.898438, 0.320312, 1 ),
	LColorf( 0.996094, 0.957031, 0.597656, 1 ),
	LColorf( 0.855469, 0.933594, 0.492188, 1 ),
	LColorf( 0.550781, 0.824219, 0.324219, 1 ),
	LColorf( 0.242188, 0.742188, 0.515625, 1 ),
	LColorf( 0.304688, 0.96875, 0.402344, 1 ),
	LColorf( 0.433594, 0.90625, 0.835938, 1 ),
	LColorf( 0.347656, 0.820312, 0.953125, 1 ),
	LColorf( 0.558594, 0.589844, 0.875, 1 ),
	LColorf( 0.285156, 0.328125, 0.726562, 1 ),
	LColorf( 0.460938, 0.378906, 0.824219, 1 ),
	LColorf( 0.546875, 0.28125, 0.75, 1 ),
	LColorf( 0.726562, 0.472656, 0.859375, 1 ),
	LColorf( 0.898438, 0.617188, 0.90625, 1 ),
	LColorf( 0.7, 0.7, 0.8, 1 ),
	LColorf( 0.3, 0.3, 0.35, 1 ),
};

LColorf CToonDNA::clothes_colors[CToonDNA::CLOTHESCOLOR_COUNT] =
{
	LColorf( 0.933594, 0.265625, 0.28125, 1.0 ),
	LColorf( 0.863281, 0.40625, 0.417969, 1.0 ),
	LColorf( 0.710938, 0.234375, 0.4375, 1.0 ),
	LColorf( 0.992188, 0.480469, 0.167969, 1.0 ),
	LColorf( 0.996094, 0.898438, 0.320312, 1.0 ),
	LColorf( 0.550781, 0.824219, 0.324219, 1.0 ),
	LColorf( 0.242188, 0.742188, 0.515625, 1.0 ),
	LColorf( 0.433594, 0.90625, 0.835938, 1.0 ),
	LColorf( 0.347656, 0.820312, 0.953125, 1.0 ),
	LColorf( 0.191406, 0.5625, 0.773438, 1.0 ),
	LColorf( 0.285156, 0.328125, 0.726562, 1.0 ),
	LColorf( 0.460938, 0.378906, 0.824219, 1.0 ),
	LColorf( 0.546875, 0.28125, 0.75, 1.0 ),
	LColorf( 0.570312, 0.449219, 0.164062, 1.0 ),
	LColorf( 0.640625, 0.355469, 0.269531, 1.0 ),
	LColorf( 0.996094, 0.695312, 0.511719, 1.0 ),
	LColorf( 0.832031, 0.5, 0.296875, 1.0 ),
	LColorf( 0.992188, 0.480469, 0.167969, 1.0 ),
	LColorf( 0.550781, 0.824219, 0.324219, 1.0 ),
	LColorf( 0.433594, 0.90625, 0.835938, 1.0 ),
	LColorf( 0.347656, 0.820312, 0.953125, 1.0 ),
	LColorf( 0.96875, 0.691406, 0.699219, 1.0 ),
	LColorf( 0.996094, 0.957031, 0.597656, 1.0 ),
	LColorf( 0.855469, 0.933594, 0.492188, 1.0 ),
	LColorf( 0.558594, 0.589844, 0.875, 1.0 ),
	LColorf( 0.726562, 0.472656, 0.859375, 1.0 ),
	LColorf( 0.898438, 0.617188, 0.90625, 1.0 ),
	LColorf( 1.0, 1.0, 1.0, 1.0 ),
	LColorf( 0.0, 0.2, 0.956862, 1.0 ),
	LColorf( 0.972549, 0.094117, 0.094117, 1.0 ),
	LColorf( 0.447058, 0.0, 0.90196, 1.0 ),
};

const char *CToonDNA::model_for_legs( int legs )
{
	switch ( legs )
	{
	case LEGS_SHORT:
		return "dgs_shorts_legs";
	case LEGS_MEDIUM:
		return "dgm_shorts_legs";
	case LEGS_TALL:
		return "dgl_shorts_legs";
	default:
		return 0;
	}
}

const char *CToonDNA::model_for_gender_torso( int gender, int torso )
{
	switch ( gender )
	{
	case GENDER_BOY:
		{
			switch ( torso )
			{
			case TORSO_SHORT:
				return "dgs_shorts_torso";
			case TORSO_MEDIUM:
				return "dgm_shorts_torso";
			case TORSO_TALL:
				return "dgl_shorts_torso";
			default:
				return 0;
			}
			break;
		}
	case GENDER_GIRL:
		{
			switch ( torso )
			{
			case TORSO_SHORT:
				return "dgs_skirt_torso";
			case TORSO_MEDIUM:
				return "dgm_skirt_torso";
			case TORSO_TALL:
				return "dgl_skirt_torso";
			default:
				return 0;
			}
			break;
		}
	default:
		return 0;
	}
}

const char *CToonDNA::model_for_animal_head( int animal, int head )
{
	switch ( animal )
	{
	case ANIMAL_DOG:
		{
			switch ( head )
			{
			case HEAD_LONGSHORT:
				return "dgs_shorts_head";
			case HEAD_SHORTSHORT:
				return "dgm_skirt_head";
			case HEAD_SHORTLONG:
				return "dgm_shorts_head";
			case HEAD_LONGLONG:
				return "dgl_shorts_head";
			default:
				return 0;
			}
			break;
		}
	case ANIMAL_CAT:
		return "cat-heads";
	case ANIMAL_DUCK:
		return "duck-heads";
	case ANIMAL_RABBIT:
		return "rabbit-heads";
	case ANIMAL_MOUSE:
		return "mouse-heads";
	case ANIMAL_HORSE:
		return "horse-heads";
	case ANIMAL_MONKEY:
		return "monkey-heads";
	case ANIMAL_BEAR:
		return "bear-heads";
	case ANIMAL_PIG:
		return "pig-heads";
	default:
		return 0;
	}
}

bool CToonDNA::is_head_animated( int animal )
{
	switch ( animal )
	{
	case ANIMAL_DOG:
		return true;
	default:
		return false;
	}
}

bool CToonDNA::has_eyelashes( int gender )
{
	switch ( gender )
	{
	case GENDER_GIRL:
		return true;
	default:
		return false;
	}
}

float CToonDNA::scale_for_animal( int animal )
{
	switch ( animal )
	{
	case ANIMAL_DOG:
		return 0.85f;
	case ANIMAL_CAT:
		return 0.73f;
	case ANIMAL_DUCK:
		return 0.66f;
	case ANIMAL_RABBIT:
		return 0.74f;
	case ANIMAL_MOUSE:
		return 0.6f;
	case ANIMAL_HORSE:
		return 0.85f;
	case ANIMAL_MONKEY:
		return 0.68f;
	case ANIMAL_BEAR:
		return 0.85f;
	case ANIMAL_PIG:
		return 0.77f;
	default:
		return 1.0f;
	}
}

void CToonDNA::read_datagram( DatagramIterator &dgi )
{
	gender = dgi.get_uint8();
	animal = dgi.get_uint8();
	head = dgi.get_uint8();
	headcolor = dgi.get_uint8();
	torso = dgi.get_uint8();
	torsocolor = dgi.get_uint8();
	legs = dgi.get_uint8();
	legscolor = dgi.get_uint8();
	top = dgi.get_uint8();
	topcolor = dgi.get_uint8();
	sleeve = dgi.get_uint8();
	sleevecolor = dgi.get_uint8();
	bottom = dgi.get_uint8();
	bottomcolor = dgi.get_uint8();
	glovecolor = dgi.get_uint8();
}

void CToonDNA::write_datagram( Datagram &dg )
{
	dg.add_uint8( gender );
	dg.add_uint8( animal );
	dg.add_uint8( head );
	dg.add_uint8( headcolor );
	dg.add_uint8( torso );
	dg.add_uint8( torsocolor );
	dg.add_uint8( legs );
	dg.add_uint8( legscolor );
	dg.add_uint8( top );
	dg.add_uint8( topcolor );
	dg.add_uint8( sleeve );
	dg.add_uint8( sleevecolor );
	dg.add_uint8( bottom );
	dg.add_uint8( bottomcolor );
	dg.add_uint8( glovecolor );
}
