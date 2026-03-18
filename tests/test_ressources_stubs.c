/*
 * test_ressources_stubs.c — Stubs for shape2d file-load functions.
 *
 * Shape2d loading is not exercised by test_ressources tests (which only
 * use resource types 7 and the default case). Return NULL so those code
 * paths are inert in the test binary.
 */

void * file_load_shape2d_nofatal(const char* shapename)         { (void)shapename; return 0; }
void * file_load_shape2d_res_nofatal(char* resname)             { (void)resname;   return 0; }
void * file_load_shape2d_res_nofatal_thunk(const char* resname) { (void)resname;   return 0; }
