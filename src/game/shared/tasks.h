#pragma once

#define DECLARE_TASK_FUNC(name) \
static AsyncTask::DoneStatus name(GenericAsyncTask *task, void *data)

#define DEFINE_TASK_FUNC(name) \
AsyncTask::DoneStatus name(GenericAsyncTask *task, void * data)

#define DEFINE_TASK(name) PT(GenericAsyncTask) name