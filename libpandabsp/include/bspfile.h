#ifndef BSPFILE_H
#define BSPFILE_H

#include "config_bsp.h"

#include <filename.h>
#include <datagramIterator.h>
#include <datagram.h>
#include <lvector3.h>
#include <pandaNode.h>
#include <planeNode.h>
#include <eggVertexPool.h>
#include <eggPolygon.h>
#include <eggVertex.h>
#include <pnmImage.h>
#include <nodePath.h>
#include <genericAsyncTask.h>
#include <boundingBox.h>

#include <bitset>

NotifyCategoryDeclNoExport( bspfile );

static const int header_lumps = 64;
static const int vbsp_magic = ( ( 'P' << 24 ) + ( 'S' << 16 ) + ( 'B' << 8 ) + 'V' );
static const int max_disp_corner_neighbors = 4;
static const int max_surfedges = 512000;
static const int max_texdatas = 2048;
static const int max_lightmap_width = 256;
static const int max_lightmap_height = 256;
static const int max_luxels = 32;
static const int max_clusters = 65536;
static const int lightmap_padding = 2;
static const int lightmaps_per_page = ( max_lightmap_width / ( max_luxels + lightmap_padding ) ) *
                                      ( max_lightmap_height / ( max_luxels + lightmap_padding ) );

