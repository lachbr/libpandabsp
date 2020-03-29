#include "qrad.h"

// =====================================================================================
//  point_in_winding
//      returns whether the point is in the winding (including its edges)
//      the point and all the vertexes of the winding can move freely along the plane's normal without changing the result
// =====================================================================================
bool            point_in_winding(const Winding& w, const dplane_t& plane, const vec_t* const point, vec_t epsilon/* = 0.0*/)
{
	int				numpoints;
	int				x;
	vec3_t			delta;
	vec3_t			normal;
	vec_t			dist;

	numpoints = w.m_NumPoints;

	for (x = 0; x < numpoints; x++)
	{
		VectorSubtract (w.m_Points[(x+ 1) % numpoints], w.m_Points[x], delta);
		CrossProduct (delta, plane.normal, normal);
		dist = DotProduct (point, normal) - DotProduct (w.m_Points[x], normal);

		if (dist < 0.0
			&& (epsilon == 0.0 || dist * dist > epsilon * epsilon * DotProduct (normal, normal)))
		{
			return false;
		}
	}

	return true;
}

// =====================================================================================
//  point_in_winding_noedge
//      assume a ball is created from the point, this function checks whether the ball is entirely inside the winding
//      parameter 'width' : the radius of the ball
//      the point and all the vertexes of the winding can move freely along the plane's normal without changing the result
// =====================================================================================
bool            point_in_winding_noedge(const Winding& w, const dplane_t& plane, const vec_t* const point, vec_t width)
{
	int				numpoints;
	int				x;
	vec3_t			delta;
	vec3_t			normal;
	vec_t			dist;

	numpoints = w.m_NumPoints;

	for (x = 0; x < numpoints; x++)
	{
		VectorSubtract (w.m_Points[(x+ 1) % numpoints], w.m_Points[x], delta);
		CrossProduct (delta, plane.normal, normal);
		dist = DotProduct (point, normal) - DotProduct (w.m_Points[x], normal);

		if (dist < 0.0 || dist * dist <= width * width * DotProduct (normal, normal))
		{
			return false;
		}
	}

	return true;
}

// =====================================================================================
//  snap_to_winding
//      moves the point to the nearest point inside the winding
//      if the point is not on the plane, the distance between the point and the plane is preserved
//      the point and all the vertexes of the winding can move freely along the plane's normal without changing the result
// =====================================================================================
void			snap_to_winding(const Winding& w, const dplane_t& plane, vec_t* const point)
{
	int				numpoints;
	int				x;
	vec_t			*p1, *p2;
	vec3_t			delta;
	vec3_t			normal;
	vec_t			dist;
	vec_t			dot1, dot2, dot;
	vec3_t			bestpoint;
	vec_t			bestdist;
	bool			in;

	numpoints = w.m_NumPoints;

	in = true;
	for (x = 0; x < numpoints; x++)
	{
		p1 = w.m_Points[x];
		p2 = w.m_Points[(x + 1) % numpoints];
		VectorSubtract (p2, p1, delta);
		CrossProduct (delta, plane.normal, normal);
		dist = DotProduct (point, normal) - DotProduct (p1, normal);

		if (dist < 0.0)
		{
			in = false;

			CrossProduct (plane.normal, normal, delta);
			dot = DotProduct (delta, point);
			dot1 = DotProduct (delta, p1);
			dot2 = DotProduct (delta, p2);
			if (dot1 < dot && dot < dot2)
			{
				dist = dist / DotProduct (normal, normal);
				VectorMA (point, -dist, normal, point);
				return;
			}
		}
	}
	if (in)
	{
		return;
	}

	for (x = 0; x < numpoints; x++)
	{
		p1 = w.m_Points[x];
		VectorSubtract (p1, point, delta);
		dist = DotProduct (delta, plane.normal) / DotProduct (plane.normal, plane.normal);
		VectorMA (delta, -dist, plane.normal, delta);
		dot = DotProduct (delta, delta);

		if (x == 0 || dot < bestdist)
		{
			VectorAdd (point, delta, bestpoint);
			bestdist = dot;
		}
	}
	if (numpoints > 0)
	{
		VectorCopy (bestpoint, point);
	}
	return;
}

