#include "ppguiimage.h"

#include <texturePool.h>

TypeDef( GuiImage );

GuiImage::GuiImage( const string &name ) :
        _tex( nullptr ),
        _card( "ImageCard" ),
        PGItem( name )
{
}

GuiImage::~GuiImage()
{
}

void GuiImage::set_image( const string &texpath )
{
        PT( Texture ) tex = TexturePool::load_texture( texpath );
        set_image( tex );
}

void GuiImage::set_image( PT( Texture ) tex )
{
        _tex = tex;

        NodePath oldcard( _card_node );
        const TransformState *trans;
        int sort;
        if ( oldcard.is_empty() )
        {
                trans = TransformState::make_identity();
                sort = 0;
        }
        else
        {
                trans = oldcard.get_transform();
                sort = oldcard.get_sort();
                oldcard.remove_node();
        }

        _card = CardMaker( "ImageCard" );
        _card.set_frame( -1, 1, -1, 1 );
        PT( PandaNode ) pnode = _card.generate();
        add_child( pnode );

        _card_node = pnode;

        NodePath pnp( pnode );
        pnp.set_texture( _tex );
        pnp.set_transform( trans );
}

Texture *GuiImage::get_image() const
{
        return _tex;
}