static const double power2_n[256] = // 2**(index - 128) / 255
{
        1.152445441982634800E-041, 2.304890883965269600E-041, 4.609781767930539200E-041, 9.219563535861078400E-041,
        1.843912707172215700E-040, 3.687825414344431300E-040, 7.375650828688862700E-040, 1.475130165737772500E-039,
        2.950260331475545100E-039, 5.900520662951090200E-039, 1.180104132590218000E-038, 2.360208265180436100E-038,
        4.720416530360872100E-038, 9.440833060721744200E-038, 1.888166612144348800E-037, 3.776333224288697700E-037,
        7.552666448577395400E-037, 1.510533289715479100E-036, 3.021066579430958200E-036, 6.042133158861916300E-036,
        1.208426631772383300E-035, 2.416853263544766500E-035, 4.833706527089533100E-035, 9.667413054179066100E-035,
        1.933482610835813200E-034, 3.866965221671626400E-034, 7.733930443343252900E-034, 1.546786088668650600E-033,
        3.093572177337301200E-033, 6.187144354674602300E-033, 1.237428870934920500E-032, 2.474857741869840900E-032,
        4.949715483739681800E-032, 9.899430967479363700E-032, 1.979886193495872700E-031, 3.959772386991745500E-031,
        7.919544773983491000E-031, 1.583908954796698200E-030, 3.167817909593396400E-030, 6.335635819186792800E-030,
        1.267127163837358600E-029, 2.534254327674717100E-029, 5.068508655349434200E-029, 1.013701731069886800E-028,
        2.027403462139773700E-028, 4.054806924279547400E-028, 8.109613848559094700E-028, 1.621922769711818900E-027,
        3.243845539423637900E-027, 6.487691078847275800E-027, 1.297538215769455200E-026, 2.595076431538910300E-026,
        5.190152863077820600E-026, 1.038030572615564100E-025, 2.076061145231128300E-025, 4.152122290462256500E-025,
        8.304244580924513000E-025, 1.660848916184902600E-024, 3.321697832369805200E-024, 6.643395664739610400E-024,
        1.328679132947922100E-023, 2.657358265895844200E-023, 5.314716531791688300E-023, 1.062943306358337700E-022,
        2.125886612716675300E-022, 4.251773225433350700E-022, 8.503546450866701300E-022, 1.700709290173340300E-021,
        3.401418580346680500E-021, 6.802837160693361100E-021, 1.360567432138672200E-020, 2.721134864277344400E-020,
        5.442269728554688800E-020, 1.088453945710937800E-019, 2.176907891421875500E-019, 4.353815782843751100E-019,
        8.707631565687502200E-019, 1.741526313137500400E-018, 3.483052626275000900E-018, 6.966105252550001700E-018,
        1.393221050510000300E-017, 2.786442101020000700E-017, 5.572884202040001400E-017, 1.114576840408000300E-016,
        2.229153680816000600E-016, 4.458307361632001100E-016, 8.916614723264002200E-016, 1.783322944652800400E-015,
        3.566645889305600900E-015, 7.133291778611201800E-015, 1.426658355722240400E-014, 2.853316711444480700E-014,
        5.706633422888961400E-014, 1.141326684577792300E-013, 2.282653369155584600E-013, 4.565306738311169100E-013,
        9.130613476622338300E-013, 1.826122695324467700E-012, 3.652245390648935300E-012, 7.304490781297870600E-012,
        1.460898156259574100E-011, 2.921796312519148200E-011, 5.843592625038296500E-011, 1.168718525007659300E-010,
        2.337437050015318600E-010, 4.674874100030637200E-010, 9.349748200061274400E-010, 1.869949640012254900E-009,
        3.739899280024509800E-009, 7.479798560049019500E-009, 1.495959712009803900E-008, 2.991919424019607800E-008,
        5.983838848039215600E-008, 1.196767769607843100E-007, 2.393535539215686200E-007, 4.787071078431372500E-007,
        9.574142156862745000E-007, 1.914828431372549000E-006, 3.829656862745098000E-006, 7.659313725490196000E-006,
        1.531862745098039200E-005, 3.063725490196078400E-005, 6.127450980392156800E-005, 1.225490196078431400E-004,
        2.450980392156862700E-004, 4.901960784313725400E-004, 9.803921568627450800E-004, 1.960784313725490200E-003,
        3.921568627450980300E-003, 7.843137254901960700E-003, 1.568627450980392100E-002, 3.137254901960784300E-002,
        6.274509803921568500E-002, 1.254901960784313700E-001, 2.509803921568627400E-001, 5.019607843137254800E-001,
        1.003921568627451000E+000, 2.007843137254901900E+000, 4.015686274509803900E+000, 8.031372549019607700E+000,
        1.606274509803921500E+001, 3.212549019607843100E+001, 6.425098039215686200E+001, 1.285019607843137200E+002,
        2.570039215686274500E+002, 5.140078431372548900E+002, 1.028015686274509800E+003, 2.056031372549019600E+003,
        4.112062745098039200E+003, 8.224125490196078300E+003, 1.644825098039215700E+004, 3.289650196078431300E+004,
        6.579300392156862700E+004, 1.315860078431372500E+005, 2.631720156862745100E+005, 5.263440313725490100E+005,
        1.052688062745098000E+006, 2.105376125490196000E+006, 4.210752250980392100E+006, 8.421504501960784200E+006,
        1.684300900392156800E+007, 3.368601800784313700E+007, 6.737203601568627400E+007, 1.347440720313725500E+008,
        2.694881440627450900E+008, 5.389762881254901900E+008, 1.077952576250980400E+009, 2.155905152501960800E+009,
        4.311810305003921500E+009, 8.623620610007843000E+009, 1.724724122001568600E+010, 3.449448244003137200E+010,
        6.898896488006274400E+010, 1.379779297601254900E+011, 2.759558595202509800E+011, 5.519117190405019500E+011,
        1.103823438081003900E+012, 2.207646876162007800E+012, 4.415293752324015600E+012, 8.830587504648031200E+012,
        1.766117500929606200E+013, 3.532235001859212500E+013, 7.064470003718425000E+013, 1.412894000743685000E+014,
        2.825788001487370000E+014, 5.651576002974740000E+014, 1.130315200594948000E+015, 2.260630401189896000E+015,
        4.521260802379792000E+015, 9.042521604759584000E+015, 1.808504320951916800E+016, 3.617008641903833600E+016,
        7.234017283807667200E+016, 1.446803456761533400E+017, 2.893606913523066900E+017, 5.787213827046133800E+017,
        1.157442765409226800E+018, 2.314885530818453500E+018, 4.629771061636907000E+018, 9.259542123273814000E+018,
        1.851908424654762800E+019, 3.703816849309525600E+019, 7.407633698619051200E+019, 1.481526739723810200E+020,
        2.963053479447620500E+020, 5.926106958895241000E+020, 1.185221391779048200E+021, 2.370442783558096400E+021,
        4.740885567116192800E+021, 9.481771134232385600E+021, 1.896354226846477100E+022, 3.792708453692954200E+022,
        7.585416907385908400E+022, 1.517083381477181700E+023, 3.034166762954363400E+023, 6.068333525908726800E+023,
        1.213666705181745400E+024, 2.427333410363490700E+024, 4.854666820726981400E+024, 9.709333641453962800E+024,
        1.941866728290792600E+025, 3.883733456581585100E+025, 7.767466913163170200E+025, 1.553493382632634000E+026,
        3.106986765265268100E+026, 6.213973530530536200E+026, 1.242794706106107200E+027, 2.485589412212214500E+027,
        4.971178824424429000E+027, 9.942357648848857900E+027, 1.988471529769771600E+028, 3.976943059539543200E+028,
        7.953886119079086300E+028, 1.590777223815817300E+029, 3.181554447631634500E+029, 6.363108895263269100E+029,
        1.272621779052653800E+030, 2.545243558105307600E+030, 5.090487116210615300E+030, 1.018097423242123100E+031,
        2.036194846484246100E+031, 4.072389692968492200E+031, 8.144779385936984400E+031, 1.628955877187396900E+032,
        3.257911754374793800E+032, 6.515823508749587500E+032, 1.303164701749917500E+033, 2.606329403499835000E+033,
        5.212658806999670000E+033, 1.042531761399934000E+034, 2.085063522799868000E+034, 4.170127045599736000E+034,
        8.340254091199472000E+034, 1.668050818239894400E+035, 3.336101636479788800E+035, 6.672203272959577600E+035
};

