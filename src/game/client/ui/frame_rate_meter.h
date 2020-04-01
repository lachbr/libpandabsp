#ifndef FRAMERATEMETER_H_
#define FRAMERATEMETER_H_

#include <referenceCount.h>
#include <textNode.h>
#include <genericAsyncTask.h>

#include "config_clientdll.h"

class EXPORT_CLIENT_DLL CFrameRateMeter : public ReferenceCount
{
public:
	CFrameRateMeter();

	void enable();
	void disable();

	void update();

private:
	static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
	PT( GenericAsyncTask ) _task;
	PT( TextNode ) _tn;
	NodePath _tnnp;
	double _last_update_time;
};

#endif // FRAMERATEMETER_H_