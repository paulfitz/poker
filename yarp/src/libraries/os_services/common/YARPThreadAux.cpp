#include "YARPThread.h"

static int app_death = 0;

void YARPThread::PrepareForDeath()
{
  app_death = 1;
}

int YARPThread::IsDying()
{
  return app_death;
}
