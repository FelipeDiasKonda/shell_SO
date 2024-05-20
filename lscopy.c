#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void print_file_info(const char *path, const struct dirent *entry, int detailed) {
    if (detailed) {
        struct stat fileStat;
        if (stat(path, &fileStat) < 0) {
            perror("stat");
            return;
        }

        // Print file type and permissions
        printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
        printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");

        // Print number of hard links
        printf(" %lu", fileStat.st_nlink);

        // Print owner and group
        struct passwd *pwd = getpwuid(fileStat.st_uid);
        struct group *grp = getgrgid(fileStat.st_gid);
        printf(" %s %s", pwd->pw_name, grp->gr_name);

        // Print file size
        printf(" %5ld", fileStat.st_size);

        // Print modification time
        char timeStr[20];
        struct tm *timeInfo = localtime(&fileStat.st_mtime);
        strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", timeInfo);
        printf(" %s", timeStr);
    }

    // Print file name
    printf(" %s\n", entry->d_name);
}

int main(int argc, char *argv[]) {
    int show_all = 0;
    int detailed = 0;
    int opt;

    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l':
                detailed = 1;
                break;
            case 'a':
                show_all = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-a] [directory]\n", argv[0]);
                return 1;
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        print_file_info(fullPath, entry, detailed);
    }

    closedir(dir);
    return 0;
}
