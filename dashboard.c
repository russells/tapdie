#include "dashboard.h"
#include "tapdie.h"


static QState dashboardInitial(struct Dashboard *me);
static QState dashboardState(struct Dashboard *me);


struct Dashboard dashboard;


void dashboard_ctor(void)
{
	QActive_ctor((QActive *)(&dashboard), (QStateHandler)&dashboardInitial);
}


static QState dashboardInitial(struct Dashboard *me)
{
	return Q_TRAN(&dashboardState);
}


static QState dashboardState(struct Dashboard *me)
{
	return Q_SUPER(&QHsm_top);
}
