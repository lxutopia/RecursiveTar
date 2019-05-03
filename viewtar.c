/*
Created by Simon Nilsson, Axenu for Sydarkivera 2017
simon@axenu.com simon.nilsson@sydarkivera.se
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//14:40

//to tar a folder:
//tar cvf archive.tar ./archive

struct posix_header
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
  char fullName[2048];             /* 500 */
};

long octalToDecimal(char* octal, int size) {
    // printf("octal number: %.12s", octal);
    long result = 0;
    long potens = 1;
    for (int i = size-2; i >= 0; i--) {
        long iVal = (octal[i] - '0');
        result += iVal * potens;
        potens *= 8;
    }
    return result;
}

void fillStruct(char *header, struct posix_header *pHeader, char *storedName, int storedNameSize) {
    int offset = 0;
    if (header[0] == '.' && header[1] == '/')
        offset = 2;
    else if (header[0] == '/')
        offset = 1;
    strncpy(pHeader->name, header+offset, 100);
    strncpy(pHeader->mode, header+100, 8);
    strncpy(pHeader->uid, header+108, 8);
    strncpy(pHeader->gid, header+116, 8);
    strncpy(pHeader->size, header+124, 12);
    strncpy(pHeader->mtime, header+136, 12);
    strncpy(pHeader->chksum, header+148, 8);
    pHeader->typeflag = header[156];
    strncpy(pHeader->linkname, header+157, 100);
    strncpy(pHeader->magic, header+257, 6);
    strncpy(pHeader->version, header+263, 2);
    strncpy(pHeader->uname, header+265, 32);
    strncpy(pHeader->gname, header+297, 32);
    strncpy(pHeader->devmajor, header+329, 8);
    strncpy(pHeader->devminor, header+337, 8);
    strncpy(pHeader->prefix, header+345, 155);
    if (storedNameSize > 0) {
      strncpy(pHeader->fullName, storedName, storedNameSize);
    } else {
      int offset2 = 0;
      if (strlen(pHeader->prefix) > 0) {
          offset2 = strlen(pHeader->prefix);
          strncpy(pHeader->fullName, header+345, offset2);
          strcpy(pHeader->fullName+offset2, "/");
          offset2+=1;
      }
      strncpy(pHeader->fullName+offset2, header+offset, strlen(pHeader->name));
    }

}

void printStruct(struct posix_header *header) {
    printf("name: %.100s\n", header->name);
    printf("mode: %.8s\n", header->mode);
    printf("uid: %.8s\n", header->uid);
    printf("gid: %.8s\n", header->gid);
    printf("size: %.12s\n", header->size);
    printf("size: %li\n", octalToDecimal(header->size, 12));
    printf("mtime: %.12s\n", header->mtime);
    printf("chksum: %.8s\n", header->chksum);
    printf("typeflag: %c\n", header->typeflag);
    printf("linkname: %.100s\n", header->linkname);
    printf("magic: %.6s\n", header->magic);
    printf("version: %.2s\n", header->version);
    printf("uname: %.32s\n", header->uname);
    printf("gname: %.32s\n", header->gname);
    printf("devmajor: %.8s\n", header->devmajor);
    printf("devminor: %.8s\n", header->devminor);
    printf("prefix: %.2048s\n\n", header->prefix);
}

long roundUp(long num) {
    long m = num % 512;
    if (m != 0) {
        num += 512-m;
    }
    return num;
}

int findNextChar(char *haystack, int length, char needle, int offset) {
    for (int i = offset; i < length; i++) {
        if (haystack[i] == needle) {
            return i;
        }
    }
    return -1;
}

long longFromCharArray(char *array, int length) {
    long res = 0;
    long factor = 1;
    for (int i = length-1; i >= 0; i--) {
        res += factor * (long)(array[i] - '0');
        factor *= 10;
    }
    return res;
}

