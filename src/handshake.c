#include "wsproxy.h"

#include <string.h> 
#include <stdlib.h> 

char *h_hoststr;
int h_port;


char *start_handshake(){

    char *handshake;
    sgetline(fd, &handshake);

    char *method = strtok(handshake, " ");
    char *path = strtok(NULL, " ")+1;
    char *protocol = strtok(NULL, " ");

    // fuck you raff
    if (!!strcasecmp(protocol,"HTTP/1.1"))
        return strdup("Wrong Protocol");

    h_hoststr = strtok(path,":");
    char* portstr = strtok(NULL," ");
    h_port = atoi(portstr);


    return NULL;
}

char *finish_handshake(){
    char* wsprotocol = NULL;
    char* wskey = NULL;
    for (;;){
        char* header;
        int count = sgetline(fd, &header);
        if (count==0) break;

        char *hkey = strtok(header,":");
        char *hvalue = strtok(NULL, "\r\n")+1;
        
        if (!strcmp(hkey, "Sec-WebSocket-Protocol"))
            wsprotocol = strdup(hvalue);
        else if (!strcmp(hkey,"Sec-WebSocket-Key"))
            wskey = strdup(hvalue);

        free(header);
    }

    if (!wskey)
        return strdup("Missing Sec-WebSocket-Key Header");

    SHA1_CTX sha;
    uint8_t results[20];
    SHA1Init(&sha);
    const char* hash= "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //... why
    SHA1Update(&sha, (uint8_t*)wskey, strlen(wskey));
    SHA1Update(&sha, (const unsigned char*)hash, strlen(hash));
    SHA1Final(results, &sha);

    char results_enc[49];
    b64_encode(results, sizeof results, (unsigned char*)results_enc);

    dprintf(fd, "HTTP/1.1 101 Switching Protocols\r\n");
    dprintf(fd, "Upgrade: websocket\r\n");
    dprintf(fd, "Connection: Upgrade\r\n");
    if (wsprotocol)
        dprintf(fd, "Sec-WebSocket-Protocol: %s\r\n", wsprotocol);
    dprintf(fd, "Sec-WebSocket-Accept: %s\r\n", results_enc);
    dprintf(fd, "\r\n");


    free(wskey);
    if (wsprotocol)
        free(wsprotocol);

    return NULL;
}
