/*
Gets each directory until it cannot find another.
*/
void listDir(char *path);

/*
Calculates the MD5 file passed as parameter.
*/
int calculateMD5(char *file_name, char *md5_sum);