long getSizeFromExtendedHeader(char *header, int length) {
    int space, equalSign, newline;
    int offset = 0;

    while (offset < length) {
        space = findNextChar(header+offset, length, ' ', 0);
        equalSign = findNextChar(header+offset, length, '=', 0);
        newline = findNextChar(header+offset, length, '\n', 1);
        if (space == -1 || equalSign == -1 || newline == -1)
            return -1;
        //test if key is "size"
        if (*(header+offset+space+1) == 's' && *(header+offset+space+2) == 'i' && *(header+offset+space+3) == 'z' && *(header+offset+space+4) == 'e') {
            return longFromCharArray(header+offset+equalSign+1, newline-1-equalSign);
        }
        //goto next attribute
        offset += newline;
    }
    return -1;
}

void listTarFile(FILE *fp, int indentation, char *pPath) {
    //for checking for end of tar
    int numberOfEmpty = 0;

    char header[512];
    size_t result;

    char storedName[2048];
    size_t storedNameSize = 0;

    //read files
    struct posix_header fileHeader;
    while (!feof(fp) && numberOfEmpty < 2) {
        result = fread (header,1,512,fp);
        //test if header consists of 0 only. if two zero block occure, end the file reading.
        int empty = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) {
                empty = 0;
                break;
            }
        }
        if (!empty) {
            numberOfEmpty = 0;
            fileHeader = (struct posix_header){ 0 };
            fillStruct(header, &fileHeader, storedName, storedNameSize);
            // printStruct(&fileHeader);
            if (fileHeader.typeflag == '0') {
                //calculate filelength
                long jump = roundUp(octalToDecimal(fileHeader.size, 12));
                //indent the file list:
                char *indent = malloc(indentation*4);
                memset(indent, ' ', indentation*4);
                printf("%s%s%s\n", indent, pPath, fileHeader.fullName);
                //if file ends with .tar -> list files inside it
                char *dot = strrchr(fileHeader.name, '.');
                if (dot && !strcmp(dot, ".tar")) {
                    //odd hack with seeking, slows down a little but works.
                    long pos = ftell(fp);
                    //calculate next ppath
                    char *nextPPath = malloc(strlen(pPath) + strlen(fileHeader.fullName) + 1);

                    strncpy(nextPPath, pPath, strlen(pPath));
                    strncpy(nextPPath+strlen(pPath), fileHeader.fullName, strlen(fileHeader.fullName));
                    strcpy(nextPPath+strlen(pPath)+strlen(fileHeader.fullName), "/");
                    listTarFile(fp, indentation+1, nextPPath);
                    fseek(fp, pos+jump, SEEK_SET);
                    free(nextPPath);
                    nextPPath = NULL;
                } else {
                    if (jump > 0)
                        fseek(fp, jump, SEEK_CUR);
                }
            } else if (fileHeader.typeflag == '5') {
                char *indent = malloc(indentation*4);
                memset(indent, ' ', indentation*4);
                printf("%s%s%s\n", indent, pPath, fileHeader.fullName);
            } else if (fileHeader.typeflag == 'x') {
                // char *indent = malloc(indentation*4);
                // memset(indent, ' ', indentation*4);
                // printf("%s%s%s\n", indent, pPath, fileHeader.fullName);
                // printf("length: %ld\n", octalToDecimal(fileHeader.size));
                // printf("length: %s\n", fileHeader.size);
                long extendedHeaderLength = roundUp(octalToDecimal(fileHeader.size, 12));
                char *extendedHeader = malloc(extendedHeaderLength);
                fread(extendedHeader, 1, extendedHeaderLength, fp);

                //get size from extended header
                long size = getSizeFromExtendedHeader(extendedHeader, extendedHeaderLength);


                // printf("%*.*s\n", 512, 512, extendedHeader);
                // for (int i = 0; i < 512; i++) {
                //     printf("%c ", header[i]);
                // }

                // printf("\nextendedHeader: \n");
                // for (int i = 0; i < extendedHeaderLength; i++) {
                //     printf(" \"%c\" ", extendedHeader[i]);
                // }

                // printf("\n NEXT HEADER: \n");
                long jump2 = roundUp(size);
                //read huge file header
                result = fread (header,1,512,fp);
                fileHeader = (struct posix_header){ 0 };
                fillStruct(header, &fileHeader, storedName, storedNameSize);

                char *indent = malloc(indentation*4);
                memset(indent, ' ', indentation*4);
                printf("%s%s%s\n", indent, pPath, fileHeader.fullName);

                //case file
                //if file ends with .tar -> list files inside it
                char *dot = strrchr(fileHeader.name, '.');
                if (dot && !strcmp(dot, ".tar")) {
                    //odd hack with seeking, slows down a little but works.
                    long pos = ftell(fp);
                    //calculate next ppath
                    char *nextPPath = malloc(strlen(pPath) + strlen(fileHeader.fullName) + 1);

                    strncpy(nextPPath, pPath, strlen(pPath));
                    strncpy(nextPPath+strlen(pPath), fileHeader.fullName, strlen(fileHeader.fullName));
                    strcpy(nextPPath+strlen(pPath)+strlen(fileHeader.fullName), "/");
                    listTarFile(fp, indentation+1, nextPPath);
                    fseek(fp, pos+jump2, SEEK_SET);
                    free(nextPPath);
                    nextPPath = NULL;
                } else {
                    if (jump2 > 0)
                        fseek(fp, jump2, SEEK_CUR);
                }

                // for (int i = 0; i < 512; i++) {
                //     printf("%c ", header[i]);
                // }
                // printf("\nend\n");
                // fseek(fp, jump2, SEEK_CUR);
                // for (int i = 0; i < 512; i++) {
                    // if (header[i] == '\0') {
                    //     printf("split space ");
                    // }
                    // printf("%d ", (int)header[i]);
                // }
            } else if (fileHeader.typeflag == 'L') {
                // printStruct(&fileHeader);
                // read next block and check it's content
                storedNameSize = octalToDecimal(fileHeader.size, 12);
                int rest = 512 - (storedNameSize % 512);
                result = fread (storedName,1,storedNameSize,fp);
                fseek(fp, rest, SEEK_CUR);
            } else {
              printf("unknown flag: %c\n", fileHeader.typeflag);
            }
        } else {
            numberOfEmpty++;
        }
    }
}

