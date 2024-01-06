#include "base64.h"
#include <endian.h>
#include <sha1.h>
#include <stdint.h>
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <signal.h>

#define die(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0) 
int sock;
int acceptfd;
FILE* conn;
void sigint_handler(){

    printf("????\n");
    if (close(sock)!=0)
        die("close");
    
    if (close(acceptfd)!=0)
        die("close2");

    printf("ripbozo\n");
    exit(0);
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

static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

uint8_t reverse(uint8_t n) {
   // Reverse the top and bottom nibble then swap them.
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}
int main() 
{ 
    if (signal(SIGINT, sigint_handler) != 0)
        die("signal handler");
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock < 0)
        die("socket");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr)); 

    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(8080); 

    if ((bind(sock, (struct sockaddr*)&addr, sizeof(addr))) != 0)
        die("bind");

    if (listen(sock, 5) != 0)
        die("listen");
    printf("Server listening..\n"); 

    struct sockaddr_in cli;
    int len = sizeof(cli); 

    // Accept the data packet from client and verification 
    int fd = accept(sock, (struct sockaddr*)&cli, &len); 
    if (fd < 0) { 
        printf("server accept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server accept the client...\n"); 

    acceptfd = fd;

    // fscanf(conn,"%s %s %s\r\n",hkey,hvalue,hvalue);


    
    char *handshake;
    sgetline(fd, &handshake);
    printf("%s\n",handshake);
    free(handshake);

    // if (strcmp(hkey, "GET"))
    //     return 2;


    char* wsprotocol;
    char* wskey;
    for (;;){
        char* header;
        int count = sgetline(fd, &header);
        if (count==0) break;

        // printf("%s\n",header);

        char *hkey = strtok(header,":");
        char *hvalue = strtok(NULL, "\r\n")+1;
        
        if (!strcmp(hkey, "Sec-WebSocket-Protocol"))
            wsprotocol = strdup(hvalue);
        else if (!strcmp(hkey,"Sec-WebSocket-Key"))
            wskey = strdup(hvalue);

        free(header);
    }

    // printf("-%s\n",wskey);

    // char wskey_dec[strlen(wskey)/2+1];

    // b64_decode(wskey, strlen(wskey), wskey_dec);

    // printf("-%s-\n",wskey_dec);

    //
    SHA1_CTX sha;
    uint8_t results[20];
    SHA1Init(&sha);
    const char* hash= "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //... why
    SHA1Update(&sha, (uint8_t*)wskey, strlen(wskey));
    SHA1Update(&sha, hash, strlen(hash));
    SHA1Final(results, &sha);

    char results_enc[49];
    b64_encode(results, sizeof results, results_enc);

    printf("wsa%s\n",results_enc);

    dprintf(fd, "HTTP/1.1 101 Switching Protocols\r\n");
    dprintf(fd, "Upgrade: websocket\r\n");
    dprintf(fd, "Connection: Upgrade\r\n");
    dprintf(fd, "Sec-WebSocket-Accept: %s\r\n", results_enc);
    dprintf(fd, "\r\n");



    unsigned char frame[10];
    frame[0] = (128 | 0x2);
    frame[1] = 126;
    frame[3] = 126;
    write(fd,frame, 4);
    write(fd,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",126);


    for (;;){
        memset(frame,0,10);
        read(fd, frame, 2);

        unsigned char fin = frame[0] & 128;
        unsigned char opcode = frame[0] & 15;

        unsigned long length = frame[1] & 127;
        unsigned char masked = frame[1] & 128;
        unsigned char mask[4];
        memset(mask,0,4);
        // printf("fin:%i, opcode:%i, masked: %i\n",fin,opcode,masked);

        if (length == 126){
            unsigned char len[2];
            memset(len,0,2);
            read(fd, &len, sizeof len);
            length = len[1];
            length |= len[0]<<8;
        }

        if (length == 127){
            unsigned long len;
            memset(&len,0,sizeof len);
            read(fd,&len,sizeof len);
            // write(5,len,sizeof len);
            unsigned long l = be64toh(len);
            // length  = (unsigned long)len[7];
            // length |= (unsigned long)len[6]<<8;
            // length |= (unsigned long)len[5]<<16;
            // length |= (unsigned long)len[4]<<24;
            // length |= (unsigned long)len[3] << 32;
            // length |= (unsigned long)len[2] << 40;
            // length |= (unsigned long)len[1] << 48;
            // length |= (unsigned long)len[0] << 56;

            printf("len: %lu-%lu\n",l,len);
            exit(0);
        }

        if (masked == 128){
            read(fd,mask,sizeof mask);
        }


        
        char payload[length];
        memset(payload,0, sizeof payload);
        read(fd, payload,length);

        // goofy ahh protocol
        if (masked == 128){
            for (unsigned long i=0; i < length;i++) {
                payload[i] ^= mask[i%4];
            }
        }



        // write(1,payload, length);
        // write(1,";\n",2);
    }

    //
    // // free(wskey_dec);
    // // free(wskey);
    // // free(wsprotocol);
    //
    //
    close(fd);
    close(sock); 
}
