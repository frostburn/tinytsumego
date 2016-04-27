char* file_to_buffer(const char *filename) {
    struct stat sb;
    stat(filename, &sb);
    char *buffer = (char*) malloc(sb.st_size * sizeof(char));
    FILE *f = fopen(filename, "rb");
    assert(fread((void*) buffer, sizeof(char), sb.st_size, f));
    fclose(f);
    return buffer;
}
