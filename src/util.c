#include <stdio.h> 
#include <errno.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 

char *aserror(char *fmt){
    int errnum = errno;
    char *err = strerror(errnum);

    char *result;
    asprintf(&result,"%s: %s",fmt, err);

    return result;
}

int sgetline(int fd, char ** out) 
{ 
    int buf_size = 0; 
    int in_buf = 0; 
    int ret;
    char ch; 
    char * buffer = NULL; 
    char * new_buffer;

    do
    {
        // read a single byte
        ret = read(fd, &ch, 1);
        if (ret < 1)
        {
            // error or disconnect
            free(buffer);
            return -1;
        }

        // has end of line been reached?
        if (ch == '\n') 
            break; // yes

        // is more memory needed?
        if ((buf_size == 0) || (in_buf == buf_size)) 
        { 
            buf_size += 128; 
            new_buffer = realloc(buffer, buf_size); 

            if (!new_buffer) 
            { 
                free(buffer);
                return -1;
            } 

            buffer = new_buffer; 
        } 

        buffer[in_buf] = ch; 
        ++in_buf; 
    } 
    while (1);

    // if the line was terminated by "\r\n", ignore the
    // "\r". the "\n" is not in the buffer
    if ((in_buf > 0) && (buffer[in_buf-1] == '\r'))
        --in_buf;

    // is more memory needed?
    if ((buf_size == 0) || (in_buf == buf_size)) 
    { 
        ++buf_size; 
        new_buffer = realloc(buffer, buf_size); 

        if (!new_buffer) 
        { 
            free(buffer);
            return -1;
        } 

        buffer = new_buffer; 
    } 

    // add a null terminator
    buffer[in_buf] = '\0';

    *out = buffer; // complete line

    return in_buf; // number of chars in the line, not counting the line break and null terminator
}
