#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct
{
    char name[19];
    int type;
    int offset;
    int size;
} Section_header;

typedef struct{
    char magic;
    int nr_sections;
    Section_header *section_header;
    int header_size;
    int version;
} Header;


Header createHeader(int fd){
    Header H;

    H.version = 0;
    H.header_size = 0;
    H.nr_sections = 0;
    lseek(fd, -1, SEEK_END);
    read(fd, &H.magic, 1);

    lseek(fd, -3, SEEK_END);
    read(fd, &H.header_size, 2);

    lseek(fd, -H.header_size, SEEK_END);
    read(fd, &H.version, 2);

    read(fd, &H.nr_sections, 1);
    H.section_header = (Section_header*)malloc(H.nr_sections * sizeof(Section_header));

    for (int i = 0; i < H.nr_sections; i++){
        read(fd, H.section_header[i].name, 18);
        H.section_header[i].name[18] = 0;
        H.section_header[i].type = 0;
        read(fd, &H.section_header[i].type, 1);

        read(fd, &H.section_header[i].offset, 4);
        read(fd, &H.section_header[i].size, 4);
    }

    return H;
}

void sort(Header H){
    for (int i = 0; i < H.nr_sections - 1; i++){
        for (int j = i + 1; j < H.nr_sections; j++){
            if (H.section_header[i].offset > H.section_header[j].offset){
                Section_header s = H.section_header[i];
                H.section_header[i] = H.section_header[j];
                H.section_header[j] = s;
            }
        }
    }
}

void print(struct dirent *entry, char *path, char* filteringOption){
    if (strncmp(filteringOption, "name_ends_with=", 15) == 0){
        int k = strlen(entry->d_name) - strlen(filteringOption + 15);
        if (k >= 0 && strncmp(entry->d_name + k, filteringOption + 15, strlen(filteringOption + 15)) == 0){
            printf("%s/%s\n", path, entry->d_name);
        }
    } else if (strncmp(filteringOption, "has_perm_execute", 16) == 0){
        struct stat statBuf;
        char *v = (char*)malloc((strlen(path) + strlen(entry->d_name) + 2) * sizeof(char));
        strcpy(v, path);
        strcat(v, "/");
        strcat(v, entry->d_name);
        if (lstat(v, &statBuf) == 0 && (statBuf.st_mode & 0777 & 0100)){
            printf("%s/%s\n", path, entry->d_name);
        }
        free(v);
    } else if (strcmp(filteringOption, "nothing") == 0){
        printf("%s/%s\n", path, entry->d_name);
    }
}

// 2.3 Listarea nerecursiva
int listDir(char *path, char *filteringOption){
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(path);
    if (dir == NULL){
        perror("Could not oped directory");
        return -1;
    }

    printf("SUCCESS\n");
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){
            print(entry, path, filteringOption);
        }
    }
    closedir(dir);
    return 0;
}

// 2.3 Listarea recursiva
void listDirRec(char *path, char *filteringOption){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if (dir == NULL){
        perror("Could not oped directory");
        return;
    }
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0){
                print(entry, path, filteringOption);
                if (S_ISDIR(statbuf.st_mode)){
                    listDirRec(fullPath, filteringOption);
                }
            }
        }
    }
    closedir(dir);
}

// 2.4 Parsarea
void parsare(char* path){
    int fd = -1;
    fd = open(path, O_RDONLY);

    if (fd == -1){
        return;
    }

    Header H = createHeader(fd);

    if (H.magic != 'V'){
        printf("ERROR\nwrong magic");
        free(H.section_header);
        close(fd);
        return;
    }

    if (H.version < 47 || H.version > 101){
        printf("ERROR\nwrong version");
        free(H.section_header);
        close(fd);
        return;
    }

    if (H.nr_sections < 2 || H.nr_sections > 17){
        printf("ERROR\nwrong sect_nr");
        free(H.section_header);
        close(fd);
        return;
    }
 
    int v[] = {39, 29, 23, 88, 43, 28, 41};
    
    for (int i = 0; i < H.nr_sections; i++){
        int ok = 0;
        for (int j = 0; j < 7; j++){
            if (H.section_header[i].type == v[j]){
                ok = 1;
                break;
            }
        }
        if (!ok){
            printf("ERROR\nwrong sect_types");
            free(H.section_header);
            close(fd);
            return;
        }
    }

    printf("SUCCESS\n");
    printf("version=%d\n", H.version);
    printf("nr_sections=%d\n", H.nr_sections);
    for (int i = 0; i < H.nr_sections; i++){
        printf("section%d: %s %d %d\n", i + 1, H.section_header[i].name, H.section_header[i].type, H.section_header[i].size);
    }

    free(H.section_header);
    close(fd);
}

// 2.5 Lucrul cu sectiuni
void printS(char *v){
    int i = 0;
    while (v[i] != 0xa){
        printf("%c", v[i]);
        i++;
    }
}

