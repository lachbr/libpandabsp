#ifndef WINDING_H__
#define WINDING_H__
#include "cmdlib.h" //--vluzacn

#if _MSC_VER >= 1000
#pragma once
#endif

#include "mathtypes.h"
#include "win32fix.h"
#include "mathlib.h"
#include "bspfile.h"
#include "boundingbox.h"

#define MAX_POINTS_ON_WINDING 128
// TODO: FIX THIS STUPID SHIT (MAX_POINTS_ON_WINDING)

#define BASE_WINDING_DISTANCE 9000

#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS		-2

class _BSPEXPORT Winding
{
public:
        // General Functions
        void            Print() const;
        void            getPlane( dplane_t& plane ) const;
        void            getPlane( vec3_t& normal, vec_t& dist ) const;
        vec_t           getArea() const;
        vec_t           getAreaAndBalancePoint( vec3_t &center ) const;
        void            getBounds( BSPBoundingBox& bounds ) const;
        void            getBounds( vec3_t& mins, vec3_t& maxs ) const;
        void            getCenter( vec3_t& center ) const;
        Winding*        Copy() const;
        void            Check(
                vec_t epsilon = ON_EPSILON
        ) const;  // Developer check for validity
        bool            Valid() const;  // Runtime/user/normal check for validity
        void            addPoint( const vec3_t newpoint );
        void            insertPoint( const vec3_t newpoint, const unsigned int offset );

        // Specialized Functions
        void            RemoveColinearPoints(
                vec_t epsilon = ON_EPSILON
        );
        bool            Clip( const dplane_t& split, bool keepon
                              , vec_t epsilon = ON_EPSILON
        ); // For hlbsp
        void            Clip( const dplane_t& split, Winding** front, Winding** back
                              , vec_t epsilon = ON_EPSILON
        );
        void            Clip( const vec3_t normal, const vec_t dist, Winding** front, Winding** back
                              , vec_t epsilon = ON_EPSILON
        );
        bool            Chop( const vec3_t normal, const vec_t dist
                              , vec_t epsilon = ON_EPSILON
        );
        void            Divide( const dplane_t& split, Winding** front, Winding** back
                                , vec_t epsilon = ON_EPSILON
        );
        int             WindingOnPlaneSide( const vec3_t normal, const vec_t dist
                                            , vec_t epsilon = ON_EPSILON
        );
        void			CopyPoints( vec3_t *points, int &numpoints );

        void			initFromPoints( vec3_t *points, UINT32 numpoints );
        void			Reset( void );	// Resets the structure

protected:
        void            resize( UINT32 newsize );

public:
        // Construction
        Winding();										// Do nothing :)
        Winding( vec3_t *points, UINT32 numpoints );		// Create from raw points
        Winding( const dface_t& face
                 , bspdata_t *data
                 , vec_t epsilon = ON_EPSILON
        );
        Winding( const dplane_t& face );
        Winding( const vec3_t normal, const vec_t dist );
        Winding( UINT32 points );
        Winding( const Winding& other );
        virtual ~Winding();
        Winding& operator=( const Winding& other );

        // Misc
private:
        void initFromPlane( const vec3_t normal, const vec_t dist );

public:
        // Data
        UINT32  m_NumPoints;
        vec3_t* m_Points;
protected:
        UINT32  m_MaxPoints;
};

#endif