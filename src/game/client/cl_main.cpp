/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file cl_main.cpp
 * @author Brian Lach
 * @date April 02, 2020
 *
 * @desc Entry point for client game
 */

#include "clientbase.h"

int main( int argc, char **argv )
{
        PT( ClientBase ) cl = new ClientBase;
        cl->startup();
        cl->run();

        return 0;
}