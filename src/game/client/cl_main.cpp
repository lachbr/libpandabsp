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
#include "cl_rendersystem.h"
#include "c_baseanimating.h"

#include "bsp_material.h"

class C_TestEnt : public C_BaseAnimating
{
        DECLARE_CLIENTCLASS( C_TestEnt, C_BaseAnimating )

public:
        virtual void spawn();
        virtual void receive_entity_message( int msgtype, DatagramIterator &dgi );
};

void C_TestEnt::spawn()
{
        C_BaseAnimating::spawn();

        // Tell CTestEnt hello
        Datagram dg;
        begin_entity_message( dg, 0 );
        dg.add_string( "HELLO FROM C_TestEnt!" );
        send_entity_message( dg );
}

void C_TestEnt::receive_entity_message( int msgtype, DatagramIterator &dgi )
{
        switch ( msgtype )
        {
        case 1:
                {
                        std::string msg = dgi.get_string();
                        std::cout << "C_TestEnt: Server responded! " << msg << std::endl;
                        break;
                }
        }
}

IMPLEMENT_CLIENTCLASS_RT( C_TestEnt, CTestEnt )
END_RECV_TABLE()

int main( int argc, char **argv )
{
        PT( ClientBase ) cl = new ClientBase;
        cl->startup();

        NodePath cam = clrender->get_camera();
        cam.set_pos( 77 / 16.0f, 776 / 16.0f, 8 / 16.0f );
        cam.set_hpr( 0, 0, 0 );
        cam.set_hpr( cam, LVector3f( 250 - 90, 10, 0 ) );

        //NodePath render = clrender->get_render();
        //render.set_attrib( BSPMaterialAttrib::make_override_shader( BSPMaterial::get_from_file( "phase_14/materials/unlit.mat" ) ) );

        cl->run_cmd( "map facility_battle_v2" );

        cl->run();

        return 0;
}