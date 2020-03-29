#include "qrad.h"

static dplane_t backplanes[MAX_MAP_PLANES];

dleaf_t*		PointInLeaf_Worst_r( int nodenum, const vec3_t point )
{
        vec_t			dist;
        dnode_t*		node;
        dplane_t*		plane;

        while ( nodenum >= 0 )
        {
                node = &g_bspdata->dnodes[nodenum];
                plane = &g_bspdata->dplanes[node->planenum];
                dist = DotProduct( point, plane->normal ) - plane->dist;
                if ( dist > HUNT_WALL_EPSILON )
                {
                        nodenum = node->children[0];
                }
                else if ( dist < -HUNT_WALL_EPSILON )
                {
                        nodenum = node->children[1];
                }
                else
                {
                        dleaf_t* result[2];
                        result[0] = PointInLeaf_Worst_r( node->children[0], point );
                        result[1] = PointInLeaf_Worst_r( node->children[1], point );
                        if ( result[0] == g_bspdata->dleafs || result[0]->contents == CONTENTS_SOLID )
                                return result[0];
                        if ( result[1] == g_bspdata->dleafs || result[1]->contents == CONTENTS_SOLID )
                                return result[1];
                        if ( result[0]->contents == CONTENTS_SKY )
                                return result[0];
                        if ( result[1]->contents == CONTENTS_SKY )
                                return result[1];
                        if ( result[0]->contents == result[1]->contents )
                                return result[0];
                        return g_bspdata->dleafs;
                }
        }

        return &g_bspdata->dleafs[-nodenum - 1];
}
dleaf_t*		PointInLeaf_Worst( const vec3_t point )
{
        return PointInLeaf_Worst_r( 0, point );
}
dleaf_t*	PointInLeafD( const vec3_t &point )
{
	float pointf[3];
	VectorCopy( point, pointf );
	return PointInLeaf( pointf );
}
dleaf_t*        PointInLeaf( const LVector3 &vpoint )
{
        float point[3];
        VectorCopy( vpoint, point );

        return PointInLeaf( point );
}
dleaf_t*        PointInLeaf( const float * point )
{
        int             nodenum;
        vec_t           dist;
        dnode_t*        node;
        dplane_t*       plane;

        nodenum = 0;
        while ( nodenum >= 0 )
        {
                node = &g_bspdata->dnodes[nodenum];
                plane = &g_bspdata->dplanes[node->planenum];
                dist = DotProduct( point, plane->normal ) - plane->dist;
                if ( dist >= 0.0 )
                {
                        nodenum = node->children[0];
                }
                else
                {
                        nodenum = node->children[1];
                }
        }

        return &g_bspdata->dleafs[-nodenum - 1];
}

/*
* ==============
* PatchPlaneDist
* Fixes up patch planes for brush models with an origin brush
* ==============
*/
vec_t           PatchPlaneDist( const patch_t* const patch )
{
        const dplane_t* plane = getPlaneFromFaceNumber( patch->facenum );

        return plane->dist + DotProduct( g_face_offset[patch->facenum], plane->normal );
}

void            MakeBackplanes()
{
        int             i;

        for ( i = 0; i < g_bspdata->numplanes; i++ )
        {
                backplanes[i].dist = -g_bspdata->dplanes[i].dist;
                VectorSubtract( vec3_origin, g_bspdata->dplanes[i].normal, backplanes[i].normal );
        }
}

const dplane_t* getPlaneFromFace( const dface_t* const face )
{
        if ( !face )
        {
                Error( "getPlaneFromFace() face was NULL\n" );
        }

        if ( face->side )
        {
                return &backplanes[face->planenum];
        }
        else
        {
                return &g_bspdata->dplanes[face->planenum];
        }
}

const dplane_t* getPlaneFromFaceNumber( const unsigned int faceNumber )
{
        dface_t*        face = &g_bspdata->dfaces[faceNumber];

        if ( face->side )
        {
                return &backplanes[face->planenum];
        }
        else
        {
                return &g_bspdata->dplanes[face->planenum];
        }
}

// Returns plane adjusted for face offset (for origin brushes, primarily used in the opaque code)
void getAdjustedPlaneFromFaceNumber( unsigned int faceNumber, dplane_t* plane )
{
        dface_t*        face = &g_bspdata->dfaces[faceNumber];
        const vec_t*    face_offset = g_face_offset[faceNumber];

        plane->type = (planetypes)0;

        if ( face->side )
        {
                vec_t dist;

                VectorCopy( backplanes[face->planenum].normal, plane->normal );
                dist = DotProduct( plane->normal, face_offset );
                plane->dist = backplanes[face->planenum].dist + dist;
        }
        else
        {
                vec_t dist;

                VectorCopy( g_bspdata->dplanes[face->planenum].normal, plane->normal );
                dist = DotProduct( plane->normal, face_offset );
                plane->dist = g_bspdata->dplanes[face->planenum].dist + dist;
        }
}

// Will modify the plane with the new dist
void            TranslatePlane( dplane_t* plane, const vec_t* delta )
{
        plane->dist += DotProduct( plane->normal, delta );
}

// ApplyMatrix: (x y z 1)T -> matrix * (x y z 1)T
void ApplyMatrix( const matrix_t &m, const vec3_t in, vec3_t &out )
{
        int i;

        hlassume( &in[0] != &out[0], assume_first );
        VectorCopy( m.v[3], out );
        for ( i = 0; i < 3; i++ )
        {
                VectorMA( out, in[i], m.v[i], out );
        }
}