// =====================================================================================
//  snap_to_winding_noedge
//      first snaps the point into the winding
//      then moves the point towards the inside for at most certain distance until:
//        either 1) the point is not close to any of the edges
//        or     2) the point can not be moved any more
//      returns the maximal distance that the point can be kept away from all the edges
//      in most of the cases, the maximal distance = width; in other cases, the maximal distance < width
// =====================================================================================
vec_t			snap_to_winding_noedge(const Winding& w, const dplane_t& plane, vec_t* const point, vec_t width, vec_t maxmove)
{
	int pass;
	int numplanes;
	dplane_t *planes;
	int x;
	vec3_t v;
	vec_t newwidth;
	vec_t bestwidth;
	vec3_t bestpoint;

	snap_to_winding (w, plane, point);

	planes = (dplane_t *)malloc (w.m_NumPoints * sizeof (dplane_t));
	hlassume (planes != NULL, assume_NoMemory);
	numplanes = 0;
	for (x = 0; x < w.m_NumPoints; x++)
	{
		VectorSubtract (w.m_Points[(x + 1) % w.m_NumPoints], w.m_Points[x], v);
		CrossProduct (v, plane.normal, planes[numplanes].normal);
		if (!VectorNormalize (planes[numplanes].normal))
		{
			continue;
		}
		planes[numplanes].dist = DotProduct (w.m_Points[x], planes[numplanes].normal);
		numplanes++;
	}
	
	bestwidth = 0;
	VectorCopy (point, bestpoint);
	newwidth = width;

	for (pass = 0; pass < 5; pass++) // apply binary search method for 5 iterations to find the maximal distance that the point can be kept away from all the edges
	{
		bool failed;
		vec3_t newpoint;
		Winding *newwinding;

		failed = true;

		newwinding = new Winding (w);
		for (x = 0; x < numplanes && newwinding->m_NumPoints > 0; x++)
		{
			dplane_t clipplane = planes[x];
			clipplane.dist += newwidth;
			newwinding->Clip (clipplane, false);
		}

		if (newwinding->m_NumPoints > 0)
		{
			VectorCopy (point, newpoint);
			snap_to_winding (*newwinding, plane, newpoint);

			VectorSubtract (newpoint, point, v);
			if (VectorLength (v) <= maxmove + ON_EPSILON)
			{
				failed = false;
			}
		}

		delete newwinding;

		if (!failed)
		{
			bestwidth = newwidth;
			VectorCopy (newpoint, bestpoint);
			if (pass == 0)
			{
				break;
			}
			newwidth += width * pow (0.5, pass + 1);
		}
		else
		{
			newwidth -= width * pow (0.5, pass + 1);
		}
	}

	free (planes);

	VectorCopy (bestpoint, point);
	return bestwidth;
}


bool			intersect_linesegment_plane(const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2, vec3_t point)
{
	vec_t			part1;
	vec_t			part2;
	int				i;
	part1 = DotProduct (p1, plane->normal) - plane->dist;
	part2 = DotProduct (p2, plane->normal) - plane->dist;
	if (part1 * part2 > 0 || part1 == part2)
		return false;
	for (i=0; i<3; ++i)
		point[i] = (part1 * p2[i] - part2 * p1[i]) / (part1 - part2);
	return true;
}

// =====================================================================================
//  plane_from_points
// =====================================================================================
void            plane_from_points(const vec3_t p1, const vec3_t p2, const vec3_t p3, dplane_t* plane)
{
    vec3_t          delta1;
    vec3_t          delta2;
    vec3_t          normal;

    VectorSubtract(p3, p2, delta1);
    VectorSubtract(p1, p2, delta2);
    CrossProduct(delta1, delta2, normal);
    VectorNormalize(normal);
    plane->dist = DotProduct(normal, p1);
    VectorCopy(normal, plane->normal);
}

//LineSegmentIntersectsBounds --vluzacn
bool LineSegmentIntersectsBounds_r (const vec_t* p1, const vec_t* p2, const vec_t* mins, const vec_t* maxs, int d)
{
	vec_t lmin, lmax;
	const vec_t* tmp;
	vec3_t x1, x2;
	int i;
	d--;
	if (p2[d]<p1[d])
		tmp=p1, p1=p2, p2=tmp;
	if (p2[d]<mins[d] || p1[d]>maxs[d])
		return false;
	if (d==0)
		return true;
	lmin = p1[d]>=mins[d]? 0 : (mins[d]-p1[d])/(p2[d]-p1[d]);
	lmax = p2[d]<=maxs[d]? 1 : (p2[d]-maxs[d])/(p2[d]-p1[d]);
	for (i=0; i<d; ++i)
	{
		x1[i]=(1-lmin)*p1[i]+lmin*p2[i];
		x2[i]=(1-lmax)*p2[i]+lmax*p2[i];
	}
	return LineSegmentIntersectsBounds_r (x1, x2, mins, maxs, d);
}
inline bool LineSegmentIntersectsBounds (const vec3_t p1, const vec3_t p2, const vec3_t mins, const vec3_t maxs)
{
	return LineSegmentIntersectsBounds_r (p1, p2, mins, maxs, 3);
}


// =====================================================================================
//  SnapToPlane
// =====================================================================================
void            SnapToPlane(const dplane_t* const plane, vec_t* const point, vec_t offset)
{
	vec_t			dist;
	dist = DotProduct (point, plane->normal) - plane->dist;
	dist -= offset;
	VectorMA (point, -dist, plane->normal, point);
}

