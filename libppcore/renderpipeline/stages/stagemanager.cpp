#include "stageManager.h"

StageManager::
StageManager()
{
}

void StageManager::
add_stage( PT( RenderStage ) stage )
{

        // Make sure this stage isn't already in the vector.
        if ( find( _stages.begin(), _stages.end(), stage ) == _stages.end() )
        {
                _stages.push_back( stage );
        }
}

void StageManager::
remove_stage( PT( RenderStage ) stage )
{
        pvector<PT( RenderStage )>::const_iterator itr = find( _stages.begin(), _stages.end(), stage );
        if ( itr != _stages.end() )
        {
                _stages.erase( itr );
        }
}

RenderStage *StageManager::
get_stage( TypeHandle &handle )
{
        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                if ( _stages[i]->get_class_type() == handle )
                {
                        return _stages[i];
                }
        }

        return ( RenderStage * )NULL;
}

void StageManager::
prepare_stages()
{
        pvector<RenderStage *> to_remove;

        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                if ( !_stages[i]->get_active() )
                {
                        to_remove.push_back( _stages[i] );
                }
        }

        for ( size_t i = 0; i < to_remove.size(); i++ )
        {
                remove_stage( to_remove[i] );
        }
}

void StageManager::
setup()
{
        prepare_stages();

        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                _stages[i]->create();
                _stages[i]->handle_window_resize();
        }

        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                // set the shader inputs that the stage needs
                _stages[i]->bind();
        }
}

void StageManager::
update()
{
        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                if ( _stages[i]->get_active() )
                {
                        _stages[i]->update();
                }
        }
}

void StageManager::
handle_window_resize()
{
        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                _stages[i]->handle_window_resize();
        }
}

void StageManager::
reload_shaders()
{
        for ( size_t i = 0; i < _stages.size(); i++ )
        {
                _stages[i]->reload_shaders();
        }
}