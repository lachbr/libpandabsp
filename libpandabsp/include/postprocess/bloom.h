#ifndef BLOOM_H
#define BLOOM_H

#include "postprocess/postprocess_effect.h"

class Texture;
class PostProcess;

class BloomEffect : public PostProcessEffect
{
	TypeDecl( BloomEffect, PostProcessEffect );

PUBLISHED:
	BloomEffect( PostProcess *pp );

	virtual Texture *get_final_texture();
};

#endif 