#ifndef __FLAG_USED_CELLS_STAGE_H__
#define __FLAG_USED_CELLS_STAGE_H__

#include "..\..\config_pandaplus.h"
#include "renderStage.h"
#include "..\image.h"

class EXPCL_PANDAPLUS FlagUsedCellsStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        FlagUsedCellsStage();

        void create();
        void update();
        void set_dimensions();
        void reload_shaders();

        void bind();

        PT( Image ) _cell_grid_flags;

private:

};

#endif // __FLAG_USED_CELLS_STAGE_H__