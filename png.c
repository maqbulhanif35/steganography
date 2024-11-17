#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
int fileCount = 0;
uint8_t f = 0;//stores whether the folder argument is on or off
uint8_t type =0;//0 for has no embedded data 1 for has data
uint8_t PNG [8]= {137, 80, 78, 71, 13, 10, 26, 10};//png header
unsigned char *IEND = "IEND";
//char *STEGSIG = "o90xpx95";
char ENDSIG[4] = {0xE7,0xF2,0x6E,0x96};
char *STEGSIG = "o90xpx95";
long long int globalLocation = 0;
char *slashPath(char *name){
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
//read signature
void readSig(FILE *file){//this function verifies png file 
    if(file == NULL){
        fprintf(stderr,"ERROR: Could not open File");
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
        fprintf(stderr,"File is not a PNG file! First Argument Should Always be the png file");
        exit(1);
    }
	free(buffer);
    
}
int checkEmbeddings(FILE* pngFile,unsigned char *data){
	fseek(pngFile,0,SEEK_END);
	int size=ftell(pngFile);
	fseek(pngFile,0,SEEK_SET);
	char *buffer = (char*)malloc(8);//for STEGSIG
	for (int i = 0; i < size; i++)
	{
		if(i<=(size-8)){
			for (int j = 0; j < 8; j++)
			{
				buffer[j]=data[i+j];
			}
		}
		if (memcmp(buffer,STEGSIG,8)==0)
		{
			//if found return 1 
			return 1;
		}
	}
	free(buffer);
	return 0;
}
int readChunk(FILE *file){//returns location of IEND signature
	//get size
	int location;
	fseek(file,0,SEEK_END);
	int size=ftell(file);
	fseek(file,0,SEEK_SET);
	unsigned char *buffer =(unsigned char*)malloc(size);
	int status = fread(buffer,1,size,file);

	if(status!=size){
		printf("ERROR: UNEXPECTED NUMBER OF BYTES\n");
		exit(1);
	}
	//loop through buffer to check for IEND signature
	unsigned char *temp = (unsigned char*)malloc(4);;
	for (int i = 0; i < size; i++)
	{
		if(i<=(size-4)){
			for (int j = 0; j < 4; j++)
			{
				int t=i+j;
				temp[j]=buffer[i+j];
			}
		}
		if (memcmp(temp,IEND,4)==0)
		{
			//note the location
			location = i;
		}
	}
	//checkEmbeddings(file,buffer);?
	free(temp);
	return location;
}
int writeData(char *fileName,FILE* pngFile){
	char *fileNameF;
    long long int location = globalLocation;
    FILE* dataFile = fopen(fileName,"rb");
    if(dataFile==NULL){
        fprintf(stderr,"ERROR: Could not open File %s\n",fileName);
        return -1;
    }
	//read data to be written
	fseek(dataFile,0,SEEK_END);
	long long int size=ftell(dataFile);
	unsigned char *data = (unsigned char*)malloc(size);
	if(data == NULL){
		printf("ERROR: COULD NOT ALLOCATE MEMORY \n");
		exit(1);
	}
	fseek(dataFile,0,SEEK_SET);
    long long int read = fread(data,1,size,dataFile);
	//first write STEG signature
	location = location+4;
	fseek(pngFile,location,SEEK_SET);
	long long int stegWrite = fwrite(STEGSIG,1,8,pngFile);
	location = location+8;
    //write file name and size of fileName
    int fileNameSize = strlen(fileName);
	fseek(pngFile,location,SEEK_SET);
    fwrite(&fileNameSize,(sizeof(int)),1,pngFile);//write size of the filename
	location = location+4;
	fseek(pngFile,location,SEEK_SET);
	fwrite(fileName,1,fileNameSize,pngFile);
	location = location+fileNameSize;
    //
	//write data
	long long int written = fwrite(data,1,size,pngFile);
	fseek(pngFile,location+8,SEEK_SET);
	if(size!=written){
		printf("ERROR: UNEXPECTED NUMBER OF BYTES\n");
		exit(1);
	}
    //write closing signature
    fwrite(ENDSIG,1,4,pngFile);
    fclose(dataFile);
	free(data);
	fileCount++;
	return 0;
	//the file pointer will be left for the next iteration of the function
	//it will point to the end of the file
}
void addFolder(FILE* pngFile,char* folderName){//adds the specified folder to the pngFile
	struct dirent *entry;
	int fileCount = 0;
	DIR *dir = opendir(folderName);
	if(dir == NULL){
		printf("ERROR : %s %s\n",strerror(errno),folderName);
		exit(1);
		}
		size_t len2 = 0;
		while((entry = readdir(dir)) != NULL){ 
			if (entry->d_type == DT_REG)
			{
				size_t len = 0;
				char *completeFileName;
				len = strlen(folderName) + strlen(entry->d_name) +2;
				completeFileName = (char*)malloc(len);
				snprintf(completeFileName,len,"%s/%s",folderName,entry->d_name);
				if( writeData(completeFileName,pngFile) == 0 ){
					printf("INFO: ADDED %s\n",completeFileName);
				}else{
					printf("ERROR: COULD NOT ADD %s\n",completeFileName);
				}
				free(completeFileName);
			}else if (entry->d_type == DT_DIR)
			{
				char *completeFolderName;

				len2 = strlen(folderName) + strlen(entry->d_name) + 3;
				completeFolderName = (char*)malloc(len2);
				snprintf(completeFolderName,len2,"%s/%s",folderName,entry->d_name);
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				{
					continue;
				}else addFolder(pngFile,completeFolderName);
				free(completeFolderName);
			}
			}
	closedir(dir);
}
void deleteFolder(char *folderName){//deletes specified folder
			struct dirent *entry;
			DIR *folder;
			folder = opendir(folderName);
			if (folder == NULL)
			{
				perror("ERROR COULD NOT DELETE FOLDER:");
				exit(0);
			}
			while((entry = readdir(folder))!=NULL){
				if (entry->d_type == DT_REG)
				{
					char *fullPath;
					fullPath = (char*)malloc(strlen(folderName)+strlen(entry->d_name)+2);
					sprintf(fullPath,"%s/%s",folderName,entry->d_name);
					if(remove(fullPath)!= 0){
						perror("ERROR REMOVNG FILE");
						printf("%s\n",entry->d_name);
					}else{
						printf("INFO: REMOVING %s \n",entry->d_name);
					}
					free(fullPath);
				}else if (entry->d_type == DT_DIR)
				{
					char *fullPathT;
					int len2 = strlen(folderName) + strlen(entry->d_name) + 3;
					fullPathT = (char*)malloc(len2);
					sprintf(fullPathT,"%s/%s",folderName,entry->d_name);
					if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)continue;
					else{
					    deleteFolder(fullPathT);
					    free(fullPathT);
					}
				}
					
			}
			if(rmdir(folderName) != 0){
				perror("ERROR DELETING FOLDER\n");
			}else{
				printf("INFO: DELETED FOLDER %s\n",folderName);
			}
			closedir(folder);
			
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

    argc = argc-1;
	int folderIndex;
	if(argc <= 1){printf("USAGE: png pngFile file1 file2...\n");return 1;}//incorrect usage error
	char *name = (char *) malloc(20);
    //array of strings
    char **strings = (++argv);//this array of strings contains the file names
                              //the first element is always the png file
        //open png file
    FILE *pngFile = fopen(strings[0],"ab+");
    if(pngFile == NULL){
        fprintf(stderr,"ERROR: Could not open File %s", strings[0]);
        exit(1);
    }
	//check validity of pngFile
	readSig(pngFile);
	//check for the --f flag
	if (searchArgs(argv,"-f",argc) == 1)
	{
			f=1;
	}
	//loop through the args to find the folder
	if (f == 1)
	{
		fseek(pngFile,0,SEEK_END);
		int sizeOfImage = ftell(pngFile);
		fseek(pngFile,0,SEEK_SET);
		//check if cover image already contains a folder
		int location = readChunk(pngFile) + 8;
		if(location >= sizeOfImage){

		}else{//all this to check if the cover image has a folder or files embedded in it
			char *checker = (char*)malloc(8);
			//if immediately after IEND there is not stegsig then it is a folder
			int read = fread(checker,1,8,pngFile);
			if(memcmp(checker,STEGSIG,8) == 0){//it is a file
			}else{
				printf("ERROR: CANNOT ADD FOLDER AS IMAGE ALREADY CONTAINS EMBEDDED FILES!\n");
			    return 1;
			}
		    fseek(pngFile,0,SEEK_SET);
		    free(checker);
		}
		int t;
		for (size_t i = 0; i < argc; i++)
		{
			t = checkIfFileOrFolder(strings[i]);
			if (t == 1)
			{
				printf("INFO: ADDING FOLDER: %s\n",strings[i]);
				folderIndex = i;
				break;
			}
		}
		if(t!=1){
			printf("USAGE: png pngFile -f folder \n");//incorrect usage error
			exit(1);
		}
	}else{
		int t;
		for (size_t i = 0; i < argc; i++)
		{
			t = checkIfFileOrFolder(strings[i]);
			if (t == 1)
			{
				printf("ERROR: Incorrect usage, to add a folder use the -f flag");
				return 1;
			}
		}
		
	}

    fseek(pngFile,0,SEEK_SET);
	globalLocation = readChunk(pngFile);
	
    //write each file in strings to pngFile using funciton writeData 
	//FOR REGULAR FILES
	if(f == 0){//adding files
		for (size_t i = 1; i < argc; i++)
		{
			if(writeData(strings[i],pngFile) == 0){
				printf("INFO: ADDED FILE %s\n",strings[i]);
			}else{
				printf("ERROR: COULD NOT ADD FILE %s\n",strings[i]);
			}
		}
	}
	else if(f == 1){//adding folder
	    char  *folderName = slashPath(strings[folderIndex]);
		folderName = slashPath(strings[folderIndex]);
	    int len = strlen(strings[folderIndex]);
		int len2 = strlen(folderName);
		char *baseFolder;
		int slash = 0;
		for (size_t i = 0; i < len; i++)
		{
			if(strings[folderIndex][i] == '/'){
				slash = i+2;
			}
		}
		baseFolder = (char*)malloc(slash+1);
		snprintf(baseFolder,slash,"%s",strings[folderIndex]);
		int changeDir = -1;
		int baseFolderLen = 2;
		baseFolderLen = strlen(baseFolder);
		if( baseFolderLen < 1 ){//we are in the parent dir
		    changeDir = 0;
			DIR *dir = opendir(folderName);
			if(dir == NULL){
				printf("ERROR : %s %s\n",strerror(errno),folderName);
				closedir(dir);
		        return 1;
			}
			closedir(dir);
		}
		    
		else{//not in parent dir
			changeDir = chdir(baseFolder);
			DIR *dir = opendir(folderName);
			if(dir == NULL){
				printf("ERROR : %s %s\n",strerror(errno),folderName);
				closedir(dir);
				return 1;
			}
			closedir(dir);
		}
		if(changeDir == -1){
			perror("ERROR CHANGING FOLDER : ");
			return 1;
		}
	    //
		fseek(pngFile,globalLocation+4,SEEK_SET);
		fwrite(&len2,4,1,pngFile);
                fwrite(folderName,1,len2,pngFile);
		fseek(pngFile,0,SEEK_SET);
		//
		addFolder(pngFile,folderName);
		free(folderName);
	}
	fclose(pngFile);
	if(f == 0){//deleting files
		printf("Remove original files? (y/n)\n");
		char deleteOrNot;
		scanf("%c",&deleteOrNot);
		if (deleteOrNot == 'y' || deleteOrNot == 'Y')
		{
			{
				for (size_t i = 1; i < argc; i++)
				{
					if(remove(argv[i]) == 0){
						printf("DELETING %s\n",argv[i]);
					}else{
						perror("ERROR: ");
					}
				}
				
			}
		}
		
	}

	else if(f == 1){//deleting folder
	    char  *folderName = slashPath(strings[folderIndex]);
		printf("Remove original folder? (y/n)\n");
		char deleteOrNot;
		scanf("%c",&deleteOrNot);
		if(deleteOrNot == 'y' || deleteOrNot == 'Y'){
			printf("DELETING FOLDER %s\n",strings[folderIndex]);
			deleteFolder( folderName );
		}
		free(folderName);
	}
	printf("INFO: ADDED %d files",fileCount);
	return 0;
}
