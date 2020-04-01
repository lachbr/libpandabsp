#include "c_entregistry.h"
#include "client_class.h"

// Entities
#include <c_baseentity.h>
#include <c_baseanimating.h>
#include <c_brushentity.h>
#include <c_world.h>
#include <c_basecombatcharacter.h>
#include <c_baseplayer.h>

C_EntRegistry *C_EntRegistry::_ptr = nullptr;

void C_EntRegistry::link_networkname_to_class( const std::string &name, C_BaseEntity *ent )
{
	_networkname_to_class[name] = ent;
}

void C_EntRegistry::add_network_class( ClientClass *cls )
{
	_network_class_list.push_back( cls );
}

void C_EntRegistry::init_client_classes()
{
	C_BaseEntity::client_class_init();
	C_BaseAnimating::client_class_init();
	C_BrushEntity::client_class_init();
	C_World::client_class_init();
	C_BaseCombatCharacter::client_class_init();
	C_BasePlayer::client_class_init();
}