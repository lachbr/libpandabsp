/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file entityshared.h
 * @author Brian Lach
 * @date January 24, 2020
 */

#ifndef ENTITYSHARED_H_
#define ENTITYSHARED_H_

#include <pvector.h>

typedef unsigned int entid_t;
typedef pvector<entid_t> vector_entnum;

#define ENTID_MIN	0U
#define ENTID_MAX	0xFFFFFFU

#endif // ENTITYSHARED_H_