#include "entregistry.h"
#include "server_class.h"

#include <baseentity.h>
#include <baseanimating.h>
#include <brushentity.h>
#include <world.h>
#include <basecombatcharacter.h>
#include <baseplayer.h>

CEntRegistry *CEntRegistry::_ptr = nullptr;

void CEntRegistry::link_networkname_to_class( const std::string &name, CBaseEntity *ent )
{
	_networkname_to_class[name] = ent;
}

void CEntRegistry::add_network_class( ServerClass *cls )
{
	_network_class_list.push_back( cls );
}

void CEntRegistry::init_server_classes()
{
	CBaseEntity::server_class_init();
	CBaseAnimating::server_class_init();
	CBrushEntity::server_class_init();
	CWorld::server_class_init();
	CBaseCombatCharacter::server_class_init();
	CBasePlayer::server_class_init();
}