void extract(char *path, int section, int line){
    int fd = -1;
    fd = open(path, O_RDONLY);

    char c;
    if (fd == -1){
        printf("ERROR\ninvalid file");
        return;
    }

    Header H = createHeader(fd);
    sort(H);
    lseek(fd, 0, SEEK_SET);

    if (section > H.nr_sections || section < 2 || section > 17){
        printf("ERROR\ninvalid section");
        free(H.section_header);
        close(fd);
        return;
    }

    lseek(fd, H.section_header[section - 1].offset, SEEK_SET);

    int k = 1;
    while(k < line){
        read(fd, &c, 1);
        if (c == 0xa){
            k++;
        }
    }
    read(fd, &c, 1);

    printf("SUCCESS\n");
    while (c != 0xa && c != 0){
        printf("%c", c);
        read(fd, &c, 1);
    }

    free(H.section_header);
    close(fd);
}

// 2.6 findall
int isSF(Header H){
    if (H.magic != 'V'){
        return -1;
    }

    if (H.version < 47 || H.version > 101){
        return -2;
    }

    if (H.nr_sections < 2 || H.nr_sections > 17){
        return -3;
    }
 
    int v[] = {39, 29, 23, 88, 43, 28, 41};
    
    for (int i = 0; i < H.nr_sections; i++){
        int ok = 0;
        for (int j = 0; j < 7; j++){
            if (H.section_header[i].type == v[j]){
                ok = 1;
                break;
            }
        }
        if (!ok){
            return -4;
        }
    }

    return 0;
}

void findAll(char *path){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if (dir == NULL){
        perror("ERROR\ninvalid directory path");
        return;
    }

    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0){
                if (S_ISDIR(statbuf.st_mode)){
                    findAll(fullPath);
                } else {
                    int fd = -1;
                    fd = open(fullPath, O_RDONLY);
                    if (fd == -1){
                        continue;
                    }
                    Header H = createHeader(fd);
                    if (isSF(H) == 0){
                        for (int i = 0; i < H.nr_sections; i++){
                            int j = 0;
                            char c[H.section_header[i].size];
                            lseek(fd, H.section_header[i].offset, SEEK_SET);
                            read(fd, c, H.section_header[i].size);
                            for (int k = 0; k < H.section_header[i].size; k++){
                                if (c[k] == 0xa){
                                    j++;
                                }
                            }
                            if (j == 12){
                                printf("%s\n", fullPath);
                                break;
                            }
                        }
                    }
                    free(H.section_header);
                    close(fd);
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("ERROR\nNu ati dat argumente");
        return -1;
    }

    if (strcmp(argv[1], "variant") == 0){
        printf("66535\n");
    } else if (strcmp(argv[1], "list") == 0){
        if (strcmp(argv[2], "recursive") != 0){
            if (strncmp(argv[2], "path=", 5) == 0){
                if (listDir(argv[2] + 5, "nothing") == -1){
                    printf("ERROR\ninvalid directory path");
                }
            } else if (strncmp(argv[2], "name_ends_with=", 15) == 0 || strncmp(argv[2], "has_perm_execute", 16) == 0){
                if (strncmp(argv[3], "path=", 5) == 0){
                    if (listDir(argv[3] + 5, argv[2]) == -1){
                        printf("ERROR\ninvalid directory path");
                    }
                }
            } else {
                printf("ERROR\nwrong arguments");
            }
        } else {
            if (strncmp(argv[3], "path=", 5) == 0){
                printf("SUCCESS\n");
                listDirRec(argv[3] + 5, "nothing");
            } else if (strncmp(argv[2], "name_ends_with=", 15) == 0 || strncmp(argv[2], "has_perm_execute", 16) == 0){
                if (strncmp(argv[4], "path=", 5) == 0){
                    printf("SUCCESS\n");
                    listDirRec(argv[4] + 5, argv[3]);
                }
            } else {
                printf("ERROR\nwrong arguments");
            }
        }
    } else if (strcmp(argv[1], "parse") == 0){
        if (strncmp(argv[2], "path=", 5) == 0){
            parsare(argv[2] + 5);
        }
    } else if (strcmp(argv[1], "extract") == 0){
        if (strncmp(argv[2], "path=", 5) == 0 && strncmp(argv[3], "section=", 8) == 0 && strncmp(argv[4], "line=", 5) == 0){
            int section, line;
            sscanf(argv[3] + 8, "%d", &section);
            sscanf(argv[4] + 5, "%d", &line);
            extract(argv[2] + 5, section, line);
        }
    } else if (strcmp(argv[1], "findall") == 0){
        if (strncmp(argv[2], "path=", 5) == 0){
            printf("SUCCESS\n");
            findAll(argv[2] + 5);
        }
    } else {
        printf("ERROR\nommand not found");
    }
}