struct BSP_Lump
{
        int32_t start_byte;
        int32_t length;
        int32_t version;
        int8_t four_cc[4];
};

struct BSP_Header
{
        int32_t ident;
        int32_t version;
        BSP_Lump lumps[header_lumps];
        int32_t map_revision;
};

struct BSP_Vector
{
        float x;
        float y;
        float z;
};

struct BSP_Plane
{
        BSP_Vector normal;
        float distance;
        int32_t type;
};

struct BSP_Edge
{
        uint16_t vert_indices[2];
};

struct BSP_Vertex
{
        BSP_Vector position;
};

struct BSP_DispVert
{
        BSP_Vector direction;
        float dist;
        float alpha;
};

struct BSP_DispTri
{
        uint16_t flags;
};

struct BSP_DispSubNeighbor
{
public:
        uint16_t get_neighbor_index() const
        {
                return neighbor;
        }
        uint8_t get_span() const
        {
                return span;
        }
        uint8_t get_neighbor_span() const
        {
                return neighbor_span;
        }
        uint8_t get_neighbor_orientation() const
        {
                return neighbor_orientation;
        }
        bool is_valid() const
        {
                return neighbor != 0xFFFF;
        }
        void set_invalid()
        {
                neighbor = 0xFFFF;
        }
public:
        uint16_t neighbor;
        uint8_t neighbor_orientation;
        uint8_t span;
        uint8_t neighbor_span;
};

struct BSP_DispNeighbor
{
public:
        void set_invalid()
        {
                subneighbors[0].set_invalid();
                subneighbors[1].set_invalid();
        }

        bool is_valid() const
        {
                return subneighbors[0].is_valid() || subneighbors[1].is_valid();
        }

public:
        BSP_DispSubNeighbor subneighbors[2];
};

