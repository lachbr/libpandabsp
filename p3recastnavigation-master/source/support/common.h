/**
 * \file common.h
 *
 * \date 2016-03-16
 * \author consultit
 */

#ifndef RECASTNAVIGATIONCOMMON_H_
#define RECASTNAVIGATIONCOMMON_H_

#include <lvector3.h>

///Macros
#ifdef RN_DEBUG
#	define PRINT_DEBUG(msg) std::cout << msg << std::endl
#	define PRINT_ERR_DEBUG(msg) std::cerr << msg << std::endl
#	define ASSERT_TRUE(cond) \
		if (!(cond)) { \
		  std::cerr << "assertion error : (" << #cond << ") at " \
		  << __LINE__ << ", " \
		  << __FILE__ << std::endl; \
		}
#else
#	define PRINT_DEBUG(msg)
#	define PRINT_ERR_DEBUG(msg)
#	define ASSERT_TRUE(cond)
#endif

#define RETURN_ON_COND(_flag_,_return_)\
	if (_flag_)\
	{\
		return _return_;\
	}

namespace rnsup
{
//https://groups.google.com/forum/?fromgroups=#!searchin/recastnavigation/z$20axis/recastnavigation/fMqEAqSBOBk/zwOzHmjRsj0J
inline void LVecBase3fToRecast(const LVecBase3f& v, float* p)
{
	p[0] = v.get_x();
	p[1] = v.get_z();
	p[2] = -v.get_y();
}
inline LVecBase3f RecastToLVecBase3f(const float* p)
{
	return LVecBase3f(p[0], -p[2], p[1]);
}
inline LVecBase3f Recast3fToLVecBase3f(const float x, const float y, const float z)
{
	return LVecBase3f(x, -z, y);
}

///XXX Moved (and modified) from NavMeshType_Tile.cpp
inline unsigned int nextPow2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline unsigned int ilog2(unsigned int v)
{
	unsigned int r;
	unsigned int shift;
	r = (v > 0xffff) << 4;
	v >>= r;
	shift = (v > 0xff) << 3;
	v >>= shift;
	r |= shift;
	shift = (v > 0xf) << 2;
	v >>= shift;
	r |= shift;
	shift = (v > 0x3) << 1;
	v >>= shift;
	r |= shift;
	r |= (v >> 1);
	return r;
}

#ifdef RN_DEBUG
#	define CTXLOG(ctx,type,msg) \
		ctx->log(type,msg)
#	define CTXLOG1(ctx,type,msg,par) \
		ctx->log(type,msg,par)
#	define CTXLOG2(ctx,type,msg,par1,par2) \
		ctx->log(type,msg,par1,par2)
#else
#	define CTXLOG(ctx,type,msg)
#	define CTXLOG1(ctx,type,msg,par)
#	define CTXLOG2(ctx,type,msg,par1,par2)
#endif

} // namespace rnsup

#endif /* RECASTNAVIGATIONCOMMON_H_ */
