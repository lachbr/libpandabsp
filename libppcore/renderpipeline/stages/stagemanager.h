#ifndef __STAGE_MANAGER_H__
#define __STAGE_MANAGER_H__

#include <pvector.h>
#include "renderStage.h"
#include "..\..\config_pandaplus.h"

class EXPCL_PANDAPLUS StageManager
{
public:
        StageManager();
        void add_stage( PT( RenderStage ) stage );
        void remove_stage( PT( RenderStage ) stage );

        void setup();
        void update();
        void handle_window_resize();
        void reload_shaders();

        RenderStage *get_stage( TypeHandle &handle );

private:
        pvector<PT( RenderStage )> _stages;

        void prepare_stages();

        bool _created;
};

#endif // __STAGE_MANAGER_H__