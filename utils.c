char* file_to_buffer(const char *filename) {
    struct stat sb;
    stat(filename, &sb);
    char *buffer = (char*) malloc(sb.st_size * sizeof(char));
    FILE *f = fopen(filename, "rb");
    assert(fread((void*) buffer, sizeof(char), sb.st_size, f));
    fclose(f);
    return buffer;
}
/*
void* mmalloc(size_t sz, char *filename) {
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    assert(fd != -1);
    char v = 0;
    for (size_t i = 0; i < sz; i++) {
        assert(write(fd, &v, sizeof(char)));
    }
    void *map = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    madvise(map, sz, MADV_RANDOM);
    return map;
}
*/