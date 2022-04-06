#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]){

    // return if no argument to copy to the driver
    if(argc <= 1){
        printf("No text to copy\n");
        return 0;
    }
    
    // print arguments
    int i;
    for(i=0; i<argc; i++){
        printf("Argument n°%d : %s\n", i, argv[i]);
    }

    // open driver file descriptor
    int fd = open("/dev/io_string_module", O_RDWR);
    if (fd < 0) {
        printf("Could not open /dev/io_string_module: error=%i\n", fd);
        return -1;
    }

    // writing to the driver
    write(fd, argv[1], strlen(argv[1]));

    // close driver file
    close(fd);

    // open driver file descriptor
    fd = open("/dev/io_string_module", O_RDWR);
    if (fd < 0) {
        printf("Could not open /dev/io_string_module: error=%i\n", fd);
        return -1;
    }

    // read from driver
    char buf[100];
    ssize_t sz = read(fd, buf, sizeof(buf) - 1);
    if(sz > 0) printf("%s", buf);

    // close driver file
    close(fd);

    return 0;
}