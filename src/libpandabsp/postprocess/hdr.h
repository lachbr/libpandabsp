/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file hdr.h
 * @author Brian Lach
 * @date July 22, 2019
 */

#include "postprocess/postprocess_pass.h"
#include "postprocess/postprocess_effect.h"

#include <geom.h>
#include <renderState.h>
#include <callbackObject.h>
#include <occlusionQueryContext.h>
#include <pta_float.h>
#include <configVariableBool.h>

extern ConfigVariableBool hdr_auto_exposure;

struct hdrbucket_t
{
	float luminance_min;
	float luminance_max;

	int pixels;

	PT( OcclusionQueryContext ) ctx;
};

static const int HDR_NUM_BUCKETS = 16;

class EXPCL_PANDABSP HDRPass : public PostProcessPass
{
	DECLARE_CLASS( HDRPass, PostProcessPass );

PUBLISHED:
	HDRPass( PostProcess *pp );

	virtual void setup_quad();
	virtual void setup_region();

	virtual void setup();

	virtual void update();

	INLINE void set_exposure_output( PTA_float output )
	{
		_exposure_output = output;
	}

	/**
	 * Returns the calculated exposure adjustment.
	 */
	INLINE float get_exposure() const
	{
		return _exposure;
	}

public:
	void draw( CallbackData *data );

private:
	float find_location_of_percent_bright_pixels(
		float percent_bright_pixels, float same_bin_snap,
		int total_pixel_count );

private:
	PTA_stdfloat _luminance_min_max;
	CPT( Geom ) _hdr_quad_geom;
	CPT( RenderState ) _hdr_geom_state;

	// Calculated exposure level based on histogram
	float _exposure;
	PTA_float _exposure_output;

	int _current_bucket;
	hdrbucket_t _buckets[HDR_NUM_BUCKETS];
};

class EXPCL_PANDABSP HDREffect : public PostProcessEffect
{
PUBLISHED:
	HDREffect( PostProcess *pp );

	virtual Texture *get_final_texture();

	INLINE HDRPass *get_hdr_pass() const
	{
		return (HDRPass *)_passes.get_data( 0 ).p();
	}
};
