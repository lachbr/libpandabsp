#ifndef __COLLECT_USED_CELLS_STAGE_H__
#define __COLLECT_USED_CELLS_STAGE_H__

#include "renderStage.h"
#include "..\image.h"

class EXPCL_PANDAPLUS CollectUsedCellsStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        CollectUsedCellsStage();

        void create();
        void update();
        void set_dimensions();
        void reload_shaders();

        PT( Image ) _cell_list_buffer;
        PT( Image ) _cell_index_buffer;

        void bind();

private:

};

#endif // __COLLECT_USED_CELLS_STAGE_H__