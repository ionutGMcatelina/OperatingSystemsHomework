#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define RESP_NAME "RESP_PIPE_66535"
#define REQ_NAME "REQ_PIPE_66535"

int main(){
    int fdResp = -1;
    int fdReq = -1;

    // 2. CONECTIUNEA PRIN PIPE

    unlink(RESP_NAME);
    if (mkfifo(RESP_NAME, 0600) != 0){
        printf("ERROR\ncannot create the response pipe");
        return 1;
    }

    fdResp = open(RESP_NAME, O_RDWR);
    if(fdResp == -1) {
        printf("ERROR\ncannot create the response pipe");
        return 1;
    }
    fdReq = open(REQ_NAME, O_RDWR);
    if(fdReq == -1) {
        printf("ERROR\ncannot open the request pipe");
        return 1;
    }

    int connectL = 7;
    write(fdResp, &connectL, sizeof(char));
    char connect[] = "CONNECT";
    write(fdResp, connect, connectL);
    printf("SUCCESS");
    unsigned int shmId;
    void *sharedFile;
    int memorySize = 0;
    int fd = -1;
    char *data = NULL;
    off_t size;

    for(;;){
        unsigned int length = 0;
        read(fdReq, &length, 1);
        char request[length];
        read(fdReq, request, length);

        // 3. PING
        if (strncmp(request, "PING", length) == 0){
            //length = 4;
            write(fdResp, &length, 1);
            write(fdResp, "PING", length);
            
            write(fdResp, &length, 1);
            write(fdResp, "PONG", length);
            
            int par = 66535;
            write(fdResp, &par, sizeof(unsigned int));
        }

        // 4. SHARED MEMORY
        else if (strncmp(request, "CREATE_SHM", 10) == 0){
            length = 10;
            read(fdReq, &memorySize, sizeof(unsigned int));
            write(fdResp, &length, 1);
            write(fdResp, "CREATE_SHM", length);
            if (memorySize == 0){
                int er = 5;
                write(fdResp, &er, 1);
                write(fdResp, "ERROR", er);
            } else{
                shmId = shmget(10700, memorySize, IPC_CREAT | 0664);
                sharedFile = shmat(shmId, NULL, 0);
                if(shmId < 0) {
                    int er = 5;
                    write(fdResp, &er, 1);
                    write(fdResp, "ERROR", er);
                } else{
                    int l = 7;
                    write(fdResp, &l, 1);
                    write(fdResp, "SUCCESS", l);
                }
            }
        }

        // 5. WRITE TO SHM
        else if (strncmp(request, "WRITE_TO_SHM", 12) == 0){
            unsigned int offset, value;
            length = 12;
            read(fdReq, &offset, sizeof(unsigned int));
            read(fdReq, &value, sizeof(unsigned int));
            write(fdResp, &length, 1);
            write(fdResp, "WRITE_TO_SHM", length);
            int len2;
            if (offset < 0 || offset + 4 > memorySize){
                len2 = 5;
                write(fdResp, &len2, 1);
                write(fdResp, "ERROR", len2);
            } else{
                unsigned int *valMod = (unsigned int*)(sharedFile + offset);
                *valMod = value;
                len2 = 7;
                write(fdResp, &len2, 1);
                write(fdResp, "SUCCESS", len2);
            }
        }
        
        // 6. MAPAREA UNUI FISIER
        else if (strncmp(request, "MAP_FILE", 8) == 0){
            int fileSize = 0;
            read(fdReq, &fileSize, 1);
            char s[fileSize + 1];
            read(fdReq, s, fileSize);

            length = 8;
            write(fdResp, &length, 1);
            write(fdResp, "MAP_FILE", length);

            s[fileSize] = 0;
            fd = open(s, O_RDONLY);

            if(fd == -1) {
                length = 5;
                write(fdResp, &length, 1);
                write(fdResp, "ERROR", length);
            } else{
                size = lseek(fd, 0, SEEK_END);
                lseek(fd, 0, SEEK_SET);
                data = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
                if(data == (void*)-1) {
                    length = 5;
                    write(fdResp, &length, 1);
                    write(fdResp, "ERROR", length);
                } else{
                    length = 7;
                    write(fdResp, &length, 1);
                    write(fdResp, "SUCCESS", length);
                }
                
            }
        }

        // 7. READ FROM FILE OFFSET
        else if (strncmp(request, "READ_FROM_FILE_OFFSET", length) == 0){
            unsigned int rdOffset = 0, nrBytes = 0;
            read(fdReq, &rdOffset, sizeof(unsigned int));
            read(fdReq, &nrBytes, sizeof(unsigned int));
            
            char *string = (char*)sharedFile;

            length = 21;
            write(fdResp, &length, 1);
            write(fdResp, "READ_FROM_FILE_OFFSET", length);
            if (rdOffset + nrBytes >= size){
                length = 5;
                write(fdResp, &length, 1);
                write(fdResp, "ERROR", length);
            } else{
                strncpy(string, data + rdOffset, nrBytes);
                length = 7;
                write(fdResp, &length, 1);
                write(fdResp, "SUCCESS", length);
            }
            
        }

        // 8. CITIREA DINTR-O SECTIUNE SF
        else if (strncmp(request, "READ_FROM_FILE_SECTION", length) == 0){
            unsigned int section = 0, rdOffset = 0, nrBytes = 0;
            read(fdReq, &section, sizeof(unsigned int));
            read(fdReq, &rdOffset, sizeof(unsigned int));
            read(fdReq, &nrBytes, sizeof(unsigned int));

            write(fdResp, &length, 1);
            write(fdResp, "READ_FROM_FILE_SECTION", length);

            int headerSize;
            memcpy(&headerSize, data + (size - 3), 2);

            int start = size - headerSize;
            int sect = start + 3 + ((section - 1) * 27) + 19;
            int offset;
            int sectSize;

            int noOfSections;
            memcpy(&noOfSections, data + start + 2, 1);
            if (noOfSections < section){
                length = 5;
                write(fdResp, &length, 1);
                write(fdResp, "ERROR", length);
            }else{
                memcpy(&offset, data + sect, 4);
                memcpy(&sectSize, data + sect + 4, 4);
                
                offset += rdOffset;
                char *string = (char*)(sharedFile);
                if (rdOffset + nrBytes > sectSize){
                    length = 5;
                    write(fdResp, &length, 1);
                    write(fdResp, "ERROR", length);
                } else{
                    strncpy(string, data + offset, nrBytes);
                    length = 7;
                    write(fdResp, &length, 1);
                    write(fdResp, "SUCCESS", length);
                }
            }
        }

        // 9. CITIREA DE LA UN OFFSET DIN SPATIUL DE MEMORIE LOGIC
        else if (strncmp(request, "READ_FROM_LOGICAL_SPACE_OFFSET", length) == 0){
            unsigned int logicalOffset = 0, nrBytes = 0;
            read(fdReq, &logicalOffset, sizeof(unsigned int));
            read(fdReq, &nrBytes, sizeof(unsigned int));

            write(fdResp, &length, 1);
            write(fdResp, "READ_FROM_LOGICAL_SPACE_OFFSET", length);

            int headerSize;
            memcpy(&headerSize, data + (size - 3), 2);

            int start = size - headerSize;
            int noOfSections;
            memcpy(&noOfSections, data + start + 2, 1);

            int nr = 0;
            int nrS = 0;
            int dim;
            for (int i = 0; i < noOfSections; i++){
                int thisSect = start + 3 + i * 27 + 23;
                int sectSize;
                memcpy(&sectSize, data + thisSect, 4);
                nrS++;
                dim = nr;
                nr += sectSize - sectSize % 1024 + 1024;
                if (nr >= logicalOffset){
                    break;
                }
            }
            if (noOfSections < nrS){
                length = 5;
                write(fdResp, &length, 1);
                write(fdResp, "ERROR", length);
            }else{
                int offset;
                int sectSize;
                int sect = start + 3 + (nrS - 1) * 27 + 19;
                memcpy(&offset, data + sect, 4);
                memcpy(&sectSize, data + sect + 4, 4);
                
                char *string = (char*)(sharedFile);

                offset += logicalOffset - dim;
                if (nrBytes > sectSize){
                    length = 5;
                    write(fdResp, &length, 1);
                    write(fdResp, "ERROR", length);
                } else{
                    strncpy(string, data + offset, nrBytes);
                    length = 7;
                    write(fdResp, &length, 1);
                    write(fdResp, "SUCCESS", length);
                }
            }
        }
        
        // 10. EXIT
        else if (strncmp(request, "EXIT", length) == 0){
            unlink(RESP_NAME);
            remove(RESP_NAME);
            unlink(REQ_NAME);
            shmdt(&sharedFile);
            munmap(data, size);
            close(fd);
            shmctl(shmId, IPC_RMID, 0);
            break;
        }
    }

    return 0;
}