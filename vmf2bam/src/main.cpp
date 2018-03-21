#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#include <vifTokenizer.h>
#include <vifParser.h>
#include "builder.h"

#include <load_prc_file.h>
#include <pandaFramework.h>
#include <cullBinManager.h>

int main( int argc, char *argv[] )
{


        if ( argc < 2 )
        {
                cerr << "No file was specfied! Program cannot run." << endl;
                return 1;
        }
        else if ( argc < 3 )
        {
                cerr << "No output file specified! Program cannot run." << endl;
                return 2;
        }

        char *file = argv[1];
        char *output_file = argv[2];

        // Load necessary prc data.
        //load_prc_file_data("", "window-type none");
        load_prc_file_data( "", "load-display pandagl" );
        load_prc_file_data( "", "egg-object-type-portal          <Scalar> portal { 1 }" );
        load_prc_file_data( "", "egg-object-type-polylight       <Scalar> polylight { 1 }" );
        load_prc_file_data( "", "egg-object-type-seq24           <Switch> { 1 } <Scalar> fps { 24 }" );
        load_prc_file_data( "", "egg-object-type-seq12           <Switch> { 1 } <Scalar> fps { 12 }" );
        load_prc_file_data( "", "egg-object-type-indexed         <Scalar> indexed { 1 }" );
        load_prc_file_data( "", "egg-object-type-seq10           <Switch> { 1 } <Scalar> fps { 10 }" );
        load_prc_file_data( "", "egg-object-type-seq8            <Switch> { 1 } <Scalar> fps { 8 }" );
        load_prc_file_data( "", "egg-object-type-seq6            <Switch> { 1 } <Scalar>  fps { 6 }" );
        load_prc_file_data( "", "egg-object-type-seq4            <Switch> { 1 } <Scalar>  fps { 4 }" );
        load_prc_file_data( "", "egg-object-type-seq2            <Switch> { 1 } <Scalar>  fps { 2 }" );
        load_prc_file_data( "", "egg-object-type-binary          <Scalar> alpha { binary }" );
        load_prc_file_data( "", "egg-object-type-dual            <Scalar> alpha { dual }" );
        load_prc_file_data( "", "egg-object-type-glass           <Scalar> alpha { blend_no_occlude }" );
        load_prc_file_data( "", "egg-object-type-model           <Model> { 1 }" );
        load_prc_file_data( "", "egg-object-type-dcs             <DCS> { 1 }" );
        load_prc_file_data( "", "egg-object-type-notouch         <DCS> { no_touch }" );
        load_prc_file_data( "", "egg-object-type-barrier         <Collide> { Polyset descend }" );
        load_prc_file_data( "", "egg-object-type-sphere          <Collide> { Sphere descend }" );
        load_prc_file_data( "", "egg-object-type-invsphere       <Collide> { InvSphere descend }" );
        load_prc_file_data( "", "egg-object-type-tube            <Collide> { Tube descend }" );
        load_prc_file_data( "", "egg-object-type-trigger         <Collide> { Polyset descend intangible }" );
        load_prc_file_data( "", "egg-object-type-trigger-sphere  <Collide> { Sphere descend intangible }" );
        load_prc_file_data( "", "egg-object-type-floor           <Collide> { Polyset descend level }" );
        load_prc_file_data( "", "egg-object-type-dupefloor       <Collide> { Polyset keep descend level }" );
        load_prc_file_data( "", "egg-object-type-bubble          <Collide> { Sphere keep descend }" );
        load_prc_file_data( "", "egg-object-type-ghost           <Scalar> collide-mask { 0 }" );
        load_prc_file_data( "", "egg-object-type-glow            <Scalar> blend { add }" );
        load_prc_file_data( "", "egg-object-type-direct-widget   <Scalar> collide-mask { 0x80000000 } <Collide> { Polyset descend }" );
        load_prc_file_data( "", "default-model-extension .egg" );

        CullBinManager *cbm = CullBinManager::get_global_ptr();
        cbm->add_bin( "ground", CullBinManager::BT_unsorted, 18 );
        cbm->add_bin( "shadow", CullBinManager::BT_back_to_front, 19 );

        PandaFramework framework;
        framework.open_framework( argc, argv );

        WindowFramework *window = framework.open_window();

        cout << "Parsing file: " << file << endl;
        ofstream input_file;
        input_file.open( file, ios::in );

        if ( !input_file.is_open() )
        {
                cerr << "Error opening file." << endl;
                return 3;
        }

        stringstream datastream;
        datastream << input_file.rdbuf();
        string data = datastream.str();

        TokenVec toks = tokenizer( data );

        Parser p( toks );

        Builder b( output_file, window, &p );

        cout << "Done!" << endl;

        framework.close_framework();

        system( "pause" );

        return 0;
}