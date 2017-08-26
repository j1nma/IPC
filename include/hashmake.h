/*
Calculates the MD5 file passed as parameter.
*/
void hashFile(int argc, const char* file);

/*
Calculates all MD5 files passed as from the command line.
*/
void readAndHash(int argc, char** files);