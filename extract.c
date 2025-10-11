#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
uint8_t PNG [8]= { 137, 80, 78, 71, 13, 10, 26, 10};
unsigned char *IEND = "IEND";
char *STEGSIG = "o90xpx95";
int numOfFiles;
uint8_t f = 0; //0 for files 1 for folder
//read signature
char *slashPath(char *name){//culls string at the first /
	char *folderName;
	uint8_t slash = 0;
	int len = strlen(name);
	for (size_t i = 0; i < len; i++)
	{
		if (name[i] == '/') slash = i+1;
		
	}
	folderName = (char*)malloc((len-slash)+1);
	int t = 0;
	for (size_t i = slash; i < len; i++)
	{
		folderName[t] = name[i];
		t++;
	}
	folderName[t] = '\0';
	return folderName;
}
void readSig(FILE *file){//this function verifies png file 
    if(file == NULL){
        fprintf(stderr,"ERROR: Could not open File!");
        exit(1);
    }
    unsigned char *buffer = (unsigned char*)malloc(8);//8 bytes is the size of the png signatuer
    int bytesRead = fread(buffer,1,8,file);

    if(bytesRead==0){//could not read file
        fprintf(stderr,"ERROR: Could not read PNG header!");
        exit(1);
    }
    int status = memcmp(PNG,buffer,8);
    if (status!=0)
    {
        fprintf(stderr,"ERROR: File is not a PNG file!");
        exit(1);
    }
	free(buffer);
    
}
int checkIfFolderExists(char *path){
    int result = 0;
    struct stat statbuf;
    if(stat(path, &statbuf) != 0){
        result = 0;
    }else{
        result = 1;
    }
    return result;
}
int readChunk(FILE *file,int offset,char *data){//returns location of STEGSIG signature given offset
	//get size
    uint8_t isValidSteg = 0;
	int location = -1;
	fseek(file,0,SEEK_END);
	int size=ftell(file);
	fseek(file,offset,SEEK_SET);
    
    int size2=size;
    size=size-offset;



	unsigned char *temp = (unsigned char*)malloc(8);//stores STEGSIG
	for (int i = offset; i < size2; i++)
	{
		if(i<=(size2-8)){
			for (int j = 0; j < 8; j++)
			{
                
				temp[j]=data[i+j];
                
			}
		}
		//check if file has STEGSIG 
		if(memcmp(temp,STEGSIG,8)==0){
            isValidSteg = 1;
            location = i;
            return location;
		}
	}
    free(temp);
	return location;
}
int checkIfFileExists(char *fileName){
    FILE* file;
    if (file=fopen(fileName,"r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
    
}
void السلام(char *fileName,char *data,long long int size){//receives file name and data and creates a file with the filename and writes the data to it
    //first check if file exists
    char yesorno;
    printf("INFO: EXTRACTING %s\n",fileName);
    if(checkIfFileExists(fileName)==1){
        printf("The File %s Already Exists, Do you want to overwrite(y/n)\n",fileName);
        scanf(" %c", &yesorno);
        if(yesorno == 'n' || yesorno == 'N'){
            return;//exit without writing anything
        }
    }
    FILE *file = fopen(fileName,"wb");
    if(file == NULL){
        fprintf(stderr,"ERROR: Failed to create File For writing, %s",fileName);
        exit(1);
    }
    fwrite(data,1,size,file);
    fclose(file);
}
int readData(FILE* pngFile,long long int location,long long int location2){
    //reads the data between the given offsets
    location+=8;//exclude STEGSIG
    fseek(pngFile,location,SEEK_SET);
    location2 = location2 - 4;//skip ENDSIG
    //
    int fileNameSize=0;
    fread(&fileNameSize,4,1,pngFile);
    location+=4;
    if (fileNameSize <= 0) {
        fprintf(stderr, "ERROR: UNEXPECTED SIZE: %d\n", fileNameSize);
        return -1;
    }
    //

    char *fileName = (char*)malloc(fileNameSize+1);
    long long int tf = fread(fileName,1,fileNameSize,pngFile);
    fileName[fileNameSize] = '\0';
    location+=fileNameSize;
    if(f == 0){
        fileName = slashPath(fileName);
    }
    if (fileName == NULL) {
        perror("malloc failed for fileName");
        return -1;
    }
    if (tf!=fileNameSize)
    {//failed STEGSIG may indicate false signature
        printf("Error: UNEXPECTED SIZE!\n");
        free(fileName);
        return -1;
    }
    //extract foldername 
    char *folderName;
    int slash = 0;
    for (size_t i = 0; i < fileNameSize; i++)
    {
        if(fileName[i] == '/')slash = i+1;//the last slash representing the complete folder structure
    }
    folderName = (char*)malloc(slash+2);
    snprintf(folderName,slash+1,"%s",fileName);
    //
    for (size_t i = 0; i < strlen(folderName); i++)
    {
        if(folderName[i] == '/'){
            char *temp = (char*)malloc(i+3);
            snprintf(temp,i+1,"%s",folderName);
            if( checkIfFolderExists(temp) == 0){
                mkdir(temp,0777);
            }else{
            }
            free(temp);
        }
    }
    //if(checkIfFolderExists(folderName) == 0)mkdir(folderName,0777);
    free(folderName);
    //
    long long int sizeOfData = location2 - location;
    char *data = (char*)malloc(sizeOfData);
    long long int read = fread(data,1,sizeOfData,pngFile);
    
    if(read!=sizeOfData){//failed STEGSIG may indicate false signature
        printf("ERROR: UNEXPECTED SIZE!\n");
        free(data);
        free(fileName);
        return -1;
    }
    
    السلام(fileName,data,sizeOfData);
    free(fileName);
    free(data);
    return 0;
}
int writeData(FILE* pngFile){
	//read data to be written
    //get size
    fseek(pngFile,0,SEEK_END);
    long long int size = ftell(pngFile);//go to end
    fseek(pngFile,0,SEEK_SET);//return to start
    char *buffer = (char*)malloc(size);
    long long int dataRead = fread(buffer,1,size,pngFile);
    if(dataRead!=size){
        fprintf(stderr,"ERROR: Data Read is not Equal to file size!");
        exit(1);
    }
    long long int location = readChunk(pngFile,0,buffer); //will give us locations of STEGSIG
                                        //start reading from there
    long long int location2 ;
    unsigned char *temp = (unsigned char*)malloc(8);
    long long int start = location;
    char preventFalseFlag[4];
	for (long long int i = start+1; i < size; i++)
	{
		if(i<=(size-8)){
			for (int j = 0; j < 8; j++)
			{
				temp[j]=buffer[i+j];
			}
		}
		if (memcmp(temp,STEGSIG,8) == 0)
		{
			//note the location
            location2 = location; //range from location2 to location
			location = i;
            //signature verification --prevent false flags
            char IEND[4] = {0xE7,0xF2,0x6E,0x96};
            for (size_t j = 0; j < 4; j++)
            {
                preventFalseFlag[j] = buffer[i-(4-j)];
            }
            if(memcmp(IEND,preventFalseFlag,4) == 0){//verified
                if(readData(pngFile,location2,location) == 0)numOfFiles++;
            }else{
                continue;//skip the affected file
            }
		}
	}
    if(location != -1){
        readData(pngFile,location,size);
        numOfFiles++;
    }
    free(temp);
    free(buffer);
}
int readNumOfFiles(FILE* pngFile){
    int result = 0;
    int location;
    //get size of pngFile
	fseek(pngFile,0,SEEK_END);
	int size=ftell(pngFile);
	fseek(pngFile,0,SEEK_SET);

    //read data from file in blocks for efficient memory usage ->TODO
	unsigned char *buffer =(unsigned char*)malloc(size);//not good for large file sizes
	int status = fread(buffer,1,size,pngFile);

	if(status!=size){
		printf("ERROR: UNEXPECTED SIZE\n");
		exit(1);
	}
	//loop through buffer to check for STEGSIG signature
	unsigned char *temp = (unsigned char*)malloc(8);;
	for (int i = 0; i < size; i++)
	{
		if(i<=(size-8)){
			for (int j = 0; j < 8; j++)
			{
				int t=i+j;
				temp[j]=buffer[i+j];
				//printf("%x\n",temp);
			}
		}
		//printf("%s\n",temp);
		if (memcmp(temp,STEGSIG,8) == 0)
		{
			//note the location
			location = i;
            result +=1 ;
		}
	}
	free(temp);
	return result;
}
int checkIfFileOrFolder(char *name){
	struct stat fileStat;
	if(stat(name,&fileStat)<0){
		return -1;
	}
	if(S_ISDIR(fileStat.st_mode)){
		return 1;//1 folder 0 for file
	}else{
		return 0;
	}
}
char *getPath(char *name){
    char *result;
    int length = strlen(name);
    int location = 1;
    int previousLocation = 0;
    result = (char*)malloc(length);
    for (size_t i = 0; i < length; i++)
    {
        if(name[i] == '/')location = i;
    }
    result = (char*) malloc(location);
    for (size_t i = 0; i < location; i++)
    {
        result[i] = name[i];
    }
    return result;
}


int getIEND(FILE *file){//returns location of IEND signature
	//get size
	int location;
	fseek(file,0,SEEK_END);
	long int size=ftell(file);
	fseek(file,0,SEEK_SET);
	unsigned char *buffer =(unsigned char*)malloc(size);
	long int status = fread(buffer,1,size,file);
    
	if(status!=size){
		printf("ERROR: UNEXPECTED NUMBER OF BYTES\n",status,size);
		exit(1);
	}
	//loop through buffer to check for IEND signature
	unsigned char *temp = (unsigned char*)malloc(4);;
	for (int i = 0; i < size; i++)
	{
		if(i<=(size-4)){
			for (int j = 0; j < 4; j++)
			{
				temp[j]=buffer[i+j];
			}
		}
		if (memcmp(temp,IEND,4)==0)
		{
			//note the location
			location = i;
            break;
		}
	}
	free(temp);
    free(buffer);
	return location;
}
int searchArgs(char **args,char *input,int len){//searches args for input string
    for (size_t i = 0; i < len; i++)
    {
        if(strcmp(args[i],input) == 0){
            return 1;
        }
    }
    return 0;
}
int main(int argc, char  *argv[])
{
    argc-=1;
    char **strings = (++argv);
    char clean = 'b';
    //check flags
    if(searchArgs(strings,"--clean",argc) == 1)clean = 'y';
    //open pngFile
    FILE* checkExistence = fopen(strings[0],"r");
    if(checkExistence == NULL){
        perror("ERROR");
        exit(1);
    }
    fclose(checkExistence);
    FILE* pngFile = fopen(strings[0],"r+b");
    if(pngFile == NULL){
        printf("ERROR: Could not Open PNG!");
        perror("ERROR");
        exit(1);
    }
    //verify png 
    readSig(pngFile);
    char *folderName = getPath(strings[0]);
    chdir(folderName);
    long long int globalLocation = getIEND(pngFile) +4;
    fseek(pngFile,0,SEEK_END);
    long long int sizeOfPngFile = ftell(pngFile);
    fseek(pngFile,0,SEEK_SET);
    if(globalLocation+4 == sizeOfPngFile){//check if there is any embedded files
        printf("NO FILES FOUND!\n");
        return 1;
    }
    //check if simply files or folder
    long long int location = getIEND(pngFile);//returns location of IEND + 4
    {
        char *stegHolder = malloc(8);
        fseek(pngFile,location+8,SEEK_SET);
        fread(stegHolder,1,8,pngFile);
        fseek(pngFile,0,SEEK_SET);
        if(memcmp(stegHolder,STEGSIG,8) == 0){
            
        }else{
            f = 1;
            fseek(pngFile,location+8,SEEK_SET);
            int folderNameSize;
            char *nameOfFolder;
            fread(&folderNameSize,sizeof(int),1,pngFile);
            nameOfFolder = (char*)malloc(folderNameSize);
            fread(nameOfFolder,1,folderNameSize,pngFile);
            nameOfFolder[folderNameSize] = '\0';
            printf("INFO: EXTRACTING FILES TO  : %s\n",nameOfFolder);
            if(checkIfFolderExists(nameOfFolder) == 1){printf("FOLDER %s ALREADY EXISTS\n"); return 1;}
            else mkdir(nameOfFolder,0777);
        }
        free(stegHolder);
    }
    
    writeData(pngFile);
	//clean pngFile
	if(clean == 'y' || clean == 'Y'){
        globalLocation = globalLocation + 4;
        char *data = (char*)malloc(globalLocation);
        fseek(pngFile,0,SEEK_SET);
        fread(data,1,globalLocation,pngFile);
        fseek(pngFile,0,SEEK_SET);
        fclose(pngFile);
        char *fileName = slashPath(strings[0]);
        FILE* cleaner = fopen(fileName,"wb");
        if (data == NULL)
        {
            printf("ERROR: COULD NOT CLEAN FILE!\n");
            return 1;
        }
        fseek(cleaner,0,SEEK_SET);
		int written = fwrite(data,1,globalLocation,cleaner);
        fclose(cleaner);
        free(data);
	}
    printf("SUCCESFULLY EXTRACTED %d FILES\n",numOfFiles);
    return 0;
}
