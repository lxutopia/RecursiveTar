/*
Created by Simon Nilsson, Axenu for Sydarkivera 2017
simon@axenu.com simon.nilsson@sydarkivera.se

Cleanup by Lexx Damm, Sydarkivera 2024
lexx.damm@sydarkivera.se
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

long octalToDecimal(char *pOctal, int size) {
    long result = 0;
    long potens = 1;
    for (int i = size-2; i >= 0; i--) {
        long iVal = (pOctal[i] - '0');
        result += iVal * potens;
        potens *= 8;
    }
    return result;
}

void fillStruct(char *pHeader, struct posix_header *pPosixHeader, char *pStoredName, int storedNameSize) {
    int offset = 0;
    if (pPosixHeader[0] == '.' && pHeader[1] == '/')
        offset = 2;
    else if (pHeader[0] == '/')
        offset = 1;
    strncpy(pPosixHeader->name, pHeader + offset, 100);
    strncpy(pPosixHeader->mode, pHeader + 100, 8);
    strncpy(pPosixHeader->uid, pHeader + 108, 8);
    strncpy(pPosixHeader->gid, pHeader + 116, 8);
    strncpy(pPosixHeader->size, pHeader + 124, 12);
    strncpy(pPosixHeader->mtime, pHeader + 136, 12);
    strncpy(pPosixHeader->chksum, pHeader + 148, 8);
    pPosixHeader->typeflag = pHeader[156];
    strncpy(pPosixHeader->linkname, pHeader + 157, 100);
    strncpy(pPosixHeader->magic, pHeader + 257, 6);
    strncpy(pPosixHeader->version, pHeader + 263, 2);
    strncpy(pPosixHeader->uname, pHeader + 265, 32);
    strncpy(pPosixHeader->gname, pHeader + 297, 32);
    strncpy(pPosixHeader->devmajor, pHeader + 329, 8);
    strncpy(pPosixHeader->devminor, pHeader + 337, 8);
    strncpy(pPosixHeader->prefix, pHeader + 345, 155);
    if (storedNameSize > 0) {
      strncpy(pPosixHeader->fullName, storedName, storedNameSize);
    } else {
      int offset2 = 0;
      if (strlen(pPosixHeader->prefix) > 0) {
          offset2 = strlen(pPosixHeader->prefix);
          strncpy(pPosixHeader->fullName, pHeader + 345, offset2);
          strcpy(pPosixHeader->fullName + offset2, "/");
          offset2 += 1;
      }
      strncpy(pPosixHeader->fullName + offset2, pHeader + offset, strlen(pHeader->name));
    }
}

void printStruct(struct posix_header *pPosixHeader) {
    printf("name: %.100s\n", pPosixHeader->name);
    printf("mode: %.8s\n", pPosixHeader->mode);
    printf("uid: %.8s\n", pPosixHeader->uid);
    printf("gid: %.8s\n", pPosixHeader->gid);
    printf("size: %.12s\n", pPosixHeader->size);
    printf("size: %li\n", octalToDecimal(pPosixHeader->size, 12));
    printf("mtime: %.12s\n", pPosixHeader->mtime);
    printf("chksum: %.8s\n", pPosixHeader->chksum);
    printf("typeflag: %c\n", pPosixHeader->typeflag);
    printf("linkname: %.100s\n", pPosixHeader->linkname);
    printf("magic: %.6s\n", pPosixHeader->magic);
    printf("version: %.2s\n", pPosixHeader->version);
    printf("uname: %.32s\n", pPosixHeader->uname);
    printf("gname: %.32s\n", pPosixHeader->gname);
    printf("devmajor: %.8s\n", pPosixHeader->devmajor);
    printf("devminor: %.8s\n", pPosixHeader->devminor);
    printf("prefix: %.2048s\n\n", pPosixHeader->prefix);
}

long roundUp(long num) {
    long m = num % 512;
    if (m != 0) {
        num += 512-m;
    }
    return num;
}

int findNextChar(char *pHaystack, int length, char needle, int offset) {
    for (int i = offset; i < length; i++) {
        if (pHaystack[i] == needle) {
            return i;
        }
    }
    return -1;
}

long longFromCharArray(char *pArray, int length) {
    long res = 0;
    long factor = 1;
    for (int i = length - 1; i >= 0; i--) {
        res += factor * (long)(pArray[i] - '0');
        factor *= 10;
    }
    return res;
}

long getSizeFromExtendedHeader(char *pHeader, int length) {
    int space, equalSign, newline;
    int offset = 0;

    while (offset < length) {
        space = findNextChar(pHeader + offset, length, ' ', 0);
        equalSign = findNextChar(pHeader + offset, length, '=', 0);
        newline = findNextChar(pHeader + offset, length, '\n', 1);
        if (space == -1 || equalSign == -1 || newline == -1)
            return -1;
        // Test if key is "size"
        if (*(pHeader + offset + space + 1) == 's' && *(pHeader + offset+space+2) == 'i' && *(pHeader + offset + space + 3) == 'z' && *(pHeader + offset + space + 4) == 'e') {
            return longFromCharArray(pHeader + offset + equalSign + 1, newline - 1 - equalSign);
        }
        // Goto next attribute
        offset += newline;
    }
    return -1;
}

void listTarFile(FILE *fp, int indentation, char *pPath) {
    // For checking for end of tar
    int numberOfEmpty = 0;

    char header[512];

    char storedName[2048];
    size_t storedNameSize = 0;

    // Read files
    struct posix_header posixFileHeader;
    while (!feof(fp) && numberOfEmpty < 2) {
        fread (header,1,512,fp);
        // Test if header consists of 0 only. if two zero block occure, end the file reading.
        int empty = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) {
                empty = 0;
                break;
            }
        }
        if (!empty) {
            numberOfEmpty = 0;
            posixFileHeader = (struct posix_header) { 0 };
            fillStruct(header, &posixFileHeader, storedName, storedNameSize);
            storedNameSize = 0;
            if (posixFileHeader.typeflag == '0') {
                //calculate filelength
                long jump = roundUp(octalToDecimal(posixFileHeader.size, 12));
                //indent the file list:
                char *pIndentStr = malloc(indentation*4);
                memset(pIndentStr, ' ', indentation*4);
                printf("%s%s%s\n", pIndentStr, pPath, posixFileHeader.fullName);
                //if file ends with .tar -> list files inside it
                char *pDot = strrchr(posixFileHeader.name, '.');
                if (pDot && !strcmp(pDot, ".tar")) {
                    //get the current position to return to after the inner tar file is read
                    long pos = ftell(fp);
                    //calculate next path
                    char *pNextPath = malloc(strlen(pPath) + strlen(posixFileHeader.fullName) + 1);

                    strncpy(pNextPath, pPath, strlen(pPath));
                    strncpy(pNextPath+strlen(pPath), posixFileHeader.fullName, strlen(posixFileHeader.fullName));
                    strcpy(pNextPath+strlen(pPath)+strlen(posixFileHeader.fullName), "/");
                    listTarFile(fp, indentation+1, pNextPath);
                    fseek(fp, pos+jump, SEEK_SET);
                    free(pNextPath);
                    pNextPath = NULL;
                } else {
                    if (jump > 0)
                        fseek(fp, jump, SEEK_CUR);
                }
            } else if (posixFileHeader.typeflag == '5') {
                char *pIndentStr = malloc(indentation*4);
                memset(pIndentStr, ' ', indentation*4);
                printf("%s%s%s\n", pIndentStr, pPath, posixFileHeader.fullName);
            } else if (posixFileHeader.typeflag == 'x') {
                long extendedHeaderLength = roundUp(octalToDecimal(posixFileHeader.size, 12));
                char *extendedHeader = malloc(extendedHeaderLength);
                fread(extendedHeader, 1, extendedHeaderLength, fp);

                //get size from extended header
                long size = getSizeFromExtendedHeader(extendedHeader, extendedHeaderLength);

                printf("\n NEXT HEADER: \n");
                long jump2 = roundUp(size);
                //read huge file header
                fread (header,1,512,fp);
                posixFileHeader = (struct posix_header){ 0 };
                fillStruct(header, &posixFileHeader, storedName, storedNameSize);
                storedNameSize = 0;
                printStruct(&posixFileHeader);

                char *pIndentStr = malloc(indentation*4);
                memset(pIndentStr, ' ', indentation*4);
                printf("%s%s%s\n", pIndentStr, pPath, posixFileHeader.fullName);

                // Case file
                // If file ends with .tar -> list files inside it
                char *pDot = strrchr(posixFileHeader.name, '.');
                if (pDot && !strcmp(pDot, ".tar")) {
                    // Odd hack with seeking, slows down a little but works.
                    long pos = ftell(fp);
                    // Calculate next ppath
                    char *pNextPath = malloc(strlen(pPath) + strlen(posixFileHeader.fullName) + 1);

                    strncpy(pNextPath, pPath, strlen(pPath));
                    strncpy(pNextPath+strlen(pPath), posixFileHeader.fullName, strlen(posixFileHeader.fullName));
                    strcpy(pNextPath+strlen(pPath)+strlen(posixFileHeader.fullName), "/");
                    listTarFile(fp, indentation+1, pNextPath);
                    fseek(fp, pos+jump2, SEEK_SET);
                    free(pNextPath);
                    pNextPath = NULL;
                } else {
                    if (jump2 > 0)
                        fseek(fp, jump2, SEEK_CUR);
                }
            } else if (posixFileHeader.typeflag == 'L') {
                // Read next block and check it's content
                storedNameSize = octalToDecimal(posixFileHeader.size, 12);
                int rest = 512 - (storedNameSize % 512);
                fread(storedName,1,storedNameSize,fp);
                fseek(fp, rest, SEEK_CUR);
            } else {
              printf("unknown flag: %c , %hhd\n", posixFileHeader.typeflag,posixFileHeader.typeflag);
              printStruct(&posixFileHeader);
              exit(-1);
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

    //read files
    struct posix_header fileHeader;
    while (!feof(fp) && numberOfEmpty < 2) {
        fread (header,1,512,fp);
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
            storedNameSize = 0;
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
                            fread (header,1,512,fp);
                            fwrite(header, 1, 512, stdout);
                            fileSize -= 512;
                        }
                        if (fileSize > 0) {
                            fread (header,1,fileSize,fp);
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
                // TODO: handle flag or remove
            } else if (fileHeader.typeflag == 'x') {
                long extendedHeaderLength = roundUp(octalToDecimal(fileHeader.size, 12));
                char *extendedHeader = malloc(extendedHeaderLength);
                fread(extendedHeader, 1, extendedHeaderLength, fp);

                //get size from extended header
                long size = getSizeFromExtendedHeader(extendedHeader, extendedHeaderLength);
                long jump = roundUp(size);
                //read huge file header
                fread (header,1,512,fp);
                fileHeader = (struct posix_header){ 0 };
                fillStruct(header, &fileHeader, storedName, storedNameSize);
                storedNameSize = 0;
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
                        strncpy(newpath, path + pathn + 1, pathl - pathn - 1);
                        parseTarForPath(fp, newpath);

                    } else { //the requested file is found, print the result:
                        //print first block //TODO print all, based on size
                        while (jump > 0) {
                            fread (header,1,512,fp);
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
                // read next block and check it's content
                storedNameSize = octalToDecimal(fileHeader.size, 12);
                int rest = 512 - (storedNameSize % 512);
                fread (storedName,1,storedNameSize,fp);
                fseek(fp, rest, SEEK_CUR);
            } else {
              printf("unknown flag: %c\n", fileHeader.typeflag);
            }
        } else {
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
        char *path = strdup(argv[2]);

        //search for file
        parseTarForPath(fp, path);
        free(path);

        fclose(fp);
    }

    return 0;
}
