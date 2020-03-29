#ifndef RADSTATICPROP_H
#define RADSTATICPROP_H

#include <nodePath.h>
#include <bitMask.h>
#include <pvector.h>
#include <collisionSegment.h>
#include <collisionPlane.h>
#include <lightMutex.h>
#include <boundingBox.h>

#include "mathtypes.h"

struct RADStaticProp
{
        int leafnum;
        NodePath mdl;
        bool shadows;
        int propnum;
};

extern pvector<RADStaticProp *> g_static_props;

extern void LoadStaticProps();
extern void DoComputeStaticPropLighting();

#endif // RADSTATICPROP_H