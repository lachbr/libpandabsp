#ifndef COMMON_FILTERS_H
#define COMMON_FILTERS_H

#include "filtermanager.h"

#include <genericAsyncTask.h>
#include <nodePathCollection.h>
#include <textureCollection.h>

class EXPCL_PANDAPLUS CommonFilters
{
public:

        enum Size
        {
                S_off,
                S_small,
                S_medium,
                S_large
        };

        class FilterConfig
        {
        };

        class CartoonInkConfig : public FilterConfig
        {
        public:
                PN_stdfloat _separation;
                LColor _color;
        };

        class BloomConfig : public FilterConfig
        {
        public:
                PN_stdfloat _max_trigger;
                PN_stdfloat _min_trigger;
                PN_stdfloat _desat;
                PN_stdfloat _intensity;
                Size _size;
                LColor _blend;
        };

        class HalfPixelShiftConfig : public FilterConfig
        {
        public:
                bool _flag;
        };

        class ViewGlowConfig : public FilterConfig
        {
        public:
                bool _flag;
        };

        class InvertedConfig : public FilterConfig
        {
        public:
                bool _flag;
        };

        class VolumetricLightingConfig : public FilterConfig
        {
        public:
                NodePath _caster;
                int _num_samples;
                PN_stdfloat _density;
                PN_stdfloat _decay;
                PN_stdfloat _exposure;
                string _source;
        };

        class BlurSharpenConfig : public FilterConfig
        {
        public:
                PN_stdfloat _amount;
        };

        class AmbientOcclusionConfig : public FilterConfig
        {
        public:
                int _num_samples;
                PN_stdfloat _radius;
                PN_stdfloat _amount;
                PN_stdfloat _strength;
                PN_stdfloat _falloff;
        };

        class GammaAdjustConfig : public FilterConfig
        {
        public:
                PN_stdfloat _gamma;
        };

        enum FilterType
        {
                FT_none,
                FT_cartoon_ink,
                FT_blur_sharpen,
                FT_bloom,
                FT_volumetric_lighting,
                FT_ambient_occlusion,
                FT_half_pixel_shift,
                FT_view_glow,
                FT_inverted,
                FT_gamma_adjust
        };

        CommonFilters( GraphicsOutput *win, NodePath &cam );

        void begin_config_mode();
        void end_config_mode();

        void cleanup();

        void reconfigure( bool full_rebuild, FilterType changed );

        void update();

        void set_cartoon_ink( PN_stdfloat separation = 1.0, LColor &color = LColor( 0, 0, 0, 1 ) );
        void del_cartoon_ink();

        void set_bloom( LColor &blend = LColor( 0.3, 0.4, 0.3, 0.0 ), PN_stdfloat min_trigger = 0.6,
                        PN_stdfloat max_trigger = 1.0, PN_stdfloat desat = 0.6, PN_stdfloat intensity = 1.0,
                        Size size = S_medium );
        void del_bloom();

        void set_half_pixel_shift();
        void del_half_pixel_shift();

        void set_view_glow();
        void del_view_glow();

        void set_inverted();
        void del_inverted();

        void set_volumetric_lighting( NodePath &caster, int num_samples = 32, PN_stdfloat density = 5.0,
                                      PN_stdfloat decay = 0.1, PN_stdfloat exposure = 0.1,
                                      string source = "color" );
        void del_volumetric_lighting();

        void set_blur_sharpen( PN_stdfloat amount = 0.0 );
        void del_blur_sharpen();

        void set_ambient_occlusion( int num_samples = 16, PN_stdfloat radius = 0.05, PN_stdfloat amount = 2.0,
                                    PN_stdfloat strength = 0.01, PN_stdfloat falloff = 0.000002 );
        void del_ambient_occlusion();

        void set_gamma_adjust( PN_stdfloat gamma );
        void del_gamma_adjust();

        void update_clears();

private:
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

        bool has_configuration( FilterType filter ) const;

private:
        typedef pmap<string, string> StringMap;
        typedef pmap<FilterType, FilterConfig *> FilterMap;
        FilterMap _configuration;

        FilterManager::TextureMap _textures;

        NodePathCollection _bloom;
        NodePathCollection _blur;
        NodePathCollection _ssao;

        bool _config_mode;

        NodePath _final_quad;

        PT( GenericAsyncTask ) _task;

        FilterManager _manager;
};


#endif // COMMON_FILTERS_H