// ApplyMatrixOnPlane: (x y z -dist) -> (x y z -dist) * matrix
void ApplyMatrixOnPlane( const matrix_t &m_inverse, const vec3_t in_normal, vec_t in_dist, vec3_t &out_normal, vec_t &out_dist )
// out_normal is not normalized
{
        int i;

        hlassume( &in_normal[0] != &out_normal[0], assume_first );
        for ( i = 0; i < 3; i++ )
        {
                out_normal[i] = DotProduct( in_normal, m_inverse.v[i] );
        }
        out_dist = -( DotProduct( in_normal, m_inverse.v[3] ) - in_dist );
}

void MultiplyMatrix( const matrix_t &m_left, const matrix_t &m_right, matrix_t &m )
// The following two processes are equivalent:
//  1) ApplyMatrix (m1, v_in, v_temp), ApplyMatrix (m2, v_temp, v_out);
//  2) MultiplyMatrix (m2, m1, m), ApplyMatrix (m, v_in, v_out);
{
        int i, j;
        const vec_t lastrow[4] = { 0, 0, 0, 1 };

        hlassume( &m != &m_left && &m != &m_right, assume_first );
        for ( i = 0; i < 3; i++ )
        {
                for ( j = 0; j < 4; j++ )
                {
                        m.v[j][i] = m_left.v[0][i] * m_right.v[j][0]
                                + m_left.v[1][i] * m_right.v[j][1]
                                + m_left.v[2][i] * m_right.v[j][2]
                                + m_left.v[3][i] * lastrow[j];
                }
        }
}

matrix_t MultiplyMatrix( const matrix_t &m_left, const matrix_t &m_right )
{
        matrix_t m;

        MultiplyMatrix( m_left, m_right, m );
        return m;
}

void MatrixForScale( const vec3_t center, vec_t scale, matrix_t &m )
{
        int i;

        for ( i = 0; i < 3; i++ )
        {
                VectorClear( m.v[i] );
                m.v[i][i] = scale;
        }
        VectorScale( center, 1 - scale, m.v[3] );
}

matrix_t MatrixForScale( const vec3_t center, vec_t scale )
{
        matrix_t m;

        MatrixForScale( center, scale, m );
        return m;
}

vec_t CalcMatrixSign( const matrix_t &m )
{
        vec3_t v;

        CrossProduct( m.v[0], m.v[1], v );
        return DotProduct( v, m.v[2] );
}

void TranslateWorldToTex( int facenum, matrix_t &m )
// without g_face_offset
{
        dface_t *f;
        texinfo_t *ti;
        const dplane_t *fp;
        int i;

        f = &g_bspdata->dfaces[facenum];
        ti = &g_bspdata->texinfo[f->texinfo];
        fp = getPlaneFromFace( f );
        for ( i = 0; i < 3; i++ )
        {
                m.v[i][0] = ti->lightmap_vecs[0][i] * ti->lightmap_scale;
                m.v[i][1] = ti->lightmap_vecs[1][i] * ti->lightmap_scale;
                m.v[i][2] = fp->normal[i];
        }
        m.v[3][0] = ti->lightmap_vecs[0][3] * ti->lightmap_scale;
        m.v[3][1] = ti->lightmap_vecs[1][i] * ti->lightmap_scale;
        m.v[3][2] = -fp->dist;
}

bool InvertMatrix( const matrix_t &m, matrix_t &m_inverse )
{
        double texplanes[2][4];
        double faceplane[4];
        int i;
        double texaxis[2][3];
        double normalaxis[3];
        double det, sqrlen1, sqrlen2, sqrlen3;
        double texorg[3];

        for ( i = 0; i < 4; i++ )
        {
                texplanes[0][i] = m.v[i][0];
                texplanes[1][i] = m.v[i][1];
                faceplane[i] = m.v[i][2];
        }

        sqrlen1 = DotProduct( texplanes[0], texplanes[0] );
        sqrlen2 = DotProduct( texplanes[1], texplanes[1] );
        sqrlen3 = DotProduct( faceplane, faceplane );
        if ( sqrlen1 <= NORMAL_EPSILON * NORMAL_EPSILON || sqrlen2 <= NORMAL_EPSILON * NORMAL_EPSILON || sqrlen3 <= NORMAL_EPSILON * NORMAL_EPSILON )
                // s gradient, t gradient or face normal is too close to 0
        {
                return false;
        }

        CrossProduct( texplanes[0], texplanes[1], normalaxis );
        det = DotProduct( normalaxis, faceplane );
        if ( det * det <= sqrlen1 * sqrlen2 * sqrlen3 * NORMAL_EPSILON * NORMAL_EPSILON )
                // s gradient, t gradient and face normal are coplanar
        {
                return false;
        }
        VectorScale( normalaxis, 1 / det, normalaxis );

        CrossProduct( texplanes[1], faceplane, texaxis[0] );
        VectorScale( texaxis[0], 1 / det, texaxis[0] );

        CrossProduct( faceplane, texplanes[0], texaxis[1] );
        VectorScale( texaxis[1], 1 / det, texaxis[1] );

        VectorScale( normalaxis, -faceplane[3], texorg );
        VectorMA( texorg, -texplanes[0][3], texaxis[0], texorg );
        VectorMA( texorg, -texplanes[1][3], texaxis[1], texorg );

        VectorCopy( texaxis[0], m_inverse.v[0] );
        VectorCopy( texaxis[1], m_inverse.v[1] );
        VectorCopy( normalaxis, m_inverse.v[2] );
        VectorCopy( texorg, m_inverse.v[3] );
        return true;
}