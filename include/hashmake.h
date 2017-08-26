/*
Calculates the MD5 file passed as parameter.
*/
int calculateMD5(const char *file_name, char *md5_sum);

/*
Calculates all MD5 files passed from the command line.
*/
void recieveAndCalculate(const int argc, char **files);