void parseTarForPath(FILE *fp, char path[]) {
    //for checking for end of tar
    int numberOfEmpty = 0;
        char storedName[2048];
        size_t storedNameSize = 0;

    char header[512];
    size_t result;

    //read files
    struct posix_header fileHeader;
    while (!feof(fp) && numberOfEmpty < 2) {
        result = fread (header,1,512,fp);
        //test if header consists of 0 only. if two zero blocks occure, end the file reading.
        int empty = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) {
                empty = 0;
                break;
            }
        }
        if (!empty) {
            numberOfEmpty = 0;
            fileHeader = (struct posix_header){ 0 };
            fillStruct(header, &fileHeader, storedName, storedNameSize);
            // printf("%s\n", path);
            // printf("filename: %s\n", fileHeader.name);
            if (fileHeader.typeflag == '0') {
                //calculate filelength
                long jump = octalToDecimal(fileHeader.size, 12);
                //convert length in bytes from octal to decimal.
                long m = jump % 512;
                if (m != 0) {
                    jump += 512-m;
                }

                //test if file is the file searched after
                int correctPath = 1;
                for (int i = 0; i < strlen(fileHeader.fullName); i++) {
                    //for security check if fileHeader.name is longer than path.
                    if (fileHeader.fullName[i] != path[i]) {
                        correctPath = 0;
                        break;
                    }
                }
                if (correctPath) {
                    //if path continues deeper in. TODO check for ".tar" ending
                    if (strlen(fileHeader.fullName) < strlen(path)) {
                        //calculate new path.
                        size_t pathl = strlen(path);
                        size_t pathn = strlen(fileHeader.fullName);
                        char *newpath = malloc(pathl - pathn);
                        memset(newpath, '\0', pathl - pathn);
                        strncpy(newpath, path + pathn + 1, pathl - pathn - 1);
                        parseTarForPath(fp, newpath);

                    } else { //the requested file is found, print the result:
                        long fileSize = octalToDecimal(fileHeader.size, 12);
                        jump -= fileSize;
                        while (fileSize > 512) {
                            result = fread (header,1,512,fp);
                            fwrite(header, 1, 512, stdout);
                            fileSize -= 512;
                        }
                        if (fileSize > 0) {
                            result = fread (header,1,fileSize,fp);
                            fwrite(header, 1, fileSize, stdout);
                            fileSize = 0;
                        }
                    }
                    //end program
                    break;
                } else {
                    fseek(fp, jump, SEEK_CUR);
                }
            } else if (fileHeader.typeflag == '5') {
                // char *indent = malloc(indentation*4);
                // memset(indent, ' ', indentation*4);
                // printf("%s%s\n", indent, fileHeader.name);
            } else if (fileHeader.typeflag == 'x') {
                long extendedHeaderLength = roundUp(octalToDecimal(fileHeader.size, 12));
                char *extendedHeader = malloc(extendedHeaderLength);
                fread(extendedHeader, 1, extendedHeaderLength, fp);

                //get size from extended header
                long size = getSizeFromExtendedHeader(extendedHeader, extendedHeaderLength);
                long jump = roundUp(size);
                //read huge file header
                result = fread (header,1,512,fp);
                fileHeader = (struct posix_header){ 0 };
                fillStruct(header, &fileHeader, storedName, storedNameSize);
                int correctPath = 1;
                for (int i = 0; i < strlen(fileHeader.fullName); i++) {
                    //for security check if fileHeader.name is longer than path.
                    if (fileHeader.fullName[i] != path[i]) {
                        correctPath = 0;
                        break;
                    }
                }
                if (correctPath) {
                    //if path continues deeper in. TODO check for ".tar" ending
                    if (strlen(fileHeader.fullName) < strlen(path)) {
                        //calculate new path.
                        // ./(path - fileheader.name)
                        size_t pathl = strlen(path);
                        size_t pathn = strlen(fileHeader.fullName);
                        char *newpath = malloc(pathl - pathn);
                        memset(newpath, '\0', pathl - pathn);
                        // memcpy(newpath, "./", 2);
                        strncpy(newpath, path + pathn + 1, pathl - pathn - 1);
                        // printf("%s\n", newpath);
                        parseTarForPath(fp, newpath);

                    } else { //the requested file is found, print the result:
                        // printf("fileHeader:\n");
                        // printStruct(&fileHeader);
                        //print first block //TODO print all, based on size
                        while (jump > 0) {
                            result = fread (header,1,512,fp);
                            printf("%.512s", header);
                            jump -= 512;
                        }
                        fseek(fp, jump, SEEK_CUR);
                    }
                    //end program
                    break;
                } else {
                    fseek(fp, jump, SEEK_CUR);
                }
            } else if (fileHeader.typeflag == 'L') {
                // printStruct(&fileHeader);
                // read next block and check it's content
                storedNameSize = octalToDecimal(fileHeader.size, 12);
                int rest = 512 - (storedNameSize % 512);
                result = fread (storedName,1,storedNameSize,fp);
                fseek(fp, rest, SEEK_CUR);
            } else {
              printf("unknown flag: %c\n", fileHeader.typeflag);
            }
        } else {
            // printf("empty!\n" );
            numberOfEmpty++;
        }
    }
}

int main(int argc, char* argv[]) {

    //argv[1] = input path

    //TODO search with prefix paths.
    //Maybe conact prefix and name in fillStruct

    if (argc < 2) {
        printf("Usage: ./peektar <path To Archive> [ <Path Within Archive> ]\n\nThe first path is the path of the .tar archive file to be read.\nIf only the first argument is specified the content of the archive will be listed.\nIf the second path, path within archive, is specified the file will be returned to stdout as binary.\n");
        return 0;
    }

    FILE * fp;
    //open tar, TODO specified in argv[2]?
    fp = fopen (argv[1], "rb");

    //list files in tar
    if (argc == 2) {
        listTarFile(fp, 0, "");
    } else if (argc == 3) { //search for file in tar archive(s)

        //change inputpath to format ./archive/a.txt
        char *path = malloc(strlen(argv[2]));
        // char inputpath[] = "./";
        // strcat(path, "./");
        strcat(path, argv[2]);
        // printf("Path: %s\n", path);

        //search for file
        parseTarForPath(fp, path);

        fclose(fp);
    }

    return 0;
}