struct BSP_DispCornerNeighbors
{
public:
        void set_invalid()
        {
                num_neighbors = 0;
        }
public:
        uint16_t neighbors[max_disp_corner_neighbors];
        uint8_t num_neighbors;
};

struct BSP_DispInfo
{
        BSP_Vector start_position;
        int32_t disp_vert_start;
        int32_t disp_tri_start;
        int32_t power;
        int32_t min_tesselation;
        float smoothing_angle;
        int32_t contents;
        uint16_t map_face;
        int32_t lightmap_alpha_start;
        int32_t lightmap_sample_position_start;
        BSP_DispNeighbor edge_neighbors[4];
        BSP_DispCornerNeighbors corner_neighbors[4];
        uint32_t allowed_verts[10];
};

struct BSP_Face
{
        uint16_t plane_idx;
        uint8_t side;
        uint8_t on_node;
        int32_t first_edge;
        int16_t num_edges;
        int16_t tex_info;
        int16_t disp_info;
        int16_t surface_fog_volume_id;
        uint8_t styles[4];
        int32_t light_offset;
        float area;
        int32_t lightmap_tex_mins[2];
        int32_t lightmap_tex_size[2];
        int32_t orig_face;
        uint16_t num_prims;
        uint16_t first_prim_id;
        uint32_t smoothing_groups;
};

struct BSP_Brush
{
        int32_t first_side;
        int32_t num_sides;
        int32_t contents;
};

struct BSP_BrushSide
{
        uint16_t plane_idx;
        int16_t texinfo;
        int16_t dispinfo;
        int16_t bevel;
};

struct BSP_TexInfo
{
        float texture_vecs[2][4];
        float lightmap_vecs[2][4];
        int32_t flags;
        int32_t texdata;
};

struct BSP_TexData
{
        BSP_Vector reflectivity;
        int32_t table_id;
        int32_t width;
        int32_t height;
        int32_t view_width;
        int32_t view_height;
};

struct BSP_Model
{
        BSP_Vector min;
        BSP_Vector max;
        BSP_Vector origin;
        int32_t head_node;
        int32_t first_face;
        int32_t num_faces;
};

struct BSP_Vis
{
        int32_t num_clusters;
        int32_t bit_offsets[max_clusters][2];
};

struct BSP_Surfedges
{
        int32_t edge_ref[max_surfedges];
};

struct BSP_ColorRGBExp32
{
        uint8_t r, g, b;
        int8_t exponent;
};

struct BSP_Node
{
        int32_t plane_idx;
        int32_t children[2];
        int16_t mins[3];
        int16_t maxs[3];
        uint16_t first_face;
        uint16_t num_faces;
        int16_t area;
        int16_t padding;
};

struct BSP_Leaf
{
        int32_t contents;
        int16_t cluster;
        int16_t area : 9;
        int16_t flags : 7;
        int16_t mins[3];
        int16_t maxs[3];
        uint16_t first_leaf_face;
        uint16_t num_leaf_faces;
        uint16_t first_leaf_brush;
        uint16_t num_leaf_brushes;
        int16_t leaf_water_data_id;
};

static const int max_lighting = 0x1000000 / sizeof( BSP_ColorRGBExp32 );
static const int max_leaffaces = 65536;
static const int max_leafbrushes = 65536;

class EXPCL_PANDABSP BSPFile
{
public:
        bool read( const Filename &file );

        void set_camera( const NodePath &cam, const NodePath &cam_group );

        PandaNode *get_result() const;

private:
        bool read_header();
        void read_lumps();
        void process_lumps();
        void make_level();
        void make_faces();

