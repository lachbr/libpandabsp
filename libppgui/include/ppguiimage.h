#ifndef __GUIIMAGE_H__
#define __GUIIMAGE_H__

//#include "ppGuiItem.h"

#include "config_ppgui.h"

#include <cardMaker.h>
#include <texture.h>

#include <pgItem.h>

#include <pp_utils.h>

class EXPCL_PPGUI GuiImage : public PGItem
{
public:
        GuiImage( const string &name );
        virtual ~GuiImage();

        void set_image( PT( Texture ) tex );
        void set_image( const string &texpath );

        Texture *get_image() const;

private:
        CardMaker _card;
        NodePath _card_np;
        PT( Texture ) _tex;

        PT( PandaNode ) _card_node;

        TypeDecl( GuiImage, PGItem );
};

#endif // __GUIIMAGE_H__