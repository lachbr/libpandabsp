#pragma once

#include "config_game_shared.h"
#include "typedReferenceCount.h"

class CBaseEntityShared;
class NetworkClass;

// Each component type is assigned a unique ID at static initialization time.
// However, when a client connects to the server, he synchronizes his
// component IDs with the server.
typedef uint16_t componentid_t;

/**
 * Base class for an entity component, which contain the data and logic for
 * each aspect of an entity. You can think of components as the building
 * blocks of an entity. They can (more or less) be mixed and matched in any
 * way you desire, providing more flexibility than an inheritance tree.
 *
 * Illustration of why components are useful:
 *		        Entity Tree
 *
 *			CBaseEntity
 *			     |
 *		       CBaseAnimating
 *                        /     \
 *                   CSuit       CToon
 *				   |
 *			       CBasePlayer
 *
 * But what if we want the player able to play as a Suit?
 * If we have components, this is no problem!
 *
 *		       Component Tree
 *
 *		       CBaseComponent
 *                           |
 *                           |-- CBaseAnimating
 *                           |         |
 *                           |         |-- CSuit
 *                           |         |
 *                           |         |-- CToon
 *                           |
 *                           |-- CPlayer
 *                           |
 *                           |-- CBaseCombatCharacter
 *                           |
 *                           |-- CBaseNPC
 *
 * Now we can create entities that are a composition of these components!
 * 
 * CToonNPC
 *  - CToon
 *  - CBaseCombatCharacter
 *  - CBaseNPC
 *
 * CToonPlayer
 *  - CToon
 *  - CBaseCombatCharacter
 *  - CPlayer
 *
 */
class EXPORT_GAME_SHARED BaseComponentShared : public TypedReferenceCount
{
	DECLARE_CLASS( BaseComponentShared, TypedReferenceCount )
public:
        BaseComponentShared();
	virtual ~BaseComponentShared()
	{
	}

        virtual void precache()
        {
        }

	virtual bool initialize()
	{
		return true;
	}

        virtual void spawn()
        {
        }

	virtual void update( double frametime )
	{
	}

	virtual void shutdown()
	{
	}

        void set_entity( CBaseEntityShared *entity );
	CBaseEntityShared *get_entity() const;

	// NOTE: These are automatically defined when you put DECLARE_NETWORKCLASS()
	virtual PT( BaseComponentShared ) make_new() = 0;
        virtual componentid_t get_type_id() = 0;
	virtual NetworkClass *get_network_class() = 0;

protected:
	CBaseEntityShared *_entity;
};

inline void BaseComponentShared::set_entity( CBaseEntityShared *entity )
{
	_entity = entity;
}

inline CBaseEntityShared *BaseComponentShared::get_entity() const
{
	return _entity;
}