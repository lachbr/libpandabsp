#include "c_baseanimating.h"
#include "cl_rendersystem.h"

class C_ToonCage : public C_BaseAnimating
{
	DECLARE_CLIENTCLASS( C_ToonCage, C_BaseAnimating )
public:
	virtual void spawn();
};

void C_ToonCage::spawn()
{
	BaseClass::spawn();
}

IMPLEMENT_CLIENTCLASS_RT( C_ToonCage, CToonCage )
END_RECV_TABLE()