        PT( PandaNode ) process_node( int node_idx );
        void make_bsp_tree();
        

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     BSP_Edge &edge, BSP_TexInfo &texinfo,
                                     BSP_Face &face, int k );

        void handle_entity_lump( DatagramIterator &dgi, int length );
        void handle_plane_lump( DatagramIterator &dgi, int length );
        void handle_vertex_lump( DatagramIterator &dgi, int length );
        void handle_face_lump( DatagramIterator &dgi, int length );
        void handle_origface_lump( DatagramIterator &dgi, int length );
        void handle_edge_lump( DatagramIterator &dgi, int length );
        void handle_brush_lump( DatagramIterator &dgi, int length );
        void handle_brushside_lump( DatagramIterator &dgi, int length );
        void handle_dispinfo_lump( DatagramIterator &dgi, int length );
        void handle_dispvert_lump( DatagramIterator &dgi, int length );
        void handle_disptri_lump( DatagramIterator &dgi, int length );
        void handle_model_lump( DatagramIterator &dgi, int length );
        void handle_vis_lump( DatagramIterator &dgi, int length );
        void handle_surfedge_lump( DatagramIterator &dgi, int length );
        void handle_texinfo_lump( DatagramIterator &dgi, int length );
        void handle_texdata_lump( DatagramIterator &dgi, int length );
        void handle_tdst_lump( DatagramIterator &dgi, int length );
        void handle_tdsd_lump( DatagramIterator &dgi, int length );
        void handle_lighting_lump( DatagramIterator &dgi, int length );
        void handle_node_lump( DatagramIterator &dgi, int length );
        void handle_leaf_lump( DatagramIterator &dgi, int length );
        void handle_leafface_lump( DatagramIterator &dgi, int length );
        void handle_leafbrush_lump( DatagramIterator &dgi, int length );
        BSP_Vector read_vector( DatagramIterator &dgi );
        BSP_DispCornerNeighbors read_disp_corner_neighbors( DatagramIterator &dgi );
        BSP_DispSubNeighbor read_disp_sub_neighbor( DatagramIterator &dgi );
        BSP_DispNeighbor read_disp_neighbor( DatagramIterator &dgi );
        BSP_ColorRGBExp32 read_lightmap_sample( DatagramIterator &dgi );
        pvector<BSP_ColorRGBExp32> read_lightmap_samples( int offset, int num_samples );
        PNMImage lightmap_image_from_face( BSP_Face &face );
        string extract_texture_name( int offset );
        void decompress_pvs( int lump_offset, int pvs_offset, const uint8_t *pvs_buffer, BSP_Vis &vis );

        int find_leaf( const LPoint3 &pos );
        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        void update();
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
        NodePath _camera;
        NodePath _camera_group;

        pvector<BSP_Plane> _planes;
        pvector<BSP_Vertex> _vertices;
        pvector<BSP_Face> _faces;
        pvector<BSP_Face> _orig_faces;
        pvector<BSP_Edge> _edges;
        pvector<BSP_Brush> _brushes;
        pvector<BSP_BrushSide> _brush_sides;
        pvector<BSP_DispInfo> _dispinfos;
        pvector<BSP_DispVert> _dispverts;
        pvector<BSP_DispTri> _disptris;
        pvector<BSP_TexInfo> _texinfos;
        pvector<BSP_TexData> _texdatas;
        pvector<BSP_Model> _bmodels;
        pvector<BSP_Vis> _visibilities;
        pvector<BSP_Leaf> _leafs;
        pvector<BSP_Node> _nodes;
        BSP_Header _header;

        pvector<uint8_t *> _cluster_bitsets;
        pvector<PT( BoundingBox )> _leaf_bboxes;

        BSP_ColorRGBExp32 *_lightmap_samples;
        int32_t *_surfedges;
        int32_t _texdata_string_table[max_texdatas];
        string _texdata_string_data;
        uint16_t _leaf_faces[max_leaffaces];
        uint16_t _leaf_brushes[max_leafbrushes];

        Datagram _dg;
        DatagramIterator _dgi;

        int _revision;
        int _version;

        PT( PandaNode ) _bsp_head_node;

        PT( PandaNode ) *_leaf_nodes;
        PT( PandaNode ) *_face_nodes;

        PT( PandaNode ) _result;
        
        PT( GenericAsyncTask ) _update_task;
};

#endif // BSPFILE_H