
#ifndef _miniunz_H
#define _miniunz_H

#ifdef __cplusplus
extern "C" {
#endif

int do_extract(unzFile uf,int opt_extract_without_path,int opt_overwrite,const char* password);
int makedir(char *newdir);

#ifdef __cplusplus
}
#endif

#endif
