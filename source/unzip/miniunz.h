
#ifndef _miniunz_H
#define _miniunz_H

#ifdef __cplusplus
extern "C" {
#endif

int extractZip(unzFile uf,int opt_extract_without_path,int opt_overwrite,const char* password);
int extractZipOnefile(unzFile uf,const char* filename,int opt_extract_without_path,int opt_overwrite,const char* password);
int makedir(char *newdir);
int zipInfo(unzFile uf);

#ifdef __cplusplus
}
#endif

#endif
