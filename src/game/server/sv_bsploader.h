#pragma once

#include "config_serverdll.h"
#include "baseentity.h"

#include <bsploader.h>
#include <pvector.h>
#include <weakPointerTo.h>
#include <nodePath.h>
#include <filename.h>

class EXPORT_SERVER_DLL CSV_BSPLoader : public BSPLoader
{
public:
	CSV_BSPLoader();

	virtual bool read( const Filename &file, bool is_transition = false );

	INLINE void set_transition_landmark( const std::string &name,
					     const LVector3 &origin,
					     const LVector3 &angles )
	{
		// Preserved entity transforms are stored relative to this node,
		// and applied relative to the destination landmark in the new level.
		_transition_source_landmark = NodePath( name );
		_transition_source_landmark.set_pos( origin );
		_transition_source_landmark.set_hpr( angles );
	}
	INLINE void clear_transition_landmark()
	{
		_transition_source_landmark = NodePath();
	}

protected:
	virtual void load_geometry();
	virtual void load_entities();
	void spawn_entities();
	virtual void cleanup_entities( bool is_transition );

protected:
	pvector<WPT( CBaseEntity )> _entities;
	NodePath _transition_source_landmark;
};