#ifndef _CRONE_PARAM_DISPATCH_H_
#define _CRONE_PARAM_DISPATCH_H_

void crone_set_cut_param(const char* name, int voice, float value);
void crone_set_cut_param_ii(const char *name, int voice, int value);
void crone_set_cut_param_iif(const char *name, int a, int b, float